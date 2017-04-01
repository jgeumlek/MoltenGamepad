#include "../moltengamepad.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include "../parser.h"

void print_profile(profile& profile, std::ostream& out) {
  profile.lock.lock();
  std::vector<std::string> unmapped_events;
  for (auto it = profile.mapping.begin(); it != profile.mapping.end(); it++) {
    if (!it->second.active_group) {
      //if it has an active group translation, don't print it out separately.
      if (it->second.trans) {
        out << profile.name << "." << it->first << (it->second.direction == -1 ? "-" : "") << " = ";
        MGTransDef def;
        it->second.trans->fill_def(def);
        MGparser::print_def(it->second.type, def, out);
        out << std::endl;
      } else {
        unmapped_events.push_back(it->first);
      }
    }
  }
  for (auto entry : profile.group_trans) {
    out << profile.name << ".(";
    if (entry.second.fields.size() >= 1) {
      out << entry.second.fields[0] << (entry.second.directions[0] == -1 ? "-" : "");
    }
    for (int i = 1; i < entry.second.fields.size(); i++) {
      out << "," << entry.second.fields[i] << (entry.second.directions[i] == -1 ? "-" : "");
    }
    out << ") = ";
    MGTransDef def;
    entry.second.trans->fill_def(def);
    MGparser::print_def(NO_ENTRY, def, out);
    out << std::endl;
  }
  if (!unmapped_events.empty()) {
    out << "#unmapped events:";
    for (auto event : unmapped_events) {
      out << " " << event;
    }
    out << std::endl;
  }
  std::vector<option_info> opts;
  profile.list_options(opts);
  for (auto opt : opts) {
    out << profile.name << ".?" << opt.name << " = ";
    if (opt.value.type != MG_STRING) {
      out << opt.stringval << std::endl;
    } else {
      std::string str = opt.stringval;
      escape_string(str);
      out << "\"" << str << "\"" << std::endl;
    }
    out << "   #  " << opt.descr << std::endl;
  }
  if (profile.mapping.empty() && profile.group_trans.empty() && opts.empty())
    out << "#(empty profile)" << std::endl;
  profile.lock.unlock();
}

int do_print_profile(moltengamepad* mg, std::string name, std::ostream& out) {
  if (name.empty()) {
    mg->for_all_profiles([&out] (auto prof) { out << prof->name << std::endl; });
    return 0;
  }

  auto prof = mg->find_profile(name);
  if (prof) {
    print_profile(*(prof.get()), out);
    return 0;
  }
  return -1;

}

void print_driver_dev_list(device_manager* man, std::ostream& out) {
  if (!man) return;
  out << "(" << man->name << ")\n";
  bool has_dev = false;
  auto print_dev = [&out, &has_dev] (const input_source* dev) {
    out << dev->get_name() << ":\t" << dev->get_description() << std::endl;
    has_dev = true;
  };

  man->for_all_devices(print_dev);

  if (!has_dev) {
    out << "  no devices" << std::endl;
  }
  
}

//shortened so that they are under 8 characters when padded by 2. Allows for nicer tab stops.
const char* event_type_str[] = {"null","opt","btn","axis","rel", "unk", "group"};

void print_event(const char* name, const char* descr, entry_type type, event_state state, std::ostream& out) {
  if (state == EVENT_DISABLED)
    return; //EVENT_DISABLED is a hack since we can't delete them. Any attempt to print them should be wrong.
  if (type >= 0 && type < DEV_EVENT_GROUP)
          out << "  " << event_type_str[type];
  const char* evstate = (state == EVENT_INACTIVE) ? " (inactive)" : "";
  out << "\t" << name << ":\t" << descr << evstate << std::endl;
}

int do_print_devs(moltengamepad* mg, std::string name, std::ostream& out) {
  if (!name.empty()) {
    std::shared_ptr<input_source> dev = mg->find_device(name.c_str());
    if (dev.get()) {
      out << dev->get_name() <<std::endl;
      out << " \"" << dev->get_description() << "\"" << std::endl;
      out << " type: " << dev->get_type() << std::endl;
      std::string uniq = dev->get_uniq();
      std::string phys = dev->get_phys();
      if (!uniq.empty()) out << " uniq: \"" << uniq << "\"" << std::endl;
      if (!phys.empty()) out << " phys: \"" << phys << "\"" << std::endl;

      out << " events:" << std::endl;
      
      const std::vector<source_event>& events = dev->get_events();
      for (auto v : events) {
        print_event(v.name, v.descr, v.type, v.state, out);
      }

      std::vector<option_info> list;
      dev->list_options(list);
      out << " options:" << std::endl;
      for (auto v : list) {
        out << "?" << v.name << " = " << v.stringval << "\n\t" << v.descr << std::endl;
      }
      if (list.empty()) {
        out << "(no options)" << std::endl;
      }

      return 0;
    }

    return -1;
  }

  for (auto man : mg->managers) {
    print_driver_dev_list(man, out);
  }

  return 0;
}

