#include "joycon.h"
#include <linux/input.h>
#include <linux/hidraw.h>
#include <sys/ioctl.h>
#include <stdio.h>


manager_methods joycon_manager::methods;

int (*joycon_manager::grab_permissions) (udev_device*, bool);

joycon_manager::joycon_manager() {

}

int joycon_manager::init(device_manager* ref) {
  this->ref = ref;

  init_profile();

  return 0;
}

int joycon_manager::start() {
  return 0;
}

joycon_manager::~joycon_manager() {
  //as of now, this is only called as MoltenGamepad exits.
  //Now would be a good time to tidy any files or sockets.
}

void joycon_manager::init_profile() {

  const event_decl* ev = &joycon_events[0];
  for (int i = 0; ev->name && *ev->name; ev = &joycon_events[++i]) {
    methods.register_event(ref, *ev);
  }

  //create a convenience function...
  auto set_alias = [&] (const char* external, const char* internal) {
    methods.register_alias(ref, external, internal);
  };

  //Init some aliases...
  set_alias("first","a");
  set_alias("second","b");
  set_alias("third","x");
  set_alias("fourth","y");
  set_alias("tr","r");
  set_alias("tl","l");
  set_alias("mode","home");
  set_alias("tr2","zr");
  set_alias("tl2","zl");
  set_alias("start","plus");
  set_alias("select","minus");

  methods.register_event_group(ref, {"solo_stick","solo_x,solo_y","Solo JoyCon Stick","stick(left_x,left_y)"});
};

int joycon_manager::accept_device(struct udev* udev, struct udev_device* dev) {
  const char* subsystem = udev_device_get_subsystem(dev);
  const char* action = udev_device_get_action(dev);
  const char* syspath = udev_device_get_syspath(dev);
  if (!subsystem || strncmp(subsystem,"hidraw",6))
    return DEVICE_UNCLAIMED;

  //check for removal events
  //This is a bit complicated, as we might remove only one half of a JoyCon partnership.

  if (action && !strcmp(action,"remove") && syspath) {
    std::lock_guard<std::mutex> guard(lock);
    std::string syspath_str(syspath);
    auto it = joycons.begin();
    joycon* jc = nullptr;
    int foundside = -1;
    //search list for any joycon with this syspath.
    //A JoyCon object might represent a partnership, where only one side is being removed...
    for (it = joycons.begin(); it != joycons.end(); it++) {
      jc = it->jc;
      if (jc->path[0] == syspath_str) {
        close(jc->fds[0]);
        foundside = 0;
        break;
      } else if (jc->path[1] == syspath_str) {
        close(jc->fds[1]);
        foundside = 1;
        break;
      }
    }

    //Did we get a match?
    if (foundside >= 0) {
      joycon* remainingside = nullptr;
      int otherside = 1 - foundside;
      
      if (jc->fds[otherside] > 0 && jc->path[otherside] != syspath_str) {
        //Our match was in a partnership!
        //Make sure to keep the other joycon around.
        jc->close_out_fd[otherside] = false;
        //We are going to delete the partnership.
        //Treat the remaining joycon as if it was a new device.
        //(Which fits with our joycon activation logic...)
        remainingside = new joycon(jc->fds[otherside], -1, jc->sides[otherside], UNKNOWN_JOYCON, jc->path[otherside].c_str(), nullptr, this);
      }
      methods.remove_device(ref,jc->ref);
      joycons.erase(it);
      
      if (remainingside) {
        joycon_info info;
        info.jc = remainingside;
        info.active_trigger = false;
        joycons.push_back(info);
        methods.add_device(ref,joycon_dev, remainingside);
      }
      return DEVICE_CLAIMED;
    }
    return DEVICE_UNCLAIMED;
  }
        

  if (action && strncmp(action,"add",3)) {
    //this is neither an enumeration (action is nullptr) or an add event
    return DEVICE_UNCLAIMED;
  }

  const char* path = udev_device_get_devnode(dev);
  if (!path)
    return DEVICE_UNCLAIMED;

  //try to open it to use some ioctls for info gathering.
  //Safe to do on unrelated hidraw nodes?
  //Also catches permission issues...

  //A bit hackish, and poor error reporting. Perhaps try to identify JoyCon before opening node.

  int fd = open(path, O_RDWR | O_NONBLOCK);
  if (fd < 0)
    return DEVICE_UNCLAIMED;

  char buffer[256];
  memset(buffer,0,256);

  int res = ioctl(fd, HIDIOCGRAWNAME(256), buffer);
  if (res < 0) {
    close(fd);
    return DEVICE_UNCLAIMED;
  }


  JoyConSide side = UNKNOWN_JOYCON;
  if (!strcmp(buffer,RIGHT_JOYCON_NAME))
    side = RIGHT_JOYCON;
  if (!strcmp(buffer,LEFT_JOYCON_NAME))
    side = LEFT_JOYCON;

  if (side == UNKNOWN_JOYCON) {
    close(fd);
    return DEVICE_UNCLAIMED;
  }

  std::lock_guard<std::mutex> guard(lock);
  grab_permissions(dev, true);
  joycon* new_joycon = new joycon(fd,-1, side, UNKNOWN_JOYCON, syspath, nullptr, this);
  joycon_info info;
  info.jc = new_joycon;
  info.active_trigger = false;
  joycons.push_back(info);
  methods.add_device(ref,joycon_dev,new_joycon);
  return DEVICE_CLAIMED;

}

int joycon_manager::process_option(const char* name, const MGField value) {
  return 0;
}


void joycon_manager::check_partnership(joycon* jc) {
  if (!jc) return;
  
  if (jc->fds[0] > 0 && jc->fds[1] > 0) {
    //this should not happen...
    //TODO: Break the pair into two pending separate joycon*
  }
  int jc_index = -1;
  int partner_index = -1;
  bool do_solo = false;
  if (jc->active_solo_btns[0]) {
    do_solo = true;
  }
  
  //search for this jc in our list...
  for (int i = 0; i < joycons.size(); i++) {
    if ( joycons[i].jc == jc) {
      jc_index = i;
      joycons[i].active_trigger = jc->active_trigger[0];
      if (!jc->active_trigger[0] || do_solo)
        break; //we are done here...
    }
    //skip ones that aren't looking to partner with this one.
    if ( joycons[i].jc->activated || joycons[i].jc->sides[0] == jc->sides[0])
      continue; //For now, don't allow partnering two of the same JoyCon.

    if (jc->active_trigger[0] && joycons[i].active_trigger)
      partner_index = i;

  }

  if (jc_index < 0) {
    //not in our list of joycons?
  }
  
  if (do_solo && jc_index >= 0) {
    jc->mode = SOLO;
    jc->activated = true;
    methods.print(ref,"Activating a solo JoyCon");
    return;
  }

  if (jc_index >= 0 && partner_index >= 0 && jc_index != partner_index) {
    joycon* partner = joycons[partner_index].jc;
    if (partner == jc)
      return;

    //remove the partner as a separate device, but keep its file descriptors.
    partner->close_out_fd[0] = false;
    joycons.erase(joycons.begin() + partner_index);

    jc->sides[1] = partner->sides[0];
    jc->path[1] = partner->path[0];
    jc->fds[1] = partner->fds[0];
    methods.remove_device(ref,partner->ref);
    jc->methods.watch_file(jc->ref, jc->fds[1], (void*) 1);
    jc->mode = PARTNERED;
    jc->activated = true;
    methods.print(ref, "Activating two combined JoyCon");
  }
}
  
