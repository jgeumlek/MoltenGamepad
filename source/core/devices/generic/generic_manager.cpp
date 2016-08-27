#include "generic.h"
#include <algorithm>
#include <unordered_map>

manager_methods generic_manager::methods;

generic_manager::generic_manager(moltengamepad* mg, generic_driver_info& descr) : mg(mg) {
  this->devname = descr.devname.c_str();

  this->descr = &descr;

  split = descr.split;
  flatten = descr.flatten;


  //We might have split devices, each with their own distinct and possibly overlapping events.
  //So when we need to keep track of events on a per-split basis
  for (int i = 1; i <= split; i++) {
    splitevents.push_back(std::vector<split_ev_info>());
  }
}

int generic_manager::init(device_manager* ref) {
  this->ref = ref;
  std::unordered_map<std::string, int> event_names;
  //Check if we have no event of this name. If so, add it.
  //If we already know an event of that name, then reuse the already registered one.
  for (gen_source_event &gen_ev : descr->events) {
    if (gen_ev.split_id < 1 || gen_ev.split_id > split) continue;
    int event_id;
    int split_index = gen_ev.split_id - 1;
    auto lookup = event_names.find(gen_ev.name);
    if (lookup == event_names.end()) {
      methods.register_event(ref, {gen_ev.name.c_str(), gen_ev.descr.c_str(), gen_ev.type, ""});
      event_id = event_count++;
      event_names.insert({gen_ev.name, event_id});
    } else {
      event_id = lookup->second;
    }
    splitevents.at(split_index).push_back({gen_ev.code, gen_ev.type, event_id});
  }

  for (auto alias : descr->aliases) {
    methods.register_alias(ref, alias.first.c_str(),alias.second.c_str());
  }

  descr->split_types.resize(split,"gamepad");
  

}

manager_plugin generic_manager::get_plugin() {
  manager_plugin plug = genericman;
  plug.name = descr->name.c_str();
  if (descr->subscribe_to_gamepad)
    plug.subscribe_to_gamepad_profile = true;
  for (auto type : descr->split_types) {
    if (type == "gamepad") {
      plug.subscribe_to_gamepad_profile = true;
      break;
    }
  }
  return plug;
}

generic_manager::~generic_manager() {

  for (auto file : openfiles) {
    delete file;
  }

  delete descr;
}

bool matched(struct udev* udev, struct udev_device* dev, const device_match& match) {
  bool result = true;
  bool valid = false; //require at least one thing be matched...
  //lots of things to compute
  const char* phys = nullptr;
  const char* uniq = nullptr;
  std::string vendor_id;
  std::string product_id;
  struct udev_device* hidparent = udev_device_get_parent_with_subsystem_devtype(dev,"hid",NULL);
  struct udev_device* parent = udev_device_get_parent(dev);
  if (hidparent) {
    phys = udev_device_get_property_value(hidparent, "HID_PHYS");
    uniq = udev_device_get_property_value(hidparent, "HID_UNIQ");
  }
  //only bother parsing this string if we will later match it
  if (parent && (match.vendor != -1 || match.product != -1) ) {
    const char* productstring = udev_device_get_property_value(parent, "PRODUCT");
    std::string ids = "";
    if (productstring) ids = std::string(productstring);
    std::stringstream stream(ids);
    std::string bus;
    std::getline(stream, bus, '/');
    std::getline(stream, vendor_id, '/');
    std::getline(stream, product_id, '/');
  }
  //start checking matches.
  //result is true if ALL criteria are met
  //valid is true if AT LEAST ONE criteria is valid
  if (!match.name.empty()) {
    valid = true;
    const char* name = nullptr;
    name = udev_device_get_sysattr_value(parent, "name");
    result = result && (name && !strcmp(match.name.c_str(), name));
  }
  if (!match.uniq.empty()) {
    valid = true;
    result = result && (uniq && !strcmp(match.uniq.c_str(), uniq));
  }
  if (!match.phys.empty()) {
    valid = true;
    result = result && (phys && !strcmp(match.phys.c_str(), phys));
  }
  if (match.vendor != -1) {
    valid = true;
    int vendor = parse_hex(vendor_id);
    result = result && (match.vendor == vendor);
  }
  if (match.product != -1) {
    valid = true;
    int product = parse_hex(product_id);
    result = result && (match.product == product);
  }
  //a match must be valid as well as meeting all criteria
  return valid && result;
}

int generic_manager::accept_device(struct udev* udev, struct udev_device* dev) {
  std::lock_guard<std::mutex> lock(devlistlock);
  const char* path = udev_device_get_syspath(dev);
  const char* subsystem = udev_device_get_subsystem(dev);
  const char* action = udev_device_get_action(dev);
  if (!action) action = "null";

  if (!strcmp(action, "remove")) {
    if (!path) return -1;
    for (auto it = openfiles.begin(); it != openfiles.end(); it++) {
      (*it)->close_node(dev, true);
      if ((*it)->nodes.empty()) {
        delete(*it);
        openfiles.erase(it);
        return 0;
      }
    }
  }
  if (!strcmp(action, "add") || !strcmp(action, "null")) {
    if (!strcmp(subsystem, "input")) {
      const char* sysname = udev_device_get_sysname(dev);
      const char* name = nullptr;
      if (!strncmp(sysname, "event", 3)) {
        for (auto it = descr->matches.begin(); it != descr->matches.end(); it++) {
          if (matched(udev,dev,*it)) {
            if (open_device(udev, dev) == SUCCESS)
              return DEVICE_CLAIMED;
          }
        }
      }
    }
  }



  return DEVICE_UNCLAIMED;
}

int generic_manager::open_device(struct udev* udev, struct udev_device* dev) {
  try {
    if (flatten && openfiles.size() >= 1) {
      openfiles.front()->open_node(dev);
    } else {
      openfiles.push_back(new generic_file(mg, dev, descr->grab_ioctl, descr->grab_chmod));
      create_inputs(openfiles.back(), openfiles.back()->fds.front(), false);
    }
  } catch (...) {
    return FAILURE; //Something went wrong opening this device...
  }
  return SUCCESS;
}

void generic_manager::create_inputs(generic_file* opened_file, int fd, bool watch) {
  for (int i = 1; i <= split; i++) {
    generic_device* gendev = new generic_device(splitevents.at(i - 1), event_count, fd, watch, descr->split_types[i-1], opened_file->uniq);
    device_plugin plug = genericdev;
    plug.name_stem = descr->devname.c_str();
    plug.uniq = gendev->uniq.c_str();
    input_source* source = mg->add_device(ref, plug, gendev).get();
    if (source) opened_file->add_dev(source);
  }
}

