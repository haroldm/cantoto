#ifndef STATE_H
#define STATE_H

#include <inttypes.h>
#include <stdbool.h>

typedef struct car_state_t
{
	bool acc;
	bool ign;

	bool darkmode;

    uint8_t swc_res_value;
} car_state_t;

static car_state_t carstate = {
    .acc = false,
	.ign = false,
	.darkmode = false,
    .swc_res_value = 0x7f,
};

#endif