#include "socket_out.h"
#include "../../../oscpkt/oscpkt.hh"
#include "../../devices/device.h"
#include "../../output_slot.h"
#include <unistd.h>

int socket_out::osc_msg(const std::string& path, int id, const std::string& argtypes, ...) {
  oscpkt::PacketWriter pw;
  oscpkt::Message msg;
  msg.init(path);
  msg.pushInt32(id);
  if (!argtypes.empty()) {
    va_list arglist;
    va_start(arglist, argtypes);
    int n = argtypes.size();
    for (int i = 0; i < n; i++) {
      if (argtypes[i] == 'i') {
        int arg = va_arg(arglist, int);
        msg.pushInt32(arg);
      }
      if (argtypes[i] == 's') {
        const char* arg = va_arg(arglist, const char*);
        if (!arg) {
          msg.pushStr("");
        } else {
          std::string str(arg);
          msg.pushStr(str);
        }
      }
    }
  }
  pw.init().addMessage(msg);
  uint32_t size = pw.packetSize();
  std::lock_guard<std::mutex> guard(write_lock);
  write(fd, &size, sizeof(size));
  write(fd, pw.packetData(), size);
}

void socket_out::text_message(int resp_id, const std::string& text) {

  osc_msg("/text",resp_id,"s",text.c_str());
}

void socket_out::err(int resp_id, const std::string& text, const std::string& path, int line_number) {
  osc_msg("/error",resp_id,"ssi",text.c_str(),path.c_str(),line_number);
}

std::string emptystr = "";

void socket_out::err(int resp_id, const std::string& text) {
  osc_msg("/error",resp_id,"ssi",text.c_str(),emptystr.c_str(),-1);
}

void socket_out::end_response(int resp_id, int ret_val) {
  osc_msg("/done",resp_id,"i",ret_val);
}

void socket_out::device_slot(int resp_id, input_source* device, output_slot* slot) {
  std::string slotname = "";
  if (slot) slotname = slot->name;
  osc_msg("/devslot",resp_id,"ss",device->get_name().c_str(),slotname.c_str());
}

void socket_out::device_plug(int resp_id, input_source* device, std::string action) {
  osc_msg("/devplug",resp_id,"ss",device->get_name().c_str(),action.c_str());
}
