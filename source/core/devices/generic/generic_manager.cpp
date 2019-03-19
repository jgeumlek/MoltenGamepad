#include "generic.h"
#include <algorithm>
#include <unordered_map>
#include <set>

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

  return 0;
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

//I don't have a 32 bit system to test how it reports its capability strings!
#ifndef WORD_SIZE
#define WORD_SIZE sizeof(size_t)*8
#endif

std::vector<int> read_capabilities(const char* capabilities) {
  std::vector<int> codes;
  int len = strlen(capabilities);
  if (len <= 0)
    return codes;
  //We are given a bitmask in hexadecimal
  //We read right to left, as the lowest codes have the last bits.
  //When we encounter a space, we skip to the next word boundary.
  //(Any leading zeroes in a word have been stripped!)
  int code = 0;
  int next_word = WORD_SIZE;

  for (const char* ptr = capabilities + len; ptr >= capabilities; ptr--) {
    if (*ptr == '\n' || *ptr == '\0')
      continue;
    if (*ptr == ' ') {
      if (code > next_word)
        debug_print(DEBUG_VERBOSE,1,"\t\t events: suspicious failure when reading device capabilities. Perhaps compiled with the wrong word size?");
      code = next_word;
      next_word += WORD_SIZE;
      continue;
    }
    int digit = *ptr;
    if (digit >= '0' && digit <= '9')
      digit -= '0';
    if (digit >= 'a' && digit <= 'f')
      digit = (digit - 'a') + 10;
    for (int i = 0;  i < 4; i++) {
      if (digit & 1) {
        codes.push_back(code);
      }
      digit >>= 1;
      code++;
    }
  }
  return codes;
}


bool events_matched(udev* udev, udev_device* dev, const generic_driver_info* gendev, device_match::ev_match match, int min_common_events) {
  if (match == device_match::EV_MATCH_IGNORED)
    return true;
  std::set<std::pair<entry_type,int>> gendev_events;
  bool superset = false;
  bool subset = true;
  char* buffer = new char[1024];
  memset(buffer, 0, 1024);
  //only used to report why a match failed;
  int superkey = -1; //a key that is evidence of a superset
  int superabs = -1; //an abs that is evidence of a superset
  for (const gen_source_event& g_ev : gendev->events)
    gendev_events.insert(std::make_pair(g_ev.type,g_ev.code));

  udev_device* parent = udev_device_get_parent(dev);
  const char* syspath = udev_device_get_syspath(parent);
  if (!syspath) {
    delete[] buffer;
    return false;
  }
  std::string base_path(syspath);
  base_path += "/capabilities/";
  //Check abs events
  int fd = open((base_path+"abs").c_str(), O_RDONLY);
  if (fd < 1) {
    delete[] buffer;
    return false;
  }
  read(fd, buffer, 1024);
  buffer[1023] = '\0';
  close(fd);
  std::vector<int> codes = read_capabilities(buffer);
  int matched_events = 0;
  for (int code : codes) {
    std::pair<entry_type,int> pair = std::make_pair(DEV_AXIS,code);
    int found = gendev_events.erase(pair);
    if (!found) {
      subset = false;
      superabs = code;
    } else {
      matched_events++;
    }
  }

  //check key events
  fd = open((base_path+"key").c_str(), O_RDONLY);
  if (fd < 1) {
    delete[] buffer;
    return false;
  }
  memset(buffer, 0, 1024);
  read(fd, buffer, 1024);
  buffer[1023] = '\0';
  close(fd);
  codes = read_capabilities(buffer);
  for (int code : codes) {
    std::pair<entry_type,int> pair = std::make_pair(DEV_KEY,code);
    int found = gendev_events.erase(pair);
    if (!found) {
      subset = false;
      superkey = code;
    } else {
      matched_events++;
    }
  }
  delete[] buffer;
  superset = gendev_events.empty();

  if (match == device_match::EV_MATCH_SUBSET) {
    int required_in_common = min_common_events >= 0 ? min_common_events : 1;
    //reject the empty set as a trivial subset.
    if (!subset) {
      if (superkey != -1)
        debug_print(DEBUG_VERBOSE, 2, "\t\t events subset: failed because device had key ", std::to_string(superkey).c_str());
      else if (superabs != -1)
        debug_print(DEBUG_VERBOSE, 2, "\t\t events subset: failed because device had abs ", std::to_string(superabs).c_str());
    }
    if (matched_events < required_in_common)
      debug_print(DEBUG_VERBOSE, 4, "\t\t events subset: failed because device had ", std::to_string(matched_events).c_str(), " in common, but needed ", std::to_string(required_in_common).c_str());
    subset = subset && (matched_events >= required_in_common);
    if (subset)
      debug_print(DEBUG_VERBOSE, 1, "\t\t events subset: check passed");
    return subset;
  }
  if (match == device_match::EV_MATCH_EXACT) {
    //check for both superset and subset.
    if (!subset) {
      if (superkey != -1)
        debug_print(DEBUG_VERBOSE, 2, "\t\t events exact: failed because device had key ", std::to_string(superkey).c_str());
      else if (superabs != -1)
        debug_print(DEBUG_VERBOSE, 2, "\t\t events exact: failed because device had abs ", std::to_string(superabs).c_str());
      return false;
    }
    if (!superset) {
      const char* types[] = {"none","opt","key","abs","rel","any"};
      debug_print(DEBUG_VERBOSE, 4, "\t\t events exact: failed because device was missing ", types[gendev_events.begin()->first], " ", std::to_string(gendev_events.begin()->second).c_str());
      return false;
    }
    debug_print(DEBUG_VERBOSE, 1, "\t\t events exact: check passed");
    return true;
  }
  if (match == device_match::EV_MATCH_SUPERSET) {
    if (!superset) {
      const char* types[] = {"none","opt","key","abs","rel","any"};
      debug_print(DEBUG_VERBOSE, 4, "\t\t events superset: failed because device was missing ", types[gendev_events.begin()->first], " ", std::to_string(gendev_events.begin()->second).c_str());
      return false;
    }
    debug_print(DEBUG_VERBOSE, 1, "\t\t events superset: check passed");
    return superset;
  }
  return false;
}

