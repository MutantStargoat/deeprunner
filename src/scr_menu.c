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
	return 0;
}

static void menu_stop(void)
{
}

static void menu_display(void)
{
}

static void menu_reshape(int x, int y)
{
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
