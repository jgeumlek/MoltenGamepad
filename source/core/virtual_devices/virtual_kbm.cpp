#include "virtual_kbm.h"
#include "../devices/device.h"

virtual_keyboard::virtual_keyboard(std::string name, std::string descr, uinput_ids keyboard_ids, uinput_ids mouse_ids, slot_manager* slot_man, uinput* ui) : virtual_device(name, descr, slot_man) {
  kb_fd = ui->make_keyboard(keyboard_ids);
  if (kb_fd < 0) throw std::runtime_error("No uinput node available.");
  
  rel_mouse_fd = ui->make_mouse(mouse_ids);
  if (rel_mouse_fd < 0) throw std::runtime_error("Making uinput mouse failed.");
  
  this->kb_ids = keyboard_ids;
  this->rel_mouse_ids = mouse_ids;
  this->ui = ui;
}

virtual_keyboard::~virtual_keyboard() {
  close_virt_device();
}

void virtual_keyboard::take_event(struct input_event in) {
  //Relative events go to a separate mouse device.
  //SYN events should go to both!
  if (in.type == EV_REL || in.type == EV_SYN) {
    ssize_t res = write(rel_mouse_fd, &in, sizeof(in));
    if (res < 0)
      perror("write to virt rel mouse");
    if (in.type == EV_REL) return;
  }
  ssize_t res = write(kb_fd, &in, sizeof(in));
  if (res < 0)
      perror("write to virt keyboard");
};

void virtual_keyboard::destroy_uinput_devs() {
  if (!ui)
    return;
  if (kb_fd >= 0) {
    ui->uinput_destroy(kb_fd);
    kb_fd = -1;
  }
  if (rel_mouse_fd >= 0) {
    ui->uinput_destroy(rel_mouse_fd);
    rel_mouse_fd= -1;
  }

}