int do_print_drivers(moltengamepad* mg, std::string name, std::ostream& out) {
  if (name.empty()) {
    for (auto man : mg->managers) {
      out << man->name <<  std::endl;
    }
    return 0;
  }
  device_manager* man = mg->find_manager(name.c_str());
  if (man) {
    print_driver_dev_list(man, out);
    //we could also print out this driver's registered events/options, but
    //I think that would be too verbose, and would obscure the useful device list.
    //Perhaps managers should have a description field too.

    //It might be worth printing driver-level options here.
  }

  return 0;

}

int do_print_slot(output_slot* slot, bool details, std::ostream& out) {
  const char* statestr = "";
  if (slot->state == SLOT_INACTIVE)
    statestr = "(inactive)";
  if (slot->state == SLOT_DISABLED)
    statestr = "(disabled)";
  out << slot->name << ":\t" << slot->descr << statestr << std::endl;
  if (details) {
    for (auto e : slot->options) {
      out << "\t" << e.first << " = " << e.second << std::endl;
    }
  }
};

int do_print_slots(moltengamepad* mg, std::string name, std::ostream& out) {
  if (name.empty()) {
    for (auto slot : mg->slots->slots) {
      do_print_slot(slot, false, out);
    }
    do_print_slot(mg->slots->keyboard, false, out);
    do_print_slot(mg->slots->dummyslot, false, out);
    if (mg->slots->debugslot) do_print_slot(mg->slots->debugslot, false, out);
    return 0;
  }
  output_slot* slot = mg->slots->find_slot(name);
  if (slot) {
    do_print_slot(slot, true, out);
  }

  return 0;

}

int do_print_options(moltengamepad* mg, std::string name, std::ostream& out) {
  if (name.empty()) {
    out << "slots" << std::endl;
    for (auto man : mg->managers) {
      if (man->has_options) out << man->name <<  std::endl;
    }
    return 0;
  }
  std::vector<option_info> list;
  mg->list_options(name, list);
  for (auto opt : list) {
    if (opt.value.type != MG_STRING) {
      out << opt.name << " = " << opt.stringval << std::endl;
    } else {
      out << opt.name << " = ";
      std::string val = opt.stringval;
      escape_string(val);
      out << "\"" << val << "\"" << std::endl;
    }
    out << "\t" << opt.descr << std::endl;
  }

  return 0;

}

int do_print_trans(moltengamepad* mg, std::string name, std::ostream& out) {
  if (name.empty()) {
    out << "event translators" << std::endl;
    for (auto gen : MGparser::trans_gens) {
      if (gen.second.generate && gen.second.decl.decl_str) out << "\t" << gen.second.decl.decl_str <<  std::endl;
    }
    out << "group translators" << std::endl;
    for (auto gen : MGparser::trans_gens) {
      if (gen.second.group_generate && gen.second.decl.decl_str) out << "\t" << gen.second.decl.decl_str <<  std::endl;
    }
    return 0;
  }

  auto it = MGparser::trans_gens.find(name);
  if (it != MGparser::trans_gens.end()) {
    if (it->second.decl.decl_str)
      out << it->second.decl.decl_str << std::endl;
  } else {
    out << "could not find translator \"" << name << "\"" << std::endl;
    return -1;
  }
  return 0;

}

int do_print_alias(moltengamepad* mg, std::string name, std::ostream& out) {
  if (name.empty()) {
    out << "USAGE: print aliases <profile>\nA profile must be specified." << std::endl;
    return -1;
  }

  auto prof = mg->find_profile(name);
  if (prof) {
    std::lock_guard<std::mutex> guard(prof->lock);
    out << "aliases used by " << name << std::endl;
    for (auto pair : prof->aliases) {
      std::string local = pair.second;
      if (local.empty())
        continue;
      if (local.front() != ' ') {
        //simple alias
        out << "\t" << pair.first << "\t->\t" << local << std::endl;
      } else {
        //group alias
        std::vector<token> tokens = tokenize(local);
        tokens.pop_back(); //ignore endline.
        out << "\t" << pair.first << "\t->\t(";
        bool comma = false;
        for (auto token : tokens) {
          if (comma) out << ",";
          out << token.value;
          comma = true;
        }
        out << ")" << std::endl;
      }
    }
  } else {
    out << "could not find profile" << std::endl;
    return -1;
  }
  return 0;

}



