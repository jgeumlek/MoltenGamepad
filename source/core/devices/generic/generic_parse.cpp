#include <iostream>
#include <string>
#include "generic.h"
#include "../../parser.h"
#include "../../moltengamepad.h"
#include "../../eventlists/eventlist.h"


#define GENDEV_INCOMPLETE_INFO -1
#define GENDEV_REJECTED_MANAGER -5
void parse_error(moltengamepad* mg, const std::string& message, const context& context) {
  if (message.empty()) {
    mg->drivers.err(0,"gendev: parsing error, entire line ignored.", context.path, context.line_number);
  } else {
    mg->drivers.err(0,message, context.path, context.line_number);
  }
}
void generic_assignment_line(std::vector<token>& line, generic_driver_info*& info, moltengamepad* mg, context context) {

  auto it = line.begin();
  if (it == line.end()) return;

  //check for a field, like one of our settings
  std::string field = line.front().value;
  std::string prefix = "";

  it++;

  if (it == line.end()) {
    parse_error(mg, "", context);
    return;
  }

  //If we see a dot, then what we saw so far was a prefix!
  //Like when defining a split event.
  if ((*it).type == TK_DOT) {
    it++;
    if (it == line.end()) {
      parse_error(mg, "", context);
      return;
    }
    prefix = field;
    field = (*it).value;
    it++;

  }

  if (it == line.end()) return parse_error(mg, "", context);
  //If the next char is "(", then we are parsing
  //a numeric event code, like "key(306)".
  int numeric_literal = -1;
  if ((*it).type == TK_LPAREN) {
    it++;
    if (it == line.end() || (*it).type != TK_IDENT) {
      parse_error(mg, "gendev: expected event code number.", context);
      return;
    }
    try {
      numeric_literal = std::stoi((*it).value);
    } catch (std::exception& e) {
      parse_error(mg, "gendev: could not parse event code number.", context);
      return;
    }
    it++;
    if (it == line.end() || (*it).type != TK_RPAREN) {
      parse_error(mg, "", context);
      return;
    }
    it++;
  }

  if (it == line.end() || (*it).type != TK_EQUAL) return parse_error(mg, "", context);

  it++; //Skip past the "="

  if (it == line.end()) {
    parse_error(mg, "", context);
    return;
  }

  std::string value = (*it).value;
  std::string descr = "(generic)";

  it++;

  if (it != line.end() && (*it).type == TK_COMMA)  {
    it++;
    if (it != line.end() && (*it).type == TK_IDENT)  {
      descr = (*it).value;
    }
  }

  if (field == "name") {
    info->name = value;
    return;
  }

  if (field == "devname") {
    info->devname = value;
    return;
  }

  if (field == "exclusive") {
    int ret = read_bool(value, [&info] (bool val) {
      info->grab_ioctl = val;
    });
    if (ret) mg->drivers.err(0,"gendev: \""+value+"\" was not recognized as true or false.", context.path, context.line_number);
    return;
  }

  if (field == "change_permissions") {
    int ret = read_bool(value, [&info] (bool val) {
      info->grab_chmod = val;
    });
    if (ret) mg->drivers.err(0,"gendev: \""+value+"\" was not recognized as true or false.", context.path, context.line_number);
    return;
  }

  if (field == "gamepad_subscription") {
    int ret = read_bool(value, [&info] (bool val) {
      info->subscribe_to_gamepad = val;
    });
    if (ret) mg->drivers.err(0,"gendev: \""+value+"\" was not recognized as true or false.", context.path, context.line_number);
    return;
  }

  if (field == "rumble") {
    int ret = read_bool(value, [&info] (bool val) {
      info->rumble = val;
    });
    if (ret) mg->drivers.err(0,"gendev: \""+value+"\" was not recognized as true or false.", context.path, context.line_number);
    return;
  }

  if (field == "flatten") {
    int ret = read_bool(value, [&info] (bool val) {
      info->flatten = val;
    });
    if (ret) mg->drivers.err(0,"gendev: \""+value+"\" was not recognized as true or false.", context.path, context.line_number);
    return;
  }

  if (field == "split") {
    try {
      int split_count  = std::stoi(value);
      if (split_count <= 0)
        throw std::runtime_error("invalid split");
      info->split = split_count;
      info->split_types.clear();
      info->split_types.assign(split_count,"gamepad");
    } catch (std::exception& e) {
      mg->drivers.err(0,"gendev: split value invalid.", context.path, context.line_number);
    }
    return;
  }

  int split_id = 1;
  if (!prefix.empty()) {
    try {
      split_id  = std::stoi(prefix);
      if (split_id <= 0 || split_id > info->split)
        throw std::runtime_error("invalid split");
    } catch (std::exception& e) {
      //catches both parse errors and out-of-range issues...
      mg->drivers.err(0,"gendev: split value invalid.", context.path, context.line_number);
    }
  }

  if (field == "device_type") {
    info->split_types.resize(info->split);
    info->split_types[split_id-1] = value;
    return;
  }
  int code = -1;
  if (field == "key" && numeric_literal >= 0) {
    code = numeric_literal;
  } else {
    code = get_key_id(field.c_str());
  }
  if (code != -1) {
    gen_source_event ev;
    ev.name = value;
    ev.code = code;
    ev.descr = descr;
    ev.type = DEV_KEY;
    ev.split_id = split_id;
    info->events.push_back(ev);
    return;
  }

  if (field == "abs" && numeric_literal >= 0) {
    code = numeric_literal;
  } else {
    code = get_axis_id(field.c_str());
  }
  if (code != -1) {
    gen_source_event ev;
    ev.name = value;
    ev.code = code;
    ev.descr = descr;
    ev.type = DEV_AXIS;
    ev.split_id = split_id;
    info->events.push_back(ev);
    return;
  }
  mg->drivers.err(0,"gendev: " + field + " was not recognized as an option or event.", context.path, context.line_number);
}

