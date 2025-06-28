#include <libopencm3/stm32/i2c.h>

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#include "system/i2c.h"
#include "system/usart.h"

#include "can_parser.h"
#include "state.h"

struct msg_desc_t can_msgs[] = {
    {0x2c3, 100, 0, 0, handler_2c3}, // Accessory and ignition
    {0x635, 100, 0, 0, handler_635}, // Illumination
    {0x5c3, 500, 0, 0, handler_5c3}, // Steering wheel control (media keys)
};
const uint8_t can_msgs_len = sizeof(can_msgs) / sizeof(can_msgs[0]);

uint8_t is_timeout(struct msg_desc_t *desc) {
  if (desc->tick >= (2 * desc->period)) {

    desc->tick = 2 * desc->period;
    return 1;
  }

  return 0;
}

static car_state_t *carstate;

void can_parser_init() { carstate = car_get_state(); }

static uint8_t prev_2c3_val = 0;
void handler_2c3(const uint8_t *msg, struct msg_desc_t *desc) {
  if (is_timeout(desc)) {
    if (debug_accessory_events() && carstate->can_id_2c3_timeout == false)
      usart_send_string("CAN ID 2c3 timeout\r\n");
    carstate->can_id_2c3_timeout = true;
    // Timeout will happen ~10 seconds after key is removed
    // TODO: remove the following line and set ignition_happened to false when
    // driver's door is closed or car is locked.
    carstate->ignition_happened = false;
    carstate->acc = false;
    carstate->ign = false;
    return;
  }

  if (debug_accessory_events() && prev_2c3_val != msg[0]) {
    char tmp[80];
    snprintf(tmp, sizeof(tmp), "0b%c%c%c%c %c%c%c%c\r\n",
             (msg[0] & 0x80) ? '1' : '0', (msg[0] & 0x40) ? '1' : '0',
             (msg[0] & 0x20) ? '1' : '0', (msg[0] & 0x10) ? '1' : '0',
             (msg[0] & 0x08) ? '1' : '0', (msg[0] & 0x04) ? '1' : '0',
             (msg[0] & 0x02) ? '1' : '0', (msg[0] & 0x01) ? '1' : '0');
    usart_send_string(tmp);
    prev_2c3_val = msg[0];
  }
  /*
     0001 0000 - 0x2C3 : 10 00 00 00 00 00 00 00 - no Key
     0000 0001 - 0x2C3 : 01 FF FF FF FF FF FF FF - Key inserted, IGN off
     0000 0111 - 0x2C3 : 07 FF FF FF FF FF FF FF - Ign on
     0000 1011 - 0x2C3 : 0B FF FF FF FF FF FF FF - Starter
  */

  if (carstate->ignition_happened && (msg[0] & 0x01))
    carstate->acc = true;
  else
    carstate->acc = false;

  if ((msg[0] & 0x08) == 0x08) {
    carstate->ign = true;
    carstate->ignition_happened = true;
  } else {
    carstate->ign = false;
  }
}

void handler_635(const uint8_t *msg, struct msg_desc_t *desc) {
  if (is_timeout(desc)) {

    carstate->darkmode = false;
    return;
  }

  // If 0x06 <= msg[1] <= 0x64: car instruments in light mode
  // If 0x0 <= msg[1] <= 5: car instruments in dark mode

  if (msg[1] < 0x6) {
    carstate->darkmode = false;
  } else {
    carstate->darkmode = true;
  }
}

typedef struct {
  uint8_t msg_code;
  uint8_t res_value;     // resistance value to set; 0xFF if no change
  const char *debug_msg; // optional debug string
} swc_action_t;

void handler_5c3(const uint8_t *msg, struct msg_desc_t *desc) {
  static uint8_t last_msg = 0;
  (void)desc;

  // Don't process empty messages if simulating key presses over USART
  if (get_simulating_media_key_press()) {
    return;
  }

  if (last_msg == msg[1])
    return;

  last_msg = msg[1];
  process_key_press(msg[1]);
}

static const swc_action_t swc_actions[] = {
    {0x06, 29, "vol+\r\n"},           // vol up
    {0x07, 38, "vol-\r\n"},           // vol down
    {0xA7, 9, "vol push\r\n"},        // vol push
    {0x08, 17, "left side push\r\n"}, // left side push
    // {0x0B, 0xFF, "up"},       // left side up
    // {0x0C, 0xFF, "down"},     // left side down
    // {0x2A, 0xFF, "mic"},      // mic
    // {0x01, 0xFF, "mode"},     // left side mode
    // 16-18 = next
    // 20-22 = prev
    {0x00, 0x7f, "zero\r\n"} // empty/reset
};

void process_key_press(uint8_t id) {
  for (uint8_t i = 0; i < sizeof(swc_actions) / sizeof(swc_actions[0]); i++) {
    if (swc_actions[i].msg_code == id) {
      if (debug_media_keys())
        usart_send_string(swc_actions[i].debug_msg);

      carstate->swc_res_value = swc_actions[i].res_value;
      hw_i2c_write(I2C1, 0x2F, carstate->swc_res_value);
      break;
    }
  }
}