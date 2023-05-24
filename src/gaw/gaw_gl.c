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
#include <string.h>
#include "util.h"
#include "gaw.h"
#include "opengl.h"


static const float *vertex_ptr, *normal_ptr, *texcoord_ptr, *color_ptr;
static int vertex_nelem, texcoord_nelem, color_nelem;
static int vertex_stride, normal_stride, texcoord_stride, color_stride;

static char *glextstr;
static int have_edgeclamp = -1;


void gaw_viewport(int x, int y, int w, int h)
{
	glViewport(x, y, w, h);
}

void gaw_matrix_mode(int mode)
{
	mode += GL_MODELVIEW;
	glMatrixMode(mode);
}

void gaw_load_identity(void)
{
	glLoadIdentity();
}

void gaw_load_matrix(const float *m)
{
	glLoadMatrixf(m);
}

void gaw_mult_matrix(const float *m)
{
	glMultMatrixf(m);
}

void gaw_push_matrix(void)
{
	glPushMatrix();
}

void gaw_pop_matrix(void)
{
	glPopMatrix();
}

void gaw_get_modelview(float *m)
{
	glGetFloatv(GL_MODELVIEW_MATRIX, m);
}

void gaw_get_projection(float *m)
{
	glGetFloatv(GL_PROJECTION_MATRIX, m);
}

void gaw_translate(float x, float y, float z)
{
	glTranslatef(x, y, z);
}

void gaw_rotate(float angle, float x, float y, float z)
{
	glRotatef(angle, x, y, z);
}

void gaw_scale(float sx, float sy, float sz)
{
	glScalef(sx, sy, sz);
}

void gaw_ortho(float l, float r, float b, float t, float n, float f)
{
	glOrtho(l, r, b, t, n, f);
}

void gaw_frustum(float l, float r, float b, float t, float n, float f)
{
	glFrustum(l, r, b, t, n, f);
}

void gaw_perspective(float vfov, float aspect, float znear, float zfar)
{
	gluPerspective(vfov, aspect, znear, zfar);
}

void gaw_save(void)
{
	glPushAttrib(GL_ENABLE_BIT);
}

void gaw_restore(void)
{
	glPopAttrib();
}

void gaw_enable(int st)
{
	switch(st) {
	case GAW_CULL_FACE:
		glEnable(GL_CULL_FACE);
		break;
	case GAW_DEPTH_TEST:
		glEnable(GL_DEPTH_TEST);
		break;
	case GAW_ALPHA_TEST:
		glEnable(GL_ALPHA_TEST);
		break;
	case GAW_BLEND:
		glEnable(GL_BLEND);
		break;
	case GAW_FOG:
		glEnable(GL_FOG);
		break;
	case GAW_DITHER:
		glEnable(GL_DITHER);
		break;
	case GAW_LIGHTING:
		glEnable(GL_LIGHTING);
		break;
	case GAW_LIGHT0:
		glEnable(GL_LIGHT0);
		break;
	case GAW_LIGHT1:
		glEnable(GL_LIGHT1);
		break;
	case GAW_LIGHT2:
		glEnable(GL_LIGHT2);
		break;
	case GAW_LIGHT3:
		glEnable(GL_LIGHT3);
		break;
	case GAW_TEXTURE_1D:
		glEnable(GL_TEXTURE_1D);
		break;
	case GAW_TEXTURE_2D:
		glEnable(GL_TEXTURE_2D);
		break;
	default:
		break;
	}
}

void gaw_disable(int st)
{
	switch(st) {
	case GAW_CULL_FACE:
		glDisable(GL_CULL_FACE);
		break;
	case GAW_DEPTH_TEST:
		glDisable(GL_DEPTH_TEST);
		break;
	case GAW_ALPHA_TEST:
		glDisable(GL_ALPHA_TEST);
		break;
	case GAW_BLEND:
		glDisable(GL_BLEND);
		break;
	case GAW_FOG:
		glDisable(GL_FOG);
		break;
	case GAW_DITHER:
		glDisable(GL_DITHER);
		break;
	case GAW_LIGHTING:
		glDisable(GL_LIGHTING);
		break;
	case GAW_LIGHT0:
		glDisable(GL_LIGHT0);
		break;
	case GAW_LIGHT1:
		glDisable(GL_LIGHT1);
		break;
	case GAW_LIGHT2:
		glDisable(GL_LIGHT2);
		break;
	case GAW_LIGHT3:
		glDisable(GL_LIGHT3);
		break;
	case GAW_TEXTURE_1D:
		glDisable(GL_TEXTURE_1D);
		break;
	case GAW_TEXTURE_2D:
		glDisable(GL_TEXTURE_2D);
		break;
	default:
		break;
	}
}

void gaw_depth_func(int func)
{
	glDepthFunc(func + GL_NEVER);
}

void gaw_blend_func(int src, int dest)
{
	static const int glbf[] = {GL_ZERO, GL_ONE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA};
	glBlendFunc(glbf[src], glbf[dest]);
}

