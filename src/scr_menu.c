#include "miniglut.h"
#include "opengl.h"
#include "game.h"
#include "mtltex.h"
#include "gfxutil.h"
#include "drawtext.h"

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

static struct texture *gamelogo;
static struct dtx_font *font;
static int font_sz;

static int menu_init(void)
{
	if(!(gamelogo = tex_load("data/gamelogo.png"))) {
		fprintf(stderr, "failed to load game logo\n");
		return -1;
	}

	if(!(font = dtx_open_font_glyphmap("data/menufont.gmp"))) {
		fprintf(stderr, "failed to open menu font\n");
		return -1;
	}
	font_sz = dtx_get_glyphmap_ptsize(dtx_get_glyphmap(font, 0));

	return 0;
}

static void menu_destroy(void)
{
	tex_free(gamelogo);
}

static int menu_start(void)
{
	glClearColor(0, 0, 0, 1);

	//dtx_set(DTX_GL_BLEND, 1);
	//dtx_set(DTX_GL_ALPHATEST, 0);
	return 0;
}

static void menu_stop(void)
{
}

static void menu_display(void)
{
	int i;
	float x, y, vwidth, strwidth;
	static const char *menustr[] = {
		"START", "OPTIONS", "HIGH SCORES", "CREDITS", "QUIT" };

	vwidth = win_aspect * 480;
	x = (vwidth - 640) / 2.0f;

	begin2d(480);

	blit_tex(x, 0, gamelogo, 1);

	y = 360;
	dtx_use_font(font, font_sz);

	glMatrixMode(GL_MODELVIEW);

	glColor3f(0.694, 0.753, 1.000);
	for(i=0; i<sizeof menustr / sizeof *menustr; i++) {
		glPushMatrix();
		strwidth = dtx_string_width(menustr[i]) * 0.7;
		glTranslatef((vwidth - strwidth) / 2, y, 0);
		glScalef(0.7, -0.7, 0.7);
		dtx_printf(menustr[i]);
		glPopMatrix();
		y += 25;
	}

	end2d();
}

static void menu_reshape(int x, int y)
{
}

static void menu_keyb(int key, int press)
{
	if(!press) return;

	switch(key) {
	case '\n':
		game_chscr(&scr_game);
		break;
	}
}

static void menu_mouse(int bn, int press, int x, int y)
{
}

static void menu_motion(int x, int y)
{
}
