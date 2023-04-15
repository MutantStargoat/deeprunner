#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/gl.h>
#include "miniglut.h"
#include "game.h"
#include "util.h"
#include "goat3d.h"
#include "input.h"
#include "cgmath/cgmath.h"
#include "audio.h"

static int ginit(void);
static void gdestroy(void);
static int gstart(void);
static void gstop(void);
static void gdisplay(void);
static void greshape(int x, int y);
static void gkeyb(int key, int press);
static void gmouse(int bn, int press, int x, int y);
static void gmotion(int x, int y);

static void set_light_dir(int idx, float x, float y, float z);
static void set_light_color(int idx, float r, float g, float b, float s);


struct game_screen scr_game = {
	"game",
	ginit, gdestroy,
	gstart, gstop,
	gdisplay, greshape,
	gkeyb, gmouse, gmotion
};

static float view_mat[16], proj_mat[16];

static float cam_theta, cam_phi, cam_dist;
static cgm_vec3 cam_pan;

static struct goat3d *gscn;
static int dlist;

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

	set_light_color(0, 1, 1, 1, 0.8);
	glEnable(GL_LIGHT0);
	set_light_color(1, 1, 0.6, 0.5, 0.2);
	glEnable(GL_LIGHT1);
	set_light_color(2, 0.5, 0.6, 1, 0.3);
	glEnable(GL_LIGHT2);


	if(!(mod = au_load_module("data/sc-fuse.it"))) {
		fprintf(stderr, "failed to open music\n");
	} else {
		au_play_module(mod);
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

static void gupdate(void)
{
	if(inpstate & INP_MOVE_BITS) {
		cgm_vec3 fwd, right;

		float dx = 0, dy = 0;

		fwd.x = -sin(cam_theta) * cos(cam_phi);
		fwd.y = sin(cam_phi);
		fwd.z = cos(cam_theta) * cos(cam_phi);
		right.x = cos(cam_theta);
		right.y = 0;
		right.z = sin(cam_theta);

		if(inpstate & INP_FWD_BIT) {
			dy += 0.1;
		}
		if(inpstate & INP_BACK_BIT) {
			dy -= 0.1;
		}
		if(inpstate & INP_RIGHT_BIT) {
			dx -= 0.1;
		}
		if(inpstate & INP_LEFT_BIT) {
			dx += 0.1;
		}

		cam_pan.x += right.x * dx + fwd.x * dy;
		cam_pan.y += fwd.y * dy;
		cam_pan.z += right.z * dx + fwd.z * dy;
	}
}

static void gdisplay(void)
{
	static long prev_msec;
	static float tm_acc;
	long msec;

	msec = glutGet(GLUT_ELAPSED_TIME);
	tm_acc += (float)(msec - prev_msec) / 1000.0f;
	prev_msec = msec;

	while(tm_acc >= TSTEP) {
		gupdate();
		tm_acc -= TSTEP;
	}

	cgm_mtranslation(view_mat, 0, 0, -cam_dist);
	cgm_mprerotate(view_mat, cam_phi, 1, 0, 0);
	cgm_mprerotate(view_mat, cam_theta, 0, 1, 0);
	cgm_mpretranslate(view_mat, cam_pan.x, cam_pan.y, cam_pan.z);
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(view_mat);

	set_light_dir(0, -1, 1, 5);
	set_light_dir(1, 5, 0, 3);
	set_light_dir(2, -0.5, -2, -3);

	glCallList(dlist);
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
		cam_theta += dx * 0.01;
		cam_phi += dy * 0.01;
		if(cam_phi < -PIHALF) cam_phi = -PIHALF;
		if(cam_phi > PIHALF) cam_phi = PIHALF;
	}
	/*
	if(mouse_state[1]) {
		float up[3], right[3];
		float theta = cam_theta * M_PI / 180.0f;
		float phi = cam_phi * M_PI / 180.0f;

		up[0] = -sin(theta) * sin(phi);
		up[1] = -cos(phi);
		up[2] = cos(theta) * sin(phi);
		right[0] = cos(theta);
		right[1] = 0;
		right[2] = sin(theta);

		cam_pan[0] += (right[0] * dx + up[0] * dy) * 0.01;
		cam_pan[1] += up[1] * dy * 0.01;
		cam_pan[2] += (right[2] * dx + up[2] * dy) * 0.01;
	}
	*/
	if(mouse_state[2]) {
		cam_dist += dy * 0.1;
		if(cam_dist < 0) cam_dist = 0;
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

