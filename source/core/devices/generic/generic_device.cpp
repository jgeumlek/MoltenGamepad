#include "generic.h"

device_methods generic_device::methods;

generic_device::generic_device(std::vector<split_ev_info>& split_events, int total_events, generic_file* file, const std::string& type, bool rumble) : type(type), rumble(rumble) {
  int fd = file->get_fd();
  this->file = file;
  eventstates = (event_state*)calloc(total_events, sizeof(event_state));
  this->total_events = total_events;

  //We will enable events one-by-one later
  for (int i = 0; i < total_events; i++) {
    eventstates[i] = EVENT_DISABLED;
  }

  for (int i = 0; i < split_events.size(); i++) {
    
    input_absinfo     abs;
    memset(&abs, 0, sizeof(abs));

    auto ev = split_events[i];

    eventstates[ev.id] = EVENT_ACTIVE;
    

    int type = EV_KEY;
    if (ev.type == DEV_AXIS) type = EV_ABS;
    if (ev.type == DEV_REL)  type = EV_REL;

    evcode code(type, ev.code);
    //Read in ABS ranges so we can rescale the generated events
    if (ev.type == DEV_AXIS) {
      if (ioctl(fd, EVIOCGABS(split_events[i].code), &abs)) {
        perror("evdev EVIOCGABS ioctl");
      }
    }
    decodedevent decoded(ev.id, abs);
    eventcodes.insert(std::pair<evcode, decodedevent>(code, decoded));
  }
}

int generic_device::init(input_source* ref) {
  this->ref = ref;
  for (int i = 0; i < total_events; i++)
    methods.toggle_event(ref, i, eventstates[i]);

  int internal[2];
  pipe(internal);
  methods.watch_file(ref, internal[0], &pipe_read);
  pipe_write = internal[1];
  pipe_read = internal[0];
  return 0;
}

generic_device::~generic_device() {
  if (node) udev_device_unref(node);
  if (eventstates) free(eventstates);
}

void generic_device::process(void* tag) {
  struct input_event ev;
  int file = pipe_read;
  int ret = read(file, &ev, sizeof(ev));
  if (ret > 0) {
    if (ev.type == EV_SYN) {
      methods.send_syn_report(ref);
    }
    evcode code(ev.type, ev.code);
    auto it = eventcodes.find(code);
    if (it == eventcodes.end()) return;
    decodedevent decoded = it->second;
    int id = decoded.first;
    if (ev.type == EV_KEY) {
      methods.send_value(ref, id, ev.value);
    }
    if (ev.type == EV_ABS) {
      //do some ABS rescaling.
      //currently ignoring old deadzone ("flat")
      input_absinfo info = decoded.second;
      int value = ev.value;
      int64_t oldscale = info.maximum - info.minimum;
      if (oldscale == 0) {
        methods.send_value(ref, id, value);
        return;
      }
      int64_t newscale = 2 * ABS_RANGE;
      int64_t scaledvalue = -ABS_RANGE + (value - info.minimum) * newscale / oldscale;
      methods.send_value(ref, id, scaledvalue);
    }
  }
}

int generic_device::upload_ff(ff_effect* effect) {
  if (!rumble)
    return -1;
  int fd = file->get_fd();
  int ret = ioctl(fd, EVIOCSFF, effect);
  return effect->id;
}
  
int generic_device::erase_ff(int id) {
  if (!rumble)
    return -1;
  int fd = file->get_fd();
  int ret = ioctl(fd, EVIOCRMFF, id);
  return 0;
}

int generic_device::play_ff(int id, int repetitions) {
  if (!rumble)
    return -1;
  int fd = file->get_fd();
  input_event ev;
  memset(&ev, 0, sizeof(ev));
  ev.type = EV_FF;
  ev.code = id;
  ev.value = repetitions;
  write(fd, &ev, sizeof(ev));
  return 0;
}
