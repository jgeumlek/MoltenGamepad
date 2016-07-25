#include "generic.h"
#include <algorithm>

generic_manager::generic_manager(moltengamepad* mg, generic_driver_info& descr) : device_manager(mg,descr.name) {
  this->devname = descr.devname.c_str();

  this->descr = &descr;

  split = descr.split;
  flatten = descr.flatten;

  for (int i = 1; i <= split; i++) {
    splitevents.push_back(std::vector<gen_source_event>());
  }

  for (auto gen_ev : descr.events) {
    if (gen_ev.split_id < 1 || gen_ev.split_id > split) continue;
    splitevents.at(gen_ev.split_id - 1).push_back(gen_ev);
    mapprofile->set_mapping(gen_ev.name, new event_translator(), gen_ev.type, true);
  }

  for (auto alias : descr.aliases) {
    mapprofile->set_alias(alias.first,alias.second);
  }

  descr.split_types.resize(split,"gamepad");
  for (auto type : descr.split_types) {
    if (type == "gamepad") {
      mg->gamepad->copy_into(mapprofile, true);
      break;
    }
  }

  mg->drivers.take_message(std::string(name) + " driver initialized.");

}

generic_manager::~generic_manager() {

  for (auto file : openfiles) {
    delete file;
  }

  delete descr;
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
        struct udev_device* parent = udev_device_get_parent(dev);
        name = udev_device_get_sysattr_value(parent, "name");
        if (!name) return -2;

        for (auto it = descr->matches.begin(); it != descr->matches.end(); it++) {
          if (!strcmp((*it).name.c_str(), name)) {
            return open_device(udev, dev);
          }
        }
      }
    }
  }



  return -2;
}

int generic_manager::open_device(struct udev* udev, struct udev_device* dev) {
  try {
    if (flatten) {
      if (openfiles.size() < 1) {
        openfiles.push_back(new generic_file(dev, descr->grab_ioctl, descr->grab_chmod));
        openfiles.front()->mg = mg;
        create_inputs(openfiles.front(), openfiles.front()->fds.front(), false);
      } else {
        openfiles.front()->open_node(dev);
      }
    } else {
      openfiles.push_back(new generic_file(dev, descr->grab_ioctl, descr->grab_chmod));
      openfiles.back()->mg = mg;
      create_inputs(openfiles.back(), openfiles.back()->fds.front(), false);
    }
  } catch (...) {
    return -1; //Something went wrong opening this device...
  }
  return 0;
}

void generic_manager::create_inputs(generic_file* opened_file, int fd, bool watch) {
  for (int i = 1; i <= split; i++) {
    generic_device* gendev = new generic_device(splitevents.at(i - 1), fd, watch, mg->slots, this, descr->split_types[i-1]);
    opened_file->add_dev(gendev);
    mg->add_device(gendev, this, descr->devname);
    auto devprofile = gendev->get_profile();
    mapprofile->copy_into(devprofile,true);
  }
}

