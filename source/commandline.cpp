#include "moltengamepad.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include "parser.h"


int do_save(moltengamepad* mg, std::vector<token>& command);
int do_print(moltengamepad* mg, std::vector<token>& command);
int do_load(moltengamepad* mg, std::vector<token>& command);
int do_move(moltengamepad* mg, std::vector<token>& command);
int do_alterslot(moltengamepad* mg, std::vector<token>& command);

#define HELP_TEXT "available commands:\n"\
"\tprint:\tprint out lists and information\n"\
"\tmove:\tmove a device to a different slot\n"\
"\talterslot:\tchange a slot setting\n"\
"\tsave:\tsave all profiles to a file\n"\
"\tload:\tload profiles from a file\n"\
"\tquit:\tquit this application\n"\
"\t<profile>.<event> = <outevent>\n"\
"\t\tchange the event mapping for <event> to <outevent> in the profile <profile>\n"
int MGparser::do_command(moltengamepad* mg, std::vector<token>& command) {
  if (command.empty()) return 0;
  if (command.front().type == TK_ENDL) return 0;
  if (command.front().value == "print") {
    do_print(mg, command);
    return 0;
  }
  if (command.front().value == "move") {
    do_move(mg, command);
    return 0;
  }

  if (command.front().value == "save") return do_save(mg, command);
  if (command.front().value == "load") return do_load(mg, command);
  if (command.front().value == "alterslot") return do_alterslot(mg, command);
  if (command.front().value == "help") {
    out.print(HELP_TEXT);
    return 0;
  };
  if (command.front().value == "quit") {
    return 0;
  };
  out.text("Command not recognized. \"help\" for available commands");
  return 0;
}



int shell_loop(moltengamepad* mg, std::istream& in, int out_fd, message_stream::listen_type type) {
  bool keep_looping = true;
  std::string header = "";
  char* buff = new char [1024];
  MGparser parser(mg, out_fd, type);

  while (!QUIT_APPLICATION && keep_looping) {
    in.getline(buff, 1024);

    auto tokens = tokenize(std::string(buff));

    if (!tokens.empty() && tokens.front().value == "quit") {
      keep_looping = false;
    }

    parser.exec_line(tokens, header);


    if (in.eof()) break;


  }

  delete[] buff;
}

