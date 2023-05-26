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
#include <string.h>
#include <math.h>
#include "cgmath/cgmath.h"
#include "gaw.h"
#include "gawswtnl.h"
#include "polyfill.h"
#include "polyclip.h"
#include "../darray.h"

#define NORMALIZE(v) \
	do { \
		float len = sqrt((v)[0] * (v)[0] + (v)[1] * (v)[1] + (v)[2] * (v)[2]); \
		if(len != 0.0) { \
			float s = 1.0 / len; \
			(v)[0] *= s; \
			(v)[1] *= s; \
			(v)[2] *= s; \
		} \
	} while(0)


static void imm_flush(void);
static __inline void xform4_vec3(const float *mat, float *vec);
static __inline void xform3_vec3(const float *mat, float *vec);
static void shade(struct vertex *v);

static struct gaw_state st;
struct gaw_state *gaw_state;

static const float idmat[] = {
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1
};


void gaw_swtnl_reset(void)
{
	int i;

	memset(&st, 0, sizeof st);

	st.polymode = POLYFILL_GOURAUD;

	for(i=0; i<NUM_MATRICES; i++) {
		gaw_matrix_mode(i);
		gaw_load_identity();
	}

	for(i=0; i<MAX_LIGHTS; i++) {
		gaw_light_dir(i, 0, 0, 1);
		gaw_light_color(i, 1, 1, 1, 1);
	}
	gaw_ambient(0.1, 0.1, 0.1);
	gaw_mtl_diffuse(1, 1, 1, 1);

	st.clear_depth = 0xffffff;

	st.cur_comp = -1;
	st.cur_tex = -1;

	gaw_color3f(1, 1, 1);

	gaw_state = &st;
}

void gaw_swtnl_init(void)
{
	gaw_swtnl_reset();
}

void gaw_swtnl_destroy(void)
{
}

void gaw_viewport(int x, int y, int w, int h)
{
	st.vport[0] = x;
	st.vport[1] = y;
	st.vport[2] = w;
	st.vport[3] = h;
}

void gaw_matrix_mode(int mode)
{
	st.mmode = mode;
}

void gaw_load_identity(void)
{
	int top = st.mtop[st.mmode];
	memcpy(st.mat[st.mmode][top], idmat, 16 * sizeof(float));
}

void gaw_load_matrix(const float *m)
{
	int top = st.mtop[st.mmode];
	memcpy(st.mat[st.mmode][top], m, 16 * sizeof(float));
}

#define M(i,j)	(((i) << 2) + (j))
void gaw_mult_matrix(const float *m2)
{
	int i, j, top = st.mtop[st.mmode];
	float m1[16];
	float *dest = st.mat[st.mmode][top];

	memcpy(m1, dest, sizeof m1);

	for(i=0; i<4; i++) {
		for(j=0; j<4; j++) {
			*dest++ = m1[M(0,j)] * m2[M(i,0)] +
				m1[M(1,j)] * m2[M(i,1)] +
				m1[M(2,j)] * m2[M(i,2)] +
				m1[M(3,j)] * m2[M(i,3)];
		}
	}
}

void gaw_push_matrix(void)
{
	int top = st.mtop[st.mmode];
	if(top >= STACK_SIZE) {
		fprintf(stderr, "push_matrix overflow\n");
		return;
	}
	memcpy(st.mat[st.mmode][top + 1], st.mat[st.mmode][top], 16 * sizeof(float));
	st.mtop[st.mmode] = top + 1;
}

void gaw_pop_matrix(void)
{
	if(st.mtop[st.mmode] <= 0) {
		fprintf(stderr, "pop_matrix underflow\n");
		return;
	}
	--st.mtop[st.mmode];
}

void gaw_get_modelview(float *m)
{
	int top = st.mtop[GAW_MODELVIEW];
	memcpy(m, st.mat[GAW_MODELVIEW][top], 16 * sizeof(float));
}

