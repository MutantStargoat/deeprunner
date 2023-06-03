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
#include "gaw/gaw.h"
#include "game.h"
#include "mtltex.h"
#include "gfxutil.h"
#include "drawtext.h"
#include "gui.h"
#include "util.h"


static int opt_init(void);
static void opt_destroy(void);
static int opt_start(void);
static void opt_stop(void);
static void opt_display(void);
static void opt_reshape(int x, int y);
static void opt_keyb(int key, int press);
static void opt_mouse(int bn, int press, int x, int y);
static void opt_motion(int x, int y);

static void act_item(int sel);


struct game_screen scr_opt = {
	"opt",
	opt_init, opt_destroy,
	opt_start, opt_stop,
	opt_display, opt_reshape,
	opt_keyb, opt_mouse, opt_motion
};

static struct gui *ui;


static int opt_init(void)
{
	ui = malloc_nf(sizeof *ui);
	gui_init(ui, 0, 0, 640, 480);
	ui->font = font_menu;
	gui_add_act(ui, "FOO");
	gui_add_act(ui, "BAR");
	gui_add_act(ui, "XYZZY");
	gui_add_act(ui, "MEGALI KOURASI");

	ui->padding = 10;
	return 0;
}

static void opt_destroy(void)
{
	gui_destroy(ui);
	free(ui);
}

static int opt_start(void)
{
	gaw_clear_color(0, 0, 0, 1);

	dtx_set(DTX_GL_BLEND, 1);
	dtx_set(DTX_GL_ALPHATEST, 0);
	return 0;
}

static void opt_stop(void)
{
}

static void opt_display(void)
{
	gaw_clear(GAW_COLORBUF);

	gui_draw(ui);
}

static void opt_reshape(int x, int y)
{
	float vwidth = 480.0 * win_aspect;

	ui->x = ui->y = 10;
	ui->width = vwidth - 20;
	ui->height = 480 - 20;

	gui_layout(ui, 0);
}

static void opt_keyb(int key, int press)
{
	if(!press) return;

	switch(key) {
	case 27:
		game_chscr(&scr_menu);
		return;
	}

	gui_evkey(ui, key);
}

static void opt_mouse(int bn, int press, int x, int y)
{
}

static void opt_motion(int x, int y)
{
}


static void act_item(int sel)
{
}
