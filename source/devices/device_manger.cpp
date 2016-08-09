#include "device.h"
#include "../parser.h"
 
void device_manager::register_event(event_decl ev) {

  events.push_back(ev);
  std::vector<token> tokens = tokenize(std::string(ev.default_mapping));
  auto it = tokens.begin();
  event_translator* trans = MGparser::parse_trans(ev.type, tokens, it);
  mapprofile->set_mapping(std::string(ev.name), trans ? trans : new event_translator(), ev.type, true);
}

void device_manager::register_option(option_decl opt) {
  option_info info;
  info.locked = false;
  info.name = std::string(opt.name);
  info.descr = std::string(opt.descr);
  info.stringval = std::string(opt.value);
  info.value.type = opt.type;
  mapprofile->register_option(info);
}

void device_manager::for_all_devices(std::function<void (const input_source*)> func) {
  mg->for_all_devices( [&, func] (std::shared_ptr<input_source> device) {
    if (device->get_manager_name() == name)
      func(device.get());
  });
}