void gaw_get_projection(float *m)
{
	int top = st.mtop[GAW_PROJECTION];
	memcpy(m, st.mat[GAW_PROJECTION][top], 16 * sizeof(float));
}

void gaw_translate(float x, float y, float z)
{
	static float m[16];
	m[0] = m[5] = m[10] = m[15] = 1.0f;
	m[12] = x;
	m[13] = y;
	m[14] = z;
	gaw_mult_matrix(m);
}

void gaw_rotate(float deg, float x, float y, float z)
{
	static float m[16];

	float angle = M_PI * deg / 180.0f;
	float sina = sin(angle);
	float cosa = cos(angle);
	float one_minus_cosa = 1.0f - cosa;
	float nxsq = x * x;
	float nysq = y * y;
	float nzsq = z * z;

	m[0] = nxsq + (1.0f - nxsq) * cosa;
	m[4] = x * y * one_minus_cosa - z * sina;
	m[8] = x * z * one_minus_cosa + y * sina;
	m[1] = x * y * one_minus_cosa + z * sina;
	m[5] = nysq + (1.0 - nysq) * cosa;
	m[9] = y * z * one_minus_cosa - x * sina;
	m[2] = x * z * one_minus_cosa - y * sina;
	m[6] = y * z * one_minus_cosa + x * sina;
	m[10] = nzsq + (1.0 - nzsq) * cosa;
	m[15] = 1.0f;

	gaw_mult_matrix(m);
}

void gaw_scale(float sx, float sy, float sz)
{
	static float m[16];
	m[0] = sx;
	m[5] = sy;
	m[10] = sz;
	m[15] = 1.0f;
	gaw_mult_matrix(m);
}

void gaw_ortho(float l, float r, float b, float t, float n, float f)
{
	static float m[16];

	float dx = r - l;
	float dy = t - b;
	float dz = f - n;

	m[0] = 2.0 / dx;
	m[5] = 2.0 / dy;
	m[10] = -2.0 / dz;
	m[12] = -(r + l) / dx;
	m[13] = -(t + b) / dy;
	m[14] = -(f + n) / dz;
	m[15] = 1.0f;

	gaw_mult_matrix(m);
}

void gaw_frustum(float left, float right, float bottom, float top, float zn, float zf)
{
	static float m[16];

	float dx = right - left;
	float dy = top - bottom;
	float dz = zf - zn;

	float a = (right + left) / dx;
	float b = (top + bottom) / dy;
	float c = -(zf + zn) / dz;
	float d = -2.0 * zf * zn / dz;

	m[0] = 2.0 * zn / dx;
	m[5] = 2.0 * zn / dy;
	m[8] = a;
	m[9] = b;
	m[10] = c;
	m[11] = -1.0f;
	m[14] = d;

	gaw_mult_matrix(m);
}

void gaw_perspective(float vfov_deg, float aspect, float znear, float zfar)
{
	static float m[16];

	float vfov = M_PI * vfov_deg / 180.0f;
	float s = 1.0f / tan(vfov * 0.5f);
	float range = znear - zfar;

	m[0] = s / aspect;
	m[5] = s;
	m[10] = (znear + zfar) / range;
	m[11] = -1.0f;
	m[14] = 2.0f * znear * zfar / range;

	gaw_mult_matrix(m);
}

void gaw_save(void)
{
	if(st.savopt_top >= STACK_SIZE) {
		return;
	}
	st.savopt[st.savopt_top++] = st.opt;
}

void gaw_restore(void)
{
	if(st.savopt_top <= 0) {
		return;
	}
	st.opt = st.savopt[--st.savopt_top];
}

void gaw_swtnl_enable(int what)
{
	st.opt |= 1 << what;
}

void gaw_swtnl_disable(int what)
{
	st.opt &= ~(1 << what);
}

void gaw_depth_func(int func)
{
	/* TODO */
}

void gaw_blend_func(int src, int dest)
{
	st.bsrc = src;
	st.bdst = dest;
}