int parse_hex(const std::string& text) {
  try {
    return std::stoi(text,0,16);
  } catch(std::exception& e) {
  }
  return -1;
}

int parse_dec(const std::string& text) {
  try {
    return std::stoi(text,0,10);
  } catch(std::exception& e) {
  }
  return -1;
}

void add_to_match(device_match& current_match, const std::string& field, const std::string& value) {
  if (!value.empty()) {
    if (field == "name")
      current_match.name = value;
    if (field == "product")
      current_match.product = parse_hex(value);
    if (field == "vendor")
      current_match.vendor = parse_hex(value);
    if (field == "phys")
      current_match.phys = value;
    if (field == "uniq")
      current_match.uniq = value;
    if (field == "driver")
      current_match.driver = value;
    if (field == "events") {
      if (value == "subset")
        current_match.events = device_match::EV_MATCH_SUBSET;
      if (value == "superset")
        current_match.events = device_match::EV_MATCH_SUPERSET;
      if (value == "exact")
        current_match.events = device_match::EV_MATCH_EXACT;
      if (value == "ignored")
        current_match.events = device_match::EV_MATCH_IGNORED;
    }
    if (field == "order") {
      int order = parse_dec(value) - 1; //Syntax in gendev *.cfg is one-indexed, internal claim ordering is zero-indexed.
      current_match.order = DEVICE_CLAIMED_DEFERRED(order);
    }
  }
}