int do_print_events(moltengamepad* mg, std::string name, std::ostream& out) {
  if (name.empty()) {
    out << "USAGE: print events <driver or device>\nA driver or a device must be specified." << std::endl;
    return -1;
  }

  //we won't print out the registered options here.
  //They aren't events, and they are already handled nicely via "print profile ..."

  //printing events for devices is already done for "print devices <device name>"
  //So this is a little redundant. Here we do a little more work to separate out active/inactive events.
  auto dev = mg->find_device(name.c_str());
  if (dev) {
    //a device has a notion of events being active or inactive.
    auto events = dev->get_events();
    out << "active events:" << std::endl;
    bool has_inactive = false;
    for (source_event &ev : events) {
      if (ev.state == EVENT_ACTIVE) {
        print_event(ev.name, ev.descr, ev.type, ev.state, out);
      }
      if (ev.state == EVENT_INACTIVE)
        has_inactive = true;
    }
    if (has_inactive) {
      out << "inactive events:" << std::endl;
      for (source_event &ev : events) {
        if (ev.state == EVENT_INACTIVE) {
          print_event(ev.name, ev.descr, ev.type, ev.state, out);
        }
      }
    }
    return 0;
  }
  //there is no other way to print out a manager's events.
  //This is where "print events ..." is not redundant.
  auto man = mg->find_manager(name.c_str());
  if (man) {
    //a manager has no notion of inactive events, and it uses a different struct.
    auto events = man->get_events();
    for (event_decl &ev : events) {
        print_event(ev.name, ev.descr, ev.type, EVENT_ACTIVE, out);
    }
    return 0;
  }
  out << "could not find driver or device" << std::endl;
  return -1;
}


const char* id_types[] = {"name", "uniq", "phys"};
int do_print_assignments(moltengamepad* mg, std::string name, std::ostream& out) {
  mg->slots->for_all_assignments([&out] (slot_manager::id_type type, std::string id, output_slot* slot) {
    out << "\t" <<  id_types[type] << " \"" << id << "\" -> " << slot->name << std::endl;
  });
}

#define PRINT_USAGE ""\
"USAGE:\n\tprint <type> [element]\n"\
"\ttypes recognized: drivers, devices, profiles, slots,\n"\
"\t\toptions, events, aliases, assignments, translators\n"\
"\tprint <type> will list all elements of that type\n"\
"\tprint <type> [element] will show detailed info on that element\n"
int do_print(moltengamepad* mg, std::vector<token>& command, response_stream* out) {
  if (command.size() < 2) {
    out->print(PRINT_USAGE);
    return -1;
  }
  std::stringstream ss;
  std::string arg = (command.size() >= 3 && command.at(2).type == TK_IDENT) ? command.at(2).value : "";
  //be generous, compare only the roots of the categories. So "slot" or "slots" is fine.
  bool matched = false;
  if (command.at(1).value.compare(0, 6, "driver") == 0) {
    do_print_drivers(mg, arg, ss);
    matched = true;
  }
  if (command.at(1).value.compare(0, 6, "device") == 0) {
    do_print_devs(mg, arg, ss);
    matched = true;
  }
  if (command.at(1).value.compare(0, 7, "profile") == 0) {
    do_print_profile(mg, arg, ss);
    matched = true;
  }
  if (command.at(1).value.compare(0, 4, "slot") == 0) {
    do_print_slots(mg, arg, ss);
    matched = true;
  }
  if (command.at(1).value.compare(0, 6, "option") == 0) {
    do_print_options(mg, arg, ss);
    matched = true;
  }
  if (command.at(1).value.compare(0, 6, "assign") == 0) {
    do_print_assignments(mg, arg, ss);
    matched = true;
  }
  if (command.at(1).value.compare(0, 5, "trans") == 0) {
    do_print_trans(mg, arg, ss);
    matched = true;
  }
  if (command.at(1).value.compare(0, 5, "alias") == 0) {
    do_print_alias(mg, arg, ss);
    matched = true;
  }
  if (command.at(1).value.compare(0, 5, "event") == 0) {
    do_print_events(mg, arg, ss);
    matched = true;
  }

  if (matched) {
    out->print(ss.str());
  } else {
    out->print(PRINT_USAGE);
  }
  return 0;
}


