#include "wiimote.h"
#include <iostream>
#include <cstring>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

wiimote::wiimote(slot_manager* slot_man, device_manager* manager, const std::string& uniq) : input_source(slot_man, manager, "gamepad") {
  this->uniq = uniq;
  descr = "Wii remote";
}

wiimote::~wiimote() {
  clear_node(&base);
  clear_node(&buttons);
  clear_node(&accel);
  clear_node(&ir);
  clear_node(&motionplus);
  clear_node(&nunchuk);
  clear_node(&classic);
  clear_node(&pro);
  clear_node(&balance);
}

int wiimote::process_option(const char* name, const char* value) {
  if (!strcmp(name, "wm_accel_active")) {
    return read_bool(std::string(value), [&] (bool val) {
      this->wm_accel_active = val;
      update_mode(mode);
    });
  }
  if (!strcmp(name, "nk_accel_active")) {
    return read_bool(std::string(value), [&] (bool val) {
      this->nk_accel_active = val;
      update_mode(mode);
    });
  }
  if (!strcmp(name, "wm_ir_active")) {
    return read_bool(std::string(value), [&] (bool val) {
      this->wm_ir_active = val;
      update_mode(mode);
    });
  }
  if (!strcmp(name, "nk_ir_active")) {
    return read_bool(std::string(value), [&] (bool val) {
      this->nk_ir_active = val;
      update_mode(mode);
    });
  }
  if (!strcmp(name, "grab_exclusive")) {
    return read_bool(std::string(value), [&] (bool val) {
      this->grab_exclusive = val;
      grab_ioctl(val);
    });
  }
  if (!strcmp(name, "grab_permissions")) {
    return read_bool(std::string(value), [&] (bool val) {
      this->grab_permissions = val;
      grab_chmod(val);
    });
    return -1;
  }

  return -1;
}

void wiimote::clear_node(struct dev_node* node) {
  if (grab_exclusive) grab_ioctl_node(node, false);
  if (node->fix_mode) grab_chmod_node(node, false);
  if (node->dev) udev_device_unref(node->dev);
  node->dev = nullptr;
  if (node->fd >= 0) close(node->fd);
}

std::string wiimote::get_description() const {
  if (pro.fd > 0) return "Wii U Pro Controller";
  if (classic.fd > 0) return "Wii Remote with Classic Controller";
  if (nunchuk.fd > 0) return "Wii Remote with Nunchuk Controller";
  if (balance.fd > 0) return "Wii Balance Board";
  if (buttons.fd > 0) return "Wii Remote";
  return "Wii device (???)";
}

std::string wiimote::get_type() const {
  if (balance.fd > 0) return "balance";
  return "gamepad";
}


void add_led(struct wii_leds* leds, struct udev_device* dev) {
  //TODO: Store LED files in the wiimote.
}

void wiimote::handle_event(struct udev_device* dev) {
  const char* subsystem = udev_device_get_subsystem(dev);
  const char* action = udev_device_get_action(dev);

  if (action && !strcmp(action, "remove")) {
    const char* nodename = udev_device_get_sysattr_value(udev_device_get_parent(dev), "name");
    if (!nodename) nodename = "";
    remove_node(nodename);
    return;
  }
  if (!subsystem || !strcmp(subsystem, "hid")) return; //Nothing to do.

  if (!strcmp(subsystem, "led")) {
    add_led(&leds, dev);
  }

  if (!strcmp(subsystem, "input")) {
    const char* sysname = udev_device_get_sysname(dev);
    const char* name = nullptr;


    if (!strncmp(sysname, "event", 3)) {
      struct udev_device* parent = udev_device_get_parent(dev);

      name = udev_device_get_sysattr_value(parent, "name");

      store_node(dev, name);

    }
  }
}

