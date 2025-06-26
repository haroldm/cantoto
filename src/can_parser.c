#include <inttypes.h>
#include <stdbool.h>

#include "can_parser.h"
#include "state.h"

uint8_t is_timeout(struct msg_desc_t *desc) {
  if (desc->tick >= (2 * desc->period)) {

    desc->tick = 2 * desc->period;
    return 1;
  }

  return 0;
}

// TODO: add logic in handler_2c3 to keep radio on as long as the key is
// inserted.
//       It's probably better to check against bit 0b10000, not bit 0b1.
void handler_2c3(const uint8_t *msg, struct msg_desc_t *desc) {
  if (is_timeout(desc)) {

    carstate.acc = false;
    carstate.ign = false;
    return;
  }

  /*
     0001 0000 - 0x2C3 : 10 00 00 00 00 00 00 00 - no Key
     0000 0001 - 0x2C3 : 01 FF FF FF FF FF FF FF - Key inserted, IGN off
     0000 0111 - 0x2C3 : 07 FF FF FF FF FF FF FF - Ign on
     0111 1011 - 0x2C3 : 0B FF FF FF FF FF FF FF - Starter
  */

  if (msg[0] & 0x01)
    carstate.acc = true;
  else
    carstate.acc = false;

  if ((msg[0] & 0x08) == 0x08)
    carstate.ign = true;
  else
    carstate.ign = false;
}

void handler_635(const uint8_t *msg, struct msg_desc_t *desc) {
  if (is_timeout(desc)) {

    carstate.darkmode = false;
    return;
  }

  // If 0x06 <= msg[1] <= 0x64: car instruments in dark mode
  // If 0x0 <= msg[1] <= 5: car instruments in light mode

  if (msg[1] < 0x6) {
    carstate.darkmode = true;
  } else {
    carstate.darkmode = false;
  }
}

static const swc_action_t swc_actions[] = {
    {0x06, 20, "vol+"},           // vol up
    {0x07, 30, "vol-"},           // vol down
    {0xA7, 10, "vol push"},       // vol push
    {0x08, 40, "left side push"}, // left side push
                                  // {0x0B, 0xFF, "up"},       // left side up
    // {0x0C, 0xFF, "down"},     // left side down
    // {0x2A, 0xFF, "mic"},      // mic
    // {0x01, 0xFF, "mode"},     // left side mode
    {0x00, 0x7F, "zero"} // empty/reset
};

void handler_5c3(const uint8_t *msg, struct msg_desc_t *desc) {
  static uint8_t last_msg = 0;

  if (last_msg == msg[1])
    return;

  last_msg = msg[1];

  // struct i2c_t *i2c = hw_i2c_get();

  for (uint8_t i = 0; i < sizeof(swc_actions) / sizeof(swc_actions[0]); i++) {
    if (swc_actions[i].msg_code == msg[1]) {
      if (swc_actions[i].res_value != 0x7F) {
        carstate.swc_res_value = swc_actions[i].res_value;
        // hw_i2c_write(i2c->baddr, 0x2E, carstate.swc_res_value);
      }
      break;
    }
  }
}