void gaw_alpha_func(int func, float ref)
{
	glAlphaFunc(func + GL_NEVER, ref);
}

void gaw_clear_color(float r, float g, float b, float a)
{
	glClearColor(r, g, b, a);
}

void gaw_clear(unsigned int flags)
{
	unsigned int glflags = 0;
	if(flags & GAW_COLORBUF) {
		glflags |= GL_COLOR_BUFFER_BIT;
	}
	if(flags & GAW_DEPTHBUF) {
		glflags |= GL_DEPTH_BUFFER_BIT;
	}
	if(flags & GAW_STENCILBUF) {
		glflags |= GL_STENCIL_BUFFER_BIT;
	}
	glClear(glflags);
}

void gaw_depth_mask(int mask)
{
	glDepthMask(mask);
}

void gaw_vertex_array(int nelem, int stride, const void *ptr)
{
	vertex_nelem = nelem;
	vertex_stride = stride;
	vertex_ptr = ptr;
}

void gaw_normal_array(int stride, const void *ptr)
{
	normal_stride = stride;
	normal_ptr = ptr;
}

void gaw_texcoord_array(int nelem, int stride, const void *ptr)
{
	texcoord_nelem = nelem;
	texcoord_stride = stride;
	texcoord_ptr = ptr;
}

void gaw_color_array(int nelem, int stride, const void *ptr)
{
	color_nelem = nelem;
	color_stride = stride;
	color_ptr = ptr;
}

static int glprim[] = {GL_POINTS, GL_LINES, GL_TRIANGLES, GL_QUADS, GL_QUAD_STRIP};

void gaw_draw(int prim, int nverts)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(vertex_nelem, GL_FLOAT, vertex_stride, vertex_ptr);
	if(normal_ptr) {
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, normal_stride, normal_ptr);
	}
	if(texcoord_ptr) {
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(texcoord_nelem, GL_FLOAT, texcoord_stride, texcoord_ptr);
	}
	if(color_ptr) {
		glEnableClientState(GL_COLOR_ARRAY);
		glTexCoordPointer(color_nelem, GL_FLOAT, color_stride, color_ptr);
	}

	glDrawArrays(glprim[prim], 0, nverts);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}

void gaw_draw_indexed(int prim, const unsigned int *idxarr, int nidx)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, vertex_ptr);
	if(normal_ptr) {
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, 0, normal_ptr);
	}
	if(texcoord_ptr) {
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, texcoord_ptr);
	}
	if(color_ptr) {
		glEnableClientState(GL_COLOR_ARRAY);
		glTexCoordPointer(color_nelem, GL_FLOAT, color_stride, color_ptr);
	}

	glDrawElements(glprim[prim], nidx, GL_UNSIGNED_INT, idxarr);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}

void gaw_begin(int prim)
{
	glBegin(glprim[prim]);
}

void gaw_end(void)
{
	glEnd();
}

void gaw_color3f(float r, float g, float b)
{
	glColor3f(r, g, b);
}

void gaw_color4f(float r, float g, float b, float a)
{
	glColor4f(r, g, b, a);
}

void gaw_color3ub(int r, int g, int b)
{
	glColor3ub(r, g, b);
}

void gaw_normal(float x, float y, float z)
{
	glNormal3f(x, y, z);
}

void gaw_texcoord1f(float u)
{
	glTexCoord1f(u);
}

void gaw_texcoord2f(float u, float v)
{
	glTexCoord2f(u, v);
}

void gaw_vertex2f(float x, float y)
{
	glVertex2f(x, y);
}

void gaw_vertex3f(float x, float y, float z)
{
	glVertex3f(x, y, z);
}

void gaw_rect(float x1, float y1, float x2, float y2)
{
	glRectf(x1, y1, x2, y2);
}

void gaw_pointsize(float sz)
{
	glPointSize(sz);
}

void gaw_linewidth(float w)
{
	glLineWidth(w);
}

int gaw_compile_begin(void)
{
	int dlist = glGenLists(1);
	glNewList(dlist, GL_COMPILE);
	return dlist;
}

void gaw_compile_end(void)
{
	glEndList();
}

void gaw_draw_compiled(int id)
{
	glCallList(id);
}

void gaw_free_compiled(int id)
{
	glDeleteLists(id, 1);
}

void gaw_mtl_diffuse(float r, float g, float b, float a)
{
	float v[4];
	v[0] = r;
	v[1] = g;
	v[2] = b;
	v[3] = a;
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, v);
}

void gaw_mtl_specular(float r, float g, float b, float shin)
{
	float v[4];
	v[0] = r;
	v[1] = g;
	v[2] = b;
	v[3] = 1.0f;
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, v);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shin);
}

void gaw_mtl_emission(float r, float g, float b)
{
	float v[4];
	v[0] = r;
	v[1] = g;
	v[2] = b;
	v[3] = 1.0f;
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, v);
}

void gaw_texenv_sphmap(int enable)
{
	if(enable) {
		glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
		glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
		glEnable(GL_TEXTURE_GEN_S);
		glEnable(GL_TEXTURE_GEN_T);
	} else {
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
	}
}

