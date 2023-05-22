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
#ifndef GAWSWTNL_H_
#define GAWSWTNL_H_

#include "polyfill.h"

enum {
	GAW_SPHEREMAP	= 24
};

/* modelview, projection, texture */
#define NUM_MATRICES	3
#define STACK_SIZE	8
typedef float gaw_matrix[16];

#define MAX_LIGHTS		4
#define MAX_TEXTURES	256
#define MAX_COMPILED	256

#define IMM_VBUF_SIZE	256

enum {LT_POS, LT_DIR};
struct light {
	int type;
	float x, y, z;
	float r, g, b;
};

struct material {
	float kd[4];
	float ks[3];
	float ke[3];
	float shin;
};

struct comp_geom {
	int prim;
	float *varr, *narr, *uvarr, *carr;	/* darr */
};


struct gaw_state {
	uint32_t opt;
	uint32_t savopt[STACK_SIZE];
	int savopt_top;

	int frontface;
	int polymode;

	const float *varr, *narr, *uvarr;

	gaw_matrix mat[NUM_MATRICES][STACK_SIZE];
	int mtop[NUM_MATRICES];
	int mmode;

	gaw_matrix norm_mat;

	float ambient[3];
	struct light lt[MAX_LIGHTS];
	struct material mtl;

	int bsrc, bdst;

	int width, height;
	gaw_pixel *pixels;

	int vport[4];

	uint32_t clear_color;
	uint32_t clear_depth;

	const float *vertex_ptr, *normal_ptr, *texcoord_ptr, *color_ptr;
	int vertex_nelem, texcoord_nelem, color_nelem;
	int vertex_stride, normal_stride, texcoord_stride, color_stride;

	/* immediate mode */
	int imm_prim;
	int imm_numv, imm_pcount;
	struct vertex imm_curv;
	float imm_curcol[4];
	struct vertex imm_vbuf[IMM_VBUF_SIZE];
	float imm_cbuf[IMM_VBUF_SIZE * 4];

	/* textures */
	int cur_tex;
	int textypes[MAX_TEXTURES];
	struct pimage textures[MAX_TEXTURES];

	/* compiled geometries */
	int cur_comp;
	struct comp_geom comp[MAX_COMPILED];
};

extern struct gaw_state *gaw_state;
#define ST	gaw_state

#endif	/* GAWSWTNL_H_ */
