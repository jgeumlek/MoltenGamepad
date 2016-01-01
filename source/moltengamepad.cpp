#include "moltengamepad.h"
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <errno.h>
#include <glob.h>

moltengamepad::moltengamepad() {
}

//FUTURE WORK: Make it easier to specify additional virtpad styles.
const virtpad_settings default_padstyle = {
  {"Virtual Gamepad (MoltenGamepad)",1,1,1}, //u_ids
  false, //dpad_as_hat
  true, //analog_triggers
  "SEWN", //facemap_1234
};

const virtpad_settings xpad_padstyle = {
  {"Microsoft X-Box 360 pad",0x045e,0x028e,0x110}, //u_ids
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
  std::cout<< "Shutting down."<< std::endl;
  
  std::ofstream fifo;
  fifo.open(options.fifo_path,std::ostream::out);
  unlink(options.fifo_path.c_str());
  fifo << "quit" << std::endl;
  if (remote_handler) {
    remote_handler->join();
    delete remote_handler;
  }
  
  for (auto it = devs.begin(); it != devs.end(); ++it) {
    delete (*it);
  }
  delete slots;
}

std::string find_config_folder() {
  const char *config_home = getenv("XDG_CONFIG_HOME");
  if (!config_home || config_home[0] == '\0') {
    return (std::string(getenv("HOME")) + std::string("/.config/moltengamepad"));
  }
  return std::string(config_home) + "/moltengamepad";
};



int fifo_loop(moltengamepad* mg) {
  bool keep_looping = true;
  while (keep_looping) {
    std::ifstream file;
    file.open(mg->options.fifo_path,std::istream::in);
    if (file.fail()) break;
    shell_loop(mg,file);
    file.close();
  }
}

int moltengamepad::init() {
  virtpad_settings padstyle = default_padstyle;
  padstyle.dpad_as_hat = options.dpad_as_hat;
  if (options.mimic_xpad) padstyle = xpad_padstyle;
  slots = new slot_manager(options.num_gamepads, options.make_keyboard, padstyle);
  
  devs.push_back( new wiimotes(slots));
  
  if (options.config_dir.empty()) options.config_dir = find_config_folder();
  
  mkdir(options.config_dir.c_str(),0770);
  
  if (options.profile_dir.empty()) options.profile_dir = options.config_dir + "/profiles/";
  mkdir((options.profile_dir).c_str(),0770);
  
  if (options.gendev_dir.empty()) options.gendev_dir = options.config_dir + "/gendevices/";
  mkdir((options.gendev_dir).c_str(),0770);
  
  glob_t globbuffer;
  std::string fileglob = options.config_dir +  "/gendevices/*.cfg";
  glob(fileglob.c_str(), 0, nullptr, &globbuffer);
  
  for (int i = 0; i < globbuffer.gl_pathc; i++) {
    std::ifstream file;
    file.open(globbuffer.gl_pathv[i], std::istream::in);
    
    if (!file.fail()) {
      int ret = generic_config_loop(this, file);
      if (ret) std::cerr << "Generic device config "<< globbuffer.gl_pathv[i] << " failed.";
    }
  }
  
  globfree(&globbuffer);
    
  
   

  udev.set_managers(&devs);
  if (options.listen_for_devices) udev.start_monitor();
  if (options.look_for_devices)   udev.enumerate();
  if (options.make_fifo) {
    const char *run_dir = getenv("XDG_RUNTIME_DIR");
    if (options.fifo_path.empty() && run_dir) {
      options.fifo_path = std::string(run_dir) + "/moltengamepad";
    }
    if (options.fifo_path.empty()) {
      std::cerr << "Could not locate fifo path. Use the --fifo-path command line argument." << std::endl;
      throw -1;
    }
    int ret = mkfifo(options.fifo_path.c_str(),0666);
    if (ret < 0)  {
      perror("making fifo:");
      options.fifo_path = "";
      throw -1;
    } else {
      remote_handler = new std::thread(fifo_loop,this);
    }
  }
  
  
}



device_manager* moltengamepad::find_manager(const char* name) {
  for (auto it = devs.begin(); it != devs.end(); it++) {
    if (!strcmp((*it)->name,name)) return (*it);
  }
  return nullptr;
}

input_source* moltengamepad::find_device(const char* name) {
  for (auto it = devs.begin(); it != devs.end(); it++) {
    input_source* dev = (*it)->find_device(name);
    if (dev) return dev;
  }
  return nullptr;
}