int generic_match_line(std::vector<token>& line, device_match& current_match) {
  if (line.size() < 3) return -1;
  if (line.at(0).type != TK_HEADER_OPEN) return -1;
  if (line.at(1).type == TK_HEADER_CLOSE) return -1;
  std::string value;
  std::string field = "name";

  for (auto it = ++line.begin(); it != line.end(); it++) {

    if ((*it).type == TK_HEADER_OPEN) return -1; //abort.

    //handle whatever assignment was in progress
    if ((*it).type == TK_HEADER_CLOSE) {
      if (!value.empty()) {
        add_to_match(current_match,field,value);
      }
      return 0;
    }
    //process key words as needed.
    if ((*it).value == "name" || (*it).value == "vendor" || (*it).value == "product" || (*it).value == "uniq" || (*it).value == "phys" || (*it).value == "driver" || (*it).value == "events" || (*it).value == "order") {
      std::string newfield = it->value;
      it++;
      if (it == line.end()) return -1; //Parse error for sure! need a closing brace
      if (it->type == TK_EQUAL) {
        add_to_match(current_match,field,value);
        value = "";
        field = newfield;
        continue;
      } else {
        //What if for some reason we are trying to set name="vendor"?
        //Unlikely, but possible. We did not find a '=', so this must
        //not have been an assignment.
        //Just fall back to appending it.
        it--;
      }
    }

    if (!value.empty()) value.push_back(' ');
    value += (*it).value;

  }
}

void generic_parse_line(std::vector<token>& line, generic_driver_info*& info, moltengamepad* mg, context context) {

  //if we have a header, check to see if it appears to be starting a new driver, or just adding an extra match.
  
  if (find_token_type(TK_HEADER_OPEN, line)) {
    std::string newhead;
    device_match match;
    int ret = generic_match_line(line, match);
    if (ret == 0) {

      ret = add_generic_manager(mg, *info);

      if (ret == 0 || ret == GENDEV_REJECTED_MANAGER) {
        //Either of these cases, we start a new driver description now.
        info = new generic_driver_info;
      }
      //If ret == GENDEV_INCOMPLETE_INFO, we don't need to do anything special.
      //In all three cases, add the match line to our (now) incomplete driver description.
      info->matches.push_back(match);
    }
    return;
  }
  if (line.front().value == "alias" && line.size() > 3) {
	  info->aliases.push_back({line[1].value,line[2].value});
	  return;
  }

  if (find_token_type(TK_EQUAL, line)) {
    generic_assignment_line(line, info, mg, context);
    return;
  }

}

int generic_config_loop(moltengamepad* mg, std::istream& in, std::string& path) {
  bool keep_looping = true;
  std::string header = "";
  char* buff = new char [1024];
  bool need_to_free_info = true;
  struct generic_driver_info* info = new generic_driver_info;
  context context;
  context.line_number = 1;
  context.path = path;

  while (keep_looping) {
    in.getline(buff, 1024);

    auto tokens = tokenize(std::string(buff));

    if (!tokens.empty() && tokens.front().value == "quit") {
      keep_looping = false;
    }

    generic_parse_line(tokens, info, mg, context);

    context.line_number++;

    if (in.eof()) break;


  }

  
  
  int ret = add_generic_manager(mg, *info);
  if (ret == GENDEV_INCOMPLETE_INFO) {
    delete info;
    mg->drivers.err(0,"gendev: missing name, devname, or events.", path, context.line_number);
  }

  delete[] buff;
  return 0;
}

void check_driver_warnings(const generic_driver_info& info) {
  bool trigger_axes[2] = {false,false};
  bool trigger_btns[2] = {false,false};
  for (auto ev : info.events) {
    if (ev.name == "tr2_axis")
      trigger_axes[0] = true;
    else if (ev.name == "tl2_axis")
      trigger_axes[1] = true;
    else if (ev.name == "tr2")
      trigger_btns[0] = true;
    else if (ev.name == "tl2")
      trigger_btns[1] = true;
  }
  for (auto pair : info.aliases) {
    const std::string& name = pair.first;
    if (name == "tr2_axis")
      trigger_axes[0] = true;
    else if (name == "tl2_axis")
      trigger_axes[1] = true;
    else if (name == "tr2")
      trigger_btns[0] = true;
    else if (name == "tl2")
      trigger_btns[1] = true;
  }
  //It is hard to generically describe this error to the user. Much clearer when concretely saying "tr2" rather than "tr2/tl2".
  if (trigger_axes[0] && trigger_btns[0]) {
    debug_print(DEBUG_NONE,3, "driver: generic driver ", info.name.c_str(), " exposes both tr2 and tr2_axis. This is likely an error, as tr2 is only for devices without analog trigger values. tr2_axis_btn can be used instead.");
  }
  //Should we only print one case when both occur? It is likely both will occur at the same time.
  //Decided to just print both to further encourage the user to change things.
  if (trigger_axes[1] && trigger_btns[1]) {
    debug_print(DEBUG_NONE,3, "driver: generic driver ", info.name.c_str(), " exposes both tl2 and tl2_axis. This is likely an error, as tl2 is only for devices without analog trigger values. tl2_axis_btn can be used instead.");
  }
}

