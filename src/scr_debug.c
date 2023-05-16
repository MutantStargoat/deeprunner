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
#include "gaw/gaw.h"
#include "game.h"
#include "geom.h"


static int dbg_init(void);
static void dbg_destroy(void);
static int dbg_start(void);
static void dbg_stop(void);
static void dbg_display(void);
static void dbg_reshape(int x, int y);
static void dbg_keyb(int key, int press);
static void dbg_motion(int x, int y);
static void dbg_sbmove(int x, int y, int z);
static void dbg_sbrot(int x, int y, int z);
static void dbg_sbbutton(int bn, int press);

struct game_screen scr_debug = {
	"debug",
	dbg_init, dbg_destroy,
	dbg_start, dbg_stop,
	dbg_display, dbg_reshape,
	dbg_keyb, 0, dbg_motion,
	dbg_sbmove, dbg_sbrot, dbg_sbbutton
};

static float cam_theta, cam_phi, cam_dist;
static int sel;

/*static struct plane plane;*/
static struct triangle tri;
static struct aabox box;

static cgm_quat prot;
static cgm_vec3 ppos;


static int dbg_init(void)
{
	return 0;
}

static void dbg_destroy(void)
{
}

static int dbg_start(void)
{
	cam_theta = cam_phi = 0;
	cam_dist = 8;
	sel = 0;

	cgm_vcons(&ppos, 0, 0, 0);
	cgm_qcons(&prot, 0, 0, 0, 1);

	/*
	cgm_vcons(&plane.norm, 0, 1, 0);
	plane.d = 0;
	*/
	cgm_vcons(tri.v, 0, 0, -1);
	cgm_vcons(tri.v + 1, -0.86, 0, 0.5);
	cgm_vcons(tri.v + 2, 0.86, 0, 0.5);
	tri_calc_normal(&tri);

	cgm_vcons(&box.vmin, -1, -1, -1);
	cgm_vcons(&box.vmax, 1, 1, 1);

	gaw_disable(GAW_LIGHTING);
	gaw_depth_func(GAW_LEQUAL);
	gaw_linewidth(3);

	return 0;
}

static void dbg_stop(void)
{
	gaw_enable(GAW_CULL_FACE);
}

static void dbg_display(void)
{
	int i;
	float col;
	float mat[16];
	struct triangle xtri;

	gaw_clear(GAW_COLORBUF | GAW_DEPTHBUF);

	cgm_mrotation_quat(mat, &prot);
	cgm_mtranslate(mat, ppos.x, ppos.y, ppos.z);
	xtri = tri;
	cgm_vmul_m4v3(xtri.v, mat);
	cgm_vmul_m4v3(xtri.v + 1, mat);
	cgm_vmul_m4v3(xtri.v + 2, mat);
	tri_calc_normal(&xtri);

	gaw_matrix_mode(GAW_MODELVIEW);
	gaw_load_identity();
	gaw_translate(0, 0, -cam_dist);
	gaw_rotate(cam_phi, 1, 0, 0);
	gaw_rotate(cam_theta, 0, 1, 0);

	for(i=0; i<2; i++) {
		if(i == 0) {
			gaw_poly_wire();
		} else {
			gaw_enable(GAW_BLEND);
			gaw_blend_func(GAW_ONE, GAW_ONE);
			col = 0.15;
			gaw_poly_gouraud();
		}

		/* plane */
		/*
		glPushMatrix();
		cgm_mrotation_quat(mat, &prot);
		glMultMatrixf(mat);
		glTranslatef(0, plane.d, 0);

		glDisable(GL_CULL_FACE);

		glBegin(GL_QUADS);
		gaw_color3f(0, col, 0);
		gaw_vertex3f(5, 0, 5);
		gaw_vertex3f(5, 0, -5);
		gaw_vertex3f(-5, 0, -5);
		gaw_vertex3f(-5, 0, 5);
		glEnd();

		glPopMatrix();

		if(i == 0) {
			glBegin(GL_LINES);
			gaw_color3f(0, col, 0);
			gaw_vertex3f(plane.norm.x * plane.d, plane.norm.y * plane.d, plane.norm.z * plane.d);
			gaw_vertex3f(plane.norm.x * (plane.d + 1), plane.norm.y * (plane.d + 1), plane.norm.z * (plane.d + 1));
			glEnd();
		}
		*/

		/* triangle */
		gaw_disable(GAW_CULL_FACE);

		gaw_begin(GAW_TRIANGLES);
		gaw_color3f(0, col, 0);
		gaw_vertex3f(xtri.v[0].x, xtri.v[0].y, xtri.v[0].z);
		gaw_vertex3f(xtri.v[1].x, xtri.v[1].y, xtri.v[1].z);
		gaw_vertex3f(xtri.v[2].x, xtri.v[2].y, xtri.v[2].z);
		gaw_end();

		if(i == 0) {
			cgm_vec3 c;

			c = xtri.v[0];
			cgm_vadd(&c, xtri.v + 1);
			cgm_vadd(&c, xtri.v + 2);
			cgm_vscale(&c, 1.0f / 3.0f);

			gaw_begin(GAW_LINES);
			gaw_vertex3f(c.x, c.y, c.z);
			gaw_vertex3f(c.x + xtri.norm.x, c.y + xtri.norm.y, c.z + xtri.norm.z);
			gaw_end();
		}


		/* box */
		gaw_enable(GAW_CULL_FACE);

		gaw_begin(GAW_QUADS);
		/*if(aabox_plane_test(&box, &plane)) {*/
		if(aabox_tri_test(&box, &xtri)) {
			gaw_color3f(col, 0, 0);
		} else {
			gaw_color3f(0, 0, col);
		}
		/* +z */
		gaw_vertex3f(box.vmin.x, box.vmin.y, box.vmax.z);
		gaw_vertex3f(box.vmax.x, box.vmin.y, box.vmax.z);
		gaw_vertex3f(box.vmax.x, box.vmax.y, box.vmax.z);
		gaw_vertex3f(box.vmin.x, box.vmax.y, box.vmax.z);
		/* +x */
		gaw_vertex3f(box.vmax.x, box.vmin.y, box.vmax.z);
		gaw_vertex3f(box.vmax.x, box.vmin.y, box.vmin.z);
		gaw_vertex3f(box.vmax.x, box.vmax.y, box.vmin.z);
		gaw_vertex3f(box.vmax.x, box.vmax.y, box.vmax.z);
		/* -z */
		gaw_vertex3f(box.vmax.x, box.vmin.y, box.vmin.z);
		gaw_vertex3f(box.vmin.x, box.vmin.y, box.vmin.z);
		gaw_vertex3f(box.vmin.x, box.vmax.y, box.vmin.z);
		gaw_vertex3f(box.vmax.x, box.vmax.y, box.vmin.z);
		/* -x */
		gaw_vertex3f(box.vmin.x, box.vmin.y, box.vmin.z);
		gaw_vertex3f(box.vmin.x, box.vmin.y, box.vmax.z);
		gaw_vertex3f(box.vmin.x, box.vmax.y, box.vmax.z);
		gaw_vertex3f(box.vmin.x, box.vmax.y, box.vmin.z);
		/* -y */
		gaw_vertex3f(box.vmin.x, box.vmin.y, box.vmin.z);
		gaw_vertex3f(box.vmax.x, box.vmin.y, box.vmin.z);
		gaw_vertex3f(box.vmax.x, box.vmin.y, box.vmax.z);
		gaw_vertex3f(box.vmin.x, box.vmin.y, box.vmax.z);
		/* +y */
		gaw_vertex3f(box.vmin.x, box.vmax.y, box.vmax.z);
		gaw_vertex3f(box.vmax.x, box.vmax.y, box.vmax.z);
		gaw_vertex3f(box.vmax.x, box.vmax.y, box.vmin.z);
		gaw_vertex3f(box.vmin.x, box.vmax.y, box.vmin.z);

		gaw_end();
	}

	gaw_disable(GAW_BLEND);
}

