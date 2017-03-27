#include "debug_messages.h"
#include <cstdarg>
#include <iostream>

int DEBUG_LEVELS[] = {DEBUG_NONE, DEBUG_INFO, DEBUG_VERBOSE, -1};
int* DEBUG_LEVEL = &DEBUG_LEVELS[0];

void debug_print(int level, int num_args...) {
  if (level > *DEBUG_LEVEL)
    return;
  va_list list;
  va_start(list,num_args);
  for (int i = 0; i < num_args; i++) {
    const char* text = va_arg(list, const char*);
    std::cerr << text;
  }
  va_end(list);
  std::cerr << std::endl;
}
