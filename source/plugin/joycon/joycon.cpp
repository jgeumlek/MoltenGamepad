#include "joycon.h"

#define BTN DEV_KEY
#define ABS DEV_AXIS
#define OPT DEV_OPTION

device_methods joycon::methods;

//static array of event declarations, loaded by example_manager init_profile();
//{"name","description",type,"default mapping"}
const event_decl joycon_events[] = {
  {"a","A Button (or rightmost)",BTN,"first"},
  {"b","B Button (or bottom)",BTN,"second"},
  {"x","X Button (or top)",BTN,"third"},
  {"y","Y Button (or leftmost)",BTN,"fourth"},
  {"up","Up Button",BTN,"up"},
  {"down","Down Button",BTN,"down"},
  {"left","Left Button",BTN,"left"},
  {"right","Right Button",BTN,"right"},
  {"plus","Plus Button",BTN,"start"},
  {"minus","Minus Button",BTN,"select"},
  {"capture","Capture Button",BTN,"mode"},
  {"home","Home Button",BTN,"home"},
  {"l","L Button",BTN,"tl"},
  {"zl","ZL Button",BTN,"tl2"},
  {"r","R Button",BTN,"tr"},
  {"zr","ZR Button",BTN,"tr2"},
  {"thumbl","Left stick click",BTN,"thumbl"},
  {"thumbr","Right stick click",BTN,"thumbr"},
  {"l_sl","Left JoyCon SL Button",BTN,""},
  {"l_sr","Left JoyCon SR Button",BTN,""},
  {"r_sl","Right JoyCon SL Button",BTN,""},
  {"r_sr","Right JoyCon SR Button",BTN,""},
  {"left_x","Left JoyCon Stick X Axis",ABS,""},
  {"left_y","Left JoyCon Stick Y Axis",ABS,""},
  {"right_x","Right JoyCon Stick X Axis",ABS,""},
  {"right_y","Right JoyCon Stick Y Axis",ABS,""},
  //Solo JoyCon: use directions to label the buttons... a rotated set of primary actions
  {"solo_up","Solo JoyCon Up Button",BTN,"third"},
  {"solo_down","Solo JoyCon Down Button",BTN,"second"},
  {"solo_left","Solo JoyCon Left Button",BTN,"fourth"},
  {"solo_right","Solo JoyCon Right Button",BTN,"first"},
  {"solo_sl","Solo JoyCon SL Button",BTN,"tl"},
  {"solo_sr","Solo JoyCon SR Button",BTN,"tr"},
  {"solo_capplus","Solo JoyCon Capture Button/Minus Button",BTN,"start"},
  {"solo_homeminus","Solo JoyCon Home Button/Plus Button",BTN,"select"},
  {"solo_lr","Solo JoyCon L Button/R Button",BTN,""},
  {"solo_zlzr","Solo JoyCon ZL Button/ZR Button",BTN,""}, 
  {"solo_thumbl","Solo JoyCon Stick Click",BTN,"thumbl"},
  {"solo_x","Solo JoyCon Stick X Axis",ABS,"left_x"},
  {"solo_y","Solo JoyCon Stick Y Axis",ABS,"left_y"},
  {nullptr, nullptr, NO_ENTRY, nullptr}
};

joycon::joycon(int fd1, int fd2, JoyConSide side1, JoyConSide side2, const char* syspath1, const char* syspath2, joycon_manager* manager) : manager(manager) {
  //try to enforce that single joycon should take the first index.
  if (fd2 >= 0 && fd1 < 0) {
    fd1 = fd2;
    fd2 = -1;
  }
  if (side2 != UNKNOWN_JOYCON && side1 == UNKNOWN_JOYCON) {
    side1 = side2;
    side2 = UNKNOWN_JOYCON;
  }
    
  fds[0] = fd1;
  fds[1] = fd2;
  sides[0] = side1;
  sides[1] = side1;
  if (syspath1)
    path[0] = std::string(syspath1);
  if (syspath2 && !syspath1)
    path[0] = std::string(syspath2);
  else if (syspath2)
    path[1] = std::string(syspath2);
  if (fds[0] >= 0 && fds[1] >= 0) {
    activated = true;
    mode = PARTNERED;
  }
}

joycon::~joycon() {
  if (close_out_fd[0] && fds[0] > 0)
    close(fds[0]);
  if (close_out_fd[1] && fds[1] > 0)
    close(fds[1]);
}

int joycon::init(input_source* ref) {
  this->ref = ref;
  if (fds[0] > 0)
    methods.watch_file(ref, fds[0], (void*)0);
  if (fds[1] > 0)
    methods.watch_file(ref, fds[1], (void*)1);
  methods.request_recurring_events(ref,true);
  process_recurring_event();
  if (!activated)
    methods.print(ref,"controller inactive. Press SL+SR for a solo JoyCon, or ZL+ZR for a JoyCon Partnership.");
  return 0;
}