unsigned int gaw_create_tex1d(int texfilter)
{
	unsigned int tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_1D, tex);
	gaw_texfilter1d(texfilter);
	return tex;
}

unsigned int gaw_create_tex2d(int texfilter)
{
	unsigned int tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	gaw_texfilter2d(texfilter);
	return tex;
}

void gaw_destroy_tex(unsigned int tex)
{
	glDeleteTextures(1, &tex);
}

void gaw_bind_tex1d(int tex)
{
	glBindTexture(GL_TEXTURE_1D, tex);
}

void gaw_bind_tex2d(int tex)
{
	glBindTexture(GL_TEXTURE_2D, tex);
}

void gaw_texfilter1d(int texfilter)
{
	switch(texfilter) {
	case GAW_NEAREST:
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		break;

	case GAW_BILINEAR:
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		break;

	case GAW_TRILINEAR:
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		break;

	default:
		break;
	}
}

void gaw_texfilter2d(int texfilter)
{
	switch(texfilter) {
	case GAW_NEAREST:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		break;

	case GAW_BILINEAR:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		break;

	case GAW_TRILINEAR:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		break;

	default:
		break;
	}
}

static int glwrap(int wrap)
{
	if(have_edgeclamp == -1) {
		if(!glextstr) {
			glextstr = strdup_nf((char*)glGetString(GL_EXTENSIONS));
		}
		have_edgeclamp = strstr(glextstr, "SGIS_texture_edge_clamp") != 0;
	}

	switch(wrap) {
	case GAW_CLAMP:
		if(have_edgeclamp) {
			return GL_CLAMP_TO_EDGE;
		} else {
			return GL_CLAMP;
		}
		break;

	case GAW_REPEAT:
	default:
		break;
	}
	return GL_REPEAT;
}

void gaw_texwrap1d(int wrap)
{
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, glwrap(wrap));
}

void gaw_texwrap2d(int uwrap, int vwrap)
{
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, glwrap(uwrap));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, glwrap(vwrap));
}


static const int glfmt[] = {GL_LUMINANCE, GL_RGB, GL_RGBA};

void gaw_tex1d(int ifmt, int xsz, int fmt, void *pix)
{
	gluBuild1DMipmaps(GL_TEXTURE_1D, glfmt[ifmt], xsz, glfmt[fmt], GL_UNSIGNED_BYTE, pix);
}

void gaw_tex2d(int ifmt, int xsz, int ysz, int fmt, void *pix)
{
	gluBuild2DMipmaps(GL_TEXTURE_2D, glfmt[ifmt], xsz, ysz, glfmt[fmt], GL_UNSIGNED_BYTE, pix);
}

void gaw_subtex2d(int lvl, int x, int y, int xsz, int ysz, int fmt, void *pix)
{
	glTexSubImage2D(GL_TEXTURE_2D, lvl, x, y, xsz, ysz, glfmt[fmt], GL_UNSIGNED_BYTE, pix);
}

void gaw_set_tex1d(unsigned int texid)
{
	if(texid) {
		glEnable(GL_TEXTURE_1D);
		glBindTexture(GL_TEXTURE_1D, texid);
	} else {
		glDisable(GL_TEXTURE_1D);
	}
}

void gaw_set_tex2d(unsigned int texid)
{
	if(texid) {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texid);
	} else {
		glDisable(GL_TEXTURE_2D);
	}
}

void gaw_ambient(float r, float g, float b)
{
	float amb[4];
	amb[0] = r;
	amb[1] = g;
	amb[2] = b;
	amb[3] = 1.0f;
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb);
}

void gaw_light_dir(int idx, float x, float y, float z)
{
	float pos[4];
	pos[0] = x;
	pos[1] = y;
	pos[2] = z;
	pos[3] = 0;
	glLightfv(GL_LIGHT0 + idx, GL_POSITION, pos);

}

void gaw_light_color(int idx, float r, float g, float b, float s)
{
	float color[4];
	color[0] = r * s;
	color[1] = g * s;
	color[2] = b * s;
	color[3] = 1;
	glLightfv(GL_LIGHT0 + idx, GL_DIFFUSE, color);
	glLightfv(GL_LIGHT0 + idx, GL_SPECULAR, color);
}

void gaw_lighting_fast(void)
{
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 0);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);
}


void gaw_fog_color(float r, float g, float b)
{
	float col[4];
	col[0] = r;
	col[1] = g;
	col[2] = b;
	col[3] = 1.0f;
	glFogfv(GL_FOG_COLOR, col);
}

void gaw_fog_linear(float z0, float z1)
{
	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogf(GL_FOG_START, z0);
	glFogf(GL_FOG_END, z1);
}

void gaw_fog_fast(void)
{
	glHint(GL_FOG_HINT, GL_FASTEST);
}

void gaw_poly_wire(void)
{
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

void gaw_poly_flat(void)
{
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glShadeModel(GL_FLAT);
}

void gaw_poly_gouraud(void)
{
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glShadeModel(GL_SMOOTH);
}
