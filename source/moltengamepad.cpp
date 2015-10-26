#include "moltengamepad.h"
#include <iostream>
#include <sys/stat.h>
#include <errno.h>

moltengamepad::moltengamepad() {
  slots = new slot_manager();
}

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
  
  unlink(options.fifo_path.c_str());
  
  for (auto it = devs.begin(); it != devs.end(); ++it) {
    delete (*it);
  }
  delete slots;
}

std::string find_config_folder() {
  const char *config_home = getenv("XDG_CONFIG_HOME");
  if (!config_home || config_home[0] == '\0') {
    return (std::string(getenv("HOME")) + std::string("/.config/"));
  }
};

std::string find_profile_folder() {
  return "";
};

int fifo_loop(moltengamepad* mg) {
  bool keep_looping = true;
  while (keep_looping) {
    std::ifstream file;
    file.open(mg->options.fifo_path,std::istream::in);
    shell_loop(mg,file);
    file.close();
  }
}

int moltengamepad::init() {
  

  
  
  devs.push_back( new wiimotes(slots));
  
  if (options.config_dir.empty()) options.config_dir = find_config_folder();
  
  std::cout<< options.config_dir+"/moltengamepad"<< std::endl;
  mkdir((options.config_dir + "/moltengamepad").c_str(),0770);
  
  options.config_dir = options.config_dir + "/moltengamepad/";
  options.profile_dir = options.config_dir + "/profiles/";
  mkdir((options.profile_dir).c_str(),0770);
  
  mkdir((options.config_dir + "/generics").c_str(),0770);
  
  std::ifstream file;
  file.open(options.config_dir + "/moltengamepad.cfg", std::istream::in);
  
  if (!file.fail()) {
    generic_config_loop(this, file);
  }
    
  
   

  udev.set_managers(&devs);
  udev.start_monitor();
  udev.enumerate();
  
  const char *run_dir = getenv("XDG_RUNTIME_DIR");
  if (!run_dir || run_dir) {
    options.fifo_path = std::string(run_dir) + "/moltengamepad";
    int ret = mkfifo(options.fifo_path.c_str(),0666);
    if (ret < 0 && errno != EEXIST)  {
      perror("making fifo:");
    } else {
      remote_handler = new std::thread(fifo_loop,this);
      remote_handler->detach();
      delete remote_handler;
      remote_handler = nullptr;
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