enum nodes {CORE, E_NK, E_CC,  IR, ACCEL, MP, BALANCE, WII_U_PRO, NONE};
int name_to_node(const char* name) {
  if (!strcmp(name, WIIMOTE_NAME)) return CORE;
  if (!strcmp(name, WIIMOTE_IR_NAME)) return IR;
  if (!strcmp(name, WIIMOTE_ACCEL_NAME)) return ACCEL;
  if (!strcmp(name, MOTIONPLUS_NAME)) return MP;
  if (!strcmp(name, NUNCHUK_NAME)) return E_NK;
  if (!strcmp(name, CLASSIC_NAME)) return E_CC;
  if (!strcmp(name, BALANCE_BOARD_NAME)) return BALANCE;
  if (!strcmp(name, WII_U_PRO_NAME)) return WII_U_PRO;
  return NONE;
}

void wiimote::store_node(struct udev_device* dev, const char* name) {
  if (!name || !dev) return;
  int node = name_to_node(name);
  switch (node) {
  case CORE:
    manager->log.take_message(this->name + " core found.");

    buttons.dev = udev_device_ref(dev);
    open_node(&buttons);
    if (mode != NUNCHUK_EXT && mode != CLASSIC_EXT)
      update_mode(NO_EXT); //A core lets us know this is a wiimote, not a Wii U pro or balance board.
      //But if they were processed out of order, NUNCHUK_EXT or CLASSIC_EXT take precedence!
    break;
  case IR:
    manager->log.take_message(this->name + " IR found.");
    ir.dev = udev_device_ref(dev);
    if ((mode == NO_EXT && wm_ir_active) || (mode == NUNCHUK_EXT && nk_ir_active))
      open_node(&ir);
    break;
  case ACCEL:
    manager->log.take_message(this->name + " accelerometers found.");
    accel.dev = udev_device_ref(dev);
    if ((mode == NO_EXT && wm_accel_active) || (mode == NUNCHUK_EXT && nk_accel_active))
      open_node(&ir);
    break;
  case MP:
    manager->log.take_message(this->name + " motion+ found.");
    motionplus.dev = udev_device_ref(dev);
    break;
  case E_NK:
    nunchuk.dev = udev_device_ref(dev);
    open_node(&nunchuk);
    update_mode(NUNCHUK_EXT);
    manager->log.take_message(this->name + " gained a nunchuk.");
    break;
  case E_CC:
    classic.dev = udev_device_ref(dev);
    open_node(&classic);
    update_mode(CLASSIC_EXT);
    manager->log.take_message(this->name + " gained a classic controller.");
    break;
  case BALANCE:
    balance.dev = udev_device_ref(dev);
    open_node(&balance);
    update_mode(BALANCE_EXT);
    break;
  case WII_U_PRO:
    pro.dev = udev_device_ref(dev);
    open_node(&pro);
    update_mode(PRO_EXT);
    break;
  }
}

