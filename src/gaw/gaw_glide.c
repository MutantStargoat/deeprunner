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
#include <stdlib.h>
#include <glide.h>
#include "gaw.h"
#include "gawswtnl.h"

static GrHwConfiguration hwcfg;

void gaw_glide_reset(void)
{
	gaw_swtnl_reset();
}

void gaw_glide_init(int xsz, int ysz)
{
	int i, res = -1;
	static const struct { int xsz, ysz, res; } glideres[] = {
		{640, 480, GR_RESOLUTION_640x480},
		{800, 600, GR_RESOLUTION_800x600},
		{1024, 768, GR_RESOLUTION_1024x768}
	};

	for(i=0; i<sizeof glideres/sizeof *glideres; i++) {
		if(xsz == glideres[i].xsz && ysz == glideres[i].ysz) {
			res = glideres[i].res;
			break;
		}
	}
	if(res == -1) {
		fprintf(stderr, "Unsupported resolution: %dx%d\n", xsz, ysz);
		abort();
	}

	gaw_swtnl_init();

	if(!grSstQueryBoards(&hwcfg)) {
		fprintf(stderr, "No 3dfx graphics board detected!\n");
		abort();
	}

	grGlideInit();
	if(!grSstQueryHardware(&hwcfg)) {
		fprintf(stderr, "No 3dfx graphics board detected!\n");
		abort();
	}

	printf("Found %d 3dfx graphics board(s)\n", hwcfg.num_sst);

	grSstSelect(0);

	if(!grSstWinOpen(0, res, GR_REFRESH_60Hz, GR_COLORFORMAT_ARGB,
			GR_ORIGIN_UPPER_LEFT, 2, 1)) {
		fprintf(stderr, "Failed to initialize 3dfx device\n");
		abort();
	}

	ST->width = xsz;
	ST->height = ysz;
}

void gaw_glide_destroy(void)
{
	gaw_swtnl_destroy();
}

void gaw_enable(int what)
{
	switch(what) {
	case GAW_DEPTH_TEST:
		grDepthBufferMode(GR_DEPTHBUFFER_WBUFFER);
		break;

	default:
		break;
	}

	gaw_swtnl_enable(what);
}

void gaw_disable(int what)
{
	switch(what) {
	case GAW_DEPTH_TEST:
		grDepthBufferMode(GR_DEPTHBUFFER_DISABLE);
		break;

	default:
		break;
	}

	gaw_swtnl_disable(what);
}

void gaw_clear(unsigned int flags)
{
	if(!(flags & GAW_COLORBUF)) {
		grColorMask(FXFALSE, FXFALSE);
	}
	if(flags & GAW_DEPTHBUF) {
		grDepthBufferMode(GR_DEPTHBUFFER_WBUFFER);
		grDepthMask(FXTRUE);
	}

	grBufferClear(ST->clear_color, 0xff, GR_WDEPTHVALUE_FARTHEST);

	if(flags & GAW_DEPTHBUF) {
		grDepthBufferMode(!(ST->opt & GAW_DEPTH_TEST) ? GR_DEPTHBUFFER_DISABLE : GR_DEPTHBUFFER_WBUFFER);
		/* TODO also revert depth mask */
	}
	if(!(flags & GAW_COLORBUF)) {
		grColorMask(FXTRUE, FXFALSE);
	}
		
}

void gaw_color_mask(int rmask, int gmask, int bmask, int amask)
{
	int rgbmask = rmask | gmask | bmask;
	grColorMask(rgbmask ? FXTRUE : FXFALSE, FXFALSE);
}

void gaw_depth_mask(int mask)
{
	grDepthMask(mask ? FXTRUE : FXFALSE);
}

void gaw_bind_tex1d(int tex)
{
	ST->cur_tex = (int)tex - 1;
	/* TODO */
}

void gaw_bind_tex2d(int tex)
{
	ST->cur_tex = (int)tex - 1;
	/* TODO */
}

void gaw_swtnl_drawprim(int prim, struct vertex *v, int vnum)
{
	int i;
	GrVertex vert[16];

	for(i=0; i<vnum; i++) {
		/* viewport transformation */
		vert[i].x = (v[i].x * 0.5f + 0.5f) * (float)ST->vport[2] + ST->vport[0];
		vert[i].y = (v[i].y * 0.5f + 0.5f) * (float)ST->vport[3] + ST->vport[1];
		vert[i].y = ST->height - vert[i].y - 1;

		vert[i].oow = 1.0f / v[i].w;

		vert[i].r = v->r;
		vert[i].g = v->g;
		vert[i].b = v->b;
		vert[i].a = v->a;

		if(i >= 2) {
			grDrawTriangle(vert, vert + (i - 1), vert + i);
		}
	}
}
