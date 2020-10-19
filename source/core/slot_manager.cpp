#include "slot_manager.h"
#include "virtual_devices/virtual_kbm.h"
#include <csignal>

slot_manager::slot_manager(int max_pads, bool keys, const virtpad_settings& padstyle) : log("slot"),  opts([&] (std::string& name, MGField value){ return process_option(name, value); }), max_pads(max_pads)
{
  ui = new uinput();
  dummyslot.virt_dev = std::make_shared<virtual_device>("blank", "Dummy slot (ignores all events)", this);
  debugslot.virt_dev = std::make_shared<debug_device>("debugslot", "Prints out all received events", this);

  debugslot.state = SLOT_ENABLED;
  debugslot.name = "debugslot";
  dummyslot.state = SLOT_ENABLED;
  dummyslot.name = "blank";

  min_pads = max_pads;
  active_pads = max_pads;
  this->padstyle = padstyle;

  for (int i = 0; i < max_pads; i++) {
    output_slot new_slot;
    new_slot.name = "virtpad" + std::to_string(i + 1);
    new_slot.state = SLOT_ENABLED;
    slots.push_back(new_slot);
    open_pad_slot(i);

  }
  opts.register_option({"min_pads", "Smallest number of virtual pads to maintain.",std::to_string(max_pads).c_str(),MG_INT});
  opts.register_option({"active_pads","Number of virtpad slots currently active for assignment.", std::to_string(max_pads).c_str(), MG_INT});
  opts.register_option({"auto_assign","Assign devices to an output slot upon connection.", "false", MG_BOOL});
  opts.register_option({"press_start_on_disconnect","Try to pause by pressing start when a slot is empty. Can be \"any\", \"last\", or \"none\".", "none", MG_STRING});
  opts.register_option({"press_start_ms","Duration in milliseconds of the simulated start press", "20", MG_INT});
  opts.register_option({"slot_numbering", "How player numbers are given to virtual devices. Can be \"chrono\", \"name\", or \"devnode\".","name",MG_STRING});


  if (keys) {
    uinput_ids kb_ids = {"Virtual Keyboard (MoltenGamepad)", "moltengamepad/keyboard", 1, 1, 1};
    uinput_ids rel_mouse_ids = {"Virtual Mouse (MoltenGamepad)", "moltengamepad/relmouse", 1, 1, 1};

    keyboard.virt_dev =  std::make_shared<virtual_keyboard>("keyboard", "A virtual keyboard", kb_ids, rel_mouse_ids, this, ui);
    keyboard.state = SLOT_ENABLED;
  } else {
    keyboard.virt_dev = nullptr;
    keyboard.state = SLOT_DISABLED;
  }
  keyboard.name = "keyboard";

  if (padstyle.rumble)
    ui->start_ff_thread();
}

void noop_signal_handler(int signum) {
  //HACK: Assumes we only destroy slot_manager at the very end of process lifespan.
  return;
}

slot_manager::~slot_manager() {
  signal(SIGINT, noop_signal_handler);
  signal(SIGTERM, noop_signal_handler);

  for (auto slot : slots) {
    while (true) {
      if (!slot.virt_dev)
        break;
      if (slot.virt_dev->close_virt_device()) {
        slot.virt_dev = nullptr;
        break;
      } else {
        std::cerr << "A virtual device cannot be closed until its force-feedback effects have been erased." << std::endl;
        sleep(2);
      }
    }
  }

  delete ui;
  dummyslot.virt_dev = nullptr;
  keyboard.virt_dev = nullptr;
  debugslot.virt_dev = nullptr;
}

//should only be called while holding a lock.
void slot_manager::open_pad_slot(int index) {
  output_slot &slot = slots[index];
  slot.virt_dev = std::make_shared<virtual_gamepad>(slot.name, "A virtual gamepad", padstyle, this, ui);
  slot.virt_dev->init();
  slot.has_devices = false;
  log.slot_event(0, &slots[index], "open");
}