bool matched(struct udev* udev, struct udev_device* dev, const device_match& match, const generic_driver_info* gendev) {
  bool result = true;
  bool valid = false; //require at least one thing be matched...
  //lots of things to compute
  const char* phys = nullptr;
  const char* uniq = nullptr;
  const char* driver = udev_device_get_driver(dev);
  std::string vendor_id;
  std::string product_id;
  struct udev_device* hidparent = udev_device_get_parent_with_subsystem_devtype(dev,"hid",NULL);
  struct udev_device* parent = udev_device_get_parent(dev);
  if (hidparent) {
    phys = udev_device_get_property_value(hidparent, "HID_PHYS");
    uniq = udev_device_get_property_value(hidparent, "HID_UNIQ");
  }

  //sometimes custom arcade devces do not register as hid, I have such a case
  //with an ultimarc board which works as hid in one computer
  // and as a generic input device in another
  if((!hidparent) && parent && !match.phys.empty()) {
    phys = (char *) udev_device_get_property_value(parent, "PHYS");
    //TODO find out if this is also possible
    //uniq = (char *) udev_device_get_property_value(parent, "UNIQ");
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
  debug_print(DEBUG_VERBOSE, 1, "\t\tchecking match line...");
  if (!match.name.empty()) {
    valid = true;
    const char* name = nullptr;
    name = udev_device_get_sysattr_value(parent, "name");
    bool check = (name && !strcmp(match.name.c_str(), name));
    result = result && check;
    debug_print(DEBUG_VERBOSE, 4, "\t\t name: ", name ?  name : "", check ? " == " : " != ", match.name.c_str());
  }
  if (!match.uniq.empty()) {
    valid = true;
    bool check = (uniq && !strcmp(match.uniq.c_str(), uniq));
    debug_print(DEBUG_VERBOSE, 4, "\t\t uniq: ", uniq ?  uniq : "", check ? " == " : " != ", match.uniq.c_str());
  }
  if (!match.phys.empty()) {
    valid = true;

    //the phys address can come in in some instances with quotes attached
    //we have to take this into consideration
    //hence the length check as well
    bool check = (phys && (!strcmp(match.phys.c_str(), phys) || (strstr(phys, "\"") == phys && strstr( phys, match.phys.c_str()) != NULL
            && (strlen(phys) - strlen(match.phys.c_str()) <= 2))));
    result = result && check;
    debug_print(DEBUG_VERBOSE, 4, "\t\t phys: ", phys ?  phys : "", check ? " == " : " != ", match.phys.c_str());
  }
  if (!match.driver.empty()) {
    valid = true;
    udev_device* driver_parent = parent;
    while (driver == nullptr && driver_parent) {
      driver = udev_device_get_driver(driver_parent);
      driver_parent = udev_device_get_parent(driver_parent);
    }
    bool check = (driver && !strcmp(match.driver.c_str(), driver));
    result = result && check;
    debug_print(DEBUG_VERBOSE, 4, "\t\t driver: ", driver ?  driver : "", check ? " == " : " != ", match.driver.c_str());
  }
  if (match.vendor != -1) {
    valid = true;
    int vendor = parse_hex(vendor_id);
    bool check = (match.vendor == vendor);
    result = result && check;
    debug_print(DEBUG_VERBOSE, 4, "\t\t vendor: ",   std::to_string(vendor).c_str(), check ? " == " : " != ", std::to_string(match.vendor).c_str());
  }
  if (match.product != -1) {
    valid = true;
    int product = parse_hex(product_id);
    bool check = (match.product == product);
    result = result && check;
    debug_print(DEBUG_VERBOSE, 4, "\t\t product: ",   std::to_string(product).c_str(), check ? " == " : " != ", std::to_string(match.product).c_str());
  }
  if (match.events != device_match::EV_MATCH_IGNORED) {
    valid = true;
    result = result && events_matched(udev, dev, gendev, match.events, match.min_common_events);
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
    if (!path) return DEVICE_UNCLAIMED;
    for (auto it = openfiles.begin(); it != openfiles.end(); it++) {
      (*it)->close_node(dev, true);
      if ((*it)->nodes.empty()) {
        delete(*it);
        openfiles.erase(it);
        return DEVICE_CLAIMED;
      }
    }
  }
  int strongest_claim = DEVICE_CLAIMED;
  bool has_claim = false;
  if (!strcmp(action, "add") || !strcmp(action, "null")) {
    if (!strcmp(subsystem, "input")) {
      const char* sysname = udev_device_get_sysname(dev);
      if (!strncmp(sysname, "event", 3)) {
        for (auto it = descr->matches.begin(); it != descr->matches.end(); it++) {
          if (matched(udev,dev,*it, descr)) {
            debug_print(DEBUG_VERBOSE,2, "\t\t match passed", it->order > 0 ? (", order = " + std::to_string(it->order+1)).c_str() : "");
            //If we are claiming this, open the device and return DEVICE_CLAIMED.
            if (it->order == DEVICE_CLAIMED && open_device(udev, dev) == SUCCESS)
              return DEVICE_CLAIMED;
            //otherwise, we are issuing a DEVICE_CLAIMED_DEFERRED() for this,
            //and we should not open the device.
            if (!has_claim) {
              strongest_claim = it->order;
              has_claim = true;
            } else {
              strongest_claim = (strongest_claim < it->order) ? strongest_claim : it->order;
            }
          } else {
            debug_print(DEBUG_VERBOSE,1, "\t\t match failed");
          }
        }
      } else {
        debug_print(DEBUG_VERBOSE, 1, "\t\tignored because it was not a /dev/input/event# device");
      }
    } else {
      debug_print(DEBUG_VERBOSE, 1, "\t\tignored because it is not in the input subsystem");
    }
  }

  if (has_claim)
    return strongest_claim;

  return DEVICE_UNCLAIMED;
}

int generic_manager::accept_deferred_device(struct udev* udev, struct udev_device* dev) {
  std::lock_guard<std::mutex> lock(devlistlock);
  return (open_device(udev,dev) == SUCCESS) ? DEVICE_CLAIMED : DEVICE_UNCLAIMED;
}

int generic_manager::open_device(struct udev* udev, struct udev_device* dev) {
  try {
    if (flatten && openfiles.size() >= 1) {
      openfiles.front()->open_node(dev);
    } else {
      openfiles.push_back(new generic_file(mg, dev, descr->grab_ioctl, descr->grab_chmod, descr->grab_hid_chmod, descr->rumble));
      create_inputs(openfiles.back());
    }
  } catch (std::exception& e) {
    return FAILURE; //Something went wrong opening this device...
  }
  return SUCCESS;
}

void generic_manager::create_inputs(generic_file* opened_file) {
  for (int i = 1; i <= split; i++) {
    generic_device* gendev = new generic_device(splitevents.at(i - 1), event_count, opened_file, descr->split_types[i-1], descr->rumble);
    device_plugin plug = genericdev;
    plug.name_stem = descr->devname.c_str();
    //if we have split devices, add identifiers for each split.
    //If split == 1 (no splitting), then no extra identifiers needed.
    std::string dev_suffix = (split > 1) ? "/split" + std::to_string(i) : "";
    std::string phys = opened_file->phys;
    if (!phys.empty())
      phys += dev_suffix;
    std::string uniq = opened_file->uniq;
    if (!uniq.empty())
      uniq += dev_suffix;
    plug.uniq = uniq.c_str();
    plug.phys = phys.c_str();
    input_source* source = mg->add_device(ref, plug, gendev).get();
    if (source) opened_file->add_dev(source);
  }
}