void wiimote::update_mode(modes new_mode) {
  //Depending on our mode and options,
  //We need to possibly open or close some device nodes,
  //and toggle various event states.
  std::lock_guard<std::mutex> guard(mode_lock);

  //Only a Wii U Pro Controller has clickable sticks.
  //So we should disabled this events for normal wiimotes.
  //But a Pro controller looks like a normal wiimote at first,
  //And events cannot be reactivated after being disabled.
  if (mode == MODE_UNCERTAIN && new_mode != MODE_UNCERTAIN && new_mode != PRO_EXT) {
    //Finally enough evidence to conclude this is not a Wii U Pro.
    toggle_event(cc_thumbl, EVENT_DISABLED);
    toggle_event(cc_thumbr, EVENT_DISABLED);
  }
  //Similar story for balance boards
  if (mode == MODE_UNCERTAIN && new_mode != MODE_UNCERTAIN && new_mode != BALANCE_EXT) {
    //Finally enough evidence to conclude this is not a Balance Board.
    toggle_event(bal_x, EVENT_DISABLED);
    toggle_event(bal_y, EVENT_DISABLED);
  }

  if (mode != MODE_UNCERTAIN && new_mode == MODE_UNCERTAIN)
    return; //MODE_UNCERTAIN is for initialization. We shouldn't go back to it.
  
  mode = new_mode;
  char event_prefix = 'w';
  event_state other_event = EVENT_INACTIVE;
  if (mode == PRO_EXT) {
    event_prefix = 'c';
    other_event = EVENT_DISABLED;
  }
  if (mode == BALANCE_EXT) {
    event_prefix = 'b';
    other_event = EVENT_DISABLED;
  }
  if (mode == NO_EXT)
    event_prefix = 'w';
  if (mode == NUNCHUK_EXT)
    event_prefix = 'n';
  if (mode == CLASSIC_EXT)
    event_prefix = 'c';
  
  
  //Just do  wild across the board state changes.
  for (int i = 0; i < events.size(); i++) {
    if (events[i].name[0] == event_prefix) {
      toggle_event(i, EVENT_ACTIVE);
    } else {
      toggle_event(i, other_event);
    }
  }
  //Now reactivate wm_* events for the classic controller mode, since that matches current behavior
  if (mode == CLASSIC_EXT) {
    for (int i = 0; i < events.size(); i++) {
      if (events[i].name[0] == 'w') {
        toggle_event(i, EVENT_ACTIVE);
      }
    }
  }

  //Now do the various extras like IR or accelerometers.
  if ((mode == NO_EXT && wm_ir_active) || (mode == NUNCHUK_EXT && nk_ir_active)) {
    if (ir.dev != nullptr && ir.fd < 0) {
      open_node(&ir);
    }
    if (mode == NO_EXT) {
      toggle_event(wm_ir_x, EVENT_ACTIVE);
      toggle_event(wm_ir_y, EVENT_ACTIVE);
    } else if (mode == NUNCHUK_EXT) {
      toggle_event(nk_ir_x, EVENT_ACTIVE);
      toggle_event(nk_ir_y, EVENT_ACTIVE);
    }
  } else {
    if (ir.fd > 0) {
      if (ir.fix_mode) chmod(udev_device_get_devnode(ir.dev), ir.orig_mode);
      close(ir.fd);
      ir.fd = -1;
    }
    toggle_event(wm_ir_x, EVENT_INACTIVE);
    toggle_event(wm_ir_y, EVENT_INACTIVE);
    toggle_event(nk_ir_x, EVENT_INACTIVE);
    toggle_event(nk_ir_y, EVENT_INACTIVE);
  }
  if ((mode == NO_EXT && wm_accel_active) || (mode == NUNCHUK_EXT && nk_accel_active)) {
    if (accel.dev != nullptr && accel.fd < 0) {
      open_node(&accel);
    }
    if (mode == NO_EXT) {
      toggle_event(wm_accel_x, EVENT_ACTIVE);
      toggle_event(wm_accel_y, EVENT_ACTIVE);
      toggle_event(wm_accel_z, EVENT_ACTIVE);
    } else if (mode == NUNCHUK_EXT) {
      toggle_event(nk_accel_x, EVENT_ACTIVE);
      toggle_event(nk_accel_y, EVENT_ACTIVE);
      toggle_event(nk_accel_z, EVENT_ACTIVE);
      toggle_event(nk_wm_accel_x, EVENT_ACTIVE);
      toggle_event(nk_wm_accel_y, EVENT_ACTIVE);
      toggle_event(nk_wm_accel_z, EVENT_ACTIVE);
    }
  } else {
    if (accel.fd > 0) {
      if (accel.fix_mode) chmod(udev_device_get_devnode(accel.dev), accel.orig_mode);
      close(accel.fd);
      accel.fd = -1;
    }
    toggle_event(nk_accel_x, EVENT_INACTIVE);
    toggle_event(nk_accel_y, EVENT_INACTIVE);
    toggle_event(nk_accel_z, EVENT_INACTIVE);
    toggle_event(nk_wm_accel_x, EVENT_INACTIVE);
    toggle_event(nk_wm_accel_y, EVENT_INACTIVE);
    toggle_event(nk_wm_accel_z, EVENT_INACTIVE);
    toggle_event(wm_accel_x, EVENT_INACTIVE);
    toggle_event(wm_accel_y, EVENT_INACTIVE);
    toggle_event(wm_accel_z, EVENT_INACTIVE);
  }

}

