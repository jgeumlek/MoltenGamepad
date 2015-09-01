#include <iostream>
#include "moltengamepad.h"

int main(int argc, char* argv[]) {
  try {

   moltengamepad mg;

   mg.init();

  } catch (int e) {
    return e;
  }
  
  return 0;
}
