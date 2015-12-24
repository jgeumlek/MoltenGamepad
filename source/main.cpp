#include <iostream>
#include "moltengamepad.h"
#include <getopt.h>


int parse_opts(moltengamepad::mg_options &options,int argc, char* argv[]);

int main(int argc, char* argv[]) {
  
  moltengamepad::mg_options options;
  options.look_for_devices = true;
  options.listen_for_devices = true;
  options.make_keyboard = true;
  options.make_mouse = false;
  options.make_pointer = false;
  options.dpad_as_hat = false;
  options.num_gamepads = 4;
  options.config_dir = "";
  options.profile_dir = "";
  options.gendev_dir = "";
  options.fifo_path = "";
  options.uinput_path = "";
  int ret = parse_opts(options,argc,argv);

  if (ret > 0) return 0;
  if (ret < 0) return ret;
  
  try {

   moltengamepad mg(options);

   mg.init();
   
   shell_loop(&mg, std::cin);

  } catch (int e) {
    return e;
  }
  
  return 0;
}

int print_usage(char* execname) {
  std::cout << "USAGE:\n";
  std::cout << "\t" << execname << " [OPTIONS]\n";
  std::cout << "\n";
  std::string help_text = ""\
"--help -h\n"\
"\tShow this message\n"\
"\n"\
"--uinput-path -u\n"\
"\tSet where the uinput node is found on the system\n"\
"\n"\
"--fifo-path -f\n"\
"\tSet where the fifo command channel should be placed.\n"\
"\n"\
"--profiles-path -p\n"\
"\tSet where the profiles are located\n"\
"\n"\
"--gendev-path -g\n"\
"\tSet where the generic device descriptions are located\n"\
"\n"\
"--config-path -c\n"\
"\tSet where the general config files are located\n"\
"\n"\
"--num-gamepads -n\n"\
"\tSet how many virtual gamepads will be created\n"\
"\n"\
"--no-make-keys\n"\
"\tDisable the creation of a virtual keyboard\n"\
"\n"\
"--no-enumerate\n"\
"\tDisable the search for already connected devices\n"\
"\n"\
"--no-monitor\n"\
"\tDisable listening for future connected devices\n"\
"\n"\
"--dpad-as-hat\n"\
"\tOutput dpad events as a hat rather than separate buttons\n"\
;

  std::cout << help_text;
  return 0;
}

  

int parse_opts(moltengamepad::mg_options &options, int argc, char* argv[]) {
  
  char c = 0;
  
  static struct option long_options[] = {
    {"help",          0,    0,  'h'},
    {"uinput-path",   1,    0,  'u'},
    {"fifo-path",     1,    0,  'f'},
    {"profiles-path", 1,    0,  'p'},
    {"gendev-path",   1,    0,  'g'},
    {"config-path",   1,    0,  'c'},
    {"num-gamepads",  1,    0,  'n'},
    {"no-make-keys",  0,    0,    0},
    {"no-enumerate",  0,    0,    0},
    {"no-monitor",    0,    0,    0},
    {"dpad-as-hat",    0,    0,    0},
    {0,               0,    0,    0},
  };
  int long_index;
  
  while (c != -1) {
    c = getopt_long(argc,argv,"u:p:g:n:c:f:h", long_options, &long_index);
    switch (c) {
      case 0:
        if (long_index == 6) {options.make_keyboard = false;};
        if (long_index == 7) {options.look_for_devices = false;};
        if (long_index == 8) {options.listen_for_devices = false;};
        if (long_index == 9) {options.dpad_as_hat = true;};
        break;
      case 'u':
        options.uinput_path = std::string(optarg);
        break;
      case 'f':
        options.fifo_path = std::string(optarg);
        break;
      case 'p':
        options.profile_dir = std::string(optarg);
        break;
      case 'g':
        options.gendev_dir = std::string(optarg);
        break;
      case 'c':
        options.config_dir = std::string(optarg);
        break;
      case 'h':
        print_usage(argv[0]);
        return 10;
        break;
      case 'n':
        try {
          options.num_gamepads = std::stoi(optarg);
          if (options.num_gamepads < 0 ) throw -3;
        } catch (...) {
          std::cerr << "could not parse numeric value for number of gamepads." << std::endl; 
          return -5; 
          
        }
        break;
    }
        
  }
  
  return 0;
}
    