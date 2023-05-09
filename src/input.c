/*
Deep Runner - 6dof shooter game for the SGI O2.
Copyright (C) 2023  John Tsiombikas <nuclear@mutantstargoat.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
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