int slot_manager::request_slot(input_source* dev) {
  std::lock_guard<std::mutex> guard(lock);
  //already has slot?
  if (dev->get_slot())
    return 0;
  //has a forced assignment?
  std::shared_ptr<virtual_device> assigned = find_id_based_assignment(dev);
  if (assigned) {
    move_device(dev,assigned);
    return 0;
  }

  //if it is a keyboard, or only the keyboard exists, just put it there.
  if ((dev->get_type() == "keyboard" || active_pads == 0) && keyboard.virt_dev) {
    move_device(dev,keyboard.virt_dev);
    return 0;
  }


  //In terms of our algorithm here, the number of active pads is more like
  //an upper bound on open pads to consider.
  //There is no guarantee that that the open pads occur at the front
  //virtpad2 might be closed, but virtpad1 and virtpad3 might be open.
  //In this case, virtpads 1 and 3 are considered as 2 active pads...
  int actual_active_pads = (active_pads <= (int)slots.size()) ? active_pads : slots.size();
  int actives_seen = 0;

  //check if any existing slots are willing to take it.
  //(a slot might refuse if it already has a device of that type)
  for (unsigned int i = 0; i < slots.size(); i++) {
    if (slots[i].virt_dev) {
      if (actives_seen < actual_active_pads) {
        //special case: if actual_active_pads = 1, then just pile all devices onto it.
        //this means ignoring whether that slot wants to accept the device.
        if (actual_active_pads == 1 || slots[i].virt_dev->accept_device(dev->shared_from_this())) {
          move_device(dev, slots[i].virt_dev);
          return 0;
        }
      }
      actives_seen++;
    }
  }

  //none of our current slots wanted it. Should we open a new slot?
  //Only possible if our minimum pad count is less than our active ones.
  int amount_could_open = actual_active_pads - actives_seen;
  if (amount_could_open > 0) {
    //looks like we have room to open one!
    //iterate just to find the first closed one...
    for (unsigned int i = 0; i < slots.size(); i++) {
      if (!slots[i].virt_dev) {
        try {
          open_pad_slot(i);
          move_device(dev, slots[i].virt_dev);
          return 0;
        } catch (std::exception& e) {
          //it failed...
        }
        //either way, break the search.
        break;
      }
    }
  }

  //all failed: just give it the dummyslot.
  move_device(dev,dummyslot.virt_dev);

  return 0;
}

void slot_manager::move_to_slot(input_source* dev, std::shared_ptr<virtual_device>& target) {
  //don't detect emptiness if we are just moving the last input source from one slot to another.
  lock.lock();
  update_slot_emptiness_prelocked();
  move_device(dev,target);
  process_slot_emptiness_prelocked();
  lock.unlock();
}

void slot_manager::move_device(input_source* dev, std::shared_ptr<virtual_device>& target) {
  //private, should only be called with lock acquired
  if (!dev) return;
  dev->set_slot(target);
  log.device_slot(0, dev, target.get());
}

void slot_manager::id_based_assign(slot_manager::id_type type, std::string id, std::shared_ptr<virtual_device> slot) {
  //TODO: fix this in case trying to set up an assignment for a slot that currently has closed
  //  its virtual device...
  std::lock_guard<std::mutex> guard(lock);
  std::pair<slot_manager::id_type, std::string> key = std::make_pair(type,id);
  if (!slot) {
    id_slot_assignments.erase(key);
    return;
  }
  id_slot_assignments[key] =  slot->name;
  return;
}

std::shared_ptr<virtual_device> slot_manager::find_id_based_assignment(input_source* dev) {
  //private func, called while lock is held.
  //Currently hardcoded priority:
  // uniq assignment -> name assignment -> phys assignment.
  //Todo: revamp this to assign to output_slot rather than the temporary virtual_device objects
  if (id_slot_assignments.empty())
    return nullptr;
  std::pair<slot_manager::id_type, std::string> key = std::make_pair(UNIQ_ID,dev->get_uniq());
  auto slot = id_slot_assignments.find(key);
  if (slot != id_slot_assignments.end()) {
    return find_slot(slot->second);
  }

  key = std::make_pair(NAME_ID,dev->get_name());
  slot = id_slot_assignments.find(key);
  if (slot != id_slot_assignments.end()) {
    return find_slot(slot->second);
  }
  key = std::make_pair(PHYS_ID,dev->get_phys());
  slot = id_slot_assignments.find(key);
  if (slot != id_slot_assignments.end()){
    return find_slot(slot->second);
  }
  return nullptr;
}

