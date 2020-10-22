#include <sys/socket.h>
#include <string>
#include <thread>
#include <unistd.h>
#include "socket_in.h"
#include "socket_out.h"

int socket_server_loop(moltengamepad* mg, struct sockaddr_un* address) {
  bool keep_looping = true;
  struct sockaddr_un addr;
  if (listen(mg->sock, 10) < 0) {
    perror("listen");
    return -1;
  }
  while (keep_looping && !QUIT_APPLICATION) {
    socklen_t address_len = sizeof(struct sockaddr_un);
    int fd = accept(mg->sock, (struct sockaddr *) &addr, &address_len);
    if (fd >= 0) {
      std::thread* thread = new std::thread(socket_connection_loop, mg, fd);
      thread->detach();
      delete thread;
    } else {
      perror("accept");
      break;
    }
  }
  return 0;
}



int make_socket(std::string& path, sockaddr_un& address) {
  const char* run_dir = getenv("XDG_RUNTIME_DIR");
  if (path.empty() && run_dir) {
    path = std::string(run_dir) + "/mg.sock";
  }
  if (path.empty()) {
    debug_print(DEBUG_NONE,1,"Failed to determine a location for the socket.");
    return -1;
  }
  int sock = socket(PF_UNIX, SOCK_STREAM, 0);
  if (sock < 0)  {
    perror("making socket");
    return sock;
  }
  memset(&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  snprintf(address.sun_path, UNIX_PATH_MAX, "%s", path.c_str());
  if (bind(sock, (struct sockaddr *) &address, sizeof(address)) != 0) {
    perror("binding socket");
    return -1;
  }
  debug_print(DEBUG_NONE,2,"Made socket at ", path.c_str());
  return sock;
}

int start_socket_listener(moltengamepad* mg, sockaddr_un& address) {
  std::thread* socket_listener = new std::thread(socket_server_loop, mg, &address);
  socket_listener->detach();
  delete socket_listener;
  return 0;
}

int socket_connection_loop(moltengamepad* mg, int fd) {
  protocol_state state;
  socket_out out(fd);
  MGparser parse(mg, &out);
  unsigned char buffer[1025];
  buffer[1024] = '\0';
  int num_read;
  oscpkt::PacketReader pr;
  state.keep_looping = true;
  state.parse = &parse;
  state.mg = mg;
  state.fd = fd;
  state.out = &out;
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
  //hardcoded list of things to unsubscribe from...
  mg->plugs.remove_listener(&out);
  mg->slots->log.remove_listener(&out);
  debug_print(DEBUG_INFO, 1, "remote client connection closed.");
  return 0;
}

void bad_command(protocol_state& state, int resp_id) {
  state.out->err(resp_id,"invalid OSC request");
  state.out->end_response(resp_id, -1);
}

int handle_message(oscpkt::Message* msg, protocol_state& state) {
  oscpkt::Message::ArgReader arg = msg->arg();
  int resp_id = -1;
  if (!arg.isInt32()) {
    bad_command(state, -1);
    return -1;
  } else {
    arg.popInt32(resp_id);
  }
  if (msg->match("/exit") && arg.isOkNoMoreArgs()) {
    state.keep_looping = false;
    return 0;
  }

  if (msg->match("/eval")) {
    std::string text;
    if (arg.popStr(text).isOkNoMoreArgs()) {
      auto tokens = tokenize(text);
      debug_print(DEBUG_INFO, 2, "client eval:", text.c_str());
      state.parse->exec_line(tokens, state.header, resp_id);
    } else {
      bad_command(state, resp_id);
    }
    return 0;
  }

  if (msg->match("/listen")) {
    std::string stream_name;
    bool listen;
    if (arg.popStr(stream_name).popBool(listen).isOkNoMoreArgs()) {
      if (stream_name == "plug") {
        if (listen)
          state.mg->plugs.add_listener(state.out);
        else
          state.mg->plugs.remove_listener(state.out);
        state.out->end_response(resp_id,0);
        return 0;
      } else if (stream_name == "slot") {
        if (listen)
          state.mg->slots->log.add_listener(state.out);
        else
          state.mg->slots->log.remove_listener(state.out);
        state.out->end_response(resp_id,0);
        return 0;
      }
    }
    bad_command(state,resp_id);
    return 0;
  }
  bad_command(state,resp_id);
  return 0; 
}
