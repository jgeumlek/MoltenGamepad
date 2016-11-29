#include "plugin.h"

int (*plugin_init) (plugin_api);

#ifdef PLUGIN
int register_plugin( int (*init) (plugin_api)) {
  plugin_init = init;
};

#else

//use the one linked in from the core, as we are being built in.
#endif

