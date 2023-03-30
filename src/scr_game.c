#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/gl.h>
#include "miniglut.h"
#include "game.h"
#include "util.h"
#include "goat3d.h"

static int ginit(void);
static void gdestroy(void);
static int gstart(void);
static void gstop(void);
static void gdisplay(void);
static void greshape(int x, int y);
static void gkeyb(int key, int press);
static void gmouse(int bn, int press, int x, int y);
static void gmotion(int x, int y);

struct game_screen scr_game = {
	"game",
	ginit, gdestroy,
	gstart, gstop,
	gdisplay, greshape,
	gkeyb, gmouse, gmotion
};

static float cam_theta, cam_phi = 20, cam_dist = 10;
static float cam_pan[3];

static struct goat3d *gscn;
static int dlist;


static int ginit(void)
{
	int i, num, nfaces;

	if(!(gscn = goat3d_create()) || goat3d_load(gscn, "data/track1.g3d")) {
		return -1;
	}

	dlist = glGenLists(1);
	glNewList(dlist, GL_COMPILE);
	num = goat3d_get_node_count(gscn);
	for(i=0; i<num; i++) {
		struct goat3d_node *node = goat3d_get_node(gscn, i);
		if(goat3d_get_node_type(node) == GOAT3D_NODE_MESH) {
			struct goat3d_mesh *mesh = goat3d_get_node_object(node);

			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, GL_FLOAT, 0, goat3d_get_mesh_attribs(mesh, GOAT3D_MESH_ATTR_VERTEX));

			nfaces = goat3d_get_mesh_face_count(mesh) / 3;
			glDrawElements(GL_TRIANGLES, nfaces * 3, GL_UNSIGNED_INT, goat3d_get_mesh_faces(mesh));

			glDisableClientState(GL_VERTEX_ARRAY);

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
	return 0;
}

static void gstop(void)
{
}

static void gdisplay(void)
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0, 0, -cam_dist);
	glRotatef(cam_phi, 1, 0, 0);
	glRotatef(cam_theta, 0, 1, 0);
	glTranslatef(cam_pan[0], cam_pan[1], cam_pan[2]);

	glCallList(dlist);
}

static void greshape(int x, int y)
{
}

static void gkeyb(int key, int press)
{
}

static void gmouse(int bn, int press, int x, int y)
{
}

static void gmotion(int x, int y)
{
	int dx = x - mouse_x;
	int dy = y - mouse_y;

	if(!(dx | dy)) return;

	if(mouse_state[0]) {
		cam_theta += dx * 0.5;
		cam_phi += dy * 0.5;
		if(cam_phi < -90) cam_phi = -90;
		if(cam_phi > 90) cam_phi = 90;
	}
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
	if(mouse_state[2]) {
		cam_dist += dy * 0.1;
		if(cam_dist < 0) cam_dist = 0;
	}
}
