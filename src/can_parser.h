#ifndef CAN_PARSER_H
#define CAN_PARSER_H

#include <inttypes.h>

struct msg_desc_t;
typedef struct msg_desc_t {
  uint32_t id;
  uint16_t period;
  uint16_t tick;
  uint32_t num;
  void (*in_handler)(const uint8_t *msg, struct msg_desc_t *desc);
} msg_desc_t;

extern struct msg_desc_t can_msgs[];
extern const uint8_t can_msgs_len;

uint8_t is_timeout(struct msg_desc_t *desc);
void can_parser_init(void);

void handler_2c3(const uint8_t *msg, struct msg_desc_t *desc);
void handler_635(const uint8_t *msg, struct msg_desc_t *desc);
void handler_5c3(const uint8_t *msg, struct msg_desc_t *desc);

#endif
