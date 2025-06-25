#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>

#include <inttypes.h>

#include "hw/can.h"
#include "hw/tick.h"

#include "can_parser.h"
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

static void in_process(struct can_t * can, uint8_t ticks, struct msg_desc_t * msg_desc, uint8_t desc_num)
{
	uint8_t msgs_num = hw_can_get_msg_nums(can);
	uint32_t all_packs = 0;

	for (uint8_t i = 0; i < msgs_num; i++) {

		struct msg_can_t msg;
		if (!hw_can_get_msg(can, &msg, i))
			continue;

		all_packs += msg.num;

		for (uint32_t j = 0; j < desc_num; j++) {

			struct msg_desc_t * desc = &msg_desc[j];

			//special purpose - any activity on the bus
			if (0 == desc->id) {

				if (desc->in_handler) {

					//last msg
					if (i == (msgs_num - 1)) {

						if (all_packs == desc->num)
							desc->tick += ticks;
						else
							desc->tick = 0;

						desc->num = all_packs;

						desc->in_handler(msg.data, desc);
					}
				}
			}
			else if (msg.id == desc->id) {

				if (desc->in_handler) {

					//no new packs, increase timeout
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

int main(void)
{
	clock_setup();
    gpio_setup();
    usart_setup();

    struct can_t* can = hw_can_get_mscan();

    hw_can_setup(can, e_speed_100);

    usart_send_string("CANTOTO firmware\r\n");

    while (1) {
        if (timer.flag_5ms) {
			timer.flag_5ms = 0;
            in_process(can, 5, can_msgs, sizeof(can_msgs)/sizeof(can_msgs[0]));
		}
    }

    return 0;
}
