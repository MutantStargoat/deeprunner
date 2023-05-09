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
static int translate_skey(int key);

static int warping;

#if defined(__unix__) || defined(unix)
#include <GL/glx.h>
static Display *xdpy;
static Window xwin;

static void (*glx_swap_interval_ext)();
static void (*glx_swap_interval_sgi)();
#endif
#ifdef _WIN32
#include <windows.h>
static PROC wgl_swap_interval_ext;
#endif



int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitWindowSize(640, 480);
	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
	glutCreateWindow("DeepRunner");

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
	glutSpaceballMotionFunc(game_sball_motion);
	glutSpaceballRotateFunc(game_sball_rotate);
	glutSpaceballButtonFunc(game_sball_button);

#if defined(__unix__) || defined(unix)
	xdpy = glXGetCurrentDisplay();
	xwin = glXGetCurrentDrawable();

	if(!(glx_swap_interval_ext = glXGetProcAddress((unsigned char*)"glXSwapIntervalEXT"))) {
		glx_swap_interval_sgi = glXGetProcAddress((unsigned char*)"glXSwapIntervalSGI");
	}
#endif
#ifdef _WIN32
	wgl_swap_interval_ext = wglGetProcAddress("wglSwapIntervalEXT");
#endif

	game_reshape(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));

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

void game_resize(int x, int y)
{
	if(x == win_width && y == win_height) return;

	glutReshapeWindow(x, y);
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

#if defined(__unix__) || defined(unix)
void game_vsync(int vsync)
{
	vsync = vsync ? 1 : 0;
	if(glx_swap_interval_ext) {
		glx_swap_interval_ext(xdpy, xwin, vsync);
	} else if(glx_swap_interval_sgi) {
		glx_swap_interval_sgi(vsync);
	}
}
#endif
#ifdef WIN32
void game_vsync(int vsync)
{
	if(wgl_swap_interval_ext) {
		wgl_swap_interval_ext(vsync ? 1 : 0);
	}
}
#endif



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
			mouse_x = x;
			mouse_y = y;
		}
	} else {
		game_motion(x, y);
	}
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
