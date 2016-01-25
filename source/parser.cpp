
#include "parser.h"
#include "event_change.h"

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

void print_tokens(std::vector<token> &tokens) {
  for (auto it = tokens.begin(); it != tokens.end(); it++) {
    std::cout << (*it).type << (*it).value << " ";
  }
  std::cout << std::endl;
}

bool isIdent(char c) {
  return isalnum(c) || c == '_' || c == '-' || c == '+' || c == '?' ;
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

void MGparser::do_assignment(std::string header, std::string field, std::vector<token> rhs) {
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
    auto it = rhs.begin();
    event_translator* trans = parse_trans(left_type, rhs,it);
    
    if (!trans) {
      std::cout << "Parsing the right-hand side failed." << std::endl;
      return; //abort
    }
    if (trans) { 
      std::stringstream ss;
      MGTransDef def;
      trans->fill_def(def);
      print_def(left_type,def,ss);
      std::cout << "parse to " << ss.str() << std::endl;
    }
    
    if (man) man->update_maps(entry,trans);
    if (dev) dev->update_map(entry,trans);
    
    if (trans) delete trans;
  }
  
  if (rhs.empty()) return;
  
  if (field.front() == '?') {
    field.erase(field.begin());
    if (man) man->update_options(field.c_str(),rhs.front().value.c_str());
    if (dev) dev->update_option(field.c_str(),rhs.front().value.c_str());
  }
  
}



void MGparser::do_adv_assignment(std::string header, const std::vector<std::string>& fields, std::vector<token> rhs) {
  if (rhs.empty()) return;
  device_manager* man = mg->find_manager(header.c_str());
  input_source* dev = (!man) ? mg->find_device(header.c_str()) : nullptr;
  
  if (!dev && !man) return;
  
  if (true) {
    if (rhs.front().value == "nothing") {
      if (dev) dev->update_advanced(fields,nullptr);
      if (man) man->update_advanceds(fields,nullptr);
      return;
    }
    advanced_event_translator* trans = parse_adv_trans(fields,rhs);
    if (!trans) return; //Abort
    if (trans) { 
      std::stringstream ss;
      MGTransDef def;
      trans->fill_def(def);
      print_def(DEV_KEY,def,ss);
      std::cout << "parse to " << ss.str() << std::endl;
    }
    
    if (dev) dev->update_advanced(fields, trans);
    if (man) man->update_advanceds(fields, trans);
    
    if (trans) delete trans;
  }
  
  if (rhs.empty()) return;
  
}

void MGparser::do_assignment_line(std::vector<token> &line, std::string &header) {
  std::string effective_header = "";
  std::string effective_field;
  std::string chord1 = "";
  std::string chord2 = "";
  std::vector<std::string> multifield;
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
   * FIELD may actually be a tuple, that is valid as well.
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
      
    } else if ((*it).type == TK_LPAREN) {
      do {
        it++;
        if (it == line.end()) return;
        multifield.push_back(it->value);
        it++;
      } while (it != line.end() && it->type == TK_COMMA);
      
      if (it == line.end() || it->type != TK_RPAREN) return;
      it++;
      if (it == line.end() || it->type != TK_EQUAL) return;
      break;
      
      
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
  
  if (multifield.size() > 0) {
    do_adv_assignment(effective_header, multifield, rightside);
    return;
  }
  
  if (effective_field.empty() || rightside.empty()) return;
  
  
  do_assignment(effective_header, effective_field, rightside);
  
  
  
}

void MGparser::parse_line(std::vector<token> &line, std::string &header) {
 
  if (find_token_type(TK_HEADER_OPEN,line)) {
    do_header_line(line, header);
    std::cout << "header is " << header << std::endl;
    return;
  }
  /*process the command, prevent input_sources from being deleted under our nose.*/
  if (find_token_type(TK_EQUAL, line)) {
    device_delete_lock.lock();
    do_assignment_line(line, header);
    device_delete_lock.unlock();
  } else {
    do_command(mg,line);
  }
  
}

void MGparser::exec_line(std::vector<token> &line, std::string &header) {
  parse_line(line,header);
}

event_translator* MGparser::parse_trans(enum entry_type intype, std::vector<token> &tokens, std::vector<token>::iterator &it) {
  //Note: this function is assumed to be called at the top of parsing a translator,
  //not as some recursive substep. If we wish to avoid the quirks, use parse_complex_trans instead.
  event_translator* trans = parse_trans_toplevel_quirks(intype,tokens,it);
  if (trans) return trans;
  trans = parse_complex_trans(intype, tokens, it);
  
  return trans;
}

