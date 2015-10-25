
#include "parser.h"
#include "event_change.h"

event_translator* parse_simple_trans(enum entry_type intype, const char* outname);
event_translator* parse_special_trans(enum entry_type intype, std::vector<token> &rhs);
event_translator* parse_complex_trans(enum entry_type intype, std::vector<token> &srhs);

void print_tokens(std::vector<token> &tokens) {
  for (auto it = tokens.begin(); it != tokens.end(); it++) {
    std::cout << (*it).type << (*it).value << " ";
  }
  std::cout << std::endl;
}

bool isIdent(char c) {
  return isalnum(c) || c == '_' || c == '-' || c == '+';
}

std::vector<token> tokenize(std::string line) {
  std::string temp;
  std::vector<token> tokens;
  bool quotemode = false;
  bool escaped = false;
  for (auto it = line.begin(); it != line.end(); it++) {
    char c = *it;
    
    if (quotemode) {
      if (escaped && ( c == '\"' || c == '\\')) {
        temp.push_back(c);
        escaped = false;
        continue;
      }
      if (c == '\\' && !escaped ) {
        escaped = true;
        continue;
      }
      if (c == '\"' && !escaped) {
        quotemode = false;
        tokens.push_back({TK_IDENT,std::string(temp)});
        temp.clear();
        continue;
      }
      temp.push_back(c);
      continue;
    }
    
    if (temp.empty() && c == '\"') {
      quotemode = true;
      continue;
    }
    
    
    if (!isIdent(c) && !temp.empty()) {
      tokens.push_back({TK_IDENT,std::string(temp)});
      temp.clear();
    }
    
    if (c == '#') { //Comment. Stop reading this line.
      break;
    }
    if (c == '\n') {
      tokens.push_back({TK_ENDL,"\\n"});
    }
    if (c == '.') {
      tokens.push_back({TK_DOT,"."});
    }
    if (c == '=') {
      tokens.push_back({TK_EQUAL,"="});
    }
    if (c == '[') {
      tokens.push_back({TK_HEADER_OPEN,"["});
    }
    if (c == ']') {
      tokens.push_back({TK_HEADER_CLOSE,"]"});
    }
    if (c == '(') {
      tokens.push_back({TK_LPAREN,"("});
    }
    if (c == ')') {
      tokens.push_back({TK_RPAREN,")"});
    }
    if (c == ',') {
      tokens.push_back({TK_COMMA,","});
    }
    
    if (isIdent(c)) {
      temp.push_back(c);
    }
  }
  
  if (!temp.empty()) {
      tokens.push_back({TK_IDENT,std::string(temp)});
      temp.clear();
  }
  tokens.push_back({TK_ENDL,"\\n"});
  
  
  return tokens;
}

/*Want a modified INI syntax.
 * Three main line formats.
 * 
 * [header]
 * 
 * this = that
 * 
 * do this
 * 
 * the square brackets or equals signs uniquely identify the first two formats.
 * 
 * The third format is arbitrary shell-style 
 *   command arg1 arg2 ...
 * 
 * The extra complication is in letting the right-hand side of
 *    this = that
 * grow complicated and nested, like redirect(mouse_slot, btn2btn(BTN_1))
 */
bool find_token_type(enum tokentype type, std::vector<token> &tokens) {
  for (auto it = tokens.begin(); it != tokens.end(); it++) {
    if ((*it).type == type) return true;
    if ((*it).type == TK_ENDL) return false;
  }
  return false;
  
}

void do_header_line(std::vector<token> &line, std::string &header) {
  if (line.empty()) return;
  if (line.at(0).type != TK_HEADER_OPEN) return;
  std::string newheader;
  for (auto it = ++line.begin(); it != line.end(); it++) {
    
    if ((*it).type == TK_HEADER_OPEN) return; //abort.
    
    if ((*it).type == TK_HEADER_CLOSE) {
      if (newheader.empty()) return;
      header = newheader;
      return;
    }
    if (!newheader.empty()) newheader.push_back(' ');
    newheader += (*it).value;
    
  }
}

