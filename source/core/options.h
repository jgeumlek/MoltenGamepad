#pragma once
#include <string>
#include <unordered_map>
#include <map>
#include <vector>
#include <mutex>
#include <functional>
#include "mg_types.h"



int read_bool(const std::string value, std::function<void (bool)> process);

class options {
public:
  std::string name;
  options() {};
  options(std::function<int (std::string& name, MGField value)> callback);
  
  std::unordered_map<std::string, option_info> opts;
  void register_option(const option_info opt);
  void register_option(const option_decl opt);
  int set(std::string opname, std::string value);
  int remove(std::string opname);
  void lock(std::string opname, bool locked);
  option_info get_option(std::string opname);
  void list_options(std::vector<option_info>& list) const;
  template<typename T> int get(std::string opname, T& target);
  template<typename T> T get(std::string opname);

protected:
  mutable std::mutex optlock;
  int set_locked(std::string& opname, std::string& value);
  std::function<int (std::string& name, MGField value)> callback = nullptr;
};