void slot_manager::for_all_assignments(std::function<void (slot_manager::id_type, std::string, virtual_device*)> func) {
  std::lock_guard<std::mutex> guard(lock);
  for (auto entry : id_slot_assignments) {
    std::shared_ptr<virtual_device> ptr = find_slot(entry.second);
    if (ptr)
      func(entry.first.first, entry.first.second, ptr.get());
  }
}

std::shared_ptr<virtual_device> slot_manager::find_slot(std::string slotname) {
  for (auto it = slots.begin(); it != slots.end(); it++) {
    if (it->name == slotname) return it->virt_dev;
  }
  if (slotname == "keyboard" || slotname == keyboard.name) {
    return keyboard.virt_dev;
  }
  if (slotname == dummyslot.name) return dummyslot.virt_dev;
  if (slotname == debugslot.name) return debugslot.virt_dev;
  return nullptr;
}

int slot_manager::process_option(std::string& name, MGField value) {
  std::lock_guard<std::mutex> guard(lock);
  if (name == "active_pads" && value.integer >= 0 && value.integer <= max_pads) {
    active_pads = value.integer;
    return 0;
  }
  if (name == "min_pads" && value.integer >= 0 && value.integer <= max_pads) {
    min_pads = value.integer;
    update_slot_emptiness_prelocked();
    process_slot_emptiness_prelocked();
    update_pad_count_prelocked();
    return 0;
  }

  if (name == "press_start_on_disconnect" && value.string) {
    std::string text(value.string);
    if (text == "any") {
      press_start_on_last_disconnect = false;
      press_start_on_any_disconnect = true;
      return 0;
    }
    if (text == "last") {
      press_start_on_any_disconnect = false;
      press_start_on_last_disconnect = true;
      return 0;
    }
    if (text == "none") {
      press_start_on_last_disconnect = false;
      press_start_on_any_disconnect = false;
      return 0;
    }
    return -1;
  }
  if (name == "press_start_ms") {
      start_press_milliseconds = value.integer;
      return 0;
  }


  return -1;
}

void slot_manager::update_slot_emptiness() {
  std::lock_guard<std::mutex> guard(lock);
  update_slot_emptiness_prelocked();
}

void slot_manager::process_slot_emptiness() {
  std::lock_guard<std::mutex> guard(lock);
  process_slot_emptiness_prelocked();
}


void slot_manager::update_pad_count() {
  std::lock_guard<std::mutex> guard(lock);
  update_pad_count_prelocked();
}