void joycon::process(void* tag) {
  intptr_t index = (intptr_t)(tag);
  if (index != 0 && index != 1)
    return;
  int res = read(fds[index], report[index], 0x31);
  if (res < 0 && errno == EAGAIN) {
    methods.print(ref, "EAGAIN");
  } else if (res < 0 && errno == EINPROGRESS) {
    methods.print(ref, "EINPROGRESS");
  } else if (res < 0) {
    //error... Just shut out device...
    methods.print(ref,"Communication error");
    close(fds[index]);
    fds[index] = -1;
  } else {
    if (res > 0)
      read_report(index);
  }
  methods.send_syn_report(ref);
}

void joycon::process_recurring_event() {
  //called every 10ms or so
  //send_request = true;
  //send packet to joycon to ask for its state.
  if (--sent_cycle < 0) {
    //we proceed here every 8th call... about 80ms between polls.
    //The controller seems to disconnect if we send requests too often.
    //Perhaps we are doing something wrong/race condition?
    sent_cycle = 7;
    //If we are representing a joycon partnership, we have two file descriptors to loop over.
    for (int i = 0; i < 2; i++) {

      //send only if the file is open and we aren't currently waiting for a reply.
      if (fds[i] > 0 && pending_reports[i] == 0) {
        int res = write(fds[i],request_state,2);
        if (res < 0) {
          std::string msg = "comm. err: " + std::string(strerror(errno));
          methods.print(ref,msg.c_str());
          close(fds[i]);
          fds[i] = -1;
        } else {
          pending_reports[i]++;
        }
      }
    }
  }
}

const char* joycon::get_description() const {
  bool has_left = (sides[0] == LEFT_JOYCON || sides[1] == LEFT_JOYCON);
  bool has_right = (sides[0] == RIGHT_JOYCON || sides[1] == RIGHT_JOYCON);
  if (has_left && has_right && activated)
    return "Paired Left/Right Joy-Con";
  //When we allow breaking a pair, save some headaches and allocate separate them then.
  if (has_left && has_right && !activated)
    return "Pending but Paired? This is an error.";
  if (has_left && activated)
    return "Left Joy-Con";
  if (has_left && !activated)
    return "Pending Left Joy-Con";
  if (has_right && activated)
    return "Right Joy-Con";
  if (has_right && !activated)
    return "Pending Right Joy-Con";

  return "Unknown Joy-Con";
};

int joycon::process_option(const char* opname, const MGField value) {
  return -1;
}

#define READ_BUTTON(byte, bit_index) \
  ((byte & (1 << bit_index)) != 0 )


#define JC_STICK_SCALE ABS_RANGE/60


