#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "miniglut.h"
#include <GL/glu.h>
#include "game.h"
#include "input.h"
#include "audio.h"
#include "options.h"
#include "util.h"

static void draw_volume_bar(void);

int mouse_x, mouse_y, mouse_state[3];
int mouse_grabbed;
unsigned int modkeys;
int win_width, win_height;
float win_aspect;
int fullscr;

long time_msec;

struct game_screen *cur_scr;

/* available screens */
extern struct game_screen scr_menu, scr_game, scr_debug;
#define MAX_SCREENS	4
static struct game_screen *screens[MAX_SCREENS];
static int num_screens;
static long last_vol_chg = -16384;


int game_init(void)
{
	int i;
	char *start_scr_name;

#ifndef NDEBUG
	enable_fpexcept();
#endif

	load_options(GAME_CFG_FILE);
	game_resize(opt.xres, opt.yres);

	if(au_init() == -1) {
		return -1;
	}
	au_music_volume(opt.vol_mus);
	au_sfx_volume(opt.vol_sfx);
	au_volume(opt.vol_master);

	/* initialize screens */
	screens[num_screens++] = &scr_menu;
	screens[num_screens++] = &scr_game;
	screens[num_screens++] = &scr_debug;

	start_scr_name = getenv("START_SCREEN");

	for(i=0; i<num_screens; i++) {
		if(screens[i]->init() == -1) {
			return -1;
		}
	}

	init_input();

	glClearColor(0.1, 0.1, 0.1, 1);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	for(i=0; i<num_screens; i++) {
		if(screens[i]->name && start_scr_name && strcmp(screens[i]->name, start_scr_name) == 0) {
			game_chscr(screens[i]);
			break;
		}
	}
	if(!cur_scr) {
		game_chscr(&scr_debug);	/* TODO: scr_menu */
	}

	return 0;
}

void game_shutdown(void)
{
	int i;

	putchar('\n');

	save_options(GAME_CFG_FILE);

	for(i=0; i<num_screens; i++) {
		if(screens[i]->destroy) {
			screens[i]->destroy();
		}
	}
}

void game_display(void)
{
	static long nframes, interv, prev_msec;

	time_msec = glutGet(GLUT_ELAPSED_TIME);

	au_update();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	cur_scr->display();

	draw_volume_bar();

	game_swap_buffers();


	interv += time_msec - prev_msec;
	prev_msec = time_msec;
	if(interv >= 1000) {
		float fps = (float)(nframes * 1000) / interv;
		printf("\rfps: %.2f    ", fps);
		fflush(stdout);
		nframes = 0;
		interv = 0;
	}
	nframes++;
}

void game_reshape(int x, int y)
{
	win_width = x;
	win_height = y;
	win_aspect = (float)x / (float)y;
	glViewport(0, 0, x, y);

	if(cur_scr && cur_scr->reshape) {
		cur_scr->reshape(x, y);
	}
}

void game_keyboard(int key, int press)
{
	if(press) {
		switch(key) {
		case 27:
			game_quit();
			return;

		case '\n':
		case '\r':
			if(modkeys & GKEY_MOD_ALT) {
		case GKEY_F11:
				game_fullscreen(-1);
			}
			return;

		case '-':
			opt.vol_master = au_volume(AU_VOLDN | 31);
			last_vol_chg = time_msec;
			break;

		case '=':
			opt.vol_master = au_volume(AU_VOLUP | 31);
			last_vol_chg = time_msec;
			break;

		case 'm':
			opt.music ^= 1;
			last_vol_chg = time_msec;
			if(opt.music) {
				au_volume(opt.vol_master);
			} else {
				au_volume(0);
			}
			break;
		}
	}

	if(cur_scr && cur_scr->keyboard) {
		cur_scr->keyboard(key, press);
	}
}

void game_mouse(int bn, int st, int x, int y)
{
	mouse_x = x;
	mouse_y = y;
	if(bn < 3) {
		mouse_state[bn] = st;
	}

	if(cur_scr && cur_scr->mouse) {
		cur_scr->mouse(bn, st, x, y);
	}
}

void game_motion(int x, int y)
{
	if(cur_scr && cur_scr->motion) {
		cur_scr->motion(x, y);
	}
	mouse_x = x;
	mouse_y = y;
}

