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

//A nested expression to handle parenthesized lists.
//ex. redirect(btn2btn(key_w),keyboard)
struct complex_expr {
  std::string ident;
  std::string name;
  std::vector<complex_expr*> params;
};

struct named_field {
  std::string name;
  std::string default_val;
  MGType type;
  bool has_default;
  bool repeating;
};

//Captures a translator declaration, so that we may parse it.
//ex. key = btn2btn(key_code, int direction=1)
struct trans_decl {
  const char* decl_str;
  std::vector<entry_type> mapped_events;
  bool variadic_mapped_events;
  std::string identifier;
  std::vector<named_field> fields;
};

class trans_generator {
public:
  trans_decl decl;
  std::function<event_translator* (std::vector<MGField>&)> generate;
  std::function<advanced_event_translator* (std::vector<MGField>&)> adv_generate;
  trans_generator(trans_decl decl, std::function<event_translator* (std::vector<MGField>&)> generate) : decl(decl), generate(generate) {};
  trans_generator(trans_decl decl, std::function<advanced_event_translator* (std::vector<MGField>&)> adv_generate) : decl(decl), adv_generate(adv_generate) {};
  trans_generator() {};
};

trans_decl build_trans_decl(const char* decl_str);

class MGparser {
public:
  MGparser(moltengamepad* mg, message_protocol* output);
  void exec_line(std::vector<token>& line, std::string& header, int resp_id);
  static event_translator* parse_trans(enum entry_type intype, std::vector<token>& tokens, std::vector<token>::iterator& it, response_stream* out);
  static event_translator* parse_special_trans(enum entry_type intype, complex_expr* expr);
  static advanced_event_translator* parse_adv_trans(const std::vector<std::string>& fields, std::vector<token>& rhs, response_stream* out);
  static bool parse_decl(enum entry_type intype, const trans_decl& decl, MGTransDef& def, complex_expr* expr, response_stream* out);
  static void print_def(enum entry_type intype, MGTransDef& def, std::ostream& output);
  static bool print_special_def(entry_type intype, MGTransDef& def, std::ostream& output);
  static void load_translators(moltengamepad* mg);
  static moltengamepad* mg;
private:
  static event_translator* parse_trans_strict(enum entry_type intype, std::vector<token>& tokens, std::vector<token>::iterator& it, response_stream* out);
  static event_translator* parse_trans_expr(enum entry_type intype, complex_expr* expr, response_stream* out);
  void do_assignment(std::string header, std::string field, std::vector<token> rhs, response_stream& out);
  void do_adv_assignment(std::string header, std::vector<std::string>& fields, std::vector<token> rhs, response_stream& out);
  void do_assignment_line(std::vector<token>& line, std::string& header, response_stream& out);
  void parse_line(std::vector<token>& line, std::string& header, response_stream& out);
  static event_translator* parse_trans_toplevel_quirks(enum entry_type intype, std::vector<token>& tokens, std::vector<token>::iterator& it);
  static std::map<std::string,trans_generator> trans_gens;
  message_stream messages;
};




struct complex_expr* read_expr(std::vector<token>& tokens, std::vector<token>::iterator& it);
void free_complex_expr(complex_expr* expr);




int do_command(moltengamepad* mg, std::vector<token>& command, response_stream* out);

int shell_loop(moltengamepad* mg, std::istream& in);

bool find_token_type(enum tokentype type, std::vector<token>& tokens);

bool do_header_line(std::vector<token>& line, std::string& header);

#endif
