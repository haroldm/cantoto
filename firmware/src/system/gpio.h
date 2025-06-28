#ifndef __GPIO_H__
#define __GPIO_H__

#include <inttypes.h>

struct gpio_t {
  uint32_t rcc;
  uint32_t port;
  uint32_t pin;
};
#define GPIO_INIT(PORT, PIN) {RCC_GPIO##PORT, GPIO##PORT, GPIO##PIN}

void gpio_setup(void);
void gpio_process(void);

#endif // __GPIO_H__
