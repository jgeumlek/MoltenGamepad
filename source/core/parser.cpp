
#include "parser.h"
#include "event_translators/event_change.h"
#include "event_translators/translators.h"
#include <cmath>

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

void print_tokens(std::vector<token>& tokens) {
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
      if (escaped && (c == '\"' || c == '\\')) {
        temp.push_back(c);
        escaped = false;
        continue;
      }
      if (c == '\\' && !escaped) {
        escaped = true;
        continue;
      }
      if (c == '\"' && !escaped) {
        quotemode = false;
        tokens.push_back({TK_IDENT, std::string(temp)});
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
      tokens.push_back({TK_IDENT, std::string(temp)});
      temp.clear();
    }

    if (c == '#') { //Comment. Stop reading this line.
      break;
    }
    if (c == '\n') {
      tokens.push_back({TK_ENDL, "\\n"});
    }
    if (c == '.') {
      tokens.push_back({TK_DOT, "."});
    }
    if (c == '=') {
      tokens.push_back({TK_EQUAL, "="});
    }
    if (c == '[') {
      tokens.push_back({TK_HEADER_OPEN, "["});
    }
    if (c == ']') {
      tokens.push_back({TK_HEADER_CLOSE, "]"});
    }
    if (c == '(') {
      tokens.push_back({TK_LPAREN, "("});
    }
    if (c == ')') {
      tokens.push_back({TK_RPAREN, ")"});
    }
    if (c == ',') {
      tokens.push_back({TK_COMMA, ","});
    }
    if (c == ':') {
      tokens.push_back({TK_COLON, ":"});
    }
    if (c == '/') {
      tokens.push_back({TK_SLASH, "/"});
    }

    if (isIdent(c)) {
      temp.push_back(c);
    }
  }

  if (!temp.empty()) {
    tokens.push_back({TK_IDENT, std::string(temp)});
    temp.clear();
  }
  tokens.push_back({TK_ENDL, "\\n"});


  return tokens;
}


bool find_token_type(enum tokentype type, std::vector<token>& tokens) {
  for (auto it = tokens.begin(); it != tokens.end(); it++) {
    if ((*it).type == type) return true;
    if ((*it).type == TK_ENDL) return false;
  }
  return false;

}

bool do_header_line(std::vector<token>& line, std::string& header) {
  if (line.empty()) return false;
  if (line.at(0).type != TK_HEADER_OPEN) return false;
  std::string newheader;
  for (auto it = ++line.begin(); it != line.end(); it++) {

    if ((*it).type == TK_HEADER_OPEN) return false; //abort.

    if ((*it).type == TK_HEADER_CLOSE) {
      if (newheader.empty()) return false;
      header = newheader;
      return true;
    }
    if (!newheader.empty()) newheader.push_back(' ');
    newheader += (*it).value;

  }
  return false;
}

//Some lazy macros to build some function pointers with associated name strings.
//TRANSGEN just associates an event_translator with its name.
//RENAME_TRANSGEN instead supplies a different name for the parser's sake.

#define TRANSGEN(X) trans_gens[#X] = trans_generator(X::fields,[] (std::vector<MGField>& fields) { return new X(fields);});
#define RENAME_TRANSGEN(name,X) trans_gens[#name] = trans_generator(X::fields,[] (std::vector<MGField>& fields) { return new X(fields);});
#define TGEN(X) [] (std::vector<MGField>& fields) { return new X(fields);}
#define MAKE_GEN(X) trans_gens[#X] = trans_generator(build_trans_decl(X::decl),TGEN(X));
#define RENAME_GEN(name,X) trans_gens[#name] = trans_generator(build_trans_decl(X::decl),TGEN(X));

//Need a static location for this array
MGType mouse_fields[] = {MG_TRANS, MG_NULL};

std::map<std::string,trans_generator> MGparser::trans_gens;
moltengamepad* MGparser::mg;

