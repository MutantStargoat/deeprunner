#include "miniglut.h"
#include "opengl.h"
#include "game.h"

static int menu_init(void);
static void menu_destroy(void);
static int menu_start(void);
static void menu_stop(void);
static void menu_display(void);
static void menu_reshape(int x, int y);
static void menu_keyb(int key, int press);
static void menu_mouse(int bn, int press, int x, int y);
static void menu_motion(int x, int y);

struct game_screen scr_menu = {
	"menu",
	menu_init, menu_destroy,
	menu_start, menu_stop,
	menu_display, menu_reshape,
	menu_keyb, menu_mouse, menu_motion
};


static int menu_init(void)
{
	return 0;
}

static void menu_destroy(void)
{
}

static int menu_start(void)
{
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glClearColor(0.2, 0.1, 0.1, 1);

	return 0;
}

static void menu_stop(void)
{
}

static void menu_display(void)
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0, 0, -8);
	glRotatef(25, 1, 0, 0);

	glFrontFace(GL_CW);
	glutSolidTeapot(1.0);
	glFrontFace(GL_CCW);
}

static void menu_reshape(int x, int y)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(50, win_aspect, 0.5, 500);
}

static void menu_keyb(int key, int press)
{
}

static void menu_mouse(int bn, int press, int x, int y)
{
}

static void menu_motion(int x, int y)
{
}
