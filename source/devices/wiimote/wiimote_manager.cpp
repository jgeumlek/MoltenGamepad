#include "wiimote.h"
#include <iostream>
#include <cstring>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>




/*NOTE: Finding uses prefixes, but destroying needs an exact match.
  This allows easily adding/removing subnodes while only deleting
  if the base node is removed                                    */
wiimote* find_wii_dev_by_path(std::vector<wiimote*>* devs, const char* syspath) {
  if (syspath == nullptr) return nullptr;
  for (auto it = devs->begin(); it != devs->end(); ++it) {
    const char* devpath = udev_device_get_syspath((*it)->base.dev);
    if (!devpath) continue;

    if (strstr(syspath, devpath) == syspath) return (*it);
  }
  return nullptr;
}

int destroy_wii_dev_by_path(moltengamepad* mg, std::vector<wiimote*>* devs, const char* syspath) {
  if (syspath == nullptr) return -1;
  for (auto it = devs->begin(); it != devs->end(); ++it) {
    const char* devpath = udev_device_get_syspath((*it)->base.dev);
    if (!devpath) continue;

    if (!strcmp(devpath, syspath)) {
      mg->remove_device(*it);
      devs->erase(it);
      return 0;
    }
  }
  return -1;
}

int wiimote_manager::accept_device(struct udev* udev, struct udev_device* dev) {
  const char* path = udev_device_get_syspath(dev);
  const char* subsystem = udev_device_get_subsystem(dev);

  const char* action = udev_device_get_action(dev);
  if (!action) action = "null";

  if (!strcmp(action, "remove")) {

    const char* syspath = udev_device_get_syspath(dev);
    wiimote* existing = find_wii_dev_by_path(&wii_devs, syspath);

    if (existing) {
      int ret = destroy_wii_dev_by_path(mg, &wii_devs, syspath);
      if (ret) {
        //exact path wasn't found, therefore this is not a parent device.
        existing->handle_event(dev);
      } else {
        //exact path found, and destroyed.
      }
      return 0;
    }
    return -1;


  }
  if (!subsystem) return -1;

  struct udev_device* parent = udev_device_get_parent_with_subsystem_devtype(dev, "hid", nullptr);
  if (!strcmp(udev_device_get_subsystem(dev), "hid")) {
    parent = dev; //We are already looking at it!
  }
  if (!parent) return -2;

  const char* driver = udev_device_get_driver(parent);
  if (!driver || strcmp(driver, "wiimote")) return -2;

  const char* parentpath = udev_device_get_syspath(parent);

  wiimote* existing = find_wii_dev_by_path(&wii_devs, parentpath);

  if (existing == nullptr) {
    //time to add a device;
    wiimote* wm = new wiimote(mg->slots, this);
    wm->base.dev = udev_device_ref(parent);
    wm->handle_event(dev);
    wii_devs.push_back(wm);
    mg->add_device(wm, this, "wm");
    std::shared_ptr<profile> devprofile = wm->get_profile();
    mapprofile->copy_into(devprofile,true);
  } else {
    //This is a subdevice of something we already track
    //pass this subdevice to it for proper storage
    existing->handle_event(dev);
  }


  return 0;
}
