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
	GAW_QUADS
};

void gaw_clear(unsigned int flags);

void gaw_setupdraw(void *varr, void *narr, void *uvarr);
void gaw_draw(int prim, int nverts);
void gaw_draw_indexed(int prim, unsigned int *idxarr, int nidx);

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

int gaw_compile_begin(void);
void gaw_compile_end(void);
void gaw_draw_compiled(int id);
void gaw_free_compiled(int id);

#endif	/* GRAPHICS_API_WRAPPER_H_ */
