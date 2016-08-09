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
  MGType type = opt.value.type;
  if (type != MG_INT && type != MG_BOOL && type != MG_STRING)
    return; //unsupported.
  options[opt.name] = opt;

}

void options::register_option(const option_decl opt) {
  std::lock_guard<std::mutex> guard(optlock);
  if (opt.type != MG_INT && opt.type != MG_BOOL && opt.type != MG_STRING)
    return; //unsupported.
  
  option_info opt_info;
  opt_info.name = std::string(opt.name);
  opt_info.descr = std::string(opt.descr);
  opt_info.value.type = opt.type;
  options[opt.name] = opt_info;
  std::string strval(opt.value);
  int ret = set_locked(opt_info.name, strval);
  if (ret)
    options.erase(opt.name); //the value was rejected.

}
int options::set(std::string opname, std::string value) {
  std::lock_guard<std::mutex> guard(optlock);
  set_locked(opname, value);
}

int options::set_locked(std::string& opname, std::string& value) {
  if (options.find(opname) == options.end()) 
    return -1; //This option was not registered, ignore it.

  if (options[opname].locked)
    return -1;
  MGType type = options[opname].value.type;
  //Weed out some bad values.
  if (type == MG_BOOL) {
    if (value == "true") {
      options[opname].value.boolean = true;
      options[opname].stringval = value;
      return 0;
    }
    if (value == "false") {
      options[opname].value.boolean = false;
      options[opname].stringval = value;
      return 0;
    }
    return -1;
  }

  if (type == MG_INT) {
    try {
      int i = std::stoi(value);
      options[opname].value.integer = i;
      options[opname].stringval = value;
      return 0;
    } catch(...) {
      return -1;
    }
  }
  
  if (type == MG_STRING) {
    options[opname].stringval = value;
    return 0;
  }
  
  return -1;
}

option_info options::get_option(std::string opname) {
  std::lock_guard<std::mutex> guard(optlock);
  auto it = options.find(opname);
  if (it == options.end()) {
    option_info opt;
    return opt;
  }
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
  target = options[opname].value.boolean;
  return 0;
}

template<>
bool options::get(std::string opname) {
  std::lock_guard<std::mutex> guard(optlock);
  if (options.find(opname) == options.end()) 
    return false;
  return options[opname].value.boolean;
}

template<>
int options::get<int>(std::string opname, int& target) {
  std::lock_guard<std::mutex> guard(optlock);
  if (options.find(opname) == options.end()) 
    return -1;
  
  target = options[opname].value.integer;
  return 0;
}

template<>
int options::get<int>(std::string opname) {
  std::lock_guard<std::mutex> guard(optlock);
  if (options.find(opname) == options.end()) 
    return -1;

  return options[opname].value.integer;

}

template<>
int options::get<std::string>(std::string opname, std::string& target) {
  std::lock_guard<std::mutex> guard(optlock);
  if (options.find(opname) == options.end()) 
    return -1;
  
  target = options[opname].stringval;
  return 0;
}

template<>
std::string options::get<std::string>(std::string opname) {
  std::lock_guard<std::mutex> guard(optlock);
  if (options.find(opname) == options.end()) 
    return "";
  
  return options[opname].stringval;
}
