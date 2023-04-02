#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "miniglut.h"
#include "game.h"

static void idle(void);
static void reshape(int x, int y);
static void keydown(unsigned char key, int x, int y);
static void keyup(unsigned char key, int x, int y);
static void skeydown(int key, int x, int y);
static void skeyup(int key, int x, int y);
static void mouse(int bn, int st, int x, int y);
static void motion(int x, int y);
static void sball_motion(int x, int y, int z);
static void sball_rotate(int x, int y, int z);
static void sball_button(int bn, int st);
static int translate_skey(int key);

static int warping;

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitWindowSize(640, 480);
	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
	glutCreateWindow("deeprace");

	glutDisplayFunc(game_display);
	glutIdleFunc(idle);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keydown);
	glutKeyboardUpFunc(keyup);
	glutSpecialFunc(skeydown);
	glutSpecialUpFunc(skeyup);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutPassiveMotionFunc(motion);
	glutSpaceballMotionFunc(sball_motion);
	glutSpaceballRotateFunc(sball_rotate);
	glutSpaceballButtonFunc(sball_button);

	if(game_init() == -1) {
		return 1;
	}
	atexit(game_shutdown);
	glutMainLoop();
	return 0;
}

void game_swap_buffers(void)
{
	glutSwapBuffers();
	assert(glGetError() == GL_NO_ERROR);
}

void game_quit(void)
{
	exit(0);
}

void game_fullscreen(int fs)
{
	static int prev_w, prev_h;
	static int prev_grab;

	if(fs == -1) {
		fs = !fullscr;
	}

	if(fs == fullscr) return;

	if(fs) {
		prev_w = glutGet(GLUT_WINDOW_WIDTH);
		prev_h = glutGet(GLUT_WINDOW_HEIGHT);
		prev_grab = mouse_grabbed;
		game_grabmouse(1);
		glutFullScreen();
	} else {
		glutReshapeWindow(prev_w, prev_h);
		if(!prev_grab) {
			game_grabmouse(0);
		}
	}
	fullscr = fs;
}

void game_grabmouse(int grab)
{
	static int prev_x, prev_y;

	if(grab == -1) {
		grab = !mouse_grabbed;
	}

	if(grab == mouse_grabbed) return;

	if(grab) {
		warping = 1;
		prev_x = mouse_x;
		prev_y = mouse_y;
		glutWarpPointer(win_width / 2, win_height / 2);
		glutSetCursor(GLUT_CURSOR_NONE);
	} else {
		warping = 1;
		glutWarpPointer(prev_x, prev_y);
		glutSetCursor(GLUT_CURSOR_INHERIT);
	}
	mouse_grabbed = grab;
}


static void idle(void)
{
	glutPostRedisplay();
}

static void reshape(int x, int y)
{
	if(fullscr) {
		warping = 1;
		glutWarpPointer(x / 2, y / 2);
	}
	game_reshape(x, y);
}

static void keydown(unsigned char key, int x, int y)
{
	modkeys = glutGetModifiers();
	game_keyboard(key, 1);
}

static void keyup(unsigned char key, int x, int y)
{
	game_keyboard(key, 0);
}

static void skeydown(int key, int x, int y)
{
	int k;
	modkeys = glutGetModifiers();
	if((k = translate_skey(key)) >= 0) {
		game_keyboard(k, 1);
	}
}

static void skeyup(int key, int x, int y)
{
	int k = translate_skey(key);
	if(k >= 0) {
		game_keyboard(k, 0);
	}
}

static void mouse(int bn, int st, int x, int y)
{
	modkeys = glutGetModifiers();
	game_mouse(bn - GLUT_LEFT_BUTTON, st == GLUT_DOWN, x, y);
}

static void motion(int x, int y)
{
	if(mouse_grabbed) {
		if(!warping) {
			game_motion(x, y);
			warping = 1;
			glutWarpPointer(win_width / 2, win_height / 2);
		} else {
			warping = 0;
		}
	} else {
		game_motion(x, y);
	}
}

static void sball_motion(int x, int y, int z)
{
}

static void sball_rotate(int x, int y, int z)
{
}

static void sball_button(int bn, int st)
{
}

static int translate_skey(int key)
{
	switch(key) {
	case GLUT_KEY_LEFT:
		return GKEY_LEFT;
	case GLUT_KEY_UP:
		return GKEY_UP;
	case GLUT_KEY_RIGHT:
		return GKEY_RIGHT;
	case GLUT_KEY_DOWN:
		return GKEY_DOWN;
	case GLUT_KEY_PAGE_UP:
		return GKEY_PGUP;
	case GLUT_KEY_PAGE_DOWN:
		return GKEY_PGDOWN;
	case GLUT_KEY_HOME:
		return GKEY_HOME;
	case GLUT_KEY_END:
		return GKEY_END;
	case GLUT_KEY_INSERT:
		return GKEY_INS;
	default:
		if(key >= GLUT_KEY_F1 && key <= GLUT_KEY_F12) {
			return key - GLUT_KEY_F1 + GKEY_F1;
		}
	}

	return -1;
}
