#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/can.h>
#include <libopencm3/stm32/f1/nvic.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/exti.h>

#include <stddef.h>

#include "hw/can.h"
#include "hw/gpio.h"

typedef struct speed_t
{
	uint32_t sjw;
	uint32_t ts1;
	uint32_t ts2;
	uint32_t brp;
} speed_t;

/* APB1 36 MHz 75% sjw=2 */
static speed_t speeds[e_speed_nums] = 
{
	{ CAN_BTR_SJW_2TQ, CAN_BTR_TS1_14TQ, CAN_BTR_TS2_5TQ, 18 },
	{ CAN_BTR_SJW_2TQ, CAN_BTR_TS1_13TQ, CAN_BTR_TS2_4TQ, 16 },
	{ CAN_BTR_SJW_2TQ, CAN_BTR_TS1_13TQ, CAN_BTR_TS2_4TQ, 8 },
	{ CAN_BTR_SJW_2TQ, CAN_BTR_TS1_13TQ, CAN_BTR_TS2_4TQ, 4 },
	{ CAN_BTR_SJW_2TQ, CAN_BTR_TS1_13TQ, CAN_BTR_TS2_4TQ, 2 },
};

#define MSGS_SIZE 80
typedef struct can_t
{
	uint32_t rcc;
	uint32_t baddr;
	uint8_t fid;

	uint32_t irq;

	struct gpio_t tx;
	struct gpio_t rx;
	struct gpio_t s;

	uint32_t nums;
	msg_can_t msgs[MSGS_SIZE];
	uint8_t msgs_size;
} can_t;

static struct can_t can1 =
{
	.rcc = RCC_CAN,
	.baddr = CAN1,
	.irq = NVIC_USB_LP_CAN_RX0_IRQ,
	.fid = 0,
	.tx = GPIO_INIT(A, 12),
	.rx = GPIO_INIT(A, 11),
	.s = GPIO_INIT(B, 5),

	.nums = 0,
	.msgs = { },
	.msgs_size = 0,
};

struct can_t * hw_can_get_mscan(void)
{
	return &can1;
}

uint8_t hw_can_set_speed(struct can_t * can, e_speed_t speed)
{
	nvic_disable_irq(can->irq);
	can_disable_irq(can->baddr, CAN_IER_FMPIE0);

	/* Reset CAN. */
	can_reset(can->baddr);

	/* CAN cell init. apb1 36 MHZ */
	int ret = can_init(can->baddr,
		     false,           /* TTCM: Time triggered comm mode? */
		     true,            /* ABOM: Automatic bus-off management? */
		     false,           /* AWUM: Automatic wakeup mode? */
		     false,           /* NART: No automatic retransmission? */
		     false,           /* RFLM: Receive FIFO locked mode? */
		     false,           /* TXFP: Transmit FIFO priority? */
		     speeds[speed].sjw,
		     speeds[speed].ts1,
		     speeds[speed].ts2,
		     speeds[speed].brp,
		     false,
		     false
		     );

	if (ret)
		return ret;

	/* CAN filter 0 init. */
	can_filter_id_mask_32bit_init(0,     /* Filter ID */
				0,     /* CAN ID */
				0,     /* CAN ID mask */
				0,     /* FIFO assignment (here: FIFO0) */
				true); /* Enable the filter. */

	/* Enable CAN RX interrupt. */
	can_enable_irq(can->baddr, CAN_IER_FMPIE0);
	nvic_enable_irq(can->irq);

	return 0;
}

enum e_can_types
{
	e_can_simple = 0x0,
	e_can_statistic = 0x1,
	e_can_odd = 0x2,
	e_can_ext = 0x40,
	e_can_rtr = 0x80,
};

uint8_t hw_can_setup(struct can_t * can, e_speed_t speed)
{
	/* Enable peripheral clocks. */
	rcc_periph_clock_enable(can->rcc);
	rcc_periph_clock_enable(can->rx.rcc);
	rcc_periph_clock_enable(can->tx.rcc);

	exti_disable_request(EXTI11);
	nvic_disable_irq(NVIC_EXTI15_10_IRQ);

	/* Configure CAN pin: RX (input pull-up). */
	gpio_set_mode(can->rx.port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, can->rx.pin);
	gpio_set(can->rx.port, can->rx.pin);

	/* Configure CAN pin: TX. */
	gpio_set_mode(can->tx.port, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, can->tx.pin);

	/* NVIC setup. */
	nvic_enable_irq(can->irq);
	nvic_set_priority(can->irq, 1);

	return hw_can_set_speed(can, speed);
}

