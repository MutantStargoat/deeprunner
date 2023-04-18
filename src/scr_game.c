#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/gl.h>
#include "miniglut.h"
#include "game.h"
#include "util.h"
#include "goat3d.h"
#include "input.h"
#include "player.h"
#include "cgmath/cgmath.h"
#include "audio.h"
#include "options.h"
#include "player.h"

static int ginit(void);
static void gdestroy(void);
static int gstart(void);
static void gstop(void);
static void gdisplay(void);
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

static struct goat3d *gscn;
static int dlist;
static unsigned int dbgtex;

static struct au_module *mod;


static int ginit(void)
{
	int i, num, nfaces;
	int *idxarr;
	float *varr, *narr, *uvarr;
	float xform[16];

	if(!(gscn = goat3d_create()) || goat3d_load(gscn, "data/level1.g3d")) {
		return -1;
	}

	dlist = glGenLists(1);
	glNewList(dlist, GL_COMPILE);
	num = goat3d_get_node_count(gscn);
	for(i=0; i<num; i++) {
		struct goat3d_node *node = goat3d_get_node(gscn, i);
		if(match_prefix(goat3d_get_node_name(node), "portal_")) {
			continue;
		}
		if(goat3d_get_node_type(node) == GOAT3D_NODE_MESH) {
			struct goat3d_mesh *mesh = goat3d_get_node_object(node);

			goat3d_get_node_matrix(node, xform);
			glPushMatrix();
			glMultMatrixf(xform);

			varr = goat3d_get_mesh_attribs(mesh, GOAT3D_MESH_ATTR_VERTEX);
			narr = goat3d_get_mesh_attribs(mesh, GOAT3D_MESH_ATTR_NORMAL);
			uvarr = goat3d_get_mesh_attribs(mesh, GOAT3D_MESH_ATTR_TEXCOORD);

			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, GL_FLOAT, 0, varr);

			if(narr) {
				glEnableClientState(GL_NORMAL_ARRAY);
				glNormalPointer(GL_FLOAT, 0, narr);
			}
			if(uvarr) {
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexCoordPointer(2, GL_FLOAT, 0, uvarr);
			}

			nfaces = goat3d_get_mesh_face_count(mesh);
			idxarr = goat3d_get_mesh_faces(mesh);
			glDrawElements(GL_TRIANGLES, nfaces * 3, GL_UNSIGNED_INT, idxarr);

			glDisableClientState(GL_VERTEX_ARRAY);
			glDisableClientState(GL_NORMAL_ARRAY);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);

			glPopMatrix();
		}
	}
	glEndList();

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

	return 0;
}

static void gdestroy(void)
{
	goat3d_free(gscn);
}

static int gstart(void)
{
	float amb[] = {0.25, 0.25, 0.25, 1};

	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb);

	glEnable(GL_LIGHTING);
	set_light_color(0, 1, 1, 1, 0.8);
	glEnable(GL_LIGHT0);
	set_light_color(1, 1, 0.6, 0.5, 0.2);
	glEnable(GL_LIGHT1);
	set_light_color(2, 0.5, 0.6, 1, 0.3);
	glEnable(GL_LIGHT2);

	init_player(&player);

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
}

#define TSTEP	(1.0f / 30.0f)
#define KB_MOVE_SPEED	0.2

static void gupdate(void)
{
	if(inpstate & INP_MOVE_BITS) {
		if(inpstate & INP_FWD_BIT) {
			player.vel.z += KB_MOVE_SPEED;
		}
		if(inpstate & INP_BACK_BIT) {
			player.vel.z -= KB_MOVE_SPEED;
		}
		if(inpstate & INP_RIGHT_BIT) {
			player.vel.x -= KB_MOVE_SPEED;
		}
		if(inpstate & INP_LEFT_BIT) {
			player.vel.x += KB_MOVE_SPEED;
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

	glBindTexture(GL_TEXTURE_2D, dbgtex);
	glEnable(GL_TEXTURE_2D);

	glCallList(dlist);

	glDisable(GL_TEXTURE_2D);
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
	cgm_vcons(&player.sball_mov, -x, -y, z);
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

