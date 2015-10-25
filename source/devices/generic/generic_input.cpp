#include "generic.h"

generic_device::generic_device(std::vector<gen_source_event> &inevents, int fd) {
  this->fd = fd;
  for (int i = 0; i < inevents.size(); i++) {
    source_event ev;
    ev.id = i;
    ev.name = inevents.at(i).name.c_str();
    ev.descr = inevents.at(i).descr.c_str();
    ev.type = inevents.at(i).type;
    ev.value = 0;
    ev.trans = nullptr;
    register_event(ev);
    eventcodes.insert(std::pair<int,int>(inevents.at(i).id,i));
  }
  
  watch_file(fd,&fd);
}

generic_device::~generic_device() {
  if (node) udev_device_unref(node);
  free (nameptr);
}
  
int generic_device::set_player(int player_num) {};


void generic_device::list_events(cat_list &list) {
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

void generic_device::list_options(name_list &list) {};
  
  
void generic_device::update_option(const char* opname, const char* value) {};
  
enum entry_type generic_device::entry_type(const char* name) {
  for (auto ev : events) {
    if (!strcmp(ev.name,name)) return ev.type;
  }
  return NO_ENTRY;
}

void generic_device::process(void* tag) {
  struct input_event ev;
  int file = fd;
  if (pipe_read >= 0 && tag == &pipe_read) {file = pipe_read;};
  int ret = read(file,&ev,sizeof(ev));
  if (ret > 0) {
    auto it = eventcodes.find(ev.code);
    if (it == eventcodes.end()) return;
    send_value(it->second,ev.value);
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


