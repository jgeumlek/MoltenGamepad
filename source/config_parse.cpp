#include "parser.h"
void config_assignment_line(moltengamepad* mg, std::vector<token>& line, context context, options& opt) {

  auto it = line.begin();
  if (it == line.end()) return;

  std::string field = line.front().value;
  std::string prefix = "";

  it++;

  if (it == line.end()) return;

  if ((*it).type == TK_DOT) {
    it++;
    if (it == line.end()) return;
    prefix = field;
    field = (*it).value;
    it++;

  }


  if ((*it).type != TK_EQUAL) return;

  it++; //Skip past the "="

  if (it == line.end()) return;

  std::string value = (*it).value;


  it++;

  if (!field.empty())
    opt.set(field,value);
}

int config_parse_line(moltengamepad* mg, std::vector<token>& line, context context, options& opt) {
  if (find_token_type(TK_EQUAL, line)) {
    config_assignment_line(mg, line, context, opt);
    return 0;
  }

  return 0;
}
