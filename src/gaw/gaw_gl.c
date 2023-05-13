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
#include "gaw.h"

#if defined(WIN32) || defined(__WIN32)
#include <windows.h>
#endif

#include <GL/gl.h>
#include <GL/glu.h>


static float *vertex_ptr, *normal_ptr, *texcoord_ptr;


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

void gaw_setupdraw(void *varr, void *narr, void *uvarr)
{
	vertex_ptr = varr;
	normal_ptr = narr;
	texcoord_ptr = uvarr;
}

static int glprim[] = {GL_POINTS, GL_LINES, GL_TRIANGLES, GL_QUADS};

void gaw_draw(int prim, int nverts)
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

	glDrawArrays(glprim[prim], 0, nverts);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void gaw_draw_indexed(int prim, unsigned int *idxarr, int nidx)
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

	glDrawElements(glprim[prim], nidx, GL_UNSIGNED_INT, idxarr);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
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
