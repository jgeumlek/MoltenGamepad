#ifndef PARSER_H
#define PARSER_H
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include "moltengamepad.h"

enum tokentype { TK_IDENT, TK_DOT, TK_EQUAL, TK_HEADER_OPEN, TK_HEADER_CLOSE, TK_LPAREN, TK_RPAREN, TK_COMMA, TK_VALUE, TK_ENDL};

struct token {
  enum tokentype type;
  std::string value;
};

std::vector<token> tokenize(std::string line);

struct complex_expr {
  std::string ident;
  std::vector<complex_expr*> params;
};

class MGparser {
public:
  MGparser(moltengamepad* mg) : mg(mg) {};
  void exec_line(std::vector<token> &line, std::string &header);
  event_translator* parse_trans(enum entry_type intype, std::vector<token> &tokens, std::vector<token>::iterator &it);
  event_translator* parse_special_trans(enum entry_type intype, complex_expr* expr);
  advanced_event_translator* parse_adv_trans(const std::vector<std::string>& fields, std::vector<token> &rhs);
  bool parse_def(enum entry_type intype, MGTransDef& def, complex_expr* expr);
  static void print_def(enum entry_type intype, MGTransDef& def, std::ostream& output);
  static bool print_special_def(entry_type intype, MGTransDef& def, std::ostream& output);
  moltengamepad* mg;
private:
  event_translator* parse_trans_strict(enum entry_type intype, std::vector<token> &tokens, std::vector<token>::iterator &it);
  event_translator* parse_trans_expr(enum entry_type intype, complex_expr* expr);
  void do_assignment(std::string header, std::string field, std::vector<token> rhs);
  void do_adv_assignment(std::string header, const std::vector<std::string>& fields, std::vector<token> rhs);
  void do_assignment_line(std::vector<token> &line, std::string &header);
  void parse_line(std::vector<token> &line, std::string &header);
  event_translator* parse_trans_toplevel_quirks(enum entry_type intype, std::vector<token> &tokens, std::vector<token>::iterator &it);
  
};




struct complex_expr* read_expr(std::vector<token> &tokens, std::vector<token>::iterator &it);
void free_complex_expr(complex_expr* expr);


int do_command(moltengamepad* mg,std::vector<token> &command);

int shell_loop(moltengamepad* mg, std::istream &in);

bool find_token_type(enum tokentype type, std::vector<token> &tokens);

void do_header_line(std::vector<token> &line, std::string &header);

#endif