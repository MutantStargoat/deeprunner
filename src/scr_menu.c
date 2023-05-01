#include "miniglut.h"
#include "opengl.h"
#include "game.h"
#include "mtltex.h"
#include "gfxutil.h"
#include "drawtext.h"

#define FONT_SCALE	0.7

static int menu_init(void);
static void menu_destroy(void);
static int menu_start(void);
static void menu_stop(void);
static void menu_display(void);
static void menu_reshape(int x, int y);
static void menu_keyb(int key, int press);
static void menu_mouse(int bn, int press, int x, int y);
static void menu_motion(int x, int y);

static void act_item(int sel);


struct game_screen scr_menu = {
	"menu",
	menu_init, menu_destroy,
	menu_start, menu_stop,
	menu_display, menu_reshape,
	menu_keyb, menu_mouse, menu_motion
};

static struct texture *gamelogo;
static struct texture *startex;
static struct dtx_font *font;
static int font_sz;
static float font_height;

static int sel;

enum { START, OPTIONS, HIGHSCORES, CREDITS, QUIT, NUM_MENU_ITEMS };
static const char *menustr[] = {
	"START", "OPTIONS", "HIGH SCORES", "CREDITS", "QUIT" };
static struct rect itemrect[NUM_MENU_ITEMS];


static int menu_init(void)
{
	int i;

	if(!(gamelogo = tex_load("data/gamelogo.png"))) {
		return -1;
	}
	if(!(startex = tex_load("data/blspstar.png"))) {
		return -1;
	}

	if(!(font = dtx_open_font_glyphmap("data/menufont.gmp"))) {
		fprintf(stderr, "failed to open menu font\n");
		return -1;
	}
	font_sz = dtx_get_glyphmap_ptsize(dtx_get_glyphmap(font, 0));
	dtx_use_font(font, font_sz);
	font_height = dtx_line_height();

	for(i=0; i<NUM_MENU_ITEMS; i++) {
		itemrect[i].width = dtx_string_width(menustr[i]) * FONT_SCALE;
		itemrect[i].height = font_height;
		itemrect[i].x = 640 - itemrect[i].width / 2;
		itemrect[i].y = i > 0 ? itemrect[i - 1].y + 25 : 360;
	}

	return 0;
}

static void menu_destroy(void)
{
	tex_free(gamelogo);
}

static int menu_start(void)
{
	glClearColor(0, 0, 0, 1);

	dtx_set(DTX_GL_BLEND, 1);
	dtx_set(DTX_GL_ALPHATEST, 0);
	return 0;
}

static void menu_stop(void)
{
}

#define STAR_RAD	80

static void menu_display(void)
{
	int i;
	float x, y, vwidth, strwidth;
	float alpha;
	float tsec = (float)time_msec / 1000.0f;

	vwidth = win_aspect * 480;
	x = (vwidth - 640) / 2.0f;

	begin2d(480);

	blit_tex_rect(x, 0, 640, 480 * 0.9, gamelogo, 1, 0, 0, 1, 0.9);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, startex->texid);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	x += 316;
	y = 310;

	glPushMatrix();
	glTranslatef(x, y, 0);
	glRotatef(tsec * 10.0f, 0, 0, 1);
	alpha = (sin(tsec) + 0.5) * 0.2;

	glBegin(GL_QUADS);
	glColor3f(alpha, alpha, alpha);
	glTexCoord2f(0, 0); glVertex2f(-STAR_RAD, -STAR_RAD);
	glTexCoord2f(0, 1); glVertex2f(-STAR_RAD, STAR_RAD);
	glTexCoord2f(1, 1); glVertex2f(STAR_RAD, STAR_RAD);
	glTexCoord2f(1, 0); glVertex2f(STAR_RAD, -STAR_RAD);
	glEnd();

	glPopMatrix();

	y = 360;
	dtx_use_font(font, font_sz);

	glMatrixMode(GL_MODELVIEW);

	for(i=0; i<NUM_MENU_ITEMS; i++) {
		glPushMatrix();
		if(sel == i) {
			glColor3f(0.694, 0.753, 1.000);
		} else {
			glColor3f(0.133, 0.161, 0.271);
		}
		strwidth = dtx_string_width(menustr[i]) * 0.7;
		x = (vwidth - strwidth) / 2;
		glTranslatef(x, y, 0);
		glScalef(0.7, -0.7, 0.7);
		dtx_printf(menustr[i]);
		glPopMatrix();

		if(sel == i) {
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);
			blit_tex_rect(x - 25, y - 16, 16, 16, gamelogo, 1, 0.00469, 0.9604, 0.025, 0.033333);
			blit_tex_rect(x + strwidth + 25 - 16, y - 16, 16, 16, gamelogo, 1, 0.00469, 0.9604, 0.025, 0.033333);
		}
		y += 25;

		glDisable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);
		glBegin(GL_LINE_LOOP);
		glColor3f(0, 1, 0);
		glVertex2f(itemrect[i].x, itemrect[i].y);
		glVertex2f(itemrect[i].x + itemrect[i].width, itemrect[i].y);
		glVertex2f(itemrect[i].x + itemrect[i].width, itemrect[i].y + itemrect[i].height);
		glVertex2f(itemrect[i].x, itemrect[i].y + itemrect[i].height);
		glEnd();
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
	case GKEY_UP:
		if(sel) sel--;
		break;

	case GKEY_DOWN:
		if(sel < NUM_MENU_ITEMS - 1) {
			sel++;
		}
		break;

	case '\n':
		act_item(sel);
		break;
	}
}

static void menu_mouse(int bn, int press, int x, int y)
{
}

static void menu_motion(int x, int y)
{
}


static void act_item(int sel)
{
	switch(sel) {
	case START:
		game_chscr(&scr_game);
		break;

	case OPTIONS:
		/* game_chscr(&scr_opt); */
		break;

	case HIGHSCORES:
		/* game_chscr(&scr_scores); */
		break;

	case CREDITS:
		/* game_chscr(&scr_credits); */
		break;

	case QUIT:
		game_quit();
		break;
	}
}