void gaw_alpha_func(int func, float ref)
{
	/* TODO */
}

#define CLAMP(x, a, b)		((x) < (a) ? (a) : ((x) > (b) ? (b) : (x)))

void gaw_clear_color(float r, float g, float b, float a)
{
	int ir = (int)(r * 255.0f);
	int ig = (int)(g * 255.0f);
	int ib = (int)(b * 255.0f);

	ir = CLAMP(ir, 0, 255);
	ig = CLAMP(ig, 0, 255);
	ib = CLAMP(ib, 0, 255);

	st.clear_color = PACK_RGB(ir, ig, ib);
}

void gaw_clear_depth(float z)
{
	int iz = (int)(z * (float)0xffffff);
	st.clear_depth = CLAMP(iz, 0, 0xffffff);
}

void gaw_swtnl_color_mask(int rmask, int gmask, int bmask, int amask)
{
	/* TODO */
}

void gaw_swtnl_depth_mask(int mask)
{
	/* TODO */
}

void gaw_vertex_array(int nelem, int stride, const void *ptr)
{
	if(stride <= 0) {
		stride = nelem * sizeof(float);
	}
	st.vertex_nelem = nelem;
	st.vertex_stride = stride;
	st.vertex_ptr = ptr;
}

void gaw_normal_array(int stride, const void *ptr)
{
	if(stride <= 0) {
		stride = 3 * sizeof(float);
	}
	st.normal_stride = stride;
	st.normal_ptr = ptr;
}

void gaw_texcoord_array(int nelem, int stride, const void *ptr)
{
	if(stride <= 0) {
		stride = nelem * sizeof(float);
	}
	st.texcoord_nelem = nelem;
	st.texcoord_stride = stride;
	st.texcoord_ptr = ptr;
}

void gaw_color_array(int nelem, int stride, const void *ptr)
{
	if(stride <= 0) {
		stride = nelem * sizeof(float);
	}
	st.color_nelem = nelem;
	st.color_stride = stride;
	st.color_ptr = ptr;
}

void gaw_draw(int prim, int nverts)
{
	gaw_draw_indexed(prim, 0, nverts);
}


#define NEED_NORMALS \
	(st.opt & ((1 << GAW_LIGHTING) | (1 << GAW_SPHEREMAP)))

static int prim_vcount[] = {1, 2, 3, 4, 0};

