#include "plugin_loader.h"
#include "moltengamepad.h"
#ifndef NO_PLUGIN_LOADING
#include <dlfcn.h>
#endif

plugin_api plugin_methods;

std::vector<std::function<int (plugin_api)>> builtin_plugins;

bool LOAD_PLUGINS = false;

int register_plugin( int (*init) (plugin_api)) {
  if (init) {
    builtin_plugins.push_back(init);
  }
  return 0;
};

moltengamepad* loader_mg;

int load_builtins(moltengamepad* mg) {
  loader_mg = mg;
  plugin_api reset = plugin_methods;
  for (auto func : builtin_plugins) {
    //Be overly cautious, keep one plugin from clearing methods for others.
    try {
      plugin_methods = reset;
      func(plugin_methods);
    } catch (std::exception& e) {
      std::cout << "PLGIN: " << e.what() << std::endl;
    }
  }
  return 0;
};

int load_plugin(const std::string& path) {
#ifndef NO_PLUGIN_LOADING
  char* err = dlerror();
  void* handle = dlopen(path.c_str(),RTLD_LAZY | RTLD_NODELETE);
  if (!handle) {
    err = dlerror();
    debug_print(DEBUG_NONE, 2, "plugin: ", err);
    return -1;
  }

  int (*plugin_init) (plugin_api);
  err = dlerror();
  *(void **) &plugin_init = dlsym(handle, "plugin_init");
  err = dlerror();
  if (err || !plugin_init) {
    debug_print(DEBUG_NONE, 2, "plugin: ", err);
    return -1;
  }
  debug_print(DEBUG_INFO, 2, "plugin: initializing ", path.c_str());
  plugin_api api = plugin_methods;
  int ret = (*plugin_init)(api);
  if (ret)
    debug_print(DEBUG_NONE, 3, "plugin: init ", path.c_str(), " failed.");
  dlclose(handle);
#endif
  return 0;
}

void init_plugin_api() {

  memset(&plugin_methods, 0, sizeof(plugin_methods));
  plugin_methods.head.size = sizeof(plugin_methods.head);


  plugin_methods.mg.size = sizeof(plugin_methods.mg);
  plugin_methods.device.size = sizeof(plugin_methods.device);
  plugin_methods.manager.size = sizeof(plugin_methods.manager);

  plugin_methods.mg.add_manager = [] (manager_plugin raw_manager, void* manager_plug_data) {
    manager_plugin manager;
    memset(&manager,0,sizeof(manager));
    memcpy(&manager,&raw_manager,raw_manager.size);
    return loader_mg->add_manager(manager, manager_plug_data);
  };
  plugin_methods.mg.request_slot = [] (input_source* dev) {
    return loader_mg->slots->request_slot(dev);
  };
  plugin_methods.mg.grab_permissions = [] (udev_device* dev, bool grabbed) {
    return loader_mg->udev.grab_permissions(dev, grabbed);
  };

  plugin_methods.manager.plug_data = [] (const device_manager* man) {
    return man->plug_data;
  };
  plugin_methods.manager.register_event = [] (device_manager* man, event_decl ev) {
    return man->register_event(ev);
  };
  plugin_methods.manager.register_dev_option = [] (device_manager* man, option_decl opt) {
    return man->register_device_option(opt);
  };
  plugin_methods.manager.register_manager_option = [] (device_manager* man, option_decl opt) {
    return man->register_manager_option(opt);
  };
  plugin_methods.manager.register_alias = [] (device_manager* man, const char* external, const char* local) {
    return man->register_alias(external, local);
  };
  plugin_methods.manager.add_device = [] (device_manager* man, device_plugin raw_dev, void* dev_plug_data) {
    device_plugin dev;
    memset(&dev,0,sizeof(dev));
    memcpy(&dev,&raw_dev,raw_dev.size);
    return man->add_device(dev, dev_plug_data);
  };
  plugin_methods.manager.remove_device = [] (device_manager* man, input_source* dev) {
    return man->mg->remove_device(dev);
  };
  plugin_methods.manager.print = [] (device_manager* man, const char* message) -> int {
    man->log.take_message(0,std::string(message));
    return 0;
  };
  plugin_methods.manager.register_event_group = [] (device_manager* man, event_group_decl decl) {
    return man->register_event_group(decl);
  };

  plugin_methods.device.plug_data = [] (const input_source* dev) {
    return dev->plug_data;
  };
  plugin_methods.device.watch_file = [] (input_source* dev, int fd, void* tag) -> int {
    dev->watch_file(fd, tag);
    return 0;
  };
  plugin_methods.device.toggle_event = [] (input_source* dev, int id, event_state state) -> int {
    dev->toggle_event(id, state);
    return 0;
  };
  plugin_methods.device.send_value = [] (input_source* dev, int id, int64_t value) -> int {
    dev->send_value(id, value);
    return 0;
  };
  plugin_methods.device.send_syn_report = [] (input_source* dev) -> int {
    dev->send_syn_report();
    return 0;
  };
  plugin_methods.device.remove_option = [] (input_source* dev, const char* opname) -> int {
    dev->devprofile->remove_option(std::string(opname));
    return 0;
  };
  plugin_methods.device.print = [] (input_source* dev, const char* text) -> int {
    dev->print(std::string(text));
    return 0;
  };
  plugin_methods.device.request_recurring_events = [] (input_source* dev, bool wants_recurring) -> int {
    dev->set_plugin_recurring(wants_recurring);
    return 0;
  };

  plugin_methods.head.mg = &plugin_methods.mg;
  plugin_methods.head.device = &plugin_methods.device;
  plugin_methods.head.manager = &plugin_methods.manager;
};
