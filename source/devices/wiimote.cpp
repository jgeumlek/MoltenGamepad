#include "wiimote.h"
#include <iostream>
#include <cstring>

wii_dev* find_wii_dev_by_path(std::vector<wii_dev*>* devs, const char* syspath) {
  if (syspath == nullptr) return nullptr;
  for (auto it = devs->begin(); it != devs->end(); ++it) {
    const char* devpath = udev_device_get_syspath( (*it)->base.dev);
    if (!devpath) continue;
    
    if (strstr(syspath,devpath) == syspath) return (*it);
  }
  return nullptr;
}

int destroy_wii_dev_by_path(std::vector<wii_dev*>* devs, const char* syspath) {
  if (syspath == nullptr) return -1;
  for (auto it = devs->begin(); it != devs->end(); ++it) {
    const char* devpath = udev_device_get_syspath( (*it)->base.dev);
    if (!devpath) continue;
    
    if (!strcmp(devpath,syspath)) {
      delete *it;
      devs->erase(it);
      return 0;
    }
  }
  return -1;
}



void add_led(struct wii_leds* leds, struct udev_device* dev) {
}

void wiimote::handle_event(struct udev_device* dev) {
  const char* subsystem = udev_device_get_subsystem(dev);
  const char* action = udev_device_get_action(dev);
  
  if (action && !strcmp(action,"remove")) {
    const char* nodename = udev_device_get_sysattr_value(udev_device_get_parent(dev),"name");
    if (!nodename) nodename = "";
    remove_node(nodename);
    return;
  }
  if (!subsystem || !strcmp(subsystem,"hid")) return; //Nothing to do.

  if (!strcmp(subsystem,"led")) {
    add_led(&leds, dev);
  }

  if (!strcmp(subsystem,"input")) {
    const char* sysname = udev_device_get_sysname(dev);
    const char* name = nullptr; 


    if (!strncmp(sysname,"event",3)) {
      struct udev_device* parent = udev_device_get_parent(dev);

      name = udev_device_get_sysattr_value(parent,"name");

      store_node(dev,name);

    }

  
  }
}

enum nodes {CORE, E_NK, E_CC,  IR, ACCEL, MP, NONE};
int name_to_node(const char* name) {
  if (!strcmp(name, WIIMOTE_NAME)) return CORE;
  if (!strcmp(name, WIIMOTE_IR_NAME)) return IR;
  if (!strcmp(name, WIIMOTE_ACCEL_NAME)) return ACCEL;
  if (!strcmp(name, MOTIONPLUS_NAME)) return MP;
  if (!strcmp(name, NUNCHUK_NAME)) return E_NK;
  if (!strcmp(name, CLASSIC_NAME)) return E_CC;
  return NONE;
}

void wiimote::store_node(struct udev_device* dev, const char* name) {
  if (!name || !dev) return;
  int node = name_to_node(name);
  switch (node) {
  case CORE:
    std::cout<< this->name << " core found." << std::endl;
    buttons.dev = udev_device_ref(dev);
    break;
  case IR:
    std::cout<< this->name << " IR found." << std::endl;
    ir.dev = udev_device_ref(dev);
    break;
  case ACCEL:
    std::cout<< this->name << " accelerometers found." << std::endl;
    accel.dev = udev_device_ref(dev);
    break;
  case MP:
    std::cout<< this->name << " motion+ found." << std::endl;
    motionplus.dev = udev_device_ref(dev);
    break;
  case E_NK:
    remove_extension();
    extension = new ext_nunchuk();
    mode = NUNCHUK_EXT;
    extension->parent = this;
    extension->node.dev = udev_device_ref(dev);
    std::cout<< this->name << " gained a nunchuk." << std::endl;
    break;
  case E_CC:
    remove_extension();
    extension = new ext_classic();
    mode = CLASSIC_EXT;
    extension->parent = this;
    extension->node.dev = udev_device_ref(dev);
    std::cout<< this->name << " gained a classic controller." << std::endl;
    break;
  }


}

void wiimote::remove_node(const char* name) {
  if (!name) return;
  int node = name_to_node(name);
  if (node == E_NK || node == E_CC) remove_extension();
  if (node == MP) {
    udev_device_unref(motionplus.dev);
    std::cout << this->name << " motion+ removed.";
    motionplus.dev = nullptr;
  }
}

void wiimote::list_events(cat_list &list) {
  struct category cat;
  struct name_descr info;
  
  cat.name = "Wiimote";
  info.data = EVENT_KEY;
  for (int i = wm_a; i <= wm_down; i++) {
    info.name = wiimote_events_keys[i].name;
    info.descr = wiimote_events_keys[i].descr;
    cat.entries.push_back(info);
  }
  info.data = EVENT_AXIS;
  for (int i = wm_accel_x; i <= wm_accel_z; i++) {
    info.name = wiimote_events_axes[i].name;
    info.descr = wiimote_events_axes[i].descr;
    cat.entries.push_back(info);
  }
  list.push_back(cat);
  cat.entries.clear();
  
  cat.name = "Nunchuk";
  info.data = EVENT_KEY;
  for (int i = nk_a; i <= nk_z; i++) {
    info.name = wiimote_events_keys[i].name;
    info.descr = wiimote_events_keys[i].descr;
    cat.entries.push_back(info);
  }
  info.data = EVENT_AXIS;
  for (int i = nk_wm_accel_x; i <= nk_accel_z; i++) {
    info.name = wiimote_events_axes[i].name;
    info.descr = wiimote_events_axes[i].descr;
    cat.entries.push_back(info);
  }
  list.push_back(cat);
  cat.entries.clear();
  
  cat.name = "Classic";
  info.data = EVENT_KEY;
  for (int i = cc_a; i <= cc_zr; i++) {
    info.name = wiimote_events_keys[i].name;
    info.descr = wiimote_events_keys[i].descr;
    cat.entries.push_back(info);
  }
  info.data = EVENT_AXIS;
  for (int i = cc_left_x; i <= cc_right_y; i++) {
    info.name = wiimote_events_axes[i].name;
    info.descr = wiimote_events_axes[i].descr;
    cat.entries.push_back(info);
  }
  list.push_back(cat);
  cat.entries.clear();
  
  
}

int wiimotes::accept_device(struct udev* udev, struct udev_device* dev) {
  const char* path = udev_device_get_syspath(dev);
  const char* subsystem = udev_device_get_subsystem(dev);



  //Check this is a hid driven by the wiimote driver. 
  //Only the base wiimote device passes this check.
  //(extensions, leds, etc. are not hid)

  const char* action = udev_device_get_action(dev);
  if (!action) action = "null";

  if (!strcmp(action,"remove")) {

    const char* syspath = udev_device_get_syspath(dev);
    wii_dev* existing = find_wii_dev_by_path(&wii_devs, syspath);

    if (existing) {
      int ret = destroy_wii_dev_by_path(&wii_devs, syspath);
      if (ret) {
        //exact path wasn't found, therefore this is not a parent device.
        existing->handle_event(dev);
      } else {
        std::cout << "Wii device removed." << std::endl;
      }
      return 0;
    }
    return -1;
    

  }
  if (!subsystem) return -1;

  struct udev_device* parent = udev_device_get_parent_with_subsystem_devtype(dev,"hid",nullptr);
  if (!strcmp(udev_device_get_subsystem(dev),"hid")) {
    parent = dev; //We are already looking at it!
  }
  if (!parent) return -2;

  const char* driver = udev_device_get_driver(parent);
  if (!driver || strcmp(driver,"wiimote")) return -2;

  const char* parentpath = udev_device_get_syspath(parent);

  wii_dev* existing = find_wii_dev_by_path(&wii_devs, parentpath);

  if (existing == nullptr) {
    //time to add a device;
    std::cout << "Wiimote found (count:" << wii_devs.size() << " )" << std::endl;;
    wiimote* wm = new wiimote();
    char* devname;
    asprintf(&devname, "wm%d",++dev_counter);
    wm->name = devname;
    wm->base.dev = udev_device_ref(parent);
    wm->handle_event(dev);
    wii_devs.push_back(wm);
  } else {
    //pass this device to it for proper storage
    existing->handle_event(dev);
  }


  return 0;
}

