#include "state.h"

static car_state_t carstate = {
    .acc = false,
    .ign = false,
    .darkmode = false,
    .swc_res_value = 0x7f,
};

static debug_t debug = {
    .log_car_media_keys = false,
    .log_illumination_events = false,
    .log_accessory_events = false,
};

car_state_t *car_get_state() { return &carstate; }

debug_t *car_get_debug() { return &debug; }

void car_enable_debug(void) {
  debug.log_car_media_keys = true;
  debug.log_illumination_events = true;
  debug.log_accessory_events = true;
}

bool debug_media_keys(void) { return debug.log_car_media_keys; }
bool debug_illumination_events(void) { return debug.log_illumination_events; }
bool debug_accessory_events(void) { return debug.log_accessory_events; }