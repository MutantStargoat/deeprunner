#include <stdio.h>
#include "opengl.h"
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

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	begin2d(vheight);

	x = (SCRX - MAXW) / 2;
	y = (vheight - MAXH) / 2;

	glColor3f(1, 1, 1);
	glRectf(x, y, x + MAXW, y + MAXH);
	x += BORD;
	y += BORD;
	glColor3f(0, 0, 0);
	glRectf(x, y, x + BARW + PAD * 2, y + BARH + PAD * 2);
	x += PAD;
	y += PAD;
	glColor3f(1, 1, 1);
	glRectf(x, y, x + barw, y + BARH);

	end2d();

	game_swap_buffers();
}