MGType parse_type(const std::string& str) {
  if (str == "trans")
    return MG_TRANS;
  if (str == "key_trans")
    return MG_KEY_TRANS;
  if (str == "axis_trans")
    return MG_AXIS_TRANS;
  if (str == "group_trans")
    return MG_GROUP_TRANS;
  if (str == "key_code")
    return MG_KEY;
  if (str == "axis_code")
    return MG_AXIS;
  if (str == "rel_code")
    return MG_REL;
  if (str == "string")
    return MG_STRING;
  if (str == "bool")
    return MG_BOOL;
  if (str == "int")
    return MG_INT;
  if (str == "slot")
    return MG_SLOT;
  if (str == "float")
    return MG_FLOAT;
  std::cerr << "error building translators, " << str << " was not a type." << std::endl;
  return MG_NULL;
}

std::string read_float(std::vector<token>::iterator& it, const std::vector<token>::iterator end) {
  //this function just builds up a string. It will be parsed to a float elsewhere.
  std::string output;
  if (it != end && it->type == TK_IDENT) {
    output += it->value;
    it++;
  }
  //allow for a dot
  if (it != end && it->type == TK_DOT) {
    output += it->value;
    it++;
    //allow for a portion after the dot.
    if (it != end && it->type == TK_IDENT) {
      output += it->value;
      it++;
    }
  }
  return output;
}
  
//ex. key = btn2btn(key_code, int direction=1)
trans_decl build_trans_decl(const char* decl_str) {
  std::vector<token> tokens = tokenize(std::string(decl_str));
  trans_decl parsed_decl;
  parsed_decl.variadic_mapped_events = false;
  parsed_decl.decl_str = decl_str;
  auto it = tokens.begin();
  while (it->value == "key" || it->value == "axis" || it->value == "event") {
    if (it->value == "key")
      parsed_decl.mapped_events.push_back(DEV_KEY);
    if (it->value == "axis")
      parsed_decl.mapped_events.push_back(DEV_AXIS);
    if (it->value == "event")
      parsed_decl.mapped_events.push_back(DEV_ANY);
    it++;
    if (it->type == TK_COMMA)
      it++;
  }
  //detect "[]"
  if (it->type == TK_HEADER_OPEN) {
    it++;
    if (it->type == TK_HEADER_CLOSE) {
      parsed_decl.variadic_mapped_events = true;
      it++;
    }
  }
  if (it->type == TK_EQUAL)
    it++;
  parsed_decl.identifier  = it->value;
  it++;
  if (it->type == TK_LPAREN)
    it++;
  while(it->type != TK_RPAREN) {
    MGType type = parse_type(it->value);
    std::string name = "";
    std::string default_value = "";
    bool has_default = false;
    bool repeating = false;
    it++;
    //check for [] after type name to denote a variable-length parameter.
    //i.e. this parameter can be repeated. Similar to var_args, but it needs to be a single type.
    if (it->type == TK_HEADER_OPEN) {
      it++;
      if (it->type == TK_HEADER_CLOSE) {
        it++;
        repeating = true;
      }
    }
    if (it->type == TK_IDENT) {
      name = it->value;
      it++;
    }
    if (it->type == TK_EQUAL) {
      it++;
      default_value = it->value;
      if (type == MG_FLOAT) {
        default_value = read_float(it, tokens.end());
      } else {
        it++;
      }
      has_default = true;
    }
    if (it->type == TK_COMMA)
      it++;
    parsed_decl.fields.push_back({name,default_value,type,has_default,repeating});
  }

  return parsed_decl;

}

void MGparser::load_translators(moltengamepad* mg) {
  MGparser::mg = mg;
  MAKE_GEN(btn2btn);
  MAKE_GEN(btn2axis);
  MAKE_GEN(axis2axis);
  MAKE_GEN(axis2btns);
  MAKE_GEN(btn2rel);
  MAKE_GEN(axis2rel);
  RENAME_GEN(redirect,redirect_trans);
  RENAME_GEN(multi,multitrans);
  //add a quick mouse redirect
  trans_decl mouse_decl;
  mouse_decl.identifier = "mouse";
  mouse_decl.decl_str = "event = mouse(trans)";
  mouse_decl.mapped_events.push_back(DEV_KEY);
  mouse_decl.fields.push_back({"","",MG_TRANS});
  trans_gens["mouse"] = trans_generator( mouse_decl, [mg] (std::vector<MGField>& fields) {
    //Need to tack on a field with the keyboard slot
    MGField keyboard_slot;
    keyboard_slot.type = MG_SLOT;
    keyboard_slot.slot = mg->slots->keyboard;
    fields.push_back(keyboard_slot);
    return new redirect_trans(fields);
  });
  //key is just a synonym to the above. It redirects events to the keyboard slot.
  trans_gens["key"] = trans_gens["mouse"];
  trans_gens["key"].decl.decl_str = "event = key(trans)";

  //add group_translators
  RENAME_GEN(chord,simple_chord);
  RENAME_GEN(exclusive,exclusive_chord);
  RENAME_GEN(stick,thumb_stick);
  RENAME_GEN(dpad,stick_dpad);
  MAKE_GEN(wiigyromouse);
}

