#ifndef __I2C_H__
#define __I2C_H__

#include <inttypes.h>

#include "gpio.h"

struct i2c_t {
  uint32_t baddr;
  uint32_t rcc;
  uint32_t irq;
  struct gpio_t scl;
  struct gpio_t sda;
};

struct i2c_t *hw_i2c_get(void);

void hw_i2c_reset(uint32_t i2c);

void hw_i2c_setup(void);

int hw_i2c_write(uint32_t i2c, uint8_t addr, uint8_t data);
#endif // __I2C_H__
