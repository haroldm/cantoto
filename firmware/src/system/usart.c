#include <libopencm3/stm32/f1/nvic.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <string.h>

#include "ring.h"
#include "usart.h"

#define USART_GPIO GPIOA
#define USART_TX_PIN GPIO9
#define USART_RX_PIN GPIO10

struct usart_t {
  struct ring_t rx_ring;
  struct ring_t tx_ring;
  uint8_t rx_ring_buffer[32];
  uint8_t tx_ring_buffer[512];
};

static struct usart_t usart;

void usart_setup(void) {
  ring_init(&usart.tx_ring, usart.tx_ring_buffer, sizeof(usart.tx_ring_buffer));
  ring_init(&usart.rx_ring, usart.rx_ring_buffer, sizeof(usart.rx_ring_buffer));

  usart_disable(USART1);

  gpio_set_mode(USART_GPIO, GPIO_MODE_OUTPUT_50_MHZ,
                GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, USART_TX_PIN);
  gpio_set_mode(USART_GPIO, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,
                USART_RX_PIN);

  usart_set_baudrate(USART1, 115200);
  usart_set_databits(USART1, 8);
  usart_set_stopbits(USART1, USART_STOPBITS_1);
  usart_set_mode(USART1, USART_MODE_TX_RX);
  usart_set_parity(USART1, USART_PARITY_NONE);
  usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);

  usart_enable_rx_interrupt(USART1);
  nvic_enable_irq(NVIC_USART1_IRQ);
  USART1_CR1 |= USART_CR1_RXNEIE;

  usart_enable(USART1);
}

int usart_send_char(const uint8_t *ch) {
  int ret = ring_write(&usart.tx_ring, (uint8_t *)ch, 1);
  USART_CR1(USART1) |= USART_CR1_TXEIE;
  return ret;
}

int usart_send_string(const char *str) {
  int ret = ring_write(&usart.tx_ring, (uint8_t *)str, strlen(str));
  USART_CR1(USART1) |= USART_CR1_TXEIE;
  return ret;
}

// uint32_t usart_isr_cnt = 0;
void usart1_isr(void) {
  // usart_isr_cnt++;

  /* Check if we were called because of RXNE. */
  if (((USART1_CR1 & USART_CR1_RXNEIE) != 0) &&
      ((USART1_SR & USART_SR_RXNE) != 0)) {

    // usart->rx_cnt++;

    /* Retrieve the data from the peripheral. */
    ring_write_ch(&usart.rx_ring, usart_recv(USART1));
  }

  /* Check if we were called because of TXE. */
  if (((USART1_CR1 & USART_CR1_TXEIE) != 0) &&
      ((USART1_SR & USART_SR_TXE) != 0)) {

    uint8_t ch;
    if (!ring_read_ch(&usart.tx_ring, &ch)) {

      /* Disable the TXE interrupt, it's no longer needed. */
      USART1_CR1 &= ~USART_CR1_TXEIE;
    } else {

      // usart->tx_cnt++;

      /* Put data into the transmit register. */
      usart_send(USART1, ch);
    }
  }
}

uint8_t usart_read_ch(uint8_t *ch) { return ring_read_ch(&usart.rx_ring, ch); }
