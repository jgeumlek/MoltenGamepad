#include "wiimote.h"
#include <iostream>
#include <cstring>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>


manager_methods wiimote_manager::methods;
int (*wiimote_manager::request_slot) (input_source*);
int (*wiimote_manager::grab_permissions) (udev_device*, bool);
bool wiimote_manager::auto_assign_balance;

/*NOTE: Finding uses prefixes, but destroying needs an exact match.
  This allows easily adding/removing subnodes while only deleting
  if the base node is removed                                    */
wiimote* wiimote_manager::find_wii_dev_by_path(const char* syspath) {
  if (syspath == nullptr) return nullptr;
  for (auto it = wii_devs.begin(); it != wii_devs.end(); ++it) {
    const char* devpath = udev_device_get_syspath((*it)->base.dev);
    if (!devpath) continue;

    if (strstr(syspath, devpath) == syspath) return (*it);
  }
  return nullptr;
}

int wiimote_manager::destroy_wii_dev_by_path(const char* syspath) {
  if (syspath == nullptr) return -1;
  for (auto it = wii_devs.begin(); it != wii_devs.end(); ++it) {
    const char* devpath = udev_device_get_syspath((*it)->base.dev);
    if (!devpath) continue;

    if (!strcmp(devpath, syspath)) {
      methods.remove_device(ref,(*it)->ref);
      wii_devs.erase(it);
      return 0;
    }
  }
  return -1;
}

int wiimote_manager::accept_device(struct udev* udev, struct udev_device* dev) {
  std::lock_guard<std::mutex> lock(devlistlock);
  const char* path = udev_device_get_syspath(dev);
  const char* subsystem = udev_device_get_subsystem(dev);

  const char* action = udev_device_get_action(dev);
  if (!action) action = "null";

  if (!strcmp(action, "remove")) {

    const char* syspath = udev_device_get_syspath(dev);
    wiimote* existing = find_wii_dev_by_path(syspath);

    if (existing) {
      int ret = destroy_wii_dev_by_path(syspath);
      if (ret) {
        //exact path wasn't found, therefore this is not a parent device.
        existing->handle_event(dev);
      } else {
        //exact path found, and destroyed.
      }
      return DEVICE_CLAIMED;
    }
    return DEVICE_UNCLAIMED;


  }
  if (!subsystem) return DEVICE_UNCLAIMED;

  struct udev_device* parent = udev_device_get_parent_with_subsystem_devtype(dev, "hid", nullptr);
  if (!strcmp(udev_device_get_subsystem(dev), "hid")) {
    parent = dev; //We are already looking at it!
  }
  if (!parent) return DEVICE_UNCLAIMED;

  const char* driver = udev_device_get_driver(parent);
  if (!driver || strcmp(driver, "wiimote")) return -2;

  const char* parentpath = udev_device_get_syspath(parent);
  const char* uniq = udev_device_get_property_value(parent, "HID_UNIQ");
  const char* phys = udev_device_get_property_value(parent, "HID_PHYS");
  wiimote* existing = find_wii_dev_by_path(parentpath);

  if (existing == nullptr) {
    //time to add a device;
    device_plugin wm = wiidev;
    wm.uniq = uniq ? uniq : "";
    wm.phys = phys ? phys : "";
    wiimote* wm_data = new wiimote();
    wm_data->base.dev = udev_device_ref(parent);
    wii_devs.push_back(wm_data);
    methods.add_device(ref, wm, wm_data);
    wm_data->handle_event(dev);
  } else {
    //This is a subdevice of something we already track
    //pass this subdevice to it for proper storage
    existing->handle_event(dev);
  }


  return DEVICE_CLAIMED;
}

int wiimote_manager::process_manager_option(const char* name, const MGField value) {
  if (!strcmp(name, "auto_assign_balance")) {
    auto_assign_balance = value.boolean;
    return SUCCESS;
  }

  return FAILURE;
}
