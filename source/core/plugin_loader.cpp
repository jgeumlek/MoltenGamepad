#include "plugin_loader.h"
#include "moltengamepad.h"

plugin_api plugin_methods;

std::vector<std::function<int (plugin_api)>> builtin_plugins;

int register_builtin_plugin( int (*init) (plugin_api)) {
  if (init) {
    builtin_plugins.push_back(init);
  }
};

moltengamepad* loader_mg;

int load_builtins(moltengamepad* mg) {
  loader_mg = mg;
  for (auto func : builtin_plugins)
    func(plugin_methods);
};



void init_plugin_api() {
  plugin_api api;
  memset(&api, 0, sizeof(api));

  api.mg.add_manager = [] (manager_plugin manager, void* manager_plug_data) {
    return loader_mg->add_manager(manager, manager_plug_data);
  };
  api.mg.request_slot = [] (input_source* dev) {
    return loader_mg->slots->request_slot(dev);
  };
  api.mg.grab_permissions = [] (udev_device* dev, bool grabbed) {
    return loader_mg->udev.grab_permissions(dev, grabbed);
  };
  
  api.manager.plug_data = [] (const device_manager* man) {
    return man->plug_data;
  };
  api.manager.register_event = [] (device_manager* man, event_decl ev) {
    return man->register_event(ev);
  };
  api.manager.register_dev_option = [] (device_manager* man, option_decl opt) {
    return man->register_device_option(opt);
  };
  api.manager.register_manager_option = [] (device_manager* man, option_decl opt) {
    return man->register_manager_option(opt);
  };
  api.manager.register_alias = [] (device_manager* man, const char* external, const char* local) {
    return man->register_alias(external, local);
  };
  api.manager.add_device = [] (device_manager* man, device_plugin dev, void* dev_plug_data) {
    return man->add_device(dev, dev_plug_data);
  };
  api.manager.remove_device = [] (device_manager* man, input_source* dev) {
    return man->mg->remove_device(dev);
  };
  api.manager.print = [] (device_manager* man, const char* message) -> int {
    man->log.take_message(std::string(message));
    return 0;
  };

  api.device.plug_data = [] (const input_source* dev) {
    return dev->plug_data;
  };
  api.device.watch_file = [] (input_source* dev, int fd, void* tag) -> int {
    dev->watch_file(fd, tag);
    return 0;
  };
  api.device.toggle_event = [] (input_source* dev, int id, event_state state) -> int {
    dev->toggle_event(id, state);
    return 0;
  };
  api.device.send_value = [] (input_source* dev, int id, int64_t value) -> int {
    dev->send_value(id, value);
    return 0;
  };
  api.device.send_syn_report = [] (input_source* dev) -> int {
    dev->send_syn_report();
    return 0;
  };
  api.device.remove_option = [] (input_source* dev, const char* opname) -> int {
    dev->devprofile->remove_option(std::string(opname));
    return 0;
  };
  api.device.print = [] (input_source* dev, const char* text) -> int {
    dev->print(std::string(text));
    return 0;
  };

  plugin_methods = api;
};
