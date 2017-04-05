#include "moltengamepad.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include "parser.h"


int do_save(moltengamepad* mg, std::vector<token>& command, response_stream* out);
int do_print(moltengamepad* mg, std::vector<token>& command, response_stream* out);
int do_load(moltengamepad* mg, std::vector<token>& command, response_stream* out);
int do_move(moltengamepad* mg, std::vector<token>& command, response_stream* out);
int do_assign(moltengamepad* mg, std::vector<token>& command, response_stream* out);
int do_clear(moltengamepad* mg, std::vector<token>& command, response_stream* out);
int do_set(moltengamepad* mg, std::vector<token>& command, response_stream* out);

#define HELP_TEXT "available commands:\n"\
"\tprint:\tprint out lists and information\n"\
"\tmove:\tmove a device to a different slot\n"\
"\tclear:\tclear (zero-out) a slot's outputs\n"\
"\tsave:\tsave all profiles to a file\n"\
"\tload:\tload profiles from a file\n"\
"\tset:\tset global options\n"\
"\tassign:\tassign a slot for a device id, even before the device is connected.\n"\
"\tquit:\tquit this application\n"\
"\t<profile>.<event> = <outevent>\n"\
"\t\tchange the event mapping for <event> to <outevent> in the profile <profile>\n"\
"\t<profile>.?<option> = <value>\n"\
"\t\tchange a local option for a profile"
int do_command(moltengamepad* mg, std::vector<token>& command, response_stream* out) {
  if (command.empty()) return 0;
  if (command.front().type == TK_ENDL) return 0;
  if (command.back().type == TK_ENDL) command.pop_back();
  if (command.front().value == "print") {
    do_print(mg, command, out);
    return 0;
  }
  if (command.front().value == "move") {
    do_move(mg, command, out);
    return 0;
  }

  if (command.front().value == "save") return do_save(mg, command, out);
  if (command.front().value == "load") return do_load(mg, command, out);
  if (command.front().value == "clear") return do_clear(mg, command, out);
  if (command.front().value == "set") return do_set(mg, command, out);
  if (command.front().value == "assign") return do_assign(mg, command, out);
  if (command.front().value == "help" || command.front().value == "?") {
    out->print(HELP_TEXT);
    return 0;
  };
  if (command.front().value == "quit") {
    return 0;
  };
  out->print("Command not recognized. \"help\" for available commands.");
  return 0;
}



int shell_loop(moltengamepad* mg, std::istream& in) {
  bool keep_looping = true;
  std::string header = "";
  char* buff = new char [1024];
  MGparser parser(mg, mg->stdout);

  while (!QUIT_APPLICATION && keep_looping) {
    in.getline(buff, 1024);

    auto tokens = tokenize(std::string(buff));

    if (!tokens.empty() && tokens.front().value == "quit") {
      keep_looping = false;
    }

    if (!tokens.empty() && tokens.front().value == "exit_process_leave_fifo") {
      //someone else wants our FIFO. Let them have it.
      //We have to ensure we don't try to delete it.
      mg->opts->lock("make_fifo",false);
      mg->opts->set("make_fifo", "false");
      mg->opts->lock("make_fifo",true);
      QUIT_APPLICATION = true;
    }

    //arbitrary response id of 1.
    //Response ids are only needed when communicating from another process.
    parser.exec_line(tokens, header,1);


    if (in.eof()) break;


  }

  delete[] buff;
}

