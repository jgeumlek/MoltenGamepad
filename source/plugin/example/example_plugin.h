#pragma once
#include "../plugin.h"

extern device_plugin example_dev;

extern const event_decl example_events[];


class example_device {
public:
  example_device();
  ~example_device();
  int init(input_source* ref);

  input_source* ref;
  //let our init func call private methods by marking it as a friend.
  friend PLUGIN_INIT_FUNC(example)(plugin_api api);
  static device_methods methods;

  constexpr static const char* name_stem = "ex";
protected:
  void process(void*);
  int process_option(const char* opname, const MGField value);
};

class example_manager {
public:

  void init_profile();

  int init(device_manager* ref);

  int start();

  int accept_device(struct udev* udev, struct udev_device* dev);

  int process_option(const char* name, const MGField value);

  example_manager();

  ~example_manager();

  friend PLUGIN_INIT_FUNC(example)(plugin_api api);
  static manager_methods methods;
  constexpr static const char* name = "example";
private:
  device_manager* ref;
};