MGparser::MGparser(moltengamepad* mg, message_protocol* output) : messages("parse") {
  messages.add_listener(output);
}

void MGparser::do_assignment(std::string header, std::string field, std::vector<token> rhs, response_stream& out) {
  enum entry_type left_type = NO_ENTRY;
  
  auto prof = mg->find_profile(header);
  if (!prof) {
    out.take_message("could not locate profile " + header);
    return;
  }
  int8_t direction = read_direction(field);
  left_type = prof->get_entry_type(field);

  if (left_type == DEV_EVENT_GROUP) {
    //whoops! this actually a group translation! Switch over to do_group_assignment
    std::vector<std::string> fields;
    fields.push_back(field);
    do_group_assignment(header, fields, rhs, out);
    return;
  }

  if (rhs.empty()) {
    out.take_message("Parsing failed, right-hand side of assignment empty.");
    return;
  }

  if (left_type == DEV_KEY || left_type == DEV_AXIS) {
    auto it = rhs.begin();
    event_translator* trans = nullptr;
    try {
      trans = parse_trans(left_type, rhs, it, &out);
    } catch (MGParseTransException& e) {
      out.take_message("Parsing the right-hand side failed.");
      return;
    }

    if (trans)  {
      prof->set_mapping(field, direction, trans->clone(), left_type, false);
      std::stringstream ss;
      MGTransDef def;
      trans->fill_def(def);
      print_def(left_type, def, ss);
      delete trans;
      std::string suffix = (direction == -1) ? "-" : "";
      out.take_message("setting " + header + "." + field + suffix + " = " + ss.str());
      return;
    } else {
      //we are clearing the mapping...
      prof->set_mapping(field, direction, nullptr, left_type, false);
      out.take_message("mapping for " + header + "." + field + " was cleared.");
      return;
    }
  }

  if (!field.empty() && field.front() == '?' && rhs.size() > 0) {
    field.erase(field.begin());
    int ret = prof->set_option(field, rhs.front().value);
    if (ret)
      out.take_message(field + " is not a registered option");
    else
      out.take_message("option \"" + field + "\" set.");
    return;
  }

  out.take_message("assignment failed for field " + field);
}



void MGparser::do_group_assignment(std::string header, std::vector<std::string>& fields, std::vector<token> rhs, response_stream& out) {
  if (rhs.empty()) return;
  auto prof = mg->find_profile(header);
  if (!prof) {
    out.take_message("could not locate profile " + header);
    return;
  }
  std::vector<int8_t> directions;
  for (std::string& field : fields) {
    directions.push_back(read_direction(field));
  }

  if (rhs.front().value == "nothing") {
    prof->set_group_mapping(fields, directions, nullptr);
    out.take_message("clearing group translator");
    return;
  }
  group_translator* trans = parse_group_trans(rhs, &out);
  if (!trans) {
    out.take_message("could not parse right hand side");
  }
  if (trans) {
    prof->set_group_mapping(fields, directions, trans->clone());
    std::stringstream ss;
    MGTransDef def;
    trans->fill_def(def);
    print_def(DEV_KEY, def, ss);
    out.take_message("setting group translator to " + ss.str());
  }

  if (trans) delete trans;


  if (rhs.empty()) return;

}

void MGparser::do_assignment_line(std::vector<token>& line, std::string& header, response_stream& out) {
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
    do_group_assignment(effective_header, multifield, rightside, out);
    return;
  }

  if (effective_field.empty() || rightside.empty()) return;


  do_assignment(effective_header, effective_field, rightside, out);



}

void MGparser::parse_line(std::vector<token>& line, std::string& header, response_stream& out) {

  if (find_token_type(TK_HEADER_OPEN, line)) {
    if (do_header_line(line, header))
      out.take_message("header is " + header);
    return;
  }

  if (find_token_type(TK_EQUAL, line) && line[0].value != "set") {
    do_assignment_line(line, header, out);
  } else {
    do_command(mg, line, &out);
  }

}