void game_sball_motion(int x, int y, int z)
{
	if(cur_scr->sball_motion) {
		cur_scr->sball_motion(x, y, z);
	}
}

void game_sball_rotate(int x, int y, int z)
{
	if(cur_scr->sball_rotate) {
		cur_scr->sball_rotate(x, y, z);
	}
}

void game_sball_button(int bn, int st)
{
	if(cur_scr->sball_button) {
		cur_scr->sball_button(bn, st == GLUT_DOWN);
	}
}

void game_chscr(struct game_screen *scr)
{
	if(!scr) return;

	if(scr->start && scr->start() == -1) {
		return;
	}

	if(cur_scr && cur_scr->stop) {
		cur_scr->stop();
	}
	cur_scr = scr;
}

#define VOL_BARS	8
static void draw_volume_bar(void)
{
	int i;

	if(time_msec - last_vol_chg < 3000) {
		glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT);

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0, 640, 0, 480, -1, 1);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glDisable(GL_TEXTURE_2D);
		glDisable(GL_LIGHTING);

		/* draw speaker icon */
		glTranslatef(520, 460, 0);
		glScalef(3.2f, 2.8f, 3.2f);
		glBegin(GL_QUADS);
		glColor3ub(128, 240, 128);
		glVertex3f(-5, -2, 0);
		glVertex3f(-3.6f, -2, 0);
		glVertex3f(-3.6f, 2, 0);
		glVertex3f(-5, 2, 0);
		glVertex3f(-3, -2, 0);
		glVertex3f(0, -5, 0);
		glVertex3f(0, 5, 0);
		glVertex3f(-3, 2, 0);
		if(opt.music) {
			glVertex3f(0.9f, 3.7f, 0);
			glVertex3f(1.9f, 1.9f, 0);
			glVertex3f(2.5f, 2.3f, 0);
			glVertex3f(1.3f, 4, 0);
			glVertex3f(2.2f, 0, 0);
			glVertex3f(3, 0, 0);
			glVertex3f(2.5f, 2.3f, 0);
			glVertex3f(1.9f, 1.9f, 0);
			glVertex3f(1.9f, -1.9f, 0);
			glVertex3f(2.5f, -2.3f, 0);
			glVertex3f(3, 0, 0);
			glVertex3f(2.2f, 0, 0);
			glVertex3f(1.3f, -4, 0);
			glVertex3f(2.5f, -2.3f, 0);
			glVertex3f(1.9f, -1.9f, 0);
			glVertex3f(0.9f, -3.7f, 0);

			glVertex3f(3.6f, 2.8f, 0);
			glVertex3f(4.3f, 3.2f, 0);
			glVertex3f(2.9f, 5.5f, 0);
			glVertex3f(2.2f, 5, 0);
			glVertex3f(4.1f, 0, 0);
			glVertex3f(5, 0, 0);
			glVertex3f(4.3f, 3.2f, 0);
			glVertex3f(3.6f, 2.8f, 0);
			glVertex3f(3.6f, -2.8f, 0);
			glVertex3f(4.3f, -3.2f, 0);
			glVertex3f(5, 0, 0);
			glVertex3f(4.1f, 0, 0);
			glVertex3f(2.2f, -5, 0);
			glVertex3f(2.9f, -5.5f, 0);
			glVertex3f(4.3f, -3.2f, 0);
			glVertex3f(3.6f, -2.8f, 0);
		} else {
			glVertex3f(-7, -5, 0);
			glVertex3f(-6, -6, 0);
			glVertex3f(4, 5, 0);
			glVertex3f(3, 6, 0);
		}

		if(opt.music) {
			for(i=0; i<VOL_BARS; i++) {
				float x = 8 + i * 3.5;
				if(opt.vol_master < 255 && i >= opt.vol_master >> 5) {
					glEnd();
					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
					glBegin(GL_QUADS);
				}
				glVertex3f(x, -3.5, 0);
				glVertex3f(x + 1.5, -3.5, 0);
				glVertex3f(x + 1.5, 3.5, 0);
				glVertex3f(x, 3.5, 0);
			}
		}
		glEnd();

		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);

		glPopAttrib();
	}
}