void hw_can_disable(struct can_t * can)
{
	nvic_disable_irq(can->irq);

	rcc_periph_clock_enable(can->rcc);
	can_reset(can->baddr);
	rcc_periph_clock_disable(can->rcc);

	rcc_periph_clock_enable(can->rx.rcc);
    gpio_set_mode(&can->rx.port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, &can->rx.pin);
	rcc_periph_clock_disable(can->rx.rcc);

	rcc_periph_clock_enable(can->tx.rcc);
    gpio_set_mode(&can->tx.port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, &can->tx.pin);
	rcc_periph_clock_disable(can->tx.rcc);
}

uint8_t hw_can_get_msg_nums(can_t * can)
{
	return can->msgs_size;
}

uint32_t hw_can_get_pack_nums(struct can_t * can)
{
	return can->nums;
}

uint8_t hw_can_get_msg(struct can_t * can, struct msg_can_t * msg, uint8_t idx)
{
	if (idx >= can->msgs_size)
		return 0;

	*msg = can->msgs[idx];

	return 1;
}

uint32_t can_isr_cnt = 0;

static void can_isr(struct can_t * can)
{
	uint32_t fmi;
	struct msg_can_t msg;
	uint8_t i, j;
	uint32_t id = 0;

	can_isr_cnt++;
	bool rtr = 0, ext = 0;

	can_receive(can->baddr, 0, false, &id, &ext, &rtr, &fmi, &msg.len, msg.data, NULL);
	msg.id = id;

	msg.type = 0;
	if (rtr)
		msg.type |= e_can_rtr;
	if (ext)
		msg.type |= e_can_ext;

	can->nums++;

	uint8_t found = 0;
	for (i = 0; i < can->msgs_size; i++) {

		if (can->msgs[i].id == msg.id) {

			can->msgs[i].len = msg.len;
			for (j = 0; j < 8; j++)
				can->msgs[i].data[j] = msg.data[j];
			can->msgs[i].num++;
			found = 1;
			break;
		}
	}

	if (!found && can->msgs_size < MSGS_SIZE) {

		can->msgs[can->msgs_size] = msg;
		can->msgs[can->msgs_size].num = 1;
		can->msgs_size++;
	}

	can_fifo_release(can->baddr, 0);
}

void usb_lp_can_rx0_isr(void)
{
	can_isr(hw_can_get_mscan());
}

void hw_can_snd_msg(struct can_t * can, struct msg_can_t * msg)
{
	if (!can_available_mailbox(can->baddr)) {

		CAN_TSR(can->baddr) |= CAN_TSR_ABRQ0 | CAN_TSR_ABRQ1 | CAN_TSR_ABRQ2;
	}

	bool rtr = msg->type & e_can_rtr;
	bool ext = msg->type & e_can_ext;
	can_transmit(can->baddr, msg->id, ext, rtr, msg->len, msg->data);
}

void hw_can_clr(struct can_t * can)
{
	can->nums = 0;
	can->msgs_size = 0;
}

void hw_can_sleep(struct can_t * can)
{
	hw_can_disable(can);

	rcc_periph_clock_enable(can->rx.rcc);
	gpio_set_mode(can->rx.port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, can->rx.pin);
	//pull-up
	gpio_set(can->rx.port, can->rx.pin);
	rcc_periph_clock_disable(can->rx.rcc);

	exti_select_source(EXTI11, can->rx.port);
	exti_set_trigger(EXTI11, EXTI_TRIGGER_BOTH);
	exti_enable_request(EXTI11);
	//exti_reset_request(EXTI11);
	nvic_enable_irq(NVIC_EXTI15_10_IRQ);
}

void exti15_10_isr(void)
{
	exti_reset_request(EXTI11);
}

