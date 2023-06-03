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
#ifndef GAME_H_
#define GAME_H_

#include "player.h"
#include "audio.h"

#define GAME_CFG_FILE	"game.cfg"

#define TSTEP	(1.0f / 30.0f)

enum {
	GKEY_ESC	= 27,
	GKEY_DEL	= 127,
	GKEY_F1		= 256,
	GKEY_F2, GKEY_F3, GKEY_F4, GKEY_F5, GKEY_F6, GKEY_F7,
	GKEY_F8, GKEY_F9, GKEY_F10, GKEY_F11, GKEY_F12,
	GKEY_UP, GKEY_DOWN, GKEY_LEFT, GKEY_RIGHT,
	GKEY_PGUP, GKEY_PGDOWN,
	GKEY_HOME, GKEY_END,
	GKEY_INS
};

enum {
	GKEY_MOD_SHIFT	= 1,
	GKEY_MOD_CTRL	= 4,
	GKEY_MOD_ALT	= 8
};


struct game_screen {
	const char *name;

	int (*init)(void);
	void (*destroy)(void);
	int (*start)(void);
	void (*stop)(void);
	void (*display)(void);
	void (*reshape)(int, int);
	void (*keyboard)(int, int);
	void (*mouse)(int, int, int, int);
	void (*motion)(int, int);
	void (*sball_motion)(int, int, int);
	void (*sball_rotate)(int, int, int);
	void (*sball_button)(int, int);
};

extern int mouse_x, mouse_y, mouse_state[3];
extern int mouse_grabbed;
extern unsigned int modkeys;
extern int win_width, win_height;
extern float win_aspect;
extern int fullscr;

extern long time_msec;
extern struct game_screen *cur_scr;
extern struct game_screen scr_logo, scr_menu, scr_game, scr_debug, scr_opt;

extern struct player *player;

extern struct au_sample *sfx_o2chime;
extern struct au_sample *sfx_laser, *sfx_gling1;

struct font;
extern struct font *font_menu;

int game_init(void);
void game_shutdown(void);

void game_display(void);
void game_reshape(int x, int y);
void game_keyboard(int key, int press);
void game_mouse(int bn, int st, int x, int y);
void game_motion(int x, int y);
void game_sball_motion(int x, int y, int z);
void game_sball_rotate(int x, int y, int z);
void game_sball_button(int bn, int st);

void game_chscr(struct game_screen *scr);

/* defined in main.c */
long game_getmsec(void);
void game_swap_buffers(void);
void game_quit(void);
void game_resize(int x, int y);
void game_fullscreen(int fs);
void game_grabmouse(int grab);
void game_vsync(int vsync);

#endif	/* GAME_H_ */
