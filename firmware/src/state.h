#ifndef __STATE_H__
#define __STATE_H__

#include <inttypes.h>
#include <stdbool.h>

typedef struct car_state_t {
  bool acc;
  bool ign;

  bool darkmode;

  uint8_t swc_res_value;
} car_state_t;

typedef struct debug_t {
  bool log_car_media_keys;
  bool log_illumination_events;
  bool log_accessory_events;
} debug_t;

car_state_t *car_get_state(void);
debug_t *car_get_debug(void);
void car_enable_debug(void);
bool debug_media_keys(void);
bool debug_illumination_events(void);
bool debug_accessory_events(void);

#endif // __STATE_H__