event_translator* MGparser::parse_trans_toplevel_quirks(enum entry_type intype, std::vector<token> &tokens, std::vector<token>::iterator &it) {
  //For backwards compatibility/ease of use, there is some automagic hard to fit in elsewhere.
  //Namely, the automagic "btn,btn" parsing of axis2btns. Without a leading parenthesis,
  //this structure is hard to detect as an expression.
  //Allowing a top level expression to include a comma would make the parse descent ambiguous.
  //So instead, it ends up here.
  
  if (it != tokens.begin()) return nullptr; //Not the toplevel? Abort?
  
  //This is very heavy on formatting, a button, a comma, a button. 
  if (intype == DEV_AXIS && tokens.size() == 3 && tokens[1].type == TK_COMMA) {
    complex_expr* expr = new complex_expr;
    expr->ident = "";
    expr->params.push_back(new complex_expr);
    expr->params.push_back(new complex_expr);
    expr->params[0]->ident = tokens[0].value;
    expr->params[1]->ident = tokens[2].value;
    event_translator* trans = parse_trans_expr(DEV_AXIS,expr);
    free_complex_expr(expr);
    return trans;
  }
  return nullptr;
}
    

event_translator* MGparser::parse_complex_trans(enum entry_type intype, std::vector<token> &tokens, std::vector<token>::iterator &it) {
  auto localit = it;
  complex_expr* expr = read_expr(tokens, localit);
  event_translator* trans =  parse_trans_expr(intype, expr);
  free_complex_expr(expr);
  it = localit;
  return trans;
  
}

event_translator* build_from_def(MGTransDef& def) {
  if (def.identifier == "btn2btn") return new btn2btn(def.fields);
  if (def.identifier == "btn2axis") return new btn2axis(def.fields);
  if (def.identifier == "axis2axis") return new axis2axis(def.fields);
  if (def.identifier == "axis2btns") return new axis2btns(def.fields);
  if (def.identifier == "redirect") return new redirect_trans(def.fields);
  if (def.identifier == "key") return new keyboard_redirect(def.fields);
  return nullptr;
}

void release_def(MGTransDef& def) {
  for (auto entry : def.fields) {
    if (entry.type == MG_TRANS || entry.type == MG_KEY_TRANS || entry.type == MG_AXIS_TRANS || entry.type == MG_REL_TRANS)
      delete entry.trans;
    if (entry.type == MG_ADVANCED_TRANS)
      delete entry.adv_trans;
    if (entry.type == MG_STRING)
      delete entry.string;
  }
}
  

event_translator* MGparser::parse_trans_expr(enum entry_type intype, complex_expr* expr) {
  if (!expr) return nullptr;
  
  event_translator* trans = parse_special_trans(intype, expr);
  if (trans) return trans;
  
  const MGType (*fields) = nullptr;
  
  if (expr->ident == "btn2btn") fields = btn2btn::fields;
  if (expr->ident == "btn2axis") fields = btn2axis::fields;
  if (expr->ident == "axis2axis") fields = axis2axis::fields;
  if (expr->ident == "axis2btns") fields = axis2btns::fields;
  if (expr->ident == "redirect") fields = redirect_trans::fields;
  if (expr->ident == "key") fields = keyboard_redirect::fields;
  
  if (!fields) return nullptr;
  
  MGTransDef def;
  def.identifier = expr->ident;
  
  for (int i = 0; (fields)[i] != MG_NULL; i++) {
    def.fields.push_back({(fields)[i],0});
  }
  
  if (!parse_def(intype, def,expr)) return nullptr;

  
  //still need to build it!
  trans = build_from_def(def);
  release_def(def);
  return trans;
}


int read_ev_code(std::string &code, out_type type) {
  int i;
  try {
    i = std::stoi(code);
    return i;
  } catch (...) {
    if (type == OUT_NONE) return 0;
    event_info info = lookup_event(code.c_str());
    if (info.type == type) return info.value;
  }
  return -1;
}