void wiimote::remove_node(const char* name) {
  if (!name) return;
  int node = name_to_node(name);
  if (node == E_NK) {
    clear_node(&nunchuk);
    remove_extension();
  };
  if (node == E_CC) {
    clear_node(&classic);
    remove_extension();
  };
  if (node == MP) {
    manager->log.take_message(this->name + " motion+ removed.");
    clear_node(&motionplus);
  }
}

void wiimote::open_node(struct dev_node* node) {
  node->fd = open(udev_device_get_devnode(node->dev), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
  if (node->fd < 0)
    perror("open subdevice:");

  node->fix_mode = false;
  struct stat filestat;
  fstat(node->fd, &filestat);
  node->orig_mode = filestat.st_mode;
  if (grab_exclusive) grab_ioctl_node(node, true);
  if (grab_permissions) grab_chmod_node(node, true);

  watch_file(node->fd, node);
};

void wiimote::grab_chmod_node(struct dev_node* node, bool grab) {
  if (node->fd < 0) return;
  if (grab) {
    struct stat filestat;
    fstat(node->fd, &filestat);
    node->orig_mode = filestat.st_mode;
    node->fix_mode = true;
    chmod(udev_device_get_devnode(node->dev), 0);
  } else {
    chmod(udev_device_get_devnode(node->dev), node->orig_mode);
    node->fix_mode = false;
  }
}

void wiimote::grab_chmod(bool grab) {
  grab_chmod_node(&base, grab);
  grab_chmod_node(&buttons, grab);
  grab_chmod_node(&accel, grab);
  grab_chmod_node(&ir, grab);
  grab_chmod_node(&motionplus, grab);
  grab_chmod_node(&nunchuk, grab);
  grab_chmod_node(&classic, grab);
  grab_chmod_node(&pro, grab);
  grab_chmod_node(&balance, grab);
}

void wiimote::grab_ioctl_node(struct dev_node* node, bool grab) {
  if (node->fd < 0) return;
  if (grab) {
    ioctl(node->fd, EVIOCGRAB, 1);
  } else {
    ioctl(node->fd, EVIOCGRAB, 0);
  }
}

void wiimote::grab_ioctl(bool grab) {
  grab_ioctl_node(&base, grab);
  grab_ioctl_node(&buttons, grab);
  grab_ioctl_node(&accel, grab);
  grab_ioctl_node(&ir, grab);
  grab_ioctl_node(&motionplus, grab);
  grab_ioctl_node(&nunchuk, grab);
  grab_ioctl_node(&classic, grab);
  grab_ioctl_node(&pro, grab);
  grab_ioctl_node(&balance, grab);
}

void wiimote::process(void* tag) {
  int type = CORE;
  if (tag == &classic) type = E_CC;
  if (tag == &nunchuk) type = E_NK;
  if (tag == &ir) type = IR;
  if (tag == &accel) type = ACCEL;
  if (tag == &pro)  type = WII_U_PRO;
  if (tag == &balance) type = BALANCE;
  switch (type) {
  case CORE:
    process_core();
    break;
  case E_CC:
    process_classic(classic.fd);
    break;
  case E_NK:
    process_nunchuk(nunchuk.fd);
    break;
  case IR:
    process_ir(ir.fd);
    break;
  case ACCEL:
    process_accel(accel.fd);
    break;
  case BALANCE:
    process_balance(balance.fd);
    break;
  case WII_U_PRO:
    process_pro(pro.fd);
    break;
  }
}
