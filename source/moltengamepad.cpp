#include "moltengamepad.h"
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <errno.h>
#include <glob.h>
#include <sys/socket.h>
#include <linux/un.h>
#include <sys/types.h>
#include <unistd.h>
#define OSCPKT_OSTREAM_OUTPUT
#define OSCPKT_DEBUG
#include "oscpkt/oscpkt.hh"
#include "devices/wiimote/wiimote.h"
#include "devices/generic/generic.h"


//FUTURE WORK: Make it easier to specify additional virtpad styles.
const virtpad_settings default_padstyle = {
  {"Virtual Gamepad (MoltenGamepad)", 1, 1, 1}, //u_ids
  false, //dpad_as_hat
  true, //analog_triggers
  "SEWN", //facemap_1234
};

const virtpad_settings xpad_padstyle = {
  {"Microsoft X-Box 360 pad", 0x045e, 0x028e, 0x110}, //u_ids
  true, //dpad_as_hat
  true, //analog_triggers
  "SEWN", //facemap_1234
};

moltengamepad::~moltengamepad() {


  udev.set_managers(nullptr);
  if (udev.monitor_thread) {
    udev.stop_thread = true;
    int signal = 0;
    write(udev.pipe_fd, &signal, sizeof(signal));
    udev.monitor_thread->join();
    delete udev.monitor_thread;
    udev.monitor_thread = nullptr;
  }
  std::cout << "Shutting down." << std::endl;

  std::ofstream fifo;
  fifo.open(options.fifo_path, std::ostream::out);
  if (!options.fifo_path.empty())
    unlink(options.fifo_path.c_str());
  if (!options.socket_path.empty())
    unlink(options.socket_path.c_str());
  fifo << "quit" << std::endl;
  if (remote_handler) {
    remote_handler->join();
    delete remote_handler;
  }

  for (auto it = managers.begin(); it != managers.end(); ++it) {
    delete(*it);
  }


  devices.clear();

  delete slots;
}

std::string find_config_folder() {
  const char* config_home = getenv("XDG_CONFIG_HOME");
  if (!config_home || config_home[0] == '\0') {
    return (std::string(getenv("HOME")) + std::string("/.config/moltengamepad"));
  }
  return std::string(config_home) + "/moltengamepad";
};



int fifo_loop(moltengamepad* mg) {
  bool keep_looping = true;
  while (keep_looping && !QUIT_APPLICATION) {
    std::ifstream file;
    file.open(mg->options.fifo_path, std::istream::in);
    if (file.fail()) break;
    shell_loop(mg, file,  1, message_stream::PLAINTEXT);
    file.close();
  }
  return 0;
}

int socket_connection_loop(moltengamepad* mg, int fd);

int socket_server_loop(moltengamepad* mg, struct sockaddr_un* address) {
  bool keep_looping = true;
  socklen_t address_len;
  struct sockaddr_un addr;
  while (keep_looping && !QUIT_APPLICATION) {
    int fd = accept(mg->sock, (struct sockaddr *) &addr, &address_len);
    if (fd >= 0) {
      std::thread* thread = new std::thread(socket_connection_loop, mg, fd);
      thread->detach();
      delete thread;
    } else {
      perror("accept:");
      break;
    }
  }
  return 0;
}

