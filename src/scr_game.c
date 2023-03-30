#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/gl.h>
#include "miniglut.h"
#include "game.h"
#include "util.h"

static int ginit(void);
static void gdestroy(void);
static int gstart(void);
static void gstop(void);
static void gdisplay(void);
static void greshape(int x, int y);
static void gkeyb(int key, int press);
static void gmouse(int bn, int press, int x, int y);
static void gmotion(int x, int y);

struct game_screen scr_game = {
	"game",
	ginit, gdestroy,
	gstart, gstop,
	gdisplay, greshape,
	gkeyb, gmouse, gmotion
};

static float cam_theta, cam_phi = 20, cam_dist = 10;
static float cam_pan[3];

static int ginit(void)
{
	return 0;
}

static void gdestroy(void)
{
}

static int gstart(void)
{
	return 0;
}

static void gstop(void)
{
}

static void gdisplay(void)
{
	static int dlist;
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0, 0, -cam_dist);
	glRotatef(cam_phi, 1, 0, 0);
	glRotatef(cam_theta, 0, 1, 0);
	glTranslatef(cam_pan[0], cam_pan[1], cam_pan[2]);

	glColor3f(1, 1, 1);
	glFrontFace(GL_CW);
	if(!dlist) {
		dlist = glGenLists(1);
		glNewList(dlist, GL_COMPILE);
		glutSolidTeapot(1);
		glEndList();
	}
	glCallList(dlist);
	glFrontFace(GL_CCW);
}

static void greshape(int x, int y)
{
}

static void gkeyb(int key, int press)
{
}

static void gmouse(int bn, int press, int x, int y)
{
}

static void gmotion(int x, int y)
{
	int dx = x - mouse_x;
	int dy = y - mouse_y;

	if(!(dx | dy)) return;

	if(mouse_state[0]) {
		cam_theta += dx * 0.5;
		cam_phi += dy * 0.5;
		if(cam_phi < -90) cam_phi = -90;
		if(cam_phi > 90) cam_phi = 90;
	}
	if(mouse_state[1]) {
		float up[3], right[3];
		float theta = cam_theta * M_PI / 180.0f;
		float phi = cam_phi * M_PI / 180.0f;

		up[0] = -sin(theta) * sin(phi);
		up[1] = -cos(phi);
		up[2] = cos(theta) * sin(phi);
		right[0] = cos(theta);
		right[1] = 0;
		right[2] = sin(theta);

		cam_pan[0] += (right[0] * dx + up[0] * dy) * 0.01;
		cam_pan[1] += up[1] * dy * 0.01;
		cam_pan[2] += (right[2] * dx + up[2] * dy) * 0.01;
	}
	if(mouse_state[2]) {
		cam_dist += dy * 0.1;
		if(cam_dist < 0) cam_dist = 0;
	}
}
