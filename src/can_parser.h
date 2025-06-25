#ifndef CAN_PARSER_H
#define CAN_PARSER_H

#include <inttypes.h>

struct msg_desc_t;
typedef struct msg_desc_t
{
	uint32_t id;
	uint16_t period;
	uint16_t tick;
	uint32_t num;
	void (*in_handler)(const uint8_t * msg, struct msg_desc_t * desc);
} msg_desc_t;

typedef struct {
    uint8_t msg_code;
    uint8_t res_value;      // resistance value to set; 0xFF if no change
    const char *debug_msg;  // optional debug string
} swc_action_t;

uint8_t is_timeout(struct msg_desc_t * desc);
void handler_2c3(const uint8_t * msg, struct msg_desc_t * desc);
void handler_635(const uint8_t * msg, struct msg_desc_t * desc);
void handler_5c3(const uint8_t * msg, struct msg_desc_t * desc);

static struct msg_desc_t can_msgs[] =
{
	{ 0x2c3,  100, 0, 0, handler_2c3 }, // Accessory and ignition
	{ 0x635,  100, 0, 0, handler_635 }, // Illumination
	{ 0x5c3,  500, 0, 0, handler_5c3 }, // Steering wheel control (media keys)
};


#endif
