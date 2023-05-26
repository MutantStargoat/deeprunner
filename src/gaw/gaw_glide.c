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
#include "glide.h"
#include "gaw.h"
#include "gawswtnl.h"

struct texture {
	int idx;
	int fmt;
	long size;
	long addr;	/* -1 if not resident */
};

static void print_board_info(GrHwConfiguration *hw, int idx);
static void init_texman(void);

static GrHwConfiguration hwcfg;
static int num_tmu;


void gaw_glide_reset(void)
{
	gaw_swtnl_reset();
}

static const char *sstnames[] = {"Voodoo", "SST96", "ATB", "Voodoo2"};

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
	for(i=0; i<hwcfg.num_sst; i++) {
		print_board_info(&hwcfg, i);
	}

	grSstSelect(0);

	switch(hwcfg.SSTs[0].type) {
	case GR_SSTTYPE_VOODOO:
	case GR_SSTTYPE_Voodoo2:
		num_tmu = hwcfg.SSTs[0].sstBoard.VoodooConfig.nTexelfx;
		break;
	case GR_SSTTYPE_SST96:
		num_tmu = hwcfg.SSTs[0].sstBoard.SST96Config.nTexelfx;
		break;
	default:
		num_tmu = 1;
	}

	if(!grSstWinOpen(0, res, GR_REFRESH_60Hz, GR_COLORFORMAT_ARGB,
			GR_ORIGIN_UPPER_LEFT, 2, 1)) {
		fprintf(stderr, "Failed to initialize 3dfx device\n");
		abort();
	}

	ST->width = xsz;
	ST->height = ysz;

	init_texman();
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

static int alloc_tex(void)
{
	int i;
	for(i=0; i<MAX_TEXTURES; i++) {
		if(st.textypes[i] == 0) {
			return i;
		}
	}
	return -1;
}

unsigned int gaw_create_tex1d(int texfilter)
{
	int idx;
	if((idx = alloc_tex()) == -1) {
		return 0;
	}
	st.textypes[idx] = 1;

	memset(st.textures + idx, 0, sizeof *st.textures);
	st.cur_tex = idx;
	return idx + 1;
}

unsigned int gaw_create_tex2d(int texfilter)
{
	int idx;
	if((idx = alloc_tex()) == -1) {
		return 0;
	}
	st.textypes[idx] = 2;

	memset(st.textures + idx, 0, sizeof *st.textures);
	st.cur_tex = idx;
	return idx + 1;
}

void gaw_destroy_tex(unsigned int texid)
{
	int idx = texid - 1;

	if(!st.textypes[idx]) return;

	free(st.textures[idx].pixels);
	st.textypes[idx] = 0;
}

void gaw_texfilter1d(int texfilter)
{
}

void gaw_texfilter2d(int texfilter)
{
}

void gaw_texwrap1d(int wrap)
{
}

void gaw_texwrap2d(int uwrap, int vwrap)
{
}


static __inline int calc_shift(unsigned int x)
{
	int res = -1;
	while(x) {
		x >>= 1;
		++res;
	}
	return res;
}

void gaw_swtnl_tex1d(int ifmt, int xsz, int fmt, void *pix)
{
	gaw_tex2d(ifmt, xsz, 1, fmt, pix);
}

void gaw_swtnl_tex2d(int ifmt, int xsz, int ysz, int fmt, void *pix)
{
	int npix;
	struct pimage *img;

	if(st.cur_tex < 0) return;
	img = st.textures + st.cur_tex;

	npix = xsz * ysz;

	free(img->pixels);
	img->pixels = malloc_nf(npix * sizeof *img->pixels);
	img->width = xsz;
	img->height = ysz;

	img->xmask = xsz - 1;
	img->ymask = ysz - 1;
	img->xshift = calc_shift(xsz);
	img->yshift = calc_shift(ysz);

	gaw_subtex2d(0, 0, 0, xsz, ysz, fmt, pix);
}

void gaw_subtex2d(int lvl, int x, int y, int xsz, int ysz, int fmt, void *pix)
{
	int i, j, r, g, b, val;
	uint32_t *dest;
	unsigned char *src;
	struct pimage *img;

	if(st.cur_tex < 0) return;
	img = st.textures + st.cur_tex;

	dest = img->pixels + (y << img->xshift) + x;
	src = pix;

	switch(fmt) {
	case GAW_LUMINANCE:
		for(i=0; i<ysz; i++) {
			for(j=0; j<xsz; j++) {
				val = *src++;
				dest[j] = PACK_RGBA(val, val, val, 255);
			}
			dest += img->width;
		}
		break;

	case GAW_RGB:
		for(i=0; i<ysz; i++) {
			for(j=0; j<xsz; j++) {
				b = src[0];
				g = src[1];
				r = src[2];
				src += 3;
				dest[j] = PACK_RGBA(r, g, b, 255);
			}
			dest += img->width;
		}
		break;

	case GAW_RGBA:
		for(i=0; i<ysz; i++) {
			for(j=0; j<xsz; j++) {
				dest[j] = *((uint32_t*)src);
				src += 4;
			}
			dest += img->width;
		}
		break;

	default:
		break;
	}
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

		vert[i].tmuvtx[0].sow = v[i].u * 256.0f / v[i].w;
		vert[i].tmuvtx[0].tow = v[i].v * 256.0f / v[i].w;
		vert[i].tmuvtx[0].oow = (float)0xfff / v[i].w;

		if(i >= 2) {
			grDrawTriangle(vert, vert + (i - 1), vert + i);
		}
	}
}

static void print_board_info(GrHwConfiguration *hw, int idx)
{
	int i, texmem, totalmem;
	int type = hw->SSTs[idx].type;
	const char *name = type < sizeof sstnames / sizeof *sstnames ? sstnames[type] : "unknown";
	GrVoodooConfig_t *voodoo;

	printf(" - 3dfx board %d: %s ", idx, name);
	switch(type) {
	case GR_SSTTYPE_VOODOO:
	case GR_SSTTYPE_Voodoo2:
		voodoo = &hw->SSTs[idx].sstBoard.VoodooConfig;
		if(voodoo->sliDetect) {
			printf("SLI ");
		}
		texmem = 0;
		for(i=0; i<voodoo->nTexelfx; i++) {
			texmem += voodoo->tmuConfig[i].tmuRam;
		}
		totalmem = voodoo->fbRam + texmem;
		printf("%dmb (%dmb texture RAM, %d tex units)\n", totalmem, texmem,
				voodoo->nTexelfx);
		break;

	default:
		putchar('\n');	/* TODO */
	}
}

/* texture memory manager */
struct tmumem {
	uint32_t start, size;
} tmumem[GLIDE_NUM_TMU];

void init_texman(void)
{
	int i;
	uint32_t end;

	for(i=0; i<num_tmu; i++) {
		tmumem[i].start = grTexMinAddress(i);
		end = grTexMaxAddress(i) + 8;
		tmumem[i].size = end - tmumem[i].start;

		printf("TMU%d TRAM range %06x - %06x (%d bytes)\n", i, tmumem[i].start, end,
			tmumem[i].size);
	}
}
