#include <string.h>
#include "input.h"

static const struct input_map def_inpmap[MAX_INPUTS] = {
	{INP_FWD, 'w', -1},
	{INP_BACK, 's', -1},
	{INP_LEFT, 'a', -1},
	{INP_RIGHT, 'd', -1},
	{INP_UP, '2', -1},
	{INP_DOWN, 'x', -1},
	{INP_FIRE, ' ', 0},
	{INP_FIRE2, '\t', 2},
	{INP_LROLL, 'q', -1},
	{INP_RROLL, 'e', -1}
};

struct input_map inpmap[MAX_INPUTS];

unsigned int inpstate;


void init_input(void)
{
	memcpy(inpmap, def_inpmap, sizeof inpmap);
	inpstate = 0;
}