int add_generic_manager(moltengamepad* mg, generic_driver_info& info) {
  if (info.events.size() > 0 && !info.name.empty() && !info.devname.empty()) {
    if (info.rumble && info.flatten) {
      mg->drivers.err(0,"gendev: " +info.name + ": flatten and rumble cannot both be active. Rumble is disabled.");
      info.rumble = false;
    }
    generic_manager* manager = new generic_manager(mg, info);
    auto man = mg->add_manager(manager->get_plugin(), manager);
    if (man)
      check_driver_warnings(info);
    if (!man)
      return GENDEV_REJECTED_MANAGER;
    return 0;
  } 
  return GENDEV_INCOMPLETE_INFO;
}

device_plugin genericdev;
manager_plugin genericman;

int init_generic_callbacks() {
  generic_device::methods = *(plugin_methods.head.device);
  generic_manager::methods = *(plugin_methods.head.manager);

  //set manager call backs
  memset(&genericman, 0, sizeof(genericman));
  genericman.size = sizeof(genericman);

  genericman.name = "generic";
  genericman.subscribe_to_gamepad_profile = true;
  genericman.init = [] (void* wm, device_manager* ref) -> int {
    return ((generic_manager*)wm)->init(ref);
  };
  genericman.destroy = [] (void* data) -> int {
    delete (generic_manager*) data;
    return 0;
  };
  genericman.start = [] (void*) { return 0;};
  genericman.process_manager_option = nullptr;
  genericman.process_udev_event = [] (void* ref, struct udev* udev, struct udev_device* dev) {
    return ((generic_manager*)ref)->accept_device(udev, dev);
  };
  genericman.process_deferred_udev_event = [] (void* ref, struct udev* udev, struct udev_device* dev) {
    return ((generic_manager*)ref)->accept_deferred_device(udev, dev);
  };

  //set device callbacks
  memset(&genericdev, 0, sizeof(genericdev));
  genericdev.size = sizeof(genericdev);
  genericdev.name_stem = "generic_device";
  genericdev.uniq = "";
  genericdev.phys = "";
  genericdev.init = [] (void* gendev, input_source* ref)  {
    return ((generic_device*)gendev)->init(ref);
  };
  genericdev.destroy = [] (void* data) -> int {
    delete (generic_device*) data;
    return 0;
  };
  genericdev.get_description = [] (const void* ref) {
    return "No description available";
  };
  genericdev.get_type = [] (const void* ref) {
    return ((const generic_device*)ref)->type.c_str();
  };
  genericdev.process_event = [] (void* ref, void* tag) -> int {
    ((generic_device*)ref)->process(tag);
    return 0;
  };
  genericdev.process_option = [] (void* ref, const char* opname, MGField opvalue) {
    return -1;
  };
  genericdev.upload_ff =  [] (void* ref, ff_effect* effect) {
    return ((generic_device*)ref)->upload_ff(effect);
  };
  genericdev.erase_ff =  [] (void* ref, int id) {
    return ((generic_device*)ref)->erase_ff(id);
  };
  genericdev.play_ff =  [] (void* ref, int id, int repetitions) {
    return ((generic_device*)ref)->play_ff(id, repetitions);
  };
}