void gaw_draw_indexed(int prim, const unsigned int *idxarr, int nidx)
{
	int i, j, vidx, vnum, nfaces;
	struct vertex v[16];
	int mvtop = st.mtop[GAW_MODELVIEW];
	int ptop = st.mtop[GAW_PROJECTION];
	struct vertex *tmpv;
	const float *vptr;

	if(prim == GAW_QUAD_STRIP) return;	/* TODO */

	if(st.cur_comp >= 0) {
		st.comp[st.cur_comp].prim = prim;
	}

	tmpv = alloca(prim * 6 * sizeof *tmpv);

	/* calc the normal matrix */
	if(NEED_NORMALS) {
		memcpy(st.norm_mat, st.mat[GAW_MODELVIEW][mvtop], 16 * sizeof(float));
		st.norm_mat[12] = st.norm_mat[13] = st.norm_mat[14] = 0.0f;
	}

	vidx = 0;
	nfaces = nidx / prim_vcount[prim];

	for(j=0; j<nfaces; j++) {
		vnum = prim_vcount[prim];	/* reset vnum for each iteration */

		for(i=0; i<vnum; i++) {
			if(idxarr) {
				vidx = *idxarr++;
			}
			vptr = (const float*)((char*)st.vertex_ptr + vidx * st.vertex_stride);
			v[i].x = vptr[0];
			v[i].y = vptr[1];
			v[i].z = st.vertex_nelem > 2 ? vptr[2] : 0.0f;
			v[i].w = st.vertex_nelem > 3 ? vptr[3] : 1.0f;

			if(st.normal_ptr) {
				vptr = (const float*)((char*)st.normal_ptr + vidx * st.normal_stride);
			} else {
				vptr = &st.imm_curv.nx;
			}
			v[i].nx = vptr[0];
			v[i].ny = vptr[1];
			v[i].nz = vptr[2];

			if(st.texcoord_ptr) {
				vptr = (const float*)((char*)st.texcoord_ptr + vidx * st.texcoord_stride);
			} else {
				vptr = &st.imm_curv.u;
			}
			v[i].u = vptr[0];
			v[i].v = vptr[1];

			if(st.color_ptr) {
				vptr = (const float*)((char*)st.color_ptr + vidx * st.color_stride);
			} else {
				vptr = st.imm_curcol;
			}
			v[i].r = (int)(vptr[0] * 255.0f);
			v[i].g = (int)(vptr[1] * 255.0f);
			v[i].b = (int)(vptr[2] * 255.0f);
			v[i].a = st.color_nelem > 3 ? (int)(vptr[3] * 255.0f) : 255;

			vidx++;

			if(st.cur_comp >= 0) {
				/* currently compiling geometry */
				struct comp_geom *cg = st.comp + st.cur_comp;
				float col[4];

				col[0] = v[i].r / 255.0f;
				col[1] = v[i].g / 255.0f;
				col[2] = v[i].b / 255.0f;
				col[3] = v[i].a / 255.0f;

				darr_push(cg->varr, &v[i].x);
				darr_push(cg->varr, &v[i].y);
				darr_push(cg->varr, &v[i].z);
				darr_push(cg->narr, &v[i].nx);
				darr_push(cg->narr, &v[i].ny);
				darr_push(cg->narr, &v[i].nz);
				darr_push(cg->uvarr, &v[i].u);
				darr_push(cg->uvarr, &v[i].v);
				darr_push(cg->carr, col);
				darr_push(cg->carr, col + 1);
				darr_push(cg->carr, col + 2);
				darr_push(cg->carr, col + 3);
				continue;	/* don't transform, just skip to the next vertex */
			}

			xform4_vec3(st.mat[GAW_MODELVIEW][mvtop], &v[i].x);

			if(NEED_NORMALS) {
				xform3_vec3(st.norm_mat, &v[i].nx);
				if(st.opt & (1 << GAW_LIGHTING)) {
					shade(v + i);
				}
				if(st.opt & (1 << GAW_SPHEREMAP)) {
					v[i].u = v[i].nx * 0.5 + 0.5;
					v[i].v = 0.5 - v[i].ny * 0.5;
				}
			}
			{
				float *mat = st.mat[GAW_TEXTURE][st.mtop[GAW_TEXTURE]];
				float x = mat[0] * v[i].u + mat[4] * v[i].v + mat[12];
				float y = mat[1] * v[i].u + mat[5] * v[i].v + mat[13];
				float w = mat[3] * v[i].u + mat[7] * v[i].v + mat[15];
				v[i].u = x / w;
				v[i].v = y / w;
			}
			xform4_vec3(st.mat[GAW_PROJECTION][ptop], &v[i].x);
		}

		if(st.cur_comp >= 0) {
			/* compiling geometry, don't draw, skip to the next primitive */
			continue;
		}

		/* clipping */
		for(i=0; i<6; i++) {
			memcpy(tmpv, v, vnum * sizeof *v);

			if(clip_frustum(v, &vnum, tmpv, vnum, i) < 0) {
				/* polygon completely outside of view volume. discard */
				vnum = 0;
				break;
			}
		}

		if(!vnum) continue;

		for(i=0; i<vnum; i++) {
			if(v[i].w != 0.0f) {
				v[i].x /= v[i].w;
				v[i].y /= v[i].w;
				if(st.opt & (1 << GAW_DEPTH_TEST)) {
					v[i].z /= v[i].w;
				}
			}
		}

		gaw_swtnl_drawprim(prim, v, vnum);
	}
}

