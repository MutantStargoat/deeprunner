#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "opengl.h"
#include "imago2.h"
#include "miniglut.h"
#include "game.h"
#include "util.h"
#include "level.h"
#include "input.h"
#include "player.h"
#include "cgmath/cgmath.h"
#include "audio.h"
#include "options.h"
#include "player.h"
#include "rendlvl.h"

static int ginit(void);
static void gdestroy(void);
static int gstart(void);
static void gstop(void);
static void gdisplay(void);
static void draw_ui(void);
static void greshape(int x, int y);
static void gkeyb(int key, int press);
static void gmouse(int bn, int press, int x, int y);
static void gmotion(int x, int y);
static void gsball_motion(int x, int y, int z);
static void gsball_rotate(int x, int y, int z);
static void gsball_button(int bn, int state);

static void set_light_dir(int idx, float x, float y, float z);
static void set_light_color(int idx, float r, float g, float b, float s);


struct game_screen scr_game = {
	"game",
	ginit, gdestroy,
	gstart, gstop,
	gdisplay, greshape,
	gkeyb, gmouse, gmotion,
	gsball_motion, gsball_rotate, gsball_button
};

static float view_mat[16], proj_mat[16];

static struct player player;

static struct level lvl;
static unsigned int dbgtex;
static unsigned int uitex;

static struct au_module *mod;

#ifdef DBG_FREEZEVIS
int dbg_freezevis;
#endif
#ifdef DBG_SHOW_COLPOLY
const struct triangle *dbg_hitpoly;
#endif
#ifdef DBG_SHOW_MAX_COL_ITER
int dbg_max_col_iter;
#endif
static cgm_vec3 vispos;
static cgm_quat visrot;

static int ginit(void)
{
	int i;

	lvl_init(&lvl);
	if(lvl_load(&lvl, "data/level1.lvl") == -1) {
		return -1;
	}

	{
		int j;
		unsigned char *pix, *pptr;

		pptr = pix = malloc(256 * 256);

		for(i=0; i<256; i++) {
			for(j=0; j<256; j++) {
				*pptr++ = (i ^ j) | 0x80;
			}
		}

		glGenTextures(1, &dbgtex);
		glBindTexture(GL_TEXTURE_2D, dbgtex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		gluBuild2DMipmaps(GL_TEXTURE_2D, 1, 256, 256, GL_LUMINANCE, GL_UNSIGNED_BYTE, pix);

		free(pix);
	}

	if((uitex = img_gltexture_load("data/uibars.png"))) {
		glBindTexture(GL_TEXTURE_2D, uitex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	return 0;
}

static void gdestroy(void)
{
	lvl_destroy(&lvl);
}

static int gstart(void)
{
	float amb[] = {0.25, 0.25, 0.25, 1};

	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb);

	glDepthFunc(GL_LEQUAL);

	glEnable(GL_LIGHTING);
	set_light_color(0, 1, 1, 1, 0.8);
	glEnable(GL_LIGHT0);
	set_light_color(1, 1, 0.6, 0.5, 0.2);
	glEnable(GL_LIGHT1);
	set_light_color(2, 0.5, 0.6, 1, 0.3);
	glEnable(GL_LIGHT2);

	if(rendlvl_init(&lvl)) {
		return -1;
	}

	init_player(&player);
	player.lvl = &lvl;

	if(opt.music) {
		if(!(mod = au_load_module("data/sc-fuse.it"))) {
			fprintf(stderr, "failed to open music\n");
		} else {
			au_play_module(mod);
		}
	}
	return 0;
}

static void gstop(void)
{
	if(mod) {
		au_stop_module(mod);
		au_free_module(mod);
		mod = 0;
	}

	rendlvl_destroy();
}

#define TSTEP	(1.0f / 30.0f)
#define KB_MOVE_SPEED	0.2

static void gupdate(void)
{
	if(inpstate & INP_MOVE_BITS) {
		if(inpstate & INP_FWD_BIT) {
			player.vel.z -= KB_MOVE_SPEED;
		}
		if(inpstate & INP_BACK_BIT) {
			player.vel.z += KB_MOVE_SPEED;
		}
		if(inpstate & INP_RIGHT_BIT) {
			player.vel.x += KB_MOVE_SPEED;
		}
		if(inpstate & INP_LEFT_BIT) {
			player.vel.x -= KB_MOVE_SPEED;
		}
		if(inpstate & INP_LROLL_BIT) {
			player.roll -= 0.1;
		}
		if(inpstate & INP_RROLL_BIT) {
			player.roll += 0.1;
		}
	}

	update_player_sball(&player);

	update_player(&player);
}

static void gdisplay(void)
{
	static long prev_msec;
	static float tm_acc;
	long msec;

	msec = glutGet(GLUT_ELAPSED_TIME);
	tm_acc += (float)(msec - prev_msec) / 1000.0f;
	prev_msec = msec;

	/* updating mouse input every frame feels more fluid */
	update_player_mouse(&player);

	/* update all other game logic once per timestep */
	while(tm_acc >= TSTEP) {
		gupdate();
		tm_acc -= TSTEP;
	}

	player_view_matrix(&player, view_mat);
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(view_mat);

	set_light_dir(0, -1, 1, 5);
	set_light_dir(1, 5, 0, 3);
	set_light_dir(2, -0.5, -2, -3);

	if(!dbg_freezevis) {
		vispos = player.pos;
		visrot = player.rot;
	}

	rendlvl_setup(&vispos, &visrot);
	render_level();

	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);

#ifdef DBG_SHOW_COLPOLY
	if(dbg_hitpoly) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);

		glBegin(GL_TRIANGLES);
		glColor3f(0.1, 0.1, 0.4);
		glVertex3fv(&dbg_hitpoly->v[0].x);
		glVertex3fv(&dbg_hitpoly->v[1].x);
		glVertex3fv(&dbg_hitpoly->v[2].x);
		glEnd();

		glDisable(GL_BLEND);
	}
