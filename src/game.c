#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "miniglut.h"
#include <GL/glu.h>
#include "game.h"
#include "input.h"

int mouse_x, mouse_y, mouse_state[3];
int win_width, win_height;
float win_aspect;

struct game_screen *cur_scr;

/* available screens */
extern struct game_screen scr_menu, scr_game;
#define MAX_SCREENS	4
static struct game_screen *screens[MAX_SCREENS];
static int num_screens;


int game_init(void)
{
	int i;
	char *start_scr_name;

	/* initialize screens */
	screens[num_screens++] = &scr_menu;
	screens[num_screens++] = &scr_game;

	start_scr_name = getenv("START_SCREEN");

	for(i=0; i<num_screens; i++) {
		if(screens[i]->init() == -1) {
			return -1;
		}
		if(screens[i]->name && start_scr_name && strcmp(screens[i]->name, start_scr_name) == 0) {
			game_chscr(screens[i]);
		}
	}
	if(!cur_scr) {
		game_chscr(&scr_game);	/* TODO: scr_menu */
	}

	init_input();

	glClearColor(0.1, 0.1, 0.1, 1);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	return 0;
}

void game_shutdown(void)
{
	int i;

	putchar('\n');

	for(i=0; i<num_screens; i++) {
		if(screens[i]->destroy) {
			screens[i]->destroy();
		}
	}
}

void game_display(void)
{
	static long nframes, interv, prev_msec;
	long msec;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	cur_scr->display();

	game_swap_buffers();


	msec = glutGet(GLUT_ELAPSED_TIME);
	interv += msec - prev_msec;
	prev_msec = msec;
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

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(50, win_aspect, 0.5, 500);

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
