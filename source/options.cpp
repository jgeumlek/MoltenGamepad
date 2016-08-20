#include "options.h"


int read_bool(const std::string value, std::function<void (bool)> process) {
  if (value == "true") {
    process(true);
    return 0;
  }
  if (value == "false") {
    process(false);
    return 0;
  }
  return -1;
}

void options::register_option(const option_info opt) {
  std::lock_guard<std::mutex> guard(optlock);
  options.erase(opt.name);
  option_info new_opt = opt;
  new_opt.locked = false;
  if (opt.type != MG_INT && opt.type != MG_BOOL && opt.type != MG_STRING)
    return; //unsupported.
  options[opt.name] = new_opt;

}

int options::set(std::string opname, std::string value) {
  std::lock_guard<std::mutex> guard(optlock);
  if (options.find(opname) == options.end()) 
    return -1; //This option was not registered, ignore it.

  if (options[opname].locked)
    return -1;
  //Weed out some bad values.
  if (options[opname].type == MG_BOOL && value != "true" && value != "false")
    return -1;

  if (options[opname].type == MG_INT) {
    try {
      int i = std::stoi(value);
    } catch(...) {
      return -1;
    }
  }
  
  options[opname].value = value;
  
  return 0;
}

option_info options::get_option(std::string opname) {
  std::lock_guard<std::mutex> guard(optlock);
  auto it = options.find(opname);
  if (it == options.end()) return {"","","", MG_NULL, false};
  return it->second;
}

void options::lock(std::string opname, bool locked) {
  std::lock_guard<std::mutex> guard(optlock);
  if (options.find(opname) == options.end()) 
    return;

  options[opname].locked = locked;
  
  return;
}

template<>
int options::get(std::string opname, bool& target) {
  std::lock_guard<std::mutex> guard(optlock);
  if (options.find(opname) == options.end()) 
    return -1;
  std::string val = options[opname].value;

  return read_bool(options[opname].value, [&target] (bool val) { target = val; });
}

template<>
bool options::get(std::string opname) {
  std::lock_guard<std::mutex> guard(optlock);
  if (options.find(opname) == options.end()) 
    return -1;
  bool target = false;
  read_bool(options[opname].value, [&target] (bool val) { target = val; });
  return target;
}

template<>
int options::get<int>(std::string opname, int& target) {
  std::lock_guard<std::mutex> guard(optlock);
  if (options.find(opname) == options.end()) 
    return -1;
  
  try {
    target = std::stoi(options[opname].value);
    return 0;
  } catch(...) {
  }
  return -1;
}

template<>
int options::get<int>(std::string opname) {
  std::lock_guard<std::mutex> guard(optlock);
  if (options.find(opname) == options.end()) 
    return -1;
  
  try {
    return std::stoi(options[opname].value);
  } catch(...) {
  }
  return -1;
}

template<>
int options::get<std::string>(std::string opname, std::string& target) {
  std::lock_guard<std::mutex> guard(optlock);
  if (options.find(opname) == options.end()) 
    return -1;
  
  target = options[opname].value;
  return 0;
}

template<>
std::string options::get<std::string>(std::string opname) {
  std::lock_guard<std::mutex> guard(optlock);
  if (options.find(opname) == options.end()) 
    return "";
  
  return options[opname].value;
}