void gaw_begin(int prim)
{
	st.imm_prim = prim;
	st.imm_pcount = prim;
	st.imm_numv = 0;
}

void gaw_end(void)
{
	imm_flush();
}

static void imm_flush(void)
{
	int numv = st.imm_numv;
	st.imm_numv = 0;

	gaw_vertex_array(3, sizeof(struct vertex), &st.imm_vbuf->x);
	gaw_normal_array(sizeof(struct vertex), &st.imm_vbuf->nx);
	gaw_texcoord_array(2, sizeof(struct vertex), &st.imm_vbuf->u);
	gaw_color_array(4, 0, st.imm_cbuf);

	gaw_draw_indexed(st.imm_prim, 0, numv);

	gaw_vertex_array(0, 0, 0);
	gaw_normal_array(0, 0);
	gaw_texcoord_array(0, 0, 0);
	gaw_color_array(0, 0, 0);
}

void gaw_color3f(float r, float g, float b)
{
	gaw_color4f(r, g, b, 1.0f);
}

void gaw_color4f(float r, float g, float b, float a)
{
	st.imm_curcol[0] = r;
	st.imm_curcol[1] = g;
	st.imm_curcol[2] = b;
	st.imm_curcol[3] = a;
}

void gaw_color3ub(int r, int g, int b)
{
	st.imm_curcol[0] = r / 255.0f;
	st.imm_curcol[1] = g / 255.0f;
	st.imm_curcol[2] = b / 255.0f;
	st.imm_curcol[3] = 1.0f;
}

void gaw_normal(float x, float y, float z)
{
	st.imm_curv.nx = x;
	st.imm_curv.ny = y;
	st.imm_curv.nz = z;
}

void gaw_texcoord1f(float u)
{
	st.imm_curv.u = u;
	st.imm_curv.v = 0.0f;
}

void gaw_texcoord2f(float u, float v)
{
	st.imm_curv.u = u;
	st.imm_curv.v = v;
}

void gaw_vertex2f(float x, float y)
{
	gaw_vertex3f(x, y, 0);
}

void gaw_vertex3f(float x, float y, float z)
{
	float *cptr = st.imm_cbuf + st.imm_numv * 4;
	struct vertex *vptr = st.imm_vbuf + st.imm_numv++;
	*vptr = st.imm_curv;
	vptr->x = x;
	vptr->y = y;
	vptr->z = z;
	vptr->w = 1.0f;

	cptr[0] = st.imm_curcol[0];
	cptr[1] = st.imm_curcol[1];
	cptr[2] = st.imm_curcol[2];
	cptr[3] = st.imm_curcol[3];

	if(!--st.imm_pcount) {
		if(st.imm_numv >= IMM_VBUF_SIZE - prim_vcount[st.imm_prim]) {
			imm_flush();
		}
		st.imm_pcount = prim_vcount[st.imm_prim];
	}
}

void gaw_rect(float x1, float y1, float x2, float y2)
{
	gaw_begin(GAW_QUADS);
	gaw_vertex2f(x1, y1);
	gaw_vertex2f(x2, y1);
	gaw_vertex2f(x2, y2);
	gaw_vertex2f(x1, y2);
	gaw_end();
}


void gaw_pointsize(float sz)
{
	/* TODO */
}

void gaw_linewidth(float w)
{
	/* TODO */
}

int gaw_compile_begin(void)
{
	int i;

	st.cur_comp = -1;
	for(i=0; i<MAX_COMPILED; i++) {
		if(st.comp[i].varr == 0) {
			st.cur_comp = i;
			break;
		}
	}
	if(st.cur_comp < 0) {
		return 0;
	}

	st.comp[i].prim = -1;
	st.comp[i].varr = darr_alloc(0, sizeof(float));
	st.comp[i].narr = darr_alloc(0, sizeof(float));
	st.comp[i].uvarr = darr_alloc(0, sizeof(float));
	st.comp[i].carr = darr_alloc(0, sizeof(float));

	return st.cur_comp + 1;
}