void MGparser::exec_line(std::vector<token>& line, std::string& header, int response_id) {
  response_stream out(response_id, &messages);
  parse_line(line, header, out);
  out.end_response(0); //Always have return value 0 for now.
}

MGParseException parse_err;
MGParseTransException parse_trans_err;

event_translator* MGparser::parse_trans(enum entry_type intype, std::vector<token>& tokens, std::vector<token>::iterator& it, response_stream* out) {
  //Note: this function is assumed to be called at the top of parsing a translator,
  //not as some recursive substep. If we wish to avoid the quirks, use parse_complex_trans instead.
  event_translator* trans = parse_trans_toplevel_quirks(intype, tokens, it);
  if (trans) return trans;
  trans = parse_trans_strict(intype, tokens, it, out);

  return trans;
}

event_translator* MGparser::parse_trans_toplevel_quirks(enum entry_type intype, std::vector<token>& tokens, std::vector<token>::iterator& it) {
  //For backwards compatibility/ease of use, there is some automagic hard to fit in elsewhere.
  //Namely, the automagic "btn,btn" parsing of axis2btns. Without a leading parenthesis,
  //this structure is hard to detect as an expression.
  //Allowing a top level expression to include a comma would make the parse descent ambiguous.
  //So instead, it ends up here.

  if (it != tokens.begin()) {
    throw parse_trans_err;
    return nullptr; //Not the toplevel? Abort?
  }

  //This is very heavy on formatting, a button, a comma, a button.
  if (intype == DEV_AXIS && tokens.size() == 3 && tokens[1].type == TK_COMMA) {
    complex_expr* expr = new complex_expr;
    expr->ident = "";
    expr->params.push_back(new complex_expr);
    expr->params.push_back(new complex_expr);
    expr->params[0]->ident = tokens[0].value;
    expr->params[1]->ident = tokens[2].value;
    event_translator* trans = nullptr;
    try {
      trans = parse_trans_expr(DEV_AXIS, expr, nullptr);
    } catch (std::exception& e) {
    }
    free_complex_expr(expr);
    return trans;
  }
  return nullptr;
}


event_translator* MGparser::parse_trans_strict(enum entry_type intype, std::vector<token>& tokens, std::vector<token>::iterator& it, response_stream* out) {
  if (it != tokens.end() && it->value == "nothing")
    return nullptr;
  auto localit = it;
  complex_expr* expr = read_expr(tokens, localit);
  event_translator* trans;
  try {
     trans =  parse_trans_expr(intype, expr, out);
  } catch (std::exception& e) {
    trans = nullptr;
  }
  free_complex_expr(expr);
  it = localit;
  if (trans)
    return trans;
  throw parse_trans_err;

}



void release_def(MGTransDef& def) {
  for (auto entry : def.fields) {
    if ((entry.type == MG_TRANS || entry.type == MG_KEY_TRANS || entry.type == MG_AXIS_TRANS || entry.type == MG_REL_TRANS) && entry.trans)
      delete entry.trans;
    if (entry.type == MG_GROUP_TRANS && entry.group_trans)
      delete entry.group_trans;
    if (entry.type == MG_STRING && entry.string)
      free((char*)entry.string);
  }
}


event_translator* MGparser::parse_trans_expr(enum entry_type intype, complex_expr* expr, response_stream* out) {
  if (!expr) {
    if (out) out->err("expression not well-formed.");
    throw parse_trans_err;
    return nullptr;
  }

  if (expr->ident == "nothing")
    return nullptr; //we actually wish to return nullptr in this case, not an exception.

  event_translator* trans = parse_special_trans(intype, expr);
  if (trans) return trans;

  

  auto generator = trans_gens.find(expr->ident);
  if (generator == trans_gens.end()) {
    if (out) out->err("no known translator \""+expr->ident+"\".");
    throw parse_trans_err;
    return nullptr;
  }

  trans_decl& decl = generator->second.decl;

  MGTransDef def;
  def.identifier = expr->ident;


  if (!parse_decl(intype, decl, def, expr, out)) {
    throw parse_trans_err;
    return nullptr;
  }


  //still need to build it!
  try {
    trans = generator->second.generate(def.fields);
  } catch (std::exception& e) {
    if (out) out->err("translator constructor failed.");
    trans = nullptr;
  }
  release_def(def);
  if (trans)
    return trans;
  throw parse_trans_err;
}