int moltengamepad::init() {
  //This whole function is pretty bad in handling the config directories not being present.
  //But at least we aren't just spilling into the user's top level home directory.
  virtpad_settings padstyle = default_padstyle;
  padstyle.dpad_as_hat = options.dpad_as_hat;
  if (options.mimic_xpad) padstyle = xpad_padstyle;
  slots = new slot_manager(options.num_gamepads, options.make_keyboard, padstyle);
  //add standard streams
  drivers.add_listener(1);
  plugs.add_listener(1);
  errors.add_listener(2);

  managers.push_back(new wiimote_manager(this));
  drivers.driver_message(managers.front(),"init");

  if (options.config_dir.empty()) options.config_dir = find_config_folder();

  mkdir(options.config_dir.c_str(), 0770);

  if (options.profile_dir.empty()) options.profile_dir = options.config_dir + "/profiles/";
  mkdir((options.profile_dir).c_str(), 0770);

  if (options.gendev_dir.empty()) options.gendev_dir = options.config_dir + "/gendevices/";
  mkdir((options.gendev_dir).c_str(), 0770);

  glob_t globbuffer;
  std::string fileglob = options.gendev_dir + "/*.cfg";
  glob(fileglob.c_str(), 0, nullptr, &globbuffer);

  for (int i = 0; i < globbuffer.gl_pathc; i++) {
    std::ifstream file;
    file.open(globbuffer.gl_pathv[i], std::istream::in);

    if (!file.fail()) {
      int ret = generic_config_loop(this, file);
      if (ret) errors.text("generic device config " + std::string(globbuffer.gl_pathv[i]) + " failed.");
    }
  }

  globfree(&globbuffer);

  udev.set_managers(&managers);
  if (options.listen_for_devices) udev.start_monitor();
  if (options.look_for_devices)   udev.enumerate();
  if (options.make_fifo) {
    const char* run_dir = getenv("XDG_RUNTIME_DIR");
    if (options.fifo_path.empty() && run_dir) {
      options.fifo_path = std::string(run_dir) + "/moltengamepad";
    }
    if (options.fifo_path.empty()) {
      errors.text("Could not locate fifo path. Use the --fifo-path command line argument.");
      throw - 1;
    }
    int ret = mkfifo(options.fifo_path.c_str(), 0666);
    if (ret < 0)  {
      perror("making fifo:");
      options.fifo_path = "";
      throw -1;
    } else {
      remote_handler = new std::thread(fifo_loop, this);
    }
  }
  if (options.make_socket) {
    const char* run_dir = getenv("XDG_RUNTIME_DIR");
    if (options.socket_path.empty() && run_dir) {
      options.socket_path = std::string(run_dir) + "/moltengamepad.sock";
    }
    if (options.socket_path.empty()) {
      errors.text("Could not locate socket path. Use the --socket-path command line argument.");
      throw - 1;
    }
    struct sockaddr_un address;
    sock = socket(PF_UNIX, SOCK_STREAM, 0);
    if (sock < 0)  {
      perror("making socket:");
      options.socket_path = "";
      throw -1;
    }
    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    snprintf(address.sun_path, UNIX_PATH_MAX, options.socket_path.c_str());
    if (bind(sock, (struct sockaddr *) &address, sizeof(address)) != 0) {
      perror("binding socket:");
      options.socket_path = "";
      throw -1;
    }
    if (listen(sock, 5) != 0) {
      perror("listening socket:");
      options.socket_path = "";
      throw -1;
    }
    std::thread* socket_listener = new std::thread(socket_server_loop, this, &address);
    socket_listener->detach();
    delete socket_listener;
    
  }

}



device_manager* moltengamepad::find_manager(const char* name) {
  for (auto it = managers.begin(); it != managers.end(); it++) {
    if (!strcmp((*it)->name.c_str(), name)) return (*it);
  }
  return nullptr;
}

std::shared_ptr<input_source> moltengamepad::find_device(const char* name) {
  for (auto it = devices.begin(); it != devices.end(); it++) {

    if (!strcmp((*it)->name.c_str(), name)) return (*it);
  }
  return nullptr;
}
void moltengamepad::add_device(input_source* source) {
  device_list_lock.lock();
  devices.push_back(std::shared_ptr<input_source>(source));
  plugs.plug_event(source,"add");
  device_list_lock.unlock();
}

void moltengamepad::remove_device(input_source* source) {
  device_list_lock.lock();
  plugs.plug_event(source,"remove");
  for (int i = 0; i < devices.size(); i++) {
    if (source == devices[i].get()) {
      devices.erase(devices.begin() + i);
      i--;
    }
  }
  device_list_lock.unlock();
}

void moltengamepad::for_all_devices(std::function<void (std::shared_ptr<input_source>&)> func) {
  device_list_lock.lock();
  for (auto dev : devices)
    func(dev);
  device_list_lock.unlock();
}

void hotplug_messenger::plug_event(const input_source* dev, const std::string& action) {
  oscpkt::PacketWriter pw;
  std::stringstream buffer;
  buffer << name << ": device " << dev->name << " (" << dev->manager->name << ") ";
  if (action == "add") {
    buffer << "added" << std::endl;
  } else if (action == "remove") {
    buffer << "removed" << std::endl;
  } else {
    buffer << action << std::endl;
  }

  if (!osc_fds.empty()) {
    oscpkt::Message msg;
    msg.init("/plug_event").pushStr(dev->name).pushStr(dev->manager->name).pushStr(action);
    pw.init().addMessage(msg);
  }
  message(buffer.str(), pw);
}

void driver_messenger::driver_message(const device_manager* man, const std::string& action) {
  oscpkt::PacketWriter pw;
  std::stringstream buffer;
  buffer << name << ": " << man->name;
  if (action == "init") {
    buffer << " initialized" << std::endl;
  }

  if (!osc_fds.empty()) {
    oscpkt::Message msg;
    msg.init("/driver_event").pushStr(man->name).pushStr(action);
    pw.init().addMessage(msg);
  }
  message(buffer.str(), pw);
}
