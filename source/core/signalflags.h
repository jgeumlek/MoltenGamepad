#ifndef SIGNALFLAGS_H
#define SIGNALFLAGS_H

//global flags, set by an interrupt signal.\

//stop the current action
extern volatile bool STOP_WORKING;

//shut down everything.
//Whichever thread sets this to true is responsibility
//for shutting down everything and exiting the process.
extern volatile bool QUIT_APPLICATION;


#endif
