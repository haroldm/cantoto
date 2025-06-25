#include <libopencm3/stm32/rcc.h>

#include "clock.h"

void clock_setup(void)
{
	rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE16_72MHZ]);
	rcc_periph_clock_enable(RCC_AFIO);
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_USART1);
}
