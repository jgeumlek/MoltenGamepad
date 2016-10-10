#ifndef PARSER_H
#define PARSER_H
#include <string>
#include <vector>
#include <iostream>
#include <map>
#include <sstream>
#include <functional>
#include "moltengamepad.h"

enum tokentype { TK_IDENT, TK_DOT, TK_EQUAL, TK_HEADER_OPEN, TK_HEADER_CLOSE, TK_LPAREN, TK_RPAREN, TK_COMMA, TK_SLASH, TK_COLON, TK_VALUE, TK_ENDL};

struct token {
  enum tokentype type;
  std::string value;
};

std::vector<token> tokenize(std::string line);

struct complex_expr {
  std::string ident;
  std::vector<complex_expr*> params;
};


class trans_generator {
public:
  const MGType* fields;
  std::function<event_translator* (std::vector<MGField>&)> generate;
  trans_generator(const MGType* fields, std::function<event_translator* (std::vector<MGField>&)> generate) : fields(fields), generate(generate) {};
  trans_generator() : fields(nullptr) {};
};

class MGparser {
public:
  MGparser(moltengamepad* mg, message_protocol* output);
  void exec_line(std::vector<token>& line, std::string& header);
  static event_translator* parse_trans(enum entry_type intype, std::vector<token>& tokens, std::vector<token>::iterator& it);
  static event_translator* parse_special_trans(enum entry_type intype, complex_expr* expr);
  static advanced_event_translator* parse_adv_trans(const std::vector<std::string>& fields, std::vector<token>& rhs);
  static bool parse_def(enum entry_type intype, MGTransDef& def, complex_expr* expr);
  static void print_def(enum entry_type intype, MGTransDef& def, std::ostream& output);
  static bool print_special_def(entry_type intype, MGTransDef& def, std::ostream& output);
  static void load_translators(moltengamepad* mg);
  static moltengamepad* mg;
private:
  static event_translator* parse_trans_strict(enum entry_type intype, std::vector<token>& tokens, std::vector<token>::iterator& it);
  static event_translator* parse_trans_expr(enum entry_type intype, complex_expr* expr);
  void do_assignment(std::string header, std::string field, std::vector<token> rhs);
  void do_adv_assignment(std::string header, std::vector<std::string>& fields, std::vector<token> rhs);
  void do_assignment_line(std::vector<token>& line, std::string& header);
  void parse_line(std::vector<token>& line, std::string& header);
  static event_translator* parse_trans_toplevel_quirks(enum entry_type intype, std::vector<token>& tokens, std::vector<token>::iterator& it);
  static std::map<std::string,trans_generator> trans_gens;
  message_stream out;
};




struct complex_expr* read_expr(std::vector<token>& tokens, std::vector<token>::iterator& it);
void free_complex_expr(complex_expr* expr);




int do_command(moltengamepad* mg, std::vector<token>& command, message_stream* out);

int shell_loop(moltengamepad* mg, std::istream& in);

bool find_token_type(enum tokentype type, std::vector<token>& tokens);

void do_header_line(std::vector<token>& line, std::string& header);

#endif
