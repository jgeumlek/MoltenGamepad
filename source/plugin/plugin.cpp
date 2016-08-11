#include "plugin.h"

int (*plugin_init) (moltengamepad*, plugin_api);

#ifdef PLUGIN
int register_plugin( int (*init) (moltengamepad*, plugin_api)) {
  plugin_init = init;
};

#else

#include "../core/plugin_loader.h"

int register_plugin( int (*init) (moltengamepad*, plugin_api)) {
  return register_builtin_plugin(init);
};

#endif

