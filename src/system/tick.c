#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>

#include "tick.h"

volatile tick_t timer = {0, 0, 0, 0, 0, 0, 0};

void hw_systick_callback(void) {
  static uint16_t div_1000ms = 0;
  static uint16_t div_250ms = 0;
  static uint16_t div_100ms = 0;
  static uint16_t div_5ms = 0;

  timer.flag_tick = 1;
  timer.msec++;

  if (++div_1000ms >= SEC_TO_TICK(1)) {

    div_1000ms = 0;
    timer.flag_1000ms = 1;
    timer.sec++;
    timer.msec = 0;
  }

  if (++div_250ms >= MSEC_TO_TICK(250)) {

    div_250ms = 0;
    timer.flag_250ms = 1;
  }

  if (++div_100ms >= MSEC_TO_TICK(100)) {

    div_100ms = 0;
    timer.flag_100ms = 1;
  }

  if (++div_5ms >= MSEC_TO_TICK(5)) {

    div_5ms = 0;
    timer.flag_5ms = 1;
  }
}

void hw_systick_disable(void) {
  systick_interrupt_disable();
  systick_counter_disable();
}

void hw_systick_setup(void) {
  /* 72MHz / 8 => 9000000 counts per second */
  systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8);
  /* clear counter so it starts right away */
  STK_CVR = 0;

  systick_set_reload(9000000 / TICK_HZ);

  systick_interrupt_enable();

  /* Start counting. */
  systick_counter_enable();
}

void sys_tick_handler(void) { hw_systick_callback(); }