void do_assignment(moltengamepad* mg, std::string header, std::string field, std::vector<token> rhs) {
  enum entry_type left_type = NO_ENTRY;
  const char* entry = field.c_str();
  device_manager* man = mg->find_manager(header.c_str());
  input_source* dev = nullptr;
  if (man != nullptr) {
    left_type = man->entry_type(entry);
  } else {
    dev =mg->find_device(header.c_str());
    if (dev != nullptr) {
    left_type = dev->entry_type(entry);
    }
  }
  
  if (left_type == DEV_KEY || left_type == DEV_AXIS) {
    event_translator* trans = parse_special_trans(left_type, rhs);
    if (!trans) trans = parse_simple_trans(left_type, rhs.front().value.c_str());
    if (!trans) trans = parse_complex_trans(left_type, rhs);
    
    if (!trans) return; //abort.it++;
    if (trans) { std::cout << "parse to " << trans->to_string() << std::endl;}
    if (man) man->update_maps(entry,trans);
    if (dev) dev->update_map(entry,trans);
    
    if (trans) delete trans;
  }
  
}

void do_assignment_line(std::vector<token> &line, std::string &header, moltengamepad* mg) {
  std::string effective_header = "";
  std::string effective_field;
  std::vector<token> leftside;
  std::vector<token> rightside;
  std::string* field = &effective_header;
  bool seen_dot = 0;
  bool seen_field = 0;
  
  auto it = line.begin();
  
  /* Only valid left hand sides:
   * 
   * HEADER DOT FIELD
   * FIELD
   * DOT FIELD
   * 
   * We start by reading an IDENT into effective_header.
   * If we see a dot, we switch to trying to read
   *   an IDENT into effective_field.
   * Afterwards, if there was no dot, then
   * our header was implicit, and we need to move
   * the read value into effective_field.
   */
  for (; it != line.end() && (*it).type != TK_EQUAL; it++) {
    
    if ((*it).type == TK_DOT) {
      if (seen_dot) return;
      seen_dot = true;
      field = &effective_field;
      seen_field = false;
      
    } else if ((*it).type == TK_IDENT) {
      if (seen_field) return;
      *field = (*it).value;
      seen_field = true;
      
    } else {
      return; //abort.
    }
      
  }
  
  if (!seen_dot) {
    effective_field = effective_header;
    effective_header = header;
  }
  
  
  if (effective_header.empty()) effective_header = "moltengamepad";
  
  if (it == line.end()) return; //Shouldn't happen.
  
  it++; //Skip past the "="
  
  for (; it != line.end() && (*it).type != TK_ENDL; it++) {
    rightside.push_back(*it);
  }
  
  
  if (effective_field.empty() || rightside.empty()) return;
  
  
  do_assignment(mg, effective_header, effective_field, rightside);
  
  
  
}

void parse_line(std::vector<token> &line, std::string &header, moltengamepad* mg) {
 
  if (find_token_type(TK_HEADER_OPEN,line)) {
    do_header_line(line, header);
    std::cout << "header is " << header << std::endl;
    return;
  }
  
  if (find_token_type(TK_EQUAL, line)) {
    do_assignment_line(line, header, mg);
    return;
  }
  
  do_command(mg,line);
}


event_translator* parse_simple_trans(enum entry_type intype, const char* outname) {
  int direction = 1;
  if (outname[0] == '-')  {
    direction = -1;
    outname++;
  } else if (outname[0] == '+') {
    outname++;
  }
  struct event_info out_info = lookup_event(outname);
  
  enum out_type outtype = out_info.type;
  
  
  if (intype == DEV_KEY) {
    if (outtype == OUT_KEY)
      return new btn2btn(out_info.value);
    if (outtype == OUT_ABS)
      return new btn2axis(out_info.value, direction);
  }
  
  if (intype == DEV_AXIS) {
    if (outtype == OUT_ABS)
      return new axis2axis(out_info.value,direction);
  }
  
  return nullptr;
}

event_translator* parse_special_trans(enum entry_type intype, std::vector<token> &rhs) {
  if (intype == DEV_AXIS) {
    //Check for two buttons "neg,pos" for mapping the axis.
    //We want exactly three tokens: TK_IDENT TK_COMMA TK_IDENT
    if (rhs.size() == 3 && rhs.at(1).type == TK_COMMA) {
      struct event_info neg = lookup_event(rhs.at(0).value.c_str());
      struct event_info pos = lookup_event(rhs.at(2).value.c_str());
      if (neg.type == OUT_KEY && pos.type == OUT_KEY)
        return new axis2btns(neg.value,pos.value);
    }
  }
  
  return nullptr;
}


