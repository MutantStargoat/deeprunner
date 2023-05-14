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
#include <stdio.h>
#include "gaw/gaw.h"
#include "loading.h"
#include "game.h"
#include "gfxutil.h"

static int steps, max_steps;

void loading_start(int nitems)
{
	steps = 0;
	max_steps = nitems;
}

void loading_additems(int num)
{
	max_steps += num;
}

void loading_step(void)
{
	steps++;
	loading_update();
}

#define SCRX	640.0f
#define BARW	512.0f
#define BARH	32.0f
#define BORD	5.0f
#define PAD		5.0f
#define MAXW	(BARW + BORD * 2 + PAD * 2)
#define MAXH	(BARH + BORD * 2 + PAD * 2)

void loading_update(void)
{
	float vheight = SCRX / win_aspect;
	float x, y;
	float barw;

	barw = BARW * (steps > max_steps ? 1.0f : (float)steps / max_steps);

	gaw_clear_color(0, 0, 0, 1);
	gaw_clear(GAW_COLORBUF);

	gaw_set_tex2d(0);

	begin2d(vheight);

	x = (SCRX - MAXW) / 2;
	y = (vheight - MAXH) / 2;

	gaw_color3f(1, 1, 1);
	gaw_rect(x, y, x + MAXW, y + MAXH);
	x += BORD;
	y += BORD;
	gaw_color3f(0, 0, 0);
	gaw_rect(x, y, x + BARW + PAD * 2, y + BARH + PAD * 2);
	x += PAD;
	y += PAD;
	gaw_color3f(1, 1, 1);
	gaw_rect(x, y, x + barw, y + BARH);

	end2d();

	game_swap_buffers();
}