void gaw_compile_end(void)
{
	st.cur_comp = -1;
}

void gaw_draw_compiled(int id)
{
	int idx = id - 1;

	if(!st.comp[idx].varr || st.comp[idx].prim == -1) {
		return;
	}

	gaw_vertex_array(3, 0, st.comp[idx].varr);
	gaw_normal_array(0, st.comp[idx].narr);
	gaw_texcoord_array(2, 0, st.comp[idx].uvarr);
	gaw_color_array(4, 0, st.comp[idx].carr);

	gaw_draw(st.comp[idx].prim, darr_size(st.comp[idx].varr) / 3);

	gaw_vertex_array(0, 0, 0);
	gaw_normal_array(0, 0);
	gaw_texcoord_array(0, 0, 0);
	gaw_color_array(0, 0, 0);
}

void gaw_free_compiled(int id)
{
	int idx = id - 1;

	darr_free(st.comp[idx].varr);
	darr_free(st.comp[idx].narr);
	darr_free(st.comp[idx].uvarr);
	darr_free(st.comp[idx].carr);
	memset(st.comp + idx, 0, sizeof *st.comp);
}

void gaw_mtl_diffuse(float r, float g, float b, float a)
{
	st.mtl.kd[0] = r;
	st.mtl.kd[1] = g;
	st.mtl.kd[2] = b;
	st.mtl.kd[3] = a;
}

void gaw_mtl_specular(float r, float g, float b, float shin)
{
	st.mtl.ks[0] = r;
	st.mtl.ks[1] = g;
	st.mtl.ks[2] = b;
	st.mtl.shin = shin;
}

void gaw_mtl_emission(float r, float g, float b)
{
	st.mtl.ke[0] = r;
	st.mtl.ke[1] = g;
	st.mtl.ke[2] = b;
}

void gaw_texenv_sphmap(int enable)
{
	if(enable) {
		st.opt |= 1 << GAW_SPHEREMAP;
	} else {
		st.opt &= ~(1 << GAW_SPHEREMAP);
	}
}

void gaw_set_tex1d(unsigned int texid)
{
	if(texid > 0) {
		gaw_bind_tex1d(texid);
		gaw_enable(GAW_TEXTURE_1D);
	} else {
		st.cur_tex = -1;
		gaw_disable(GAW_TEXTURE_1D);
	}
}

void gaw_set_tex2d(unsigned int texid)
{
	if(texid > 0) {
		gaw_bind_tex2d(texid);
		gaw_enable(GAW_TEXTURE_2D);
	} else {
		st.cur_tex = -1;
		gaw_disable(GAW_TEXTURE_2D);
	}
}

void gaw_ambient(float r, float g, float b)
{
	st.ambient[0] = r;
	st.ambient[1] = g;
	st.ambient[2] = b;
}

void gaw_light_pos(int idx, float x, float y, float z)
{
	int mvtop = st.mtop[GAW_MODELVIEW];

	st.lt[idx].type = LT_POS;
	st.lt[idx].x = x;
	st.lt[idx].y = y;
	st.lt[idx].z = z;

	xform4_vec3(st.mat[GAW_MODELVIEW][mvtop], &st.lt[idx].x);
}

void gaw_light_dir(int idx, float x, float y, float z)
{
	int mvtop = st.mtop[GAW_MODELVIEW];

	st.lt[idx].type = LT_DIR;
	st.lt[idx].x = x;
	st.lt[idx].y = y;
	st.lt[idx].z = z;

	/* calc the normal matrix */
	memcpy(st.norm_mat, st.mat[GAW_MODELVIEW][mvtop], 16 * sizeof(float));
	st.norm_mat[12] = st.norm_mat[13] = st.norm_mat[14] = 0.0f;

	xform4_vec3(st.norm_mat, &st.lt[idx].x);

	NORMALIZE(&st.lt[idx].x);
}

