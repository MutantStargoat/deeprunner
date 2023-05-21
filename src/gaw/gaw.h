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
#ifndef GRAPHICS_API_WRAPPER_H_
#define GRAPHICS_API_WRAPPER_H_

enum {
	GAW_COLORBUF	= 1,
	GAW_DEPTHBUF	= 2,
	GAW_STENCILBUF	= 4
};

enum {
	GAW_POINTS,
	GAW_LINES,
	GAW_TRIANGLES,
	GAW_QUADS,
	GAW_QUAD_STRIP
};

enum {
	GAW_DIFFUSE,
	GAW_SPECULAR,
	GAW_SHININESS,
	GAW_EMISSION
};

enum {
	GAW_MODELVIEW,
	GAW_PROJECTION,
	GAW_TEXTURE
};

enum {
	GAW_CULL_FACE,
	GAW_DEPTH_TEST,
	GAW_ALPHA_TEST,
	GAW_BLEND,
	GAW_FOG,
	GAW_DITHER,
	GAW_LIGHTING,
	GAW_LIGHT0,
	GAW_LIGHT1,
	GAW_LIGHT2,
	GAW_LIGHT3,
	GAW_TEXTURE_1D,
	GAW_TEXTURE_2D
};

enum {
	GAW_NEVER,
	GAW_LESS,
	GAW_EQUAL,
	GAW_LEQUAL,
	GAW_GREATER,
	GAW_NOTEQUAL,
	GAW_GEQUAL,
	GAW_ALWAYS
};

enum {
	GAW_ZERO,
	GAW_ONE,
	GAW_SRC_ALPHA,
	GAW_ONE_MINUS_SRC_ALPHA
};

enum {
	GAW_LUMINANCE,
	GAW_RGB,
	GAW_RGBA
};

enum {
	GAW_NEAREST,
	GAW_BILINEAR,
	GAW_TRILINEAR
};

enum {
	GAW_REPEAT,
	GAW_CLAMP
};

void gaw_viewport(int x, int y, int w, int h);

void gaw_matrix_mode(int mode);
void gaw_load_identity(void);
void gaw_load_matrix(const float *m);
void gaw_mult_matrix(const float *m);
void gaw_push_matrix(void);
void gaw_pop_matrix(void);
void gaw_get_modelview(float *m);
void gaw_get_projection(float *m);

void gaw_translate(float x, float y, float z);
void gaw_rotate(float angle, float x, float y, float z);
void gaw_scale(float sx, float sy, float sz);
void gaw_ortho(float l, float r, float b, float t, float n, float f);
void gaw_frustum(float l, float r, float b, float t, float n, float f);
void gaw_perspective(float vfov, float aspect, float znear, float zfar);

void gaw_save(void);
void gaw_restore(void);
void gaw_enable(int st);
void gaw_disable(int st);

void gaw_depth_func(int func);
void gaw_blend_func(int src, int dest);
void gaw_alpha_func(int func, float ref);

void gaw_clear_color(float r, float g, float b, float a);
void gaw_clear(unsigned int flags);
void gaw_depth_mask(int mask);

void gaw_vertex_array(int nelem, int stride, const void *ptr);
void gaw_normal_array(int stride, const void *ptr);
void gaw_texcoord_array(int nelem, int stride, const void *ptr);
void gaw_color_array(int nelem, int stride, const void *ptr);
void gaw_draw(int prim, int nverts);
void gaw_draw_indexed(int prim, const unsigned int *idxarr, int nidx);

void gaw_begin(int prim);
void gaw_end(void);
void gaw_color3f(float r, float g, float b);
void gaw_color4f(float r, float g, float b, float a);
void gaw_color3ub(int r, int g, int b);
void gaw_normal(float x, float y, float z);
void gaw_texcoord1f(float u);
void gaw_texcoord2f(float u, float v);
void gaw_vertex2f(float x, float y);
void gaw_vertex3f(float x, float y, float z);

void gaw_rect(float x1, float y1, float x2, float y2);

void gaw_pointsize(float sz);
void gaw_linewidth(float w);

int gaw_compile_begin(void);
void gaw_compile_end(void);
void gaw_draw_compiled(int id);
void gaw_free_compiled(int id);

void gaw_mtl_diffuse(float r, float g, float b, float a);
void gaw_mtl_specular(float r, float g, float b, float shin);
void gaw_mtl_emission(float r, float g, float b);
void gaw_texenv_sphmap(int enable);

unsigned int gaw_create_tex1d(int texfilter);
unsigned int gaw_create_tex2d(int texfilter);
void gaw_destroy_tex(unsigned int tex);

void gaw_bind_tex1d(int tex);
void gaw_bind_tex2d(int tex);

void gaw_texfilter1d(int texfilter);
void gaw_texfilter2d(int texfilter);

void gaw_texwrap1d(int wrap);
void gaw_texwrap2d(int uwrap, int vwrap);

void gaw_tex1d(int ifmt, int xsz, int fmt, void *pix);
void gaw_tex2d(int ifmt, int xsz, int ysz, int fmt, void *pix);
void gaw_subtex2d(int lvl, int x, int y, int xsz, int ysz, int fmt, void *pix);

void gaw_set_tex1d(unsigned int texid);
void gaw_set_tex2d(unsigned int texid);

void gaw_ambient(float r, float g, float b);
void gaw_light_dir(int idx, float x, float y, float z);
void gaw_light_color(int idx, float r, float g, float b, float s);
void gaw_lighting_fast(void);

void gaw_fog_color(float r, float g, float b);
void gaw_fog_linear(float z0, float z1);
void gaw_fog_fast(void);

void gaw_poly_wire(void);
void gaw_poly_flat(void);
void gaw_poly_gouraud(void);

#endif	/* GRAPHICS_API_WRAPPER_H_ */
