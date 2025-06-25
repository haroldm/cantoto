#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>

#include "clock.h"

#define ILL_PORT     GPIOB
#define ILL_PIN      GPIO1

#define USART_GPIO   GPIOA
#define USART_TX_PIN GPIO9
#define USART_RX_PIN GPIO10

static void gpio_setup(void)
{
    gpio_set_mode(ILL_PORT, GPIO_MODE_OUTPUT_2_MHZ,
                  GPIO_CNF_OUTPUT_PUSHPULL, ILL_PIN);
}

static void usart_setup(void)
{
    usart_disable(USART1);

	gpio_set_mode(USART_GPIO, GPIO_MODE_OUTPUT_50_MHZ,
				  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, USART_TX_PIN);
	gpio_set_mode(USART_GPIO, GPIO_MODE_INPUT,
				  GPIO_CNF_INPUT_FLOAT, USART_RX_PIN);

    usart_set_baudrate(USART1, 115200);
    usart_set_databits(USART1, 8);
    usart_set_stopbits(USART1, USART_STOPBITS_1);
    usart_set_mode(USART1, USART_MODE_TX_RX); 
    usart_set_parity(USART1, USART_PARITY_NONE);
    usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);

	usart_enable(USART1);
}

static void usart_send_string(const char *str)
{
    while (*str) {
        usart_send_blocking(USART1, *str++);
    }
}

int main(void)
{
	clock_setup();
    gpio_setup();
    usart_setup();

    usart_send_string("Hello from STM32F1\r\n");

    while (1) {
		usart_send_string("Toggling\r\n");
        gpio_toggle(ILL_PORT, ILL_PIN);
        for (volatile int i = 0; i < 4000000; i++) {
            __asm__("nop");
        }
    }

    return 0;
}