static void dbg_reshape(int x, int y)
{
	gaw_matrix_mode(GAW_PROJECTION);
	gaw_load_identity();
	gaw_perspective(50, win_aspect, 0.5, 500);
}

static void dbg_keyb(int key, int press)
{
	if(!press) return;

	if(key == '\t') {
		sel ^= 1;
		/*printf("sel: %s\n", sel ? "plane" : "box");*/
		printf("sel: %s\n", sel ? "triangle" : "box");
	}
}

static void dbg_motion(int x, int y)
{
	int dx, dy;

	dx = x - mouse_x;
	dy = y - mouse_y;

	if(!(dx | dy)) return;

	if(mouse_state[0]) {
		cam_theta += dx * 0.5f;
		cam_phi += dy * 0.5f;
		if(cam_phi < -90) cam_phi = -90;
		if(cam_phi > 90) cam_phi = 90;
	}
	if(mouse_state[2]) {
		cam_dist += dy * 0.1;
		if(cam_dist < 0) cam_dist = 0;
	}
}

static void dbg_sbmove(int x, int y, int z)
{
	cgm_vec3 offs;
	cgm_vcons(&offs, x, y, -z);
	cgm_vscale(&offs, 0.001);

	if(sel == 0) {
		cgm_vadd(&box.vmin, &offs);
		cgm_vadd(&box.vmax, &offs);
	} else {
		/*
		plane.d += y * 0.01;

		printf("plane: n(%f %f %f) - d(%f)\n", plane.norm.x, plane.norm.y,
				plane.norm.z, plane.d);
		*/
		cgm_vadd(&ppos, &offs);
	}
}

static void dbg_sbrot(int x, int y, int z)
{
	float s;

	if(sel == 0 || (x | y | z) == 0) return;

	s = 1.0f / (float)sqrt(x * x + y * y + z * z);
	if(s != 0.0f) {
		cgm_qrotate(&prot, 0.001f / s, x * s, y * s, -z * s);

		/*
		cgm_vcons(&plane.norm, 0, 1, 0);
		cgm_vrotate_quat(&plane.norm, &prot);

		printf("plane: n(%f %f %f) - d(%f)\n", plane.norm.x, plane.norm.y,
				plane.norm.z, plane.d);
		*/
	}
}

static void dbg_sbbutton(int bn, int press)
{
}