#endif

#ifdef DBG_FREEZEVIS
	if(dbg_freezevis) {
		glBegin(GL_LINES);
		glColor3f(0, 1, 0);
		glVertex3f(vispos.x - 100, vispos.y, vispos.z);
		glVertex3f(vispos.x + 100, vispos.y, vispos.z);
		glVertex3f(vispos.x, vispos.y - 100, vispos.z);
		glVertex3f(vispos.x, vispos.y + 100, vispos.z);
		glVertex3f(vispos.x, vispos.y, vispos.z - 100);
		glVertex3f(vispos.x, vispos.y, vispos.z + 100);
		glEnd();

		glPushMatrix();
		glColor3f(0.2, 0.2, 0.2);
		glTranslatef(vispos.x, vispos.y, vispos.z);
		glutSolidSphere(COL_RADIUS, 10, 5);
		glPopMatrix();
	}
#endif

	glPopAttrib();

#ifdef DBG_FREEZEVIS
	if(!dbg_freezevis)
#endif
		draw_ui();
}

#define UIW		256
#define UIH		128
static void draw_ui(void)
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, win_aspect * 480, 0, 480, -1, 1);

	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_LIGHTING);
	glEnable(GL_ALPHA_TEST);
	if(opt.gfx.blendui) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glAlphaFunc(GL_GREATER, 0.01);
	} else {
		glAlphaFunc(GL_GREATER, 0.25);
	}

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, uitex);

	glBegin(GL_QUADS);
	glColor3f(1, 1, 1);
	glTexCoord2f(0, 1); glVertex2f(0, 480 - UIH);
	glTexCoord2f(1, 1); glVertex2f(UIW, 480 - UIH);
	glTexCoord2f(1, 0); glVertex2f(UIW, 480);
	glTexCoord2f(0, 0); glVertex2f(0, 480);
	glEnd();

	glDisable(GL_TEXTURE_2D);

	/* crosshair */
	glBegin(GL_LINES);
	glColor3f(0.5, 0.8, 0.5);
	glVertex2f(320 - 6, 240);
	glVertex2f(320 - 2, 240);
	glVertex2f(320 + 2, 240);
	glVertex2f(320 + 6, 240);
	glVertex2f(320, 240 - 6);
	glVertex2f(320, 240 - 2);
	glVertex2f(320, 240 + 6);
	glVertex2f(320, 240 + 2);
	glEnd();

	glPopAttrib();
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}

static void greshape(int x, int y)
{
	cgm_mperspective(proj_mat, cgm_deg_to_rad(60), win_aspect, 0.1, 100);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(proj_mat);
}

static void gkeyb(int key, int press)
{
	int i;

	for(i=0; i<MAX_INPUTS; i++) {
		if(inpmap[i].key == key) {
			if(press) {
				inpstate |= 1 << inpmap[i].inp;
			} else {
				inpstate &= ~(1 << inpmap[i].inp);
			}
			break;
		}
	}

	if(press) {
		switch(key) {
		case '`':
			if(!fullscr) {
				game_grabmouse(-1);	/* toggle */
			}
			break;

		case '\t':
			dbg_freezevis ^= 1;
			if(!dbg_freezevis) {
				player.pos = vispos;
				player.rot = visrot;
			}

		}
	}
}

static void gmouse(int bn, int press, int x, int y)
{
}

#define PIHALF	(M_PI / 2.0)

static void gmotion(int x, int y)
{
	int dx, dy;

	if(mouse_grabbed) {
		dx = x - win_width / 2;
		dy = y - win_height / 2;
	} else {
		dx = x - mouse_x;
		dy = y - mouse_y;
	}

	if(!(dx | dy)) return;

	if(mouse_state[0] || mouse_grabbed) {
		player.mouse_input.x += dx;
		player.mouse_input.y += opt.inv_mouse_y ? -dy : dy;
	}
}

static void gsball_motion(int x, int y, int z)
{
	cgm_vcons(&player.sball_mov, x, y, -z);
}

static void gsball_rotate(int x, int y, int z)
{
	cgm_vcons(&player.sball_rot, -x, -y, z);
}

static void gsball_button(int bn, int press)
{
	if(press) {
		init_player(&player);
	}
}

static void set_light_dir(int idx, float x, float y, float z)
{
	float pos[4];
	pos[0] = x;
	pos[1] = y;
	pos[2] = z;
	pos[3] = 0;
	glLightfv(GL_LIGHT0 + idx, GL_POSITION, pos);
}

static void set_light_color(int idx, float r, float g, float b, float s)
{
	float color[4];
	color[0] = r * s;
	color[1] = g * s;
	color[2] = b * s;
	color[3] = 1;
	glLightfv(GL_LIGHT0 + idx, GL_DIFFUSE, color);
	glLightfv(GL_LIGHT0 + idx, GL_SPECULAR, color);
}

