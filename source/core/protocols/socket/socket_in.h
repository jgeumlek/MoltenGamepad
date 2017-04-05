#pragma once

#include "../../../oscpkt/oscpkt.hh"
#include "../../parser.h"
#include <sys/socket.h>
#include <linux/un.h>
#include <unistd.h>
#include <errno.h>
#include <iostream>

class socket_out;
struct protocol_state {
  int fd;
  moltengamepad* mg;
  MGparser* parse;
  socket_out* out;
  volatile bool keep_looping;
  std::string header;
};


int socket_server_loop(moltengamepad* mg, struct sockaddr_un* address);
int make_socket(std::string& path, sockaddr_un& address);
int start_socket_listener(moltengamepad* mg, sockaddr_un& address);
int socket_connection_loop(moltengamepad* mg, int fd);
int handle_message(oscpkt::Message* msg, protocol_state& state);
