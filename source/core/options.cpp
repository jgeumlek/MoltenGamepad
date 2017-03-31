#include "options.h"
#include <cstring>

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


options::options(std::function<int (std::string& name, MGField value)> callback) {
  this->callback = callback;
}

void options::register_option(const option_info opt) {
  std::lock_guard<std::mutex> guard(optlock);
  MGType type = opt.value.type;
  if (type != MG_INT && type != MG_BOOL && type != MG_STRING)
    return; //unsupported.
  if (type == MG_STRING && opt.value.string != nullptr)
    return; //Data integrity assumption violated
  opts[opt.name] = opt;

  if (callback) {
    MGField value = opt.value;
    if (type == MG_STRING)
      value.string = opt.stringval.c_str();
    std::string name(opt.name);
    callback(name, value);
  }
}

void options::register_option(const option_decl opt) {
  std::lock_guard<std::mutex> guard(optlock);
  if (opt.type != MG_INT && opt.type != MG_BOOL && opt.type != MG_STRING)
    return; //unsupported.

  option_info opt_info;
  memset(&opt_info.value,0,sizeof(opt_info.value));
  opt_info.name = std::string(opt.name);
  opt_info.descr = std::string(opt.descr);
  opt_info.value.type = opt.type;
  opt_info.value.flags = 0;
  opts[opt.name] = opt_info;
  std::string strval(opt.value);
  int ret = set_locked(opt_info.name, strval);
  if (ret)
    opts.erase(opt.name); //the value was rejected.

}
int options::set(std::string opname, std::string value) {
  std::lock_guard<std::mutex> guard(optlock);
  return set_locked(opname, value);
}

int options::remove(std::string opname) {
  std::lock_guard<std::mutex> guard(optlock);
  int ret = opts.erase(opname);
  return (ret > 0) ? 0 : -1;
}

int options::set_locked(std::string& opname, std::string& value) {
  if (opts.find(opname) == opts.end())
    return -1; //This option was not registered, ignore it.

  if (opts[opname].locked)
    return -1;
  MGType type = opts[opname].value.type;
  MGField newval;
  memset(&newval, 0, sizeof(newval));
  newval.type = type;
  newval.flags = 0;
  //Weed out some bad values.
  if (type == MG_BOOL) {
    if (value == "true") {
      opts[opname].value.boolean = true;
      opts[opname].stringval = value;
      newval.boolean = true;
      if (callback)
        callback(opname, newval);
      return 0;
    }
    if (value == "false") {
      opts[opname].value.boolean = false;
      opts[opname].stringval = value;
      newval.boolean = false;
      if (callback)
        callback(opname, newval);
      return 0;
    }
    return -1;
  }

  if (type == MG_INT) {
    try {
      int i = std::stoi(value);
      opts[opname].value.integer = i;
      opts[opname].stringval = value;
      newval.integer = i;
      if (callback)
        callback(opname, newval);
      return 0;
    } catch(std::exception& e) {
      return -1;
    }
  }
  
  if (type == MG_STRING) {
    opts[opname].stringval = value;
    opts[opname].value.string = nullptr;
    newval.string = value.c_str();
    if (callback)
      callback(opname, newval);
    return 0;
  }
  
  return -1;
}

option_info options::get_option(std::string opname) {
  std::lock_guard<std::mutex> guard(optlock);
  auto it = opts.find(opname);
  if (it == opts.end()) {
    option_info opt;
    return opt;
  }
  return it->second;
}

void options::list_options(std::vector<option_info>& list) const {
  std::lock_guard<std::mutex> guard(optlock);
  for (auto opt : opts)
    list.push_back(opt.second);
}

void options::lock(std::string opname, bool locked) {
  std::lock_guard<std::mutex> guard(optlock);
  if (opts.find(opname) == opts.end())
    return;

  opts[opname].locked = locked;
  
  return;
}

template<>
int options::get(std::string opname, bool& target) {
  std::lock_guard<std::mutex> guard(optlock);
  if (opts.find(opname) == opts.end())
    return -1;
  target = opts[opname].value.boolean;
  return 0;
}

template<>
bool options::get(std::string opname) {
  std::lock_guard<std::mutex> guard(optlock);
  if (opts.find(opname) == opts.end())
    return false;
  return opts[opname].value.boolean;
}

template<>
int options::get<int>(std::string opname, int& target) {
  std::lock_guard<std::mutex> guard(optlock);
  if (opts.find(opname) == opts.end())
    return -1;
  
  target = opts[opname].value.integer;
  return 0;
}

template<>
int options::get<int>(std::string opname) {
  std::lock_guard<std::mutex> guard(optlock);
  if (opts.find(opname) == opts.end())
    return -1;

  return opts[opname].value.integer;

}

template<>
int options::get<std::string>(std::string opname, std::string& target) {
  std::lock_guard<std::mutex> guard(optlock);
  if (opts.find(opname) == opts.end())
    return -1;
  
  target = opts[opname].stringval;
  return 0;
}

template<>
std::string options::get<std::string>(std::string opname) {
  std::lock_guard<std::mutex> guard(optlock);
  if (opts.find(opname) == opts.end())
    return "";
  
  return opts[opname].stringval;
}
