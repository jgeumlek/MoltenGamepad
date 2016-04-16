#include "device.h"
#include "../event_change.h"

void device_manager::update_maps(const char* evname, event_translator* trans) {
  auto intype = entry_type(evname);
  mapprofile->set_mapping(evname, trans->clone(), intype);
}

void device_manager::update_options(const char* opname, const char* value) {
  mapprofile->set_option(opname, value);
}

void device_manager::update_advanceds(const std::vector<std::string>& names, advanced_event_translator* trans) {
  if (trans) {
    mapprofile->set_advanced(names, trans->clone());
  } else {
    mapprofile->set_advanced(names, nullptr);
  }
}
