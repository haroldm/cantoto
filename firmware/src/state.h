#ifndef __STATE_H__
#define __STATE_H__

#include <inttypes.h>
#include <stdbool.h>

typedef struct car_state_t {
  bool acc;
  bool ign;
  bool ignition_happened;
  bool darkmode;

  bool can_id_2c3_timeout;

  uint8_t swc_res_value;
} car_state_t;

typedef struct debug_t {
  bool log_car_media_keys;
  bool log_illumination_events;
  bool log_accessory_events;
  bool manual_on;
  bool simulating_media_key_press;
} debug_t;

car_state_t *car_get_state(void);
debug_t *car_get_debug(void);
void car_enable_debug(void);
bool car_get_acc(void);
bool car_get_darkmode(void);

bool debug_media_keys(void);
bool debug_illumination_events(void);
bool debug_accessory_events(void);
bool toggle_manual_on(void);
bool get_manual_on(void);
bool get_simulating_media_key_press(void);
void set_simulating_media_key_press(bool val);

#endif // __STATE_H__