//Handles those automagic simple cases.
event_translator* MGparser::parse_special_trans(enum entry_type intype, complex_expr* expr) {
  if (!expr) return nullptr;
  
  if (expr->ident == "nothing") return new event_translator();
  
  //Key for a key.
  if (intype == DEV_KEY && expr->params.size() == 0) {
    int out_button = read_ev_code(expr->ident, OUT_KEY);
    if (out_button >= 0) return new btn2btn(out_button);
  }
  
  //Axis for an axis or key.
  if ((intype == DEV_AXIS || intype == DEV_KEY) && expr->params.size() == 0) {
    std::string axisname = expr->ident;
    int direction = 1;
    if (axisname.size() > 0) {
      if (axisname[0] == '+') {
        axisname.erase(axisname.begin());
      }
      if (axisname[0] == '-') {
        axisname.erase(axisname.begin());
        direction = -1;
      }
      int out_axis = read_ev_code(axisname, OUT_ABS);
      if (out_axis >= 0 && intype == DEV_AXIS) return new axis2axis(out_axis,direction);
      if (out_axis >= 0 && intype == DEV_KEY) return new btn2axis(out_axis,direction);
    }
  }
  
  //Axis to buttons.
  if ((intype == DEV_AXIS) && expr->ident.empty() && expr->params.size() == 2) {
    int neg_btn = read_ev_code(expr->params[0]->ident,OUT_KEY);
    int pos_btn = read_ev_code(expr->params[1]->ident,OUT_KEY);
    if (neg_btn >= 0 && pos_btn >= 0) return new axis2btns(neg_btn,pos_btn);
  }
  
  return nullptr;
}
  

bool MGparser::parse_def(enum entry_type intype, MGTransDef& def, complex_expr* expr) {
  if (!expr) return false;
  if (def.identifier != expr->ident) return false;
  int fieldsfound = expr->params.size();
  std::cout<<"Parse def of " << def.fields.size() << " given " << fieldsfound << std::endl;
  int j = 0;
  for (int i = 0; i < def.fields.size(); i++) {
    MGType type = def.fields[i].type;
    if (type == MG_KEYBOARD_SLOT) {def.fields[i].slot = mg->slots->keyboard; continue;};
    if (j >= fieldsfound) return false;
    
    std::cout<<"Parsing type " << type << " at " << expr->params[j]->ident << std::endl;
    if (type == MG_TRANS) def.fields[i].trans = parse_trans_expr(intype, expr->params[j]);
    if (type == MG_KEY_TRANS) def.fields[i].trans = parse_trans_expr(DEV_KEY, expr->params[j]);
    if (type == MG_REL_TRANS) def.fields[i].trans = parse_trans_expr(DEV_REL, expr->params[j]);
    if (type == MG_AXIS_TRANS) def.fields[i].trans = parse_trans_expr(DEV_AXIS, expr->params[j]);
    if (type == MG_KEY) def.fields[i].key = read_ev_code(expr->params[j]->ident, OUT_KEY);
    if (type == MG_REL) def.fields[i].rel = read_ev_code(expr->params[j]->ident, OUT_REL);
    if (type == MG_AXIS) def.fields[i].axis = read_ev_code(expr->params[j]->ident, OUT_ABS);
    if (type == MG_INT) def.fields[i].integer = read_ev_code(expr->params[j]->ident, OUT_NONE);
    if (type == MG_SLOT) {
      virtual_device* slot = mg->slots->find_slot(expr->params[j]->ident);
      if (!slot) return false;
      def.fields[i].slot = slot;
    }
    if (type == MG_STRING) def.fields[i].string = new std::string(expr->params[j]->ident);
    //TODO: float
    j++;
  }
  
  return true;
}

void MGparser::print_def(entry_type intype, MGTransDef& def, std::ostream& output) {
  //Check for the possibility of some automagic.
  if (print_special_def(intype, def, output)) return;
  
  output << def.identifier;
  if (def.fields.size() > 0) output << "(";
  bool needcomma = false;
  for (auto field : def.fields) {
    if (needcomma) output << ",";
    MGType type = field.type;
    if (type == MG_KEY) {
      const char* name = get_key_name(field.key);
      if (name) {
        output << name;
      } else {
        output << field.key;
      }
    }
    if (type == MG_AXIS) {
      const char* name = get_axis_name(field.axis);
      if (name) {
        output << name;
      } else {
        output << field.axis;
      }
    }
    if (type == MG_REL) {
      const char* name = get_rel_name(field.rel);
      if (name) {
        output << name;
      } else {
        output << field.rel;
      }
    }
    if (type == MG_INT) output << field.integer;
    if (type == MG_FLOAT) output << field.real;
    if (type == MG_STRING) output << *(field.string);
    if (type == MG_SLOT) output << field.slot->name;
    if (type == MG_TRANS || type == MG_KEY_TRANS || type == MG_AXIS_TRANS || type == MG_REL_TRANS) {
      MGTransDef innerdef;
      entry_type context = intype;
      if (type == MG_KEY_TRANS) context = DEV_KEY;
      if (type == MG_AXIS_TRANS) context = DEV_AXIS;
      if (type == MG_REL_TRANS) context = DEV_REL;
      field.trans->fill_def(innerdef);
      print_def(context,innerdef,output);
    }
    needcomma = true;
  }
  if (def.fields.size() > 0) output << ")";
}

