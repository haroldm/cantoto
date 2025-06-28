#include "state.h"

static car_state_t carstate = {
    .acc = false,
    .ign = false,
    .ignition_happened = false,
    .can_id_2c3_timeout = false,
    .darkmode = false,
    .swc_res_value = 0x7f,
};

static debug_t debug = {
    .log_car_media_keys = false,
    .log_illumination_events = false,
    .log_accessory_events = false,
    .manual_on = false,
    .simulating_media_key_press = false,
};

car_state_t *car_get_state() { return &carstate; }

debug_t *car_get_debug() { return &debug; }

void car_enable_debug(void) {
  debug.log_car_media_keys = true;
  debug.log_illumination_events = true;
  debug.log_accessory_events = true;
}
bool car_get_acc(void) { return carstate.acc; }
bool car_get_darkmode(void) { return carstate.darkmode; }

bool debug_media_keys(void) { return debug.log_car_media_keys; }
bool debug_illumination_events(void) { return debug.log_illumination_events; }
bool debug_accessory_events(void) { return debug.log_accessory_events; }
bool toggle_manual_on(void) {
  debug.manual_on = !debug.manual_on;
  return debug.manual_on;
}
bool get_manual_on(void) { return debug.manual_on; }
bool get_simulating_media_key_press() {
  return debug.simulating_media_key_press;
}
void set_simulating_media_key_press(bool val) {
  debug.simulating_media_key_press = val;
}