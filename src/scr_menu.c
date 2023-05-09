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
static struct texture *startex;/*, *blurmenu;*/
struct dtx_font *font_menu;
int font_menu_sz;
static float font_height, font_baseline;

static int sel, hover = -1;

enum { START, OPTIONS, HIGHSCORES, CREDITS, QUIT, NUM_MENU_ITEMS };
static const char *menustr[] = {
	"START", "OPTIONS", "HIGH SCORES", "CREDITS", "QUIT" };
static struct rect itemrect[NUM_MENU_ITEMS];

static float xoffs;


static int menu_init(void)
{
	int i;

	if(!(gamelogo = tex_load("data/gamelogo.png"))) {
		return -1;
	}
	if(!(startex = tex_load("data/blspstar.png"))) {
		return -1;
	}
	/*
	if(!(blurmenu = tex_load("data/blurmenu.png"))) {
		return -1;
	}
	*/

	if(!(font_menu = dtx_open_font_glyphmap("data/menufont.gmp"))) {
		fprintf(stderr, "failed to open menu font\n");
		return -1;
	}
	font_menu_sz = dtx_get_glyphmap_ptsize(dtx_get_glyphmap(font_menu, 0));
	dtx_use_font(font_menu, font_menu_sz);
	font_height = dtx_line_height() * FONT_SCALE;
	font_baseline = font_height * 0.2;

	for(i=0; i<NUM_MENU_ITEMS; i++) {
		itemrect[i].width = dtx_string_width(menustr[i]) * FONT_SCALE;
		itemrect[i].height = font_height;
		itemrect[i].x = 320 - itemrect[i].width / 2;
		itemrect[i].y = i > 0 ? itemrect[i - 1].y + font_height : 360 - font_height + font_baseline;
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
	float x, y, alpha;
	float tsec = (float)time_msec / 1000.0f;

	glClear(GL_COLOR_BUFFER_BIT);

	begin2d(480);

	blit_tex_rect(xoffs, 0, 640, 480 * 0.9, gamelogo, 1, 0, 0, 1, 0.9);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, startex->texid);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	glPushMatrix();
	glTranslatef(xoffs + 316, 310, 0);
	glRotatef(tsec * 10.0f, 0, 0, 1);
	alpha = (sin(tsec) + 0.6) * 0.2;

	glBegin(GL_QUADS);
	glColor3f(alpha, alpha, alpha);
	glTexCoord2f(0, 0); glVertex2f(-STAR_RAD, -STAR_RAD);
	glTexCoord2f(0, 1); glVertex2f(-STAR_RAD, STAR_RAD);
	glTexCoord2f(1, 1); glVertex2f(STAR_RAD, STAR_RAD);
	glTexCoord2f(1, 0); glVertex2f(STAR_RAD, -STAR_RAD);
	glEnd();

	glPopMatrix();

	dtx_use_font(font_menu, font_menu_sz);

	glMatrixMode(GL_MODELVIEW);

	for(i=0; i<NUM_MENU_ITEMS; i++) {
		glPushMatrix();
		if(sel == i) {
			glColor3f(0.694, 0.753, 1.000);
		} else {
			glColor3f(0.133, 0.161, 0.271);
		}
		x = xoffs + itemrect[i].x;
		y = itemrect[i].y + itemrect[i].height - font_baseline;

		glTranslatef(x, y, 0);
		glScalef(FONT_SCALE, -FONT_SCALE, FONT_SCALE);
		dtx_printf(menustr[i]);
		glPopMatrix();

		if(sel == i) {
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);
			blit_tex_rect(x - 25, y - 16, 16, 16, gamelogo, 1, 0.00469, 0.9604, 0.025, 0.033333);
			blit_tex_rect(x + itemrect[i].width + 25 - 16, y - 16, 16, 16, gamelogo, 1, 0.00469, 0.9604, 0.025, 0.033333);
		}
		y += font_height;
	}

	end2d();
}

static void menu_reshape(int x, int y)
{
	float vwidth;

	vwidth = win_aspect * 480;
	xoffs = (vwidth - 640) / 2.0f;
}

static void menu_keyb(int key, int press)
{
	if(!press) return;

	switch(key) {
	case 27:
		game_quit();
		break;

	case GKEY_UP:
	case 'w':
		if(sel) sel--;
		break;

	case GKEY_DOWN:
	case 's':
		if(sel < NUM_MENU_ITEMS - 1) {
			sel++;
		}
		break;

	case '\r':
	case 'e':
		act_item(sel);
		break;
	}
}

static void menu_mouse(int bn, int press, int x, int y)
{
	if(bn == 0 && !press && hover != -1) {
		act_item(hover);
	}
}

static void menu_motion(int x, int y)
{
	int i;
	float vx = x * win_aspect * 480.0f / win_width - xoffs;
	float vy = y * 480.0f / win_height;

	hover = -1;
	for(i=0; i<NUM_MENU_ITEMS; i++) {
		if(vx < itemrect[i].x) continue;
		if(vx >= itemrect[i].x + itemrect[i].width) continue;
		if(vy < itemrect[i].y) continue;
		if(vy >= itemrect[i].y + itemrect[i].height) continue;
		sel = hover = i;
		break;
	}
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
