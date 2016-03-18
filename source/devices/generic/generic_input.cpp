#include "generic.h"

generic_device::generic_device(std::vector<gen_source_event>& inevents, int fd, bool watch, slot_manager* slot_man,input_source::devtype type) : input_source(slot_man, type) {
  this->fd = fd;
  for (int i = 0; i < inevents.size(); i++) {
    source_event ev;
    input_absinfo     abs;
    memset(&abs, 0, sizeof(abs));
    ev.id = i;
    ev.name = inevents.at(i).name.c_str();
    ev.descr = inevents.at(i).descr.c_str();
    ev.type = inevents.at(i).type;
    ev.value = 0;
    ev.trans = nullptr;
    register_event(ev);

    int type = EV_KEY;
    if (ev.type == DEV_AXIS) type = EV_ABS;
    if (ev.type == DEV_REL)  type = EV_REL;

    evcode code(type, inevents.at(i).id);
    if (ev.type == DEV_AXIS) {
      if (ioctl(fd, EVIOCGABS(inevents.at(i).id), &abs)) {
        perror("evdev EVIOCGABS ioctl");
      }
    }
    decodedevent decoded(i, abs);
    eventcodes.insert(std::pair<evcode, decodedevent>(code, decoded));
  }

  if (watch) watch_file(fd, &fd);
}

generic_device::~generic_device() {
  if (node) udev_device_unref(node);
  free(nameptr);
}

int generic_device::set_player(int player_num) {};


void generic_device::list_events(cat_list& list) {
  struct category cat;
  struct name_descr info;

  cat.name = "Generic";
  for (int i = 0; i < events.size(); i++) {
    info.name = events[i].name;
    info.descr = events[i].descr;
    info.data = events[i].type;
    cat.entries.push_back(info);
  }

  list.push_back(cat);
  cat.entries.clear();
}

void generic_device::list_options(name_list& list) {};


void generic_device::update_option(const char* opname, const char* value) {};

enum entry_type generic_device::entry_type(const char* name) {
  auto alias = aliases.find(std::string(name));
  if (alias != aliases.end())
    name = alias->second.c_str();
  for (auto ev : events) {
    if (!strcmp(ev.name, name)) return ev.type;
  }
  return NO_ENTRY;
}

void generic_device::process(void* tag) {
  struct input_event ev;
  int file = fd;
  if (pipe_read >= 0 && tag == &pipe_read) {
    file = pipe_read;
  };
  int ret = read(file, &ev, sizeof(ev));
  if (ret > 0) {
    if (ev.type == EV_SYN && out_dev) {
      out_dev->take_event(ev);
      return;
    }
    evcode code(ev.type, ev.code);
    auto it = eventcodes.find(code);
    if (it == eventcodes.end()) return;
    decodedevent decoded = it->second;
    int id = decoded.first;
    if (ev.type == EV_KEY) {
      send_value(id, ev.value);
    }
    if (ev.type == EV_ABS) {
      //do some ABS rescaling.
      //currently ignoring old deadzone ("flat")
      input_absinfo info = decoded.second;
      int value = ev.value;
      int oldscale = info.maximum - info.minimum;
      if (oldscale == 0) {
        send_value(id, value);
        return;
      }
      int newscale = 2 * ABS_RANGE;
      long long int scaledvalue = -ABS_RANGE + (value - info.minimum) * newscale / oldscale;
      send_value(id, scaledvalue);
    }
  }
}

int generic_device::get_pipe() {
  if (pipe_write >= 0) return pipe_write;
  int internal[2];
  pipe(internal);
  watch_file(internal[0], &pipe_read);
  pipe_write = internal[1];
  pipe_read = internal[0];

  return pipe_write;
}