int read_ev_code(std::string& code, out_type type) {
  int i;
  try {
    i = std::stoi(code);
    return i;
  } catch (std::exception& e) {
    if (type == OUT_NONE) return 0;
    event_info info = lookup_event(code.c_str());
    if (info.type == type) return info.value;
  }
  return -1;
}

//Handles those automagic simple cases.
#define SPEC_REL_BTN 3
#define SPEC_REL_AXIS 10
event_translator* MGparser::parse_special_trans(enum entry_type intype, complex_expr* expr) {
  if (!expr) return nullptr;

  //Key to a key.
  if (intype == DEV_KEY && expr->params.size() == 0) {
    int out_button = read_ev_code(expr->ident, OUT_KEY);
    if (out_button >= 0) return new btn2btn(out_button);
  }

  //Axis or key to an axis or rel. (Detect +/- directions)
  if ((intype == DEV_AXIS || intype == DEV_KEY) && expr->params.size() == 0) {
    std::string outevent = expr->ident;
    int direction = 1;
    if (outevent.size() > 0) {
      if (outevent.back() == '+') {
        outevent.pop_back();
      } else if (outevent.back() == '-') {
        outevent.pop_back();
        direction = -1;
      } else if (outevent[0] == '+') {
        //For backwards compatibility, allow +/- to be in front as well.
        outevent.erase(outevent.begin());
      } else if (outevent[0] == '-') {
        outevent.erase(outevent.begin());
        direction = -1;
      }
      //Check for it being an axis
      int out_axis = read_ev_code(outevent, OUT_ABS);
      if (out_axis >= 0 && intype == DEV_AXIS) return new axis2axis(out_axis, direction);
      if (out_axis >= 0 && intype == DEV_KEY)  return new btn2axis(out_axis, direction);

      //Check for it being a rel
      int out_rel = read_ev_code(outevent, OUT_REL);
      if (out_rel >= 0 && intype == DEV_AXIS) return new axis2rel(out_rel, SPEC_REL_AXIS*direction);
      if (out_rel >= 0 && intype == DEV_KEY)  return new btn2rel(out_rel, SPEC_REL_BTN*direction);
    }
  }


  //Axis to buttons.
  if ((intype == DEV_AXIS) && expr->ident.empty() && expr->params.size() == 2) {
    int neg_btn = read_ev_code(expr->params[0]->ident, OUT_KEY);
    int pos_btn = read_ev_code(expr->params[1]->ident, OUT_KEY);
    if (neg_btn >= 0 && pos_btn >= 0) return new axis2btns(neg_btn, pos_btn);
  }

  return nullptr;
}


