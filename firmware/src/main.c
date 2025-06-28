#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

#include <inttypes.h>

#include "system/can.h"
#include "system/tick.h"
#include "system/usart.h"

#include "can_parser.h"
#include "clock.h"
#include "state.h"

#define ILL_PORT GPIOB
#define ILL_PIN GPIO1

static void gpio_setup(void) {
  gpio_set_mode(ILL_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,
                ILL_PIN);
}

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
  uint8_t ch = 0;
  if (!usart_read_ch(&ch))
    return;
  usart_send_char(&ch);
  if (ch == 'd') {
    car_enable_debug();
    usart_send_string("\r\ndebug mode enabled\r\n");
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

  usart_send_string("CANTOTO firmware\r\n");

  while (1) {
    // gpio_process();
    usart_process();
    if (timer.flag_5ms) {
      timer.flag_5ms = 0;
      in_process(can, 5, can_msgs, can_msgs_len);
    }
  }

  return 0;
}