//just caches the boolean values on if devices are empty.
void slot_manager::update_slot_emptiness_prelocked() {
  for (output_slot& slot : slots) {
    if (slot.virt_dev)
      slot.has_devices = slot.virt_dev->input_source_count() > 0;
    else
      slot.has_devices = 0;
  }
}
//checks against cached info to perform any actions upon a slot emptying.
void slot_manager::process_slot_emptiness_prelocked() {
  //Only reasons to do something:
  //  - we need to press start on last disconnect (when all slots go empty)
  //  - we need to dynamically remove a slot
  if (!press_start_on_last_disconnect && (min_pads == max_pads))
    return; //nothing to do!

  //Check if our cache states that all were already empty
  bool already_all_empty = true;
  int open_virt_devs = 0;
  for (output_slot& slot : slots) {
    if (slot.has_devices)
      already_all_empty = false;
    if (slot.virt_dev)
      open_virt_devs++;
  }


  //We need to detect:
  // - the case where a single slot went
  //    nonempty to empty and this caused all slots to be empty.
  // - the case where an appropriate number of slots should be removed
  //can skip this work if we know we don't need to remove and 
  //we know everything is already empty.
  if (!already_all_empty || (min_pads < max_pads) ) {
    bool now_all_empty = true;
    output_slot last_slot;
    last_slot.virt_dev = nullptr;
    int most_slots_to_remove = open_virt_devs - min_pads;
    most_slots_to_remove =  most_slots_to_remove < 0 ? 0 : most_slots_to_remove;
    int slots_removed = 0;

    //For removing slots, it makes the most sense to remove from the later slots.
    //So for this reason, we iterate the slots list in reverse.
    for (auto rit = slots.rbegin(); rit != slots.rend(); ++rit) {
      output_slot& slot = *rit;
      //check if this is a slot that is newly empty.
      //If so, set last_slot to point to it.
      bool had_devices = slot.has_devices;
      slot.has_devices = false;
      if (slot.virt_dev)
        slot.has_devices = slot.virt_dev->input_source_count() > 0;
      if (had_devices && !slot.has_devices)
        last_slot = slot;
      //If now empty, see if we should remove it.
      //Otherwise, keep note that there exists a non-empty slot.
      if (!slot.has_devices && slot.virt_dev) {
        if (slots_removed < most_slots_to_remove) {
          slots_removed++;
          slot.virt_dev = nullptr;
          log.slot_event(0, &slot, "close");
        }
      } else {
        now_all_empty = false;
      }
    }

    //we weren't all empty in the cache, but now we are...
    if (press_start_on_last_disconnect && !already_all_empty && now_all_empty && last_slot.virt_dev) {
      last_slot.virt_dev->send_start_press(start_press_milliseconds);
      //if min_pads == 0, last_slot might be holding the last ref to the virt_dev
      //the start press will be sent, but the device will likely be closed out before
      //the start release fires...
    }
  }
}


//A little general method to call when sweeping config changes occur
//where we either need to open or close multiple
void slot_manager::update_pad_count_prelocked() {

  int open_virt_devs = 0;
  for (output_slot& slot : slots) {
    if (slot.virt_dev)
      open_virt_devs++;
  }

  //might as well try not to crash when invalid configurations arise.
  //at least until the options class can handle rejecting invalid values.
  int actual_max_pads = max_pads >= 0 ? max_pads : 0;
  int actual_min_pads = min_pads >= 0 ? min_pads : 0;

  if (actual_max_pads < actual_min_pads)
    actual_min_pads = actual_max_pads;

  if (open_virt_devs > actual_min_pads) {
    //For removing slots, it makes the most sense to remove from the later slots.
    //So for this reason, we iterate the slots list in reverse.
    int most_slots_to_remove = open_virt_devs - actual_min_pads;
    int slots_removed = 0;

    most_slots_to_remove =  most_slots_to_remove < 0 ? 0 : most_slots_to_remove;
    for (auto rit = slots.rbegin(); rit != slots.rend(); ++rit) {
      output_slot& slot = *rit;
      bool has_devices = false;
      if (slot.virt_dev)
        has_devices = slot.virt_dev->input_source_count() > 0;

      if (!has_devices && slot.virt_dev) {
        if (slots_removed < most_slots_to_remove) {
          slots_removed++;
          slot.virt_dev = nullptr;
          open_virt_devs--;
          log.slot_event(0, &slot, "close");
        } else {
          break;
        }
      }
    }
  }

  if (open_virt_devs < actual_min_pads) {
    //for opening slots, we can go in the normal order.
    int most_slots_to_open = actual_min_pads - open_virt_devs;
    int slots_opened = 0;
    for (auto it = slots.begin(); it != slots.end(); it++) {
      if (slots_opened >= most_slots_to_open)
        break;
      output_slot& slot = *it;
      if (!slot.virt_dev) {
        try {
            open_pad_slot(it - slots.begin());
            open_virt_devs++;
            slots_opened++;
        } catch (std::exception& e) {
            //it failed...
        }
      }
    }
  }
}


void slot_manager::tick_all_slots() {
  std::lock_guard<std::mutex> guard(lock);
  if (dummyslot.virt_dev)
    dummyslot.virt_dev->check_delayed_events();
  if (debugslot.virt_dev)
    debugslot.virt_dev->check_delayed_events();
  if (keyboard.virt_dev)
    keyboard.virt_dev->check_delayed_events();
  for (output_slot& slot : slots) {
    if (slot.virt_dev)
      slot.virt_dev->check_delayed_events();
  }
}
