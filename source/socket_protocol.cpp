#include "moltengamepad.h"
#define OSCPKT_OSTREAM_OUTPUT
#define OSCPKT_DEBUG
#include "oscpkt/oscpkt.hh"
#include "parser.h"
#include <unistd.h>
#include <errno.h>
#include <iostream>

struct protocol_state {
  bool initialized = false;
  int fd;
  moltengamepad* mg;
  MGparser* parse;
  volatile bool keep_looping;
  oscpkt::PacketWriter pw;
  std::string header;
};

int write_packet(int fd, oscpkt::PacketWriter& pw) {
  uint32_t size = pw.packetSize();
  write(fd, &size, sizeof(size));
  write(fd, pw.packetData(), size);
  return 0;
}
/*
 * Client must send /protocol/version with a version number.
 * If the server can handle that version, it responds the same.
 * Otherwise, the server sends an error and disconnects
 */
int protocol_init(oscpkt::Message* msg, protocol_state& state) {
  int version;
  if (msg->match("/protocol/version").popInt32(version).isOkNoMoreArgs()) {
    if (version == 1) {
      oscpkt::Message reply;
      reply.init("/protocol/version").pushInt32(1);
      state.pw.init().addMessage(reply);
      write_packet(state.fd, state.pw);
      state.initialized = true;
    } else {
      oscpkt::Message reply;
      reply.init("/error").pushInt32(2).pushStr("Unsupported Protocol");
      state.pw.init().addMessage(reply);
      write_packet(state.fd, state.pw);
    }
  } else {
      oscpkt::Message reply;
      reply.init("/error").pushInt32(1).pushStr("Protocol not initialized");
      state.pw.init().addMessage(reply);
      write_packet(state.fd, state.pw);
  }
  return 0;
}

void bad_command(protocol_state& state) {
  oscpkt::Message reply;
  reply.init("/error").pushInt32(3).pushStr("Unrecognized Command");
  state.pw.init().addMessage(reply);
  write_packet(state.fd, state.pw);
}

int handle_message(oscpkt::Message* msg, protocol_state& state) {
  if (!state.initialized) {
    protocol_init(msg,state);
    if (!state.initialized)
      state.keep_looping = false;
    return 0;
  }
  if (msg->match("/exit").isOkNoMoreArgs()) {
    state.keep_looping = false;
    state.initialized = false;
    return 0;
  }
  if (msg->match("/test")) {
    std::string text;
    if (msg->arg().popStr(text).isOkNoMoreArgs()) {
      oscpkt::Message reply;
      reply.init("/test").pushStr("echo: " + text);
      state.pw.init().addMessage(reply);
      write_packet(state.fd, state.pw);
    } else {
      bad_command(state);
    }
    return 0;
  }
  if (msg->match("/eval")) {
    std::string text;
    if (msg->arg().popStr(text).isOkNoMoreArgs()) {
      auto tokens = tokenize(text);
      state.parse->exec_line(tokens, state.header);
      } else {
        bad_command(state);
      }
    return 0;
  }
  
  bad_command(state);
  
  return 0;
}

int socket_connection_loop(moltengamepad* mg, int fd) {
  protocol_state state;
  state.fd = fd;
  state.keep_looping = true;
  state.mg = mg;
  state.parse = new MGparser(mg, fd, message_stream::OSC);
  unsigned char buffer[1025];
  buffer[1024] = '\0';
  int num_read;
  oscpkt::PacketReader pr;
  while (state.keep_looping && !QUIT_APPLICATION) {
    int size = 0;
    num_read = read(fd, &size, sizeof(size));
    if (num_read != sizeof(size) || size > 1024)
      break; //Just disconnect if we get too confused.
    num_read = read(fd, buffer, size);
    if (num_read == -EAGAIN) continue;
    if (num_read == 0) break;
    if (num_read < 0) { perror("read"); break; };
    pr.init(buffer,num_read);
    oscpkt::Message* msg;
    while (pr.isOk() && (msg = pr.popMessage()) != 0) {
      handle_message(msg, state);
    }
  }
  close(fd);
  delete state.parse;
  return 0;
}
