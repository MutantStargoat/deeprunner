#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "miniglut.h"
#include "game.h"

static void idle(void);
static void keydown(unsigned char key, int x, int y);
static void keyup(unsigned char key, int x, int y);
static void skeydown(int key, int x, int y);
static void skeyup(int key, int x, int y);
static void mouse(int bn, int st, int x, int y);
static int translate_skey(int key);


int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitWindowSize(640, 480);
	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
	glutCreateWindow("deeprace");

	glutDisplayFunc(game_display);
	glutIdleFunc(idle);
	glutReshapeFunc(game_reshape);
	glutKeyboardFunc(keydown);
	glutKeyboardUpFunc(keyup);
	glutSpecialFunc(skeydown);
	glutSpecialUpFunc(skeyup);
	glutMouseFunc(mouse);
	glutMotionFunc(game_motion);

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


static void idle(void)
{
	glutPostRedisplay();
}

static void keydown(unsigned char key, int x, int y)
{
	game_keyboard(key, 1);
}

static void keyup(unsigned char key, int x, int y)
{
	game_keyboard(key, 0);
}

static void skeydown(int key, int x, int y)
{
	int k = translate_skey(key);
	if(k >= 0) {
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
	game_mouse(bn - GLUT_LEFT_BUTTON, st == GLUT_DOWN, x, y);
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
			return key - (GLUT_KEY_F1 + GKEY_F1);
		}
	}

	return -1;
}
