#include <libopencm3/stm32/gpio.h>

#include "gpio.h"
#include "state.h"

#define ACC_PORT GPIOB
#define ACC_PIN GPIO0
#define ILL_PORT GPIOB
#define ILL_PIN GPIO1

void gpio_setup(void) {
  gpio_set_mode(ACC_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,
                ACC_PIN);
  gpio_set_mode(ILL_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,
                ILL_PIN);
}

void gpio_process(void) {
  if (car_get_acc() || get_manual_on()) {
    gpio_set(ACC_PORT, ACC_PIN);
  } else {
    gpio_clear(ACC_PORT, ACC_PIN);
  }
  if (car_get_darkmode()) {
    gpio_set(ILL_PORT, ILL_PIN);
  } else {
    gpio_clear(ILL_PORT, ILL_PIN);
  }
}