bool MGparser::parse_decl(enum entry_type intype, const trans_decl& decl, MGTransDef& def, complex_expr* expr, response_stream* out) {
  if (!expr) return false;

  int fieldsfound = expr->params.size();

  //initialize two vectors to match our number of fields.
  //set all to default
  std::vector<complex_expr> values;
  std::vector<bool> valid;
  def.fields.clear();
  for (int i = 0; i < decl.fields.size(); i++) {
    def.fields.push_back({decl.fields[i].type,0,FLAG_DEFAULT});
    complex_expr default_val;
    default_val.ident = decl.fields[i].default_val;
    values.push_back(default_val);
    valid.push_back(false);
  }
  //compute whether the last parameter is repeating...
  //if so, store the type.
  MGType variadic_type = MG_NULL;
  if (decl.fields.size() > 0 && decl.fields.back().repeating)
    variadic_type = decl.fields.back().type;

  //assign each given parameter to its correct spot.
  //do this by maintaining a count of which positional parameter we are on,
  //while allowing named parameters to find their spot without affecting that count.
  //Also: store metadata like whether each param was positional/defaulted/named.
  int offset = 0;
  bool named = false;
  for (int i = 0; i < fieldsfound; i++) {
    int spot = i - offset;
    named = false;
    if (!expr->params[i]->name.empty()) {
      for (spot = 0; spot < decl.fields.size(); spot++) {
        if (decl.fields[spot].name == expr->params[i]->name) {
          //we have a name and found its spot!
          named = true;
          break;
        }
      }
      if (spot == decl.fields.size()) {
        if (out) out->err("translator \""+expr->ident+"\" has no parameter named \""+expr->params[i]->name+"\"");
        return false; //name not found?
      }
    }
    if (spot >= def.fields.size() && variadic_type != MG_NULL) {
      //we got more than the decl, but this decl was flexible.
      //just push some dummy values to resize the list.
      complex_expr dummy;
      values.push_back(dummy);
      valid.push_back(true);
      def.fields.push_back({variadic_type,0});
    }
    values[spot] = *(expr->params[i]);
    valid[spot] = true;
    def.fields[spot].flags = 0; //clear the default flag, as we do have a value given.
    if (named)
      def.fields[spot].flags |= FLAG_NAMED;
    if (spot != i-offset)
      offset++; //stay at current spot for the next loop.
  }

  for (int i = 0; i < decl.fields.size(); i++) {
    //mandatory param missing.
    if (!decl.fields[i].has_default && !valid[i]) {
      if (out) out->err("required parameter missing for translator \""+expr->ident+"\"");
      return false;
    }
  }
    
  for (int i = 0; i < def.fields.size(); i++) {
    MGType type = def.fields[i].type;
    if (type == MG_KEYBOARD_SLOT) {
      def.fields[i].slot = mg->slots->keyboard;
      continue;
    };

    //We have to get fancy and actually do yet more parsing if we want to use a default value for a translator.
    //If we aren't using a default value, it was already parsed!
    //If it isn't a translator, there is no need to parse beyond a flat string.
    complex_expr* temp_expr = nullptr;
    if (!valid[i] && (type == MG_TRANS || type == MG_KEY_TRANS || type == MG_REL_TRANS || type == MG_AXIS_TRANS)) {
      std::vector<token> tokens = tokenize(values[i].ident);
      auto it = tokens.begin();

      temp_expr = read_expr(tokens,it);
      values[i] = *temp_expr;
    }
      
    try {
      if (type == MG_TRANS) def.fields[i].trans = parse_trans_expr(intype, &values[i], out);
      if (type == MG_KEY_TRANS) def.fields[i].trans = parse_trans_expr(DEV_KEY, &values[i], out);
      if (type == MG_REL_TRANS) def.fields[i].trans = parse_trans_expr(DEV_REL, &values[i], out);
      if (type == MG_AXIS_TRANS) def.fields[i].trans = parse_trans_expr(DEV_AXIS, &values[i], out);
    } catch (std::exception& e) {
      if (out) out->err("translator-type parameter invalid.");
      if (temp_expr)
        free_complex_expr(temp_expr);
      return false;
    }
    if (temp_expr)
      free_complex_expr(temp_expr);
    if (type >= MG_KEY_TRANS && type <= MG_TRANS && !def.fields[i].trans) {
      def.fields[i].trans = new event_translator(); //save some headaches, keep translators from dealing with nulls.
    }
    if (type == MG_KEY) def.fields[i].key = read_ev_code(values[i].ident, OUT_KEY);
    if (type == MG_REL) def.fields[i].rel = read_ev_code(values[i].ident, OUT_REL);
    if (type == MG_AXIS) def.fields[i].axis = read_ev_code(values[i].ident, OUT_ABS);
    if (type == MG_AXIS_DIR) {
      std::string axis = values[1].ident;
      bool negative = false;
      if (axis.size() > 0) {
        if (axis.back() == '+') {
          axis.pop_back();
        } else if (axis[0] == '-') {
          axis.pop_back();
          negative = true;
        }
      }
      def.fields[i].axis = read_ev_code(axis,OUT_ABS);
      if (def.fields[i].axis >= 0 && negative)
        def.fields[i].axis |= NEGATIVE_AXIS_DIR;
    }
    if (type == MG_INT) def.fields[i].integer = read_ev_code(values[i].ident, OUT_NONE);
    if (type == MG_FLOAT) {
      try {
        def.fields[i].real = std::atof(values[i].ident.c_str());
      } catch (std::exception& e) {
        def.fields[i].real = std::nanf("");
      }
    }
    if (type == MG_SLOT) {
      output_slot* slot = mg->slots->find_slot(values[i].ident);
      if (!slot) {
        if (out) out->err("slot-type parameter invalid.");
        return false;
      }
      def.fields[i].slot = slot;
    }
    if (type == MG_BOOL) {
      def.fields[i].boolean = 0;
      read_bool(values[i].ident, [&def, i] (bool val) {
        def.fields[i].boolean = val;
      });
    }
    if (type == MG_STRING) {
      size_t size = values[i].ident.size();
      char* copy = (char*) calloc(size+1,sizeof(char));
      strncpy(copy,values[i].ident.c_str(), size);
      copy[size] = '\0';
      def.fields[i].string = copy;
    }
  }
  return true;
}

void MGparser::print_def(entry_type intype, MGTransDef& def, std::ostream& output) {
  //Check for the possibility of some automagic.
  if (print_special_def(intype, def, output)) return;

  trans_decl* decl = nullptr;
  bool decl_failed = false;
  output << def.identifier;
  if (def.fields.size() > 0) output << "(";
  bool needcomma = false;
  for (int i = 0; i < def.fields.size(); i++) {
    auto field = def.fields[i];
    if (field.flags & FLAG_DEFAULT)
      continue; //apparently we made this without specifying a value. It'd be wrong to remove that flexibility.
    if (needcomma) output << ",";
    MGType type = field.type;

    if (field.flags & FLAG_NAMED) {
      //gotta look up that trans_decl to actually get the field name...
      if (!decl && !decl_failed) {
        auto finder = trans_gens.find(def.identifier);
        if (finder != trans_gens.end()) {
          decl = &(finder->second.decl);
        } else {
          decl_failed = true;
        }
      }
      if (decl && decl->fields.size() > i)
        output << decl->fields[i].name << "=";
    }
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
    if (type == MG_AXIS_DIR) {
      int dir = EXTRACT_DIR(field.axis);
      int axis = EXTRACT_AXIS(field.axis);
      const char* name = get_axis_name(axis);
      const char* suffix = dir > 0 ? "+" : "-";
      if (name) {
        output << name << suffix;
      } else {
        output << field.axis << suffix;
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
    if (type == MG_STRING)  {
      std::string str = field.string;
      escape_string(str);
      output << "\""<<str<<"\"";
    }
    if (type == MG_SLOT) output << field.slot->name;
    if (type == MG_BOOL) output << (field.boolean ? "true":"false");
    if (type == MG_TRANS || type == MG_KEY_TRANS || type == MG_AXIS_TRANS || type == MG_REL_TRANS) {
      MGTransDef innerdef;
      entry_type context = intype;
      if (type == MG_KEY_TRANS) context = DEV_KEY;
      if (type == MG_AXIS_TRANS) context = DEV_AXIS;
      if (type == MG_REL_TRANS) context = DEV_REL;
      field.trans->fill_def(innerdef);
      print_def(context, innerdef, output);
    }
    needcomma = true;
  }
  if (def.fields.size() > 0) output << ")";
}

bool MGparser::print_special_def(entry_type intype, MGTransDef& def, std::ostream& output) {
  //Check if we are in a setting where some magic can be applied.
  //e.g. print "primary" instead of "btn2btn(primary)" where sensible.
  //This method is essentially a lot of code checking for special cases.

  //Check for btn2btn on a button
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

  //check for being simply mapped to an axis or rel
  if ((intype == DEV_KEY && (def.identifier == "btn2axis" || def.identifier == "btn2rel"))
    || (intype == DEV_AXIS && (def.identifier == "axis2axis" || def.identifier == "axis2rel"))) {
    if (def.fields.size() >= 2 &&  def.fields[1].type == MG_INT) {
      const char* name = nullptr;
      const char* suffix = "";
      //Axis: get axis name and check for default speeds of +/- one.
      if (def.fields[0].type == MG_AXIS) {
        name = get_axis_name(def.fields[0].axis);
        if (def.fields[1].integer == -1) {
          suffix = "-";
        }
        if (def.fields[1].integer == +1) {
          suffix = "+";
        }
      } else if (def.fields[0].type == MG_REL) {
        //Rel: default speeds depend on the intype as well!
        name = get_rel_name(def.fields[0].axis);
        int speed = def.fields[1].integer;
        if ((intype == DEV_KEY && speed == -SPEC_REL_BTN) || (intype == DEV_AXIS && speed == -SPEC_REL_AXIS)) {
          suffix = "-";
        }
        if ((intype == DEV_KEY && speed == SPEC_REL_BTN) || (intype == DEV_AXIS && speed == SPEC_REL_AXIS)) {
          suffix = "+";
        }
      }
      
      if (!suffix) return false;
      if (!name) return false;
      output << name << suffix;
      return true;
    }
  }
  //Check for simple mappings of an axis to two buttons
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

  //Check for redirecting to the keyboard_slot
  if (def.identifier == "redirect" && def.fields.size() >= 2 && def.fields[1].type == MG_SLOT) {
    if (def.fields[1].slot && def.fields[1].slot->name == "keyboard") {
      MGTransDef innerdef;
      entry_type context = intype;
      def.fields[0].trans->fill_def(innerdef);
      //Quick heuristic: if it is a "2rel" translation, it is a mouse movement.
      if (innerdef.identifier == "btn2rel" || innerdef.identifier == "axis2rel") {
        output << "mouse(";
      } else {
        output << "key(";
      }
      print_def(context, innerdef, output);
      output << ")";
      return true;
    }
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
  for (int i = 0; i < depth; i++) std::cout << " ";
  if (!expr) {
    std::cout << "(null expr)" << std::endl;
    return;
  }

  std::cout << expr->ident << std::endl;
  for (auto it = expr->params.begin(); it != expr->params.end(); it++) {
    print_expr((*it), depth + 1);
  }

}

struct complex_expr* read_expr(std::vector<token>& tokens, std::vector<token>::iterator& it) {
  if (tokens.empty() || it == tokens.end()) return nullptr;

  bool abort = false;


  //Are we an IDENT, a string of some sort?
  //Or perhaps an LPAREN, that we might want to recurse?
  //Or maybe even a dot, in the off chance of a numeric literal like ".7" instead of "0.7"
  if ((*it).type == TK_IDENT || (*it).type == TK_LPAREN || (*it).type == TK_DOT) {
    complex_expr* expr = new complex_expr;
    //If we have ident, read it in.
    //If we see '=' next, then our ident was actually a name!
    if ((*it).type == TK_IDENT) {
      expr->ident = (*it).value;
      it++;
    }

    if (it == tokens.end()) return expr;

    if ((*it).type == TK_EQUAL) {
      it++;
      if (it == tokens.end()) return expr;
      expr->name = expr->ident;
      expr->ident = (*it).value;
      it++;
    }
    //just append idents and dots to blindly cover the case of recombining numeric values.
    // "-5" "." "3"  ---> "-5.3"
    while((*it).type == TK_IDENT || (*it).type == TK_DOT) {
      expr->ident += (*it).value;
      it++;
    }
    //Otherwise, we have a paren, start reading children
    if ((*it).type == TK_LPAREN) {
      it++;
      complex_expr* subexpr = read_expr(tokens, it);
      if (subexpr) expr->params.push_back(subexpr);
      while (it != tokens.end() && (*it).type == TK_COMMA) {
        it++;
        subexpr = read_expr(tokens, it);
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

group_translator* MGparser::parse_group_trans(std::vector<token>& rhs, response_stream* out) {
  auto it = rhs.begin();
  try {
    event_translator* trans = parse_trans(DEV_KEY, rhs, it, nullptr);
    if (trans) {
      return new simple_chord(trans);
    }
  } catch (MGParseTransException& e) {
    //just move along.
  }

  it = rhs.begin();
  complex_expr* expr = read_expr(rhs, it);

  if (!expr) return nullptr;

  auto generator = trans_gens.find(expr->ident);
  if (generator == trans_gens.end()) {
    if (out) out->err("no known group translator \""+expr->ident+"\".");
    return nullptr;
  }

  trans_decl& decl = generator->second.decl;

  MGTransDef def;
  def.identifier = expr->ident;


  if (!parse_decl(DEV_KEY, decl, def, expr, out)) return nullptr;


  //still need to build it!
  group_translator* group_trans;
  try {
    group_trans = generator->second.group_generate(def.fields);
  } catch (std::exception& e) {
    group_trans = nullptr;
  }
  release_def(def);

  free_complex_expr(expr);

  return group_trans;
}
