#ifndef HW_GPIO_H
#define HW_GPIO_H

#include <inttypes.h>

struct gpio_t
{
	uint32_t rcc;
	uint32_t port;
	uint32_t pin;
};
#define GPIO_INIT(PORT,PIN) { RCC_GPIO##PORT, GPIO##PORT, GPIO##PIN }

#endif