void joycon::read_report(int index) {
  uint8_t* buffer = report[index];
  //We can get button data from 0x3F reports or 0x21 reports.

  //To simplify the final output, we condense the events across the left/right sides.
  //e.g. one "homeminus" event, as a JoyCon has either home or minus, but not both.
  //we also try to parse both of these reports, but their bit layouts are quite different.

  
  bool face_btns[4] = {false, false, false, false}; //face btns are down right left up from the perspective of a solo joycon.
  bool sl,sr,capplus,homeminus,stickclick,lr,zlzr;
  
  if (buffer[0] == 0x3F) {
    //This is a report that contains button data, but the stick data is not usable.
    //It is sent by the controller automatically when an event occurs.
    //The face buttons are in the same order whether on a left or right joycon, so
    //this is what motivates our down right left up ordering.
    for (int i = 0; i < 4; i++) {
      face_btns[i] = READ_BUTTON(buffer[1], i);
    }
    sl =  READ_BUTTON(buffer[1], 4);
    sr =  READ_BUTTON(buffer[1], 5);

    //some events are still separate across left vs. right joycon. Might as well condense them.
    capplus = (buffer[2] & ((1 << 1) | (1 << 5))) != 0;
    homeminus = (buffer[2] & ((1 << 0) | (1 << 4))) != 0;
    stickclick = (buffer[2] & ((1 << 2) | (1 << 3))) != 0;
    lr =  READ_BUTTON(buffer[2], 6);
    zlzr =  READ_BUTTON(buffer[2], 7);

  } else if (buffer[0] == 0x21) {
    //Received an 0x21 report. This is a reply to our polling request, and it holds both
    //button data and usable analog stick values.
    if (pending_reports[index] > 0)
        pending_reports[index]--;

    //horz/vert are again from the perspective of a solo Joycon.
    //The signs/directions will be fixed later.
    //we | buffer bytes together to catch either left joycon or right joycon values at the same time.
    int horz = buffer[11] | buffer[8];
    //annoyingly, this axis is made up of two nibbles from adjacent bytes?
    int vert = (((buffer[10] | buffer[7]) & 0x0F) << 4) | (((buffer[9] | buffer[6]) & 0xF0) >> 4);

    horz -= 127;
    vert -= 127;

    uint8_t button_data = buffer[3] | buffer[5];
    sr = READ_BUTTON(button_data,4);
    sl = READ_BUTTON(button_data,5);
    lr = READ_BUTTON(button_data,6);
    zlzr = READ_BUTTON(button_data,7);

    stickclick = (buffer[4] & ((1 << 2) | (1 << 3))) != 0;
    capplus = (buffer[4] & ((1 << 1) | (1 << 5))) != 0;
    homeminus = (buffer[4] & ((1 << 4) | (1 << 0))) != 0;

    //still need to do some side-specific/mode-specific logic:
    // - fix the ordering of the face_btns
    // - handle outputting the correct orientation of the stick values.
    //only do this extra work if activated, though.
    if (activated) {
      if (sides[index] == RIGHT_JOYCON) {
        //first and fourth don't match the desired ordering.
        face_btns[0] = READ_BUTTON(button_data,3);
        face_btns[1] = READ_BUTTON(button_data,1);
        face_btns[2] = READ_BUTTON(button_data,2);
        face_btns[3] = READ_BUTTON(button_data,0);
        if (mode == PARTNERED) {
          take_joycon_event(right_x, vert*JC_STICK_SCALE);
          take_joycon_event(right_y, -horz*JC_STICK_SCALE);
        } else {
          //Solo
          take_joycon_event(solo_y, vert*JC_STICK_SCALE);
          take_joycon_event(solo_x, horz*JC_STICK_SCALE);
        }
      } else if (sides[index] == LEFT_JOYCON) {
        //face is down right left up, but our button_data is right left up down
        face_btns[0] = READ_BUTTON(button_data,3);
        face_btns[1] = READ_BUTTON(button_data,0);
        face_btns[2] = READ_BUTTON(button_data,1);
        face_btns[3] = READ_BUTTON(button_data,2);
        if (mode == PARTNERED) {
          take_joycon_event(left_x, vert*JC_STICK_SCALE);
          take_joycon_event(left_y, -horz*JC_STICK_SCALE);
        } else {
          //solo mode
          take_joycon_event(solo_y, -vert*JC_STICK_SCALE);
          take_joycon_event(solo_x, -horz*JC_STICK_SCALE);
        }
      }
    }
  } else {
    //unknown packet
  }

  //finally, we have parsed the report regardless of which type it was.
  //Time to send out events, which again depends on our mode/side.
  if (!activated) {
    //use the button data to check whether we can activate this joycon.
    active_trigger[index] = zlzr || lr;
    active_solo_btns[index] = sl && sr;
    manager->check_partnership(this);
  } else {
    if (mode == SOLO) {
      take_joycon_event(solo_down,face_btns[0]);
      take_joycon_event(solo_right,face_btns[1]);
      take_joycon_event(solo_left,face_btns[2]);
      take_joycon_event(solo_up,face_btns[3]);
      take_joycon_event(solo_sr,sr);
      take_joycon_event(solo_sl,sl);
      take_joycon_event(solo_thumbl,stickclick);
      take_joycon_event(solo_capplus,capplus);
      take_joycon_event(solo_homeminus,homeminus);
      take_joycon_event(solo_lr,lr);
      take_joycon_event(solo_zlzr,zlzr);
    } else if (mode == PARTNERED && sides[index] == LEFT_JOYCON) {
      take_joycon_event(left,face_btns[0]);
      take_joycon_event(down,face_btns[1]);
      take_joycon_event(up,face_btns[2]);
      take_joycon_event(right,face_btns[3]);
      take_joycon_event(l_sr,sr);
      take_joycon_event(l_sl,sl);
      take_joycon_event(thumbl,stickclick);
      take_joycon_event(capture,capplus);
      take_joycon_event(minus,homeminus);
      take_joycon_event(l,lr);
      take_joycon_event(zl,zlzr);
    } else if (mode == PARTNERED && sides[index] == RIGHT_JOYCON) {
      take_joycon_event(a,face_btns[0]);
      take_joycon_event(x,face_btns[1]);
      take_joycon_event(b,face_btns[2]);
      take_joycon_event(y,face_btns[3]);
      take_joycon_event(r_sr,sr);
      take_joycon_event(r_sl,sl);
      take_joycon_event(thumbr,stickclick);
      take_joycon_event(plus,capplus);
      take_joycon_event(home,homeminus);
      take_joycon_event(r,lr);
      take_joycon_event(zr,zlzr);
    }
    methods.send_syn_report(ref);
  }
}

void joycon::take_joycon_event(int id, uint64_t value) {
  methods.send_value(ref,id,value);
}
