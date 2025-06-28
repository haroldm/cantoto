#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/rcc.h>

#include "i2c.h"

#define HW_I2C_TIMEOUT 10000000

// Adjust I2C peripheral here (I2C1 used as example)
static struct i2c_t i2c2 = {
    .baddr = I2C1,
    .rcc = RCC_I2C1,
    .scl = {GPIOB, GPIO6},
    .sda = {GPIOB, GPIO7},
};

struct i2c_t *hw_i2c_get(void) { return &i2c2; }
void hw_i2c_reset(uint32_t i2c) {
  I2C_CR1(i2c) |= I2C_CR1_SWRST;
  I2C_CR1(i2c) &= ~I2C_CR1_SWRST;
}

void hw_i2c_setup(void) {

  rcc_periph_clock_enable(RCC_GPIOB);

  // Configure PB6 (SCL) and PB7 (SDA) as AF open-drain
  gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN,
                GPIO6);
  gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN,
                GPIO7);

  rcc_periph_clock_enable(RCC_I2C1);

  // Disable I2C1 before configuring
  i2c_peripheral_disable(I2C1);

  // Set APB1 clock frequency in MHz (usually 36MHz on STM32F1)
  i2c_set_clock_frequency(I2C1, 36);

  // Set standard mode (100kHz) timings
  i2c_set_standard_mode(I2C1);
  i2c_set_ccr(I2C1, 180);  // 36MHz / (2 * 100kHz)
  i2c_set_trise(I2C1, 37); // 36MHz + 1

  // Enable ACK (optional)
  i2c_enable_ack(I2C1);

  // Set own address (not used in master mode, but required)
  i2c_set_own_7bit_slave_address(I2C1, 0x00);

  // Enable I2C1
  i2c_peripheral_enable(I2C1);
}

static uint32_t dummy_reg;

static int hw_i2c_start(uint32_t i2c, uint8_t addr, uint8_t direction) {
  uint32_t timeout = HW_I2C_TIMEOUT;

  i2c_send_start(i2c);

  while (!(I2C_SR1(i2c) & I2C_SR1_SB))
    if (--timeout == 0)
      return -1;

  i2c_send_7bit_address(i2c, addr, direction);
  timeout = HW_I2C_TIMEOUT;
  while (!(I2C_SR1(i2c) & I2C_SR1_ADDR))
    if (--timeout == 0)
      return -2;

  dummy_reg = I2C_SR2(i2c); // clear ADDR
  return 0;
}

static int hw_i2c_write_byte(uint32_t i2c, uint8_t byte) {
  uint32_t timeout = HW_I2C_TIMEOUT;
  while (!(I2C_SR1(i2c) & I2C_SR1_TxE))
    if (--timeout == 0)
      return -1;

  i2c_send_data(i2c, byte);
  return 0;
}

static int hw_i2c_stop(uint32_t i2c) {
  uint32_t timeout = HW_I2C_TIMEOUT;
  while (!(I2C_SR1(i2c) & I2C_SR1_BTF))
    if (--timeout == 0)
      return -1;

  i2c_send_stop(i2c);
  return 0;
}

int hw_i2c_write(uint32_t i2c, uint8_t addr, uint8_t data) {
  if (hw_i2c_start(i2c, addr, I2C_WRITE))
    return -1;
  if (hw_i2c_write_byte(i2c, data))
    return -3;
  if (hw_i2c_stop(i2c))
    return -4;
  return 0;
}
