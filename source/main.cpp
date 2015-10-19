#include <iostream>
#include "moltengamepad.h"

int shell_loop(moltengamepad* mg, std::istream &in);

int main(int argc, char* argv[]) {
  try {

   moltengamepad mg;

   mg.init();
   
   shell_loop(&mg, std::cin);

  } catch (int e) {
    return e;
  }
  
  return 0;
}
