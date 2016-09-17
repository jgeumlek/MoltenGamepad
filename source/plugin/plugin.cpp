#include "plugin.h"

int (*plugin_init) (plugin_api);

#ifdef PLUGIN
int register_plugin( int (*init) (plugin_api)) {
  plugin_init = init;
};

#else

#include "../core/plugin_loader.h"

int register_plugin( int (*init) (plugin_api)) {
  return register_builtin_plugin(init);
};

#endif

