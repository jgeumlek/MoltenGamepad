#pragma once

#define DEBUG_NONE 0
#define DEBUG_INFO 5
#define DEBUG_VERBOSE 10
extern int DEBUG_LEVELS[];
extern int* DEBUG_LEVEL;
//pass the number of args, and then that number of strings aftwerwards to print.
//No newline is necessary.
void debug_print(int level, int num_args...);