void free_complex_expr(complex_expr* expr) {
  if (!expr) return;
  for (auto it = expr->params.begin(); it != expr->params.end(); it++) {
    if ((*it)) free_complex_expr(*it);
  }
  delete expr;
}

void print_expr(complex_expr* expr, int depth) {
  for (int i = 0; i < depth; i++) std::cout <<" ";
  if (!expr) { 
    std::cout <<"(null expr)" << std::endl;
    return;
  }
  
  std::cout << expr->ident << std::endl;
  for (auto it = expr->params.begin(); it != expr->params.end(); it++) {
    print_expr((*it),depth+1);
  }
  
}

struct complex_expr* read_expr(std::vector<token> &tokens, std::vector<token>::iterator &it) {
  if (tokens.empty() || it == tokens.end()) return nullptr;
  
  bool abort = false;
  
  if ((*it).type == TK_IDENT) {
    complex_expr* expr = new complex_expr;
    expr->ident = (*it).value;
    
    it++;
    
    if (it == tokens.end()) return expr;
    
    if ((*it).type == TK_LPAREN) {
      it++;
      complex_expr* subexpr = read_expr(tokens,it);
      expr->params.push_back(subexpr);
      while (it != tokens.end() && (*it).type == TK_COMMA) {
        it++;
        subexpr = read_expr(tokens,it);
        expr->params.push_back(subexpr);
      }
      
      
      if ((*it).type == TK_RPAREN) {
        it++;
        return expr;
      }
    } else {
      return expr;
    }
        
    free(expr); //failed to parse.
  }
  
 
    
  return nullptr;
}

int read_ev_code(std::string &code) {
  int i;
  try {
    i = std::stoi(code);
    return i;
  } catch (...) {
    event_info info = lookup_event(code.c_str());
    if (info.type != OUT_NONE) return info.value;
  }
  return -1;
}
  
event_translator* expr_btn2btn(complex_expr* expr) {
 if (!expr) return nullptr;
 if (expr->params.size() != 1) return nullptr;
 int i = read_ev_code(expr->params.at(0)->ident);
 if (i >= 0) return new btn2btn(i);
 return nullptr;
}

event_translator* expr_btn2axis(complex_expr* expr) {
 if (!expr) return nullptr;
 if (expr->params.size() != 2) return nullptr;
 int i = read_ev_code(expr->params.at(0)->ident);
 int dir = read_ev_code(expr->params.at(1)->ident);
 if (i >= 0) return new btn2axis(i, dir);
 return nullptr;
}

event_translator* expr_axis2axis(complex_expr* expr) {
 if (!expr) return nullptr;
 if (expr->params.size() != 2) return nullptr;
 int i = read_ev_code(expr->params.at(0)->ident);
 int dir = read_ev_code(expr->params.at(1)->ident);
 if (i >= 0) return new axis2axis(i, dir);
 return nullptr;
}

event_translator* expr_axis2btns(complex_expr* expr) {
 if (!expr) return nullptr;
 if (expr->params.size() != 2) return nullptr;
 int i = read_ev_code(expr->params.at(0)->ident);
 int j = read_ev_code(expr->params.at(1)->ident);
 if (i >= 0 && j >= 0) return new axis2btns(i, j);
 return nullptr;
}

event_translator* expr_to_trans(complex_expr* expr) {
  if (expr->ident == "btn2btn") return expr_btn2btn(expr);
  if (expr->ident == "btn2axis") return expr_btn2axis(expr);
  if (expr->ident == "axis2axis") return expr_axis2axis(expr);
  if (expr->ident == "axis2btns") return expr_axis2btns(expr);
  
  return nullptr;
}

event_translator* parse_complex_trans(enum entry_type intype, std::vector<token> &rhs) {
  
  auto it = rhs.begin();
  complex_expr* expr = read_expr(rhs,it);
  
  
  
  int depth = 0;
  complex_expr* current = expr;
  std::cout << "PARSED COMPLEX" << std::endl;
  print_expr(expr,0);
  
  if (!expr) return nullptr;
  
  event_translator* trans = expr_to_trans(expr);
  
  free_complex_expr(expr);
  
  return trans; 
}



