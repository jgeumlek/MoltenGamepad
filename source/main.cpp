#include <iostream>
#include "moltengamepad.h"
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <csignal>
#include <fstream>


int parse_opts(moltengamepad::mg_options& options, int argc, char* argv[]);
volatile bool STOP_WORKING = true;
volatile bool QUIT_APPLICATION = false;
moltengamepad* app;
void signal_handler(int signum) {
  if (!STOP_WORKING) {
    //A long running action is to be interrupted.
    STOP_WORKING = true;
    return;
  }
  //Otherwise, we want to interrupt everything! (And let them shut down appropriately)
  QUIT_APPLICATION = true;
  delete app;
  exit(0);

  return;
}


int main(int argc, char* argv[]) {
  STOP_WORKING = true;
  QUIT_APPLICATION = false;
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);
  //signal(SIGHUP, signal_handler);

  moltengamepad::mg_options options;
  options.look_for_devices = true;
  options.listen_for_devices = true;
  options.make_keyboard = true;
  options.make_mouse = false;
  options.make_pointer = false;
  options.dpad_as_hat = false;
  options.mimic_xpad = false;
  options.daemon = false;
  options.num_gamepads = 4;
  options.config_dir = "";
  options.profile_dir = "";
  options.gendev_dir = "";
  options.fifo_path = "";
  options.uinput_path = "";
  options.pidfile = "";
  int ret = parse_opts(options, argc, argv);

  if (ret > 0) return 0;
  if (ret < 0) return ret;

  try {

    moltengamepad* mg = new moltengamepad(options);
    app = mg;

    if (options.daemon) {
      int pid = fork();
      if(pid == 0) {

        mg->init();

        while(!QUIT_APPLICATION)
          sleep(1);
      } else if (pid == -1) {
        std::cerr <<  "Failed to fork.\n";
      } else {
        if(options.pidfile.size() > 0) {
          std::ofstream pf;
          pf.open (options.pidfile);
          pf << pid << "\n";
          pf.close();
        }
      }
    }
    else {
      mg->init();
      shell_loop(mg, std::cin);
      delete mg;
    }

  } catch (int e) {
    return e;
  }

  return 0;
}

int print_version() {
  std::cout <<  "MoltenGamepad version " << VERSION_STRING << "\n";
  return 0;
}

int print_usage(char* execname) {
  print_version();
  std::cout << "USAGE:\n";
  std::cout << "\t" << execname << " [OPTIONS]\n";
  std::cout << "\n";
  std::string help_text = ""\
                          "--help -h\n"\
                          "\tShow this message\n"\
                          "\n"\
                          "--version -v\n"\
                          "\tDisplay the version string\n"\
                          "\n"\
                          "--uinput-path -u\n"\
                          "\tSet where the uinput node is found on the system\n"\
                          "\n"\
                          "--make-fifo -m\n"\
                          "\tCreate a fifo command channel, and exit if it can't be made.\n"\
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
                          "\n"\
                          "--mimic-xpad\n"\
                          "\tMake the virtual output devices appear as xpad-style XBox 360 devices\n"\
                          "--daemon -d\n"\
                          "\tFork and exit immediately, leaving the daemon running in the background.\n"\
                          "--pidfile -P\n"\
                          "\tOnly used for daemon, where store the PID of the process.\n"\
                          ;

  std::cout << help_text;
  return 0;
}



int parse_opts(moltengamepad::mg_options& options, int argc, char* argv[]) {

  int c = 0;

  static struct option long_options[] = {
    {"help",          0,    0,  'h'},
    {"version",       0,    0,  'v'},
    {"uinput-path",   1,    0,  'u'},
    {"profiles-path", 1,    0,  'p'},
    {"gendev-path",   1,    0,  'g'},
    {"config-path",   1,    0,  'c'},
    {"num-gamepads",  1,    0,  'n'},
    {"make-fifo",     0,    0,  'm'},
    {"fifo-path",     1,    0,  'f'},
    {"pidfile",       1,    0,  'P'},
    {"no-make-keys",  0,    0,    0},
    {"no-enumerate",  0,    0,    0},
    {"no-monitor",    0,    0,    0},
    {"dpad-as-hat",   0,    0,    0},
    {"mimic-xpad",    0,    0,    0},
    {"daemon",        0,    0,  'd'},
    {0,               0,    0,    0},
  };
  int long_index;

  while (c != -1) {
    c = getopt_long(argc, argv, "u:p:g:n:c:f:P:mhvd", long_options, &long_index);
    switch (c) {
    case 0:
      if (long_index == 10) {
        options.make_keyboard = false;
      };
      if (long_index == 11) {
        options.look_for_devices = false;
      };
      if (long_index == 12) {
        options.listen_for_devices = false;
      };
      if (long_index == 13) {
        options.dpad_as_hat = true;
      };
      if (long_index == 14) {
        options.mimic_xpad = true;
      };
      break;
    case 'd':
      options.daemon = true;
      break;
    case 'u':
      options.uinput_path = std::string(optarg);
      break;
    case 'm':
      options.make_fifo = true;
      break;
    case 'f':
      options.fifo_path = std::string(optarg);
      break;
    case 'p':
      options.profile_dir = std::string(optarg);
      break;
    case 'P':
      options.pidfile = std::string(optarg);
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
    case 'v':
      print_version();
      return 10;
      break;
    case 'n':
      try {
        options.num_gamepads = std::stoi(optarg);
        if (options.num_gamepads < 0) throw - 3;
      } catch (...) {
        std::cerr << "could not parse numeric value for number of gamepads." << std::endl;
        return -5;

      }
      break;
    }

  }

  return 0;
}
