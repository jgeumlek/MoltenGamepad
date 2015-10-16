#include "wiimote.h"
#include <iostream>
#include <cstring>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
wiimote::wiimote(){
    epfd = epoll_create(1);
    if (epfd < 1) perror("epoll create");
    
}

wiimote::~wiimote() {
   clear_node(&buttons);
   clear_node(&accel);
   clear_node(&ir);
   clear_node(&motionplus);
   clear_node(&nunchuk);
   clear_node(&classic);
   
   for (int i = 0; i < wii_key_max; i++) {
     delete key_trans[i];
   }
   for (int i = 0; i < wii_abs_max; i++) {
     delete abs_trans[i];
   }
   void *ptr = name;
   free (ptr);
}

void wiimote::clear_node(struct dev_node* node) {
  if (node->dev) udev_device_unref(node->dev);
  node->dev = nullptr;
  if (node->fd >= 0) close(node->fd);
}

/*NOTE: Finding uses prefixes, but destroying needs an exact match.*/
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
  //TODO: Store LED files in the wiimote.
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
  struct wii_msg msg;
  switch (node) {
  case CORE:
    std::cout<< this->name << " core found." << std::endl;
    
    buttons.dev = udev_device_ref(dev);
    open_node(&buttons);
    listen_node(CORE,buttons.fd);
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
    mode = NUNCHUK_EXT;
    nunchuk.dev = udev_device_ref(dev);
    open_node(&nunchuk);
    listen_node(E_NK,nunchuk.fd);
    std::cout<< this->name << " gained a nunchuk." << std::endl;
    break;
  case E_CC:
    mode = CLASSIC_EXT;
    classic.dev = udev_device_ref(dev);
    open_node(&classic);
    listen_node(E_CC,classic.fd);
    std::cout<< this->name << " gained a classic controller." << std::endl;
    break;
  }


}

void wiimote::remove_node(const char* name) {
  if (!name) return;
  int node = name_to_node(name);
  if (node == E_NK) clear_node(&nunchuk);
  if (node == E_CC) clear_node(&classic);
  if (node == MP) {
    std::cout << this->name << " motion+ removed.";
    clear_node(&motionplus);
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
  for (int i = wm_accel_x; i <= wm_ir_y; i++) {
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
  for (int i = nk_wm_accel_x; i <= nk_stick_y; i++) {
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

void wiimote::open_node(struct dev_node* node) {
  node->fd = open(udev_device_get_devnode(node->dev), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
  if (node->fd < 0)
    perror("open subdevice:");
  ioctl(node->fd, EVIOCGRAB, this);
};

void wiimote::listen_node(int type, int fd) {
  struct epoll_event event;
  memset(&event,0,sizeof(event));


  event.events = EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP;
  event.data.u32 = type;
  int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
  if (ret< 0) perror("epoll add");
}

struct new_event {
  int type;
  int id;
  event_translator* trans;
};

int lookup_key(const char* evname) {
  if (evname == nullptr) return -1;
  for (int i = 0; i < wii_key_max; i++) {
    if (!strcmp(evname,wiimote_events_keys[i].name))
      return i;
  }
  return -1;
}
int lookup_abs(const char* evname) {
  for (int i = 0; i < wii_abs_max; i++) {
    if (!strcmp(evname,wiimote_events_axes[i].name))
      return i;
  }
  return -1;
}

void wiimote::update_map(const char* evname, event_translator* trans) {
  struct new_event msg = {0,0,trans->clone()};
  int key_id = lookup_key(evname);
  if (key_id >= 0) {
    msg.type = EVENT_KEY; msg.id = key_id;
    write(priv_pipe,&msg,sizeof(msg));
    return;
  }
  int abs_id = lookup_abs(evname);
  if (abs_id >= 0) {
    msg.type = EVENT_AXIS; msg.id = abs_id;
    write(priv_pipe,&msg,sizeof(msg));
    return;
  }
  
}

enum entry_type wiimote::entry_type(const char* name) {
  int ret = lookup_key(name);
  if (ret != -1) return DEV_KEY;
  
  ret = lookup_abs(name);
  if (ret != -1) return DEV_AXIS;
  
  return NO_ENTRY;
}

enum entry_type wiimotes::entry_type(const char* name) {
  int ret = lookup_key(name);
  if (ret != -1) return DEV_KEY;
  
  ret = lookup_abs(name);
  if (ret != -1) return DEV_AXIS;
  
  return NO_ENTRY;
}


void wiimote::read_wiimote() {
  struct epoll_event event;
  struct epoll_event events[1];

  memset(&event,0,sizeof(event));

  event.events = EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP;
  
  int internal[2];
  pipe(internal);
  event.data.ptr = this;
  epoll_ctl(epfd, EPOLL_CTL_ADD, internal[0], &event);

  
  priv_pipe = internal[1];

  while (!(keep_looping)) {
    int n = epoll_wait(epfd, events, 1, -1);
    if (n < 0) {perror("epoll wait:");break;}
    if (events[0].data.ptr == this) {
      struct new_event msg;
      int ret = 1;
      ret = read(internal[0],&msg,sizeof(msg));
      if (ret = sizeof(msg)) {
        event_translator** array = (msg.type = EVENT_KEY) ? key_trans : abs_trans;
        delete (array + msg.id);
        *(array + msg.id) = msg.trans;
      }
    } else {
      switch (events[0].data.u32) {
        case CORE:
          process_core();
          break;
        case E_CC:
          process_classic(classic.fd);
          break;
        case E_NK:
          process_nunchuk(nunchuk.fd);
      }
    }
  }
  std::cout << "stopping wiimote thread" << std::endl;
};


int wiimotes::accept_device(struct udev* udev, struct udev_device* dev) {
  const char* path = udev_device_get_syspath(dev);
  const char* subsystem = udev_device_get_subsystem(dev);

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
        //exact path found, and destroyed.
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
    slot_man->request_slot(wm);
    wm->thread = new std::thread(&wiimote::read_wiimote,wm);
    wm->load_profile(&mapprofile);
  } else {
    //pass this device to it for proper storage
    existing->handle_event(dev);
  }


  return 0;
}

void wiimotes::update_maps(const char* evname, event_translator* trans) {
  mapprofile.set_mapping(evname, trans);
  for (auto it = wii_devs.begin(); it != wii_devs.end(); it++)
    (*it)->update_map(evname,trans);
}

input_source* wiimotes::find_device(const char* name) {
  for (auto it = wii_devs.begin(); it != wii_devs.end(); it++) {
    if (!strcmp((*it)->name,name)) return (*it);
  }
  return nullptr;
}