bool MGparser::print_special_def(entry_type intype, MGTransDef& def, std::ostream& output) {
  //Check if we are in a setting where some magic can be applied.
  if (intype == DEV_KEY) {
    if (def.identifier == "btn2btn" && def.fields.size() > 0 && def.fields[0].type == MG_KEY) {
      const char* name = get_key_name(def.fields[0].key);
      if (name) {
        output << name;
      } else {
        output << def.fields[0].key;
      }
      return true;
    }
  }
  if ((intype == DEV_KEY && def.identifier == "btn2axis") || (intype == DEV_AXIS && def.identifier == "axis2axis")) {
    if (def.fields.size() >= 2 && def.fields[0].type == MG_AXIS && def.fields[1].type == MG_INT) {
      const char* name = get_axis_name(def.fields[0].axis);
      const char* prefix = "";
      if (def.fields[1].integer == -1) {
        prefix = "-";
      }
      if (def.fields[1].integer == +1) {
        prefix = "+";
      }
      if (!prefix) return false;
      if (!name) return false;
      output << prefix << name;
      return true;
    }
  }
  if (intype == DEV_AXIS && def.identifier == "axis2btns" && def.fields.size() >= 2 && def.fields[0].type == MG_KEY && def.fields[1].type == MG_KEY) {
    const char* nameneg = get_key_name(def.fields[0].key);
    const char* namepos = get_key_name(def.fields[1].key);
    output << "(";
    if (nameneg) output << nameneg;
    if (!nameneg) output << def.fields[0].key;
    output << ",";
    if (namepos) output << namepos;
    if (!namepos) output << def.fields[1].key;
    output << ")";
    return true;
  }
  
  return false;
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
  
  
  if ((*it).type == TK_IDENT || (*it).type == TK_LPAREN) {
    complex_expr* expr = new complex_expr;
    //If we have ident, read it in.
    //Otherwise, we have a paren, start reading children and leave the ident empty.
    if ((*it).type == TK_IDENT) {
      expr->ident = (*it).value;
      it++;
    }
    
    if (it == tokens.end()) return expr;
    
    if ((*it).type == TK_LPAREN) {
      it++;
      complex_expr* subexpr = read_expr(tokens,it);
      if (subexpr) expr->params.push_back(subexpr);
      while (it != tokens.end() && (*it).type == TK_COMMA) {
        it++;
        subexpr = read_expr(tokens,it);
        if (subexpr) expr->params.push_back(subexpr);
      }
      
      
      if (it != tokens.end() && (*it).type == TK_RPAREN) {
        it++;
        return expr;
      }
    } else {
      return expr;
    }
        
    free_complex_expr(expr); //failed to parse.
  }
  
 
    
  return nullptr;
}

advanced_event_translator* build_adv_from_def(const std::vector<std::string>& event_names, MGTransDef& def) {
  if (def.identifier == "simple") return new simple_chord(event_names,def.fields);
  if (def.identifier == "exclusive") return new exclusive_chord(event_names,def.fields);
  return nullptr;
}

advanced_event_translator* MGparser::parse_adv_trans(const std::vector<std::string>& event_names, std::vector<token> &rhs) {
  auto it = rhs.begin();
  event_translator* trans = parse_trans(DEV_KEY, rhs, it);
  if (trans) return new simple_chord(event_names,trans);
  
  it = rhs.begin();
  complex_expr* expr = read_expr(rhs,it);
  
  if (!expr) return nullptr;
  
  if (expr->params.empty()) return nullptr;
  
  advanced_event_translator* adv_trans = nullptr;
  
  const MGType (*fields) = nullptr;
  
  if (expr->ident == "simple") fields = simple_chord::fields;
  if (expr->ident == "exclusive") fields = exclusive_chord::fields;
  
  if (!fields) return nullptr;
  
  MGTransDef def;
  def.identifier = expr->ident;
  
  for (int i = 0; (fields)[i] != MG_NULL; i++) {
    def.fields.push_back({(fields)[i],0});
  }
  
  if (!parse_def(DEV_KEY, def,expr)) return nullptr;

  
  //still need to build it!
  adv_trans = build_adv_from_def(event_names,def);
  release_def(def);
  
  free_complex_expr(expr);

  return adv_trans;
}
