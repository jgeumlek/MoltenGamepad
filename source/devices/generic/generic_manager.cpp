#include "generic.h"

generic_manager::generic_manager(moltengamepad* mg, generic_driver_info& descr) : device_manager(mg) {
  this->name = descr.name.c_str();
  this->devname = descr.devname.c_str();
  mapprofile.name = name;

  this->descr = &descr;

  split = descr.split;
  flatten = descr.flatten;

  if (split > 1) {
    for (int i = 1; i <= split; i++) {
      splitevents.push_back(std::vector<gen_source_event>());
    }

    for (auto gen_ev : descr.events) {
      if (gen_ev.split_id < 1 || gen_ev.split_id > split) continue;
      splitevents.at(gen_ev.split_id - 1).push_back(gen_ev);

    }

  }
  for (auto alias : descr.aliases) {
    mapprofile.set_alias(alias.first,alias.second);
  }

  std::cout << name << " driver initialized." << std::endl;

}

generic_manager::~generic_manager() {

  for (auto file : openfiles) {
    delete file;
  }

  delete descr;
}

int generic_manager::accept_device(struct udev* udev, struct udev_device* dev) {
  const char* path = udev_device_get_syspath(dev);
  const char* subsystem = udev_device_get_subsystem(dev);
  const char* action = udev_device_get_action(dev);
  if (!action) action = "null";

  if (!strcmp(action, "remove")) {
    if (!path) return -1;
    for (auto it = openfiles.begin(); it != openfiles.end(); it++) {
      (*it)->close_node(dev, true);
      if ((*it)->nodes.empty()) {
        std::cout << "Generic driver " << name << " lost a device." << std::endl;
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
  if (split == 1) {
    generic_device* gendev = new generic_device(descr->events, fd, watch, mg->slots, descr->split_types[1]);
    char* newdevname = nullptr;
    asprintf(&newdevname, "%s%d", devname.c_str(), ++dev_counter);
    gendev->nameptr = newdevname;
    gendev->name = newdevname;
    opened_file->add_dev(gendev);
    mg->add_device(gendev);
    gendev->start_thread();
    gendev->load_profile(&mapprofile);
  } else {
    for (int i = 1; i <= split; i++) {
      generic_device* gendev = new generic_device(splitevents.at(i - 1), fd, watch, mg->slots, descr->split_types[i]);
      char* newdevname = nullptr;
      asprintf(&newdevname, "%s%d", devname.c_str(), ++dev_counter);
      gendev->nameptr = newdevname;
      gendev->name = newdevname;
      opened_file->add_dev(gendev);
      mg->add_device(gendev);
      gendev->start_thread();
      gendev->load_profile(&mapprofile);
    }
  }
}

void generic_manager::update_maps(const char* evname, event_translator* trans) {
  auto type = entry_type(evname);
  mapprofile.set_mapping(evname, trans->clone(), type);

  for (auto file : openfiles) {
    for (auto dev : file->devices) {
      dev->update_map(evname, trans);
    }
  }
}


void generic_manager::update_option(const char* opname, const char* value) {
  //Generic devices currently have no options.
};

void generic_manager::update_advanceds(const std::vector<std::string>& names, advanced_event_translator* trans) {
  if (trans) {
    mapprofile.set_advanced(names, trans->clone());
  } else {
    mapprofile.set_advanced(names, nullptr);
  }
  for (auto file : openfiles) {
    for (auto dev : file->devices) {
      dev->update_advanced(names, trans);
    }
  }
}

input_source* generic_manager::find_device(const char* name) {

  for (auto file : openfiles) {
    for (auto dev : file->devices) {
      if (!strcmp(dev->name, name)) return dev;
    }
  }
  return nullptr;
}
enum entry_type generic_manager::entry_type(const char* name) {
  auto alias = mapprofile.get_alias(std::string(name));
  if (!alias.empty())
    name = alias.c_str();
  for (auto ev : descr->events) {
    if (!strcmp(ev.name.c_str(), name)) return ev.type;
  }
  return NO_ENTRY;
}
