#pragma once
#include "../plugin/plugin.h"
#include <vector>
#include <functional>

extern plugin_api plugin_methods;

extern std::vector<std::function<int (moltengamepad*, plugin_api)>> builtin_plugins;

int register_builtin_plugin( int (*init) (moltengamepad*, plugin_api));

void init_plugin_api();

int load_builtins(moltengamepad* mg);
