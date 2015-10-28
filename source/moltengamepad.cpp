#include "moltengamepad.h"
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <errno.h>
#include <glob.h>

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
    if (file.fail()) break;
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
  
  glob_t globbuffer;
  glob((options.config_dir + "/gendevices/*.cfg").c_str(), 0, nullptr, &globbuffer);
  
  for (int i = 0; i < globbuffer.gl_pathc; i++) {
    std::ifstream file;
    file.open(globbuffer.gl_pathv[i], std::istream::in);
    
    if (!file.fail()) {
      generic_config_loop(this, file);
    }
  }
  
  globfree(&globbuffer);
    
  
   

  udev.set_managers(&devs);
  udev.start_monitor();
  udev.enumerate();
  
  const char *run_dir = getenv("XDG_RUNTIME_DIR");
  if (!run_dir || run_dir) {
    options.fifo_path = std::string(run_dir) + "/moltengamepad";
    int ret = mkfifo(options.fifo_path.c_str(),0666);
    if (ret < 0)  {
      perror("making fifo:");
      options.fifo_path = "";
      
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
