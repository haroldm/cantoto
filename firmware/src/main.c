#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/rcc.h>

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "system/can.h"
#include "system/gpio.h"
#include "system/i2c.h"
#include "system/tick.h"
#include "system/usart.h"

#include "can_parser.h"
#include "clock.h"
#include "state.h"

#define CMD_BUFFER_SIZE 64

static void in_process(struct can_t *can, uint8_t ticks,
                       struct msg_desc_t *msg_desc, uint8_t desc_num) {
  uint8_t msgs_num = hw_can_get_msg_nums(can);
  uint32_t all_packs = 0;

  for (uint8_t i = 0; i < msgs_num; i++) {

    struct msg_can_t msg;
    if (!hw_can_get_msg(can, &msg, i))
      continue;

    all_packs += msg.num;

    for (uint32_t j = 0; j < desc_num; j++) {

      struct msg_desc_t *desc = &msg_desc[j];

      // special purpose - any activity on the bus
      if (0 == desc->id) {

        if (desc->in_handler) {

          // last msg
          if (i == (msgs_num - 1)) {

            if (all_packs == desc->num)
              desc->tick += ticks;
            else
              desc->tick = 0;

            desc->num = all_packs;

            desc->in_handler(msg.data, desc);
          }
        }
      } else if (msg.id == desc->id) {

        if (desc->in_handler) {

          // no new packs, increase timeout
          if (msg.num == desc->num)
            desc->tick += ticks;
          else
            desc->tick = 0;

          desc->num = msg.num;

          desc->in_handler(msg.data, desc);
        }

        break;
      }
    }
  }
}

static void usart_process(void) {
  static uint8_t debugtmp = 25;
  static char cmd_buffer[CMD_BUFFER_SIZE];
  static uint8_t idx = 0;
  static bool last_was_cr = false;
  uint8_t ch = 0;

  if (!usart_read_ch(&ch))
    return;

  if (ch == '\n' && last_was_cr) {
    last_was_cr = false;
    return;
  }

  if (ch != '\n')
    usart_send_char(&ch);

  if (ch == '\r' || ch == '\n') {
    last_was_cr = (ch == '\r');

    if (idx == 0) {
      usart_send_string("\r\n> ");
      return;
    }

    cmd_buffer[idx] = '\0';

    if (strcmp(cmd_buffer, "d") == 0) {
      car_enable_debug();
      usart_send_string("\r\ndebug mode enabled");
    } else if (strcmp(cmd_buffer, "T") == 0) {
      toggle_manual_on();
      usart_send_string("\r\nToggled manual on");
    } else if (strcmp(cmd_buffer, "+") == 0) {
      set_simulating_media_key_press(true);
      process_key_press(0x06);
      usart_send_string("\r\nPressing vol+ for two seconds");
    } else if (strcmp(cmd_buffer, "-") == 0) {
      set_simulating_media_key_press(true);
      process_key_press(0x07);
      usart_send_string("\r\nPressing vol+ for two seconds");
    } else if (strcmp(cmd_buffer, "vol") == 0) {
      set_simulating_media_key_press(true);
      process_key_press(0xA7);
      usart_send_string("\r\nPressing vol button for two seconds");
    } else if (strcmp(cmd_buffer, "left") == 0) {
      set_simulating_media_key_press(true);
      process_key_press(0x08);
      usart_send_string("\r\nPressing left button for two seconds");
    } else if (strcmp(cmd_buffer, "l") == 0) {
      char tmp[80];
      snprintf(tmp, 80, "sending value %d for ~250ms", debugtmp);
      usart_send_string(tmp);
      set_simulating_media_key_press(true);
      hw_i2c_write(I2C1, 0x2F, debugtmp);
      debugtmp += 1;
      if (debugtmp >= 0x7f)
        debugtmp = 0;
    } else {
      usart_send_string("\r\nunknown command");
    }

    idx = 0;
    usart_send_string("\r\n> ");
    return;
  }

  if (idx < CMD_BUFFER_SIZE - 1) {
    cmd_buffer[idx++] = ch;
  } else {
    usart_send_string("\r\ncommand too long\r\n> ");
    idx = 0;
  }
}

static void reset_simulated_key_presses(void) {
  static uint8_t count = 0;
  if (get_simulating_media_key_press()) {
    if (count < 2) {
      count++;
    } else {
      count = 0;
      process_key_press(0);
      set_simulating_media_key_press(false);
    }
  }
}

int main(void) {
  clock_setup();
  gpio_setup();
  usart_setup();
  hw_systick_setup();

  struct can_t *can = hw_can_get_mscan();
  hw_can_setup(can, e_speed_100);
  can_parser_init();

  hw_i2c_setup();

  usart_send_string("\r\nCANTOTO firmware\r\n\r\n> ");

  while (1) {
    gpio_process();
    usart_process();
    if (timer.flag_5ms) {
      timer.flag_5ms = 0;
      in_process(can, 5, can_msgs, can_msgs_len);
    }
    if (timer.flag_250ms) {
      timer.flag_250ms = 0;
      reset_simulated_key_presses();
    }
  }

  return 0;
}
