#include "parser.h"

void parse_error_direct(moltengamepad* mg, const std::string& message, const context& context) {
  if (message.empty()) {
    mg->stdout->err(0,"options: parsing error, entire line ignored.", context.path, context.line_number);
  } else {
    mg->stdout->err(0,message, context.path, context.line_number);
  }
}
void config_assignment_line(moltengamepad* mg, std::vector<token>& line, context context, options& opt) {

  auto it = line.begin();
  if (it == line.end()) return;

  std::string field = line.front().value;
  std::string prefix = "";

  it++;

  if (it == line.end()) {
    parse_error_direct(mg, "", context);
    return;
  }

  if ((*it).type == TK_DOT) {
    it++;
    if (it == line.end()) {
      parse_error_direct(mg, "", context);
      return;
    }
    prefix = field;
    field = (*it).value;
    it++;

  }


  if ((*it).type != TK_EQUAL) {
    parse_error_direct(mg, "", context);
    return;
  }

  it++; //Skip past the "="

  if (it == line.end()) {
    parse_error_direct(mg, "", context);
    return;
  }

  std::string value = (*it).value;


  it++;

  if (!field.empty()) {
    int ret = opt.set(field,value);
    if (ret)
      parse_error_direct(mg, "options: could not set " + field + " to " + value, context);
  }

}

int config_parse_line(moltengamepad* mg, std::vector<token>& line, context context, options& opt, config_extras* extra) {
  //<option> = <value>
  if (find_token_type(TK_EQUAL, line)) {
    config_assignment_line(mg, line, context, opt);
  }

  //load profiles from <file>
  if (extra && line.size() >= 4 && line[0].value == "load") {
    if (line[1].value != "profiles" && line[1].value != "profile")
      return 0;
    if (line[2].value != "from")
      return 0;
    std::string filename = line.at(3).value;
    for (int i = 4; i < line.size(); i++) 
      if (line.at(i).type != TK_ENDL) filename += line.at(i).value;

    extra->startup_profiles.push_back(filename);
  }


  return 0;
}