void gaw_light_color(int idx, float r, float g, float b, float s)
{
	st.lt[idx].r = r;
	st.lt[idx].g = g;
	st.lt[idx].b = b;
}

void gaw_lighting_fast(void)
{
}

void gaw_fog_color(float r, float g, float b)
{
}

void gaw_fog_linear(float z0, float z1)
{
}

void gaw_fog_fast(void)
{
}


void gaw_poly_wire(void)
{
	st.polymode = POLYFILL_WIRE;
}

void gaw_poly_flat(void)
{
	st.polymode = POLYFILL_FLAT;
}

void gaw_poly_gouraud(void)
{
	st.polymode = POLYFILL_GOURAUD;
}


static __inline void xform4_vec3(const float *mat, float *vec)
{
	float x = mat[0] * vec[0] + mat[4] * vec[1] + mat[8] * vec[2] + mat[12];
	float y = mat[1] * vec[0] + mat[5] * vec[1] + mat[9] * vec[2] + mat[13];
	float z = mat[2] * vec[0] + mat[6] * vec[1] + mat[10] * vec[2] + mat[14];
	vec[3] = mat[3] * vec[0] + mat[7] * vec[1] + mat[11] * vec[2] + mat[15];
	vec[2] = z;
	vec[1] = y;
	vec[0] = x;
}

static __inline void xform3_vec3(const float *mat, float *vec)
{
	float x = mat[0] * vec[0] + mat[4] * vec[1] + mat[8] * vec[2];
	float y = mat[1] * vec[0] + mat[5] * vec[1] + mat[9] * vec[2];
	vec[2] = mat[2] * vec[0] + mat[6] * vec[1] + mat[10] * vec[2];
	vec[1] = y;
	vec[0] = x;
}

static void shade(struct vertex *v)
{
	int i, r, g, b;
	float color[3];

	color[0] = st.ambient[0] * st.mtl.kd[0];
	color[1] = st.ambient[1] * st.mtl.kd[1];
	color[2] = st.ambient[2] * st.mtl.kd[2];

	for(i=0; i<MAX_LIGHTS; i++) {
		float ldir[3];
		float ndotl;

		if(!(st.opt & (GAW_LIGHT0 << i))) {
			continue;
		}

		ldir[0] = st.lt[i].x;
		ldir[1] = st.lt[i].y;
		ldir[2] = st.lt[i].z;

		if(st.lt[i].type != LT_DIR) {
			ldir[0] -= v->x;
			ldir[1] -= v->y;
			ldir[2] -= v->z;
			NORMALIZE(ldir);
		}

		if((ndotl = v->nx * ldir[0] + v->ny * ldir[1] + v->nz * ldir[2]) < 0.0f) {
			ndotl = 0.0f;
		}

		color[0] += st.mtl.kd[0] * st.lt[i].r * ndotl;
		color[1] += st.mtl.kd[1] * st.lt[i].g * ndotl;
		color[2] += st.mtl.kd[2] * st.lt[i].b * ndotl;

		/*
		if(st.opt & (1 << GAW_SPECULAR)) {
			float ndoth;
			ldir[2] += 1.0f;
			NORMALIZE(ldir);
			if((ndoth = v->nx * ldir[0] + v->ny * ldir[1] + v->nz * ldir[2]) < 0.0f) {
				ndoth = 0.0f;
			}
			ndoth = pow(ndoth, st.mtl.shin);

			color[0] += st.mtl.ks[0] * st.lt[i].r * ndoth;
			color[1] += st.mtl.ks[1] * st.lt[i].g * ndoth;
			color[2] += st.mtl.ks[2] * st.lt[i].b * ndoth;
		}
		*/
	}

	r = cround64(color[0] * 255.0);
	g = cround64(color[1] * 255.0);
	b = cround64(color[2] * 255.0);

	v->r = r > 255 ? 255 : r;
	v->g = g > 255 ? 255 : g;
	v->b = b > 255 ? 255 : b;
}
