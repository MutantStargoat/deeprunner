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

#define MAX_MIPMAP_LEVELS	9

#define PACK_RGB565(r, g, b)	\
	(((uint16_t)((r) & 0xf8) << 8) | \
	 ((uint16_t)((g) & 0xfc) << 3) | \
	 ((uint16_t)((b) & 0xf8) >> 3))

#define PACK_RGBA4444(r, g, b, a) \
	(((uint16_t)((a) & 0xf0) << 8) | \
	 ((uint16_t)((r) & 0xf0) << 4) | \
	 ((uint16_t)(g) & 0xf0) | \
	 ((uint16_t)((b) & 0xf0) >> 4))

struct mipmap {
	int width, height;
	int size;
	void *pixels;
};

struct teximg {
	int idx;
	int fmt;
	long size, res_size;
	long addr;	/* -1 if not resident */

	struct mipmap mip[MAX_MIPMAP_LEVELS];
	int levels;

	long last_bind;
};

static void print_board_info(GrHwConfiguration *hw, int idx);

static int grlod(int x, int y);
static int graspect(int x, int y);
static int grfmt(int ifmt);
static int texdim(int x, int lvl);
static int texlevels(int x, int y);
static int texsize(int x, int y, int pixsz);
static void halve_image_lum(uint8_t *dest, uint8_t *src, int xsz, int ysz);
static void halve_image_rgb(uint8_t *dest, uint8_t *src, int xsz, int ysz);
static void halve_image_rgba(uint32_t *dest, uint32_t *src, int xsz, int ysz);

static void init_texman(void);

static GrHwConfiguration hwcfg;
static int num_tmu;

static struct teximg textures[MAX_TEXTURES];


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
	grSstWinClose();
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
		if(ST->textypes[i] == 0) {
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
	ST->textypes[idx] = 1;

	memset(textures + idx, 0, sizeof *textures);
	ST->cur_tex = idx;
	return idx + 1;
}

unsigned int gaw_create_tex2d(int texfilter)
{
	int idx;
	if((idx = alloc_tex()) == -1) {
		return 0;
	}
	ST->textypes[idx] = 2;

	memset(textures + idx, 0, sizeof *textures);
	ST->cur_tex = idx;
	return idx + 1;
}

void gaw_destroy_tex(unsigned int texid)
{
	int idx = texid - 1;

	if(!ST->textypes[idx]) return;

	free(textures[idx].mip[0].pixels);
	memset(textures + idx, 0, sizeof *textures);
	ST->textypes[idx] = 0;
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

void gaw_tex1d(int ifmt, int xsz, int fmt, void *pix)
{
	gaw_tex2d(ifmt, xsz, 1, fmt, pix);
}

void gaw_tex2d(int ifmt, int xsz, int ysz, int fmt, void *pix)
{
	int i, sz, pixsz;
	struct teximg *tex;
	GrTexInfo tinf;
	void *tmpbuf = 0;

	if(ST->cur_tex < 0) return;
	if(xsz > 256 || ysz > 256) {
		fprintf(stderr, "unsupported texture size: %dx%d\n", xsz, ysz);
		return;
	}
	if(xsz / ysz > 8 || ysz / xsz > 8) {
		fprintf(stderr, "unsupported texture aspect: %dx%d\n", xsz, ysz);
		return;
	}

	pixsz = ifmt == GAW_LUMINANCE ? 1 : 2;

	tex = textures + ST->cur_tex;
	memset(tex, 0, sizeof *tex);

	tinf.smallLod = GR_LOD_1;
	tinf.largeLod = grlod(xsz, ysz);
	tinf.aspectRatio = graspect(xsz, ysz);
	tinf.format = grfmt(ifmt);
	tex->res_size = grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH, &tinf);

	tex->levels = texlevels(xsz, ysz);
	tex->size = texsize(xsz, ysz, pixsz);

	free(tex->mip[0].pixels);
	tex->mip[0].pixels = malloc_nf(tex->size);
	if(pix) {
		tmpbuf = malloc_nf(xsz * ysz * 2);
	}

	for(i=0; i<tex->levels; i++) {
		tex->mip[i].width = xsz;
		tex->mip[i].height = ysz;
		tex->mip[i].size = xsz * ysz * pixsz;
		if(i > 0) {
			tex->mip[i].pixels = (uint16_t*)((char*)tex->mip[i - 1].pixels + tex->mip[i - 1].size);
		}

		if(pix) {
			if(i > 0) {
				void *dest, *src;

				if(i & 1) {
					dest = tmpbuf;
					src = (char*)tmpbuf + xsz * ysz;
				} else {
					dest = (char*)tmpbuf + xsz * ysz;
					src = tmpbuf;
				}

				switch(fmt) {
				case GAW_LUMINANCE:
					halve_image_lum(dest, i > 1 ? src : pix, xsz, ysz);
					break;

				case GAW_RGB:
					halve_image_rgb(dest, i > 1 ? src : pix, xsz, ysz);
					break;

				case GAW_RGBA:
					halve_image_rgba(dest, i > 1 ? src : pix, xsz, ysz);
					break;
				}
				gaw_subtex2d(i, 0, 0, xsz, ysz, fmt, tmpbuf);
			} else {
				gaw_subtex2d(0, 0, 0, xsz, ysz, fmt, pix);
			}
		}

		if(xsz > 1) xsz >>= 1;
		if(ysz > 1) ysz >>= 1;
	}

	free(tmpbuf);
}

void gaw_subtex2d(int lvl, int x, int y, int xsz, int ysz, int fmt, void *pix)
{
	int i, j, r, g, b, a, val, lvlwidth;
	uint16_t *dest;
	unsigned char *src;
	struct teximg *tex;

	if(ST->cur_tex < 0) return;
	tex = textures + ST->cur_tex;
	if(!tex->mip[lvl].pixels) {
		fprintf(stderr, "subtex2d error: texture data not allocated\n");
		return;
	}
	if(x + xsz > tex->mip[lvl].width) {
		xsz = tex->mip[lvl].width - x;
	}
	if(y + ysz > tex->mip[lvl].height) {
		ysz = tex->mip[lvl].height - y;
	}

	lvlwidth = tex->mip[lvl].width;
	src = pix;

	switch(fmt) {
	case GAW_LUMINANCE:
		dest = (uint16_t*)((char*)tex->mip[lvl].pixels + y * lvlwidth + x);
		for(i=0; i<ysz; i++) {
			memcpy(dest, src, xsz);
			src += xsz;
			dest += lvlwidth >> 1;
		}
		break;

	case GAW_RGB:
		dest = (uint16_t*)tex->mip[lvl].pixels + y * lvlwidth + x;
		for(i=0; i<ysz; i++) {
			for(j=0; j<xsz; j++) {
				b = src[0];
				g = src[1];
				r = src[2];
				src += 3;
				dest[j] = PACK_RGB565(r, g, b);
			}
			dest += lvlwidth;
		}
		break;

	case GAW_RGBA:
		dest = (uint16_t*)tex->mip[lvl].pixels + y * lvlwidth + x;
		for(i=0; i<ysz; i++) {
			for(j=0; j<xsz; j++) {
				b = src[0];
				g = src[1];
				r = src[2];
				a = src[3];
				src += 4;
				dest[j] = PACK_RGBA4444(r, g, b, a);
			}
			dest += lvlwidth;
		}
		break;

	default:
		break;
	}
}

void gaw_bind_tex1d(int tex)
{
	ST->cur_tex = (int)tex - 1;
}

void gaw_bind_tex2d(int tex)
{
	ST->cur_tex = (int)tex - 1;
}


void gaw_sw_dump_textures(void)
{
	/* TODO dump whole pyramid for each tex */
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

/* texture helper functions */
static int grlod(int x, int y)
{
	if(y > x) x = y;

	switch(x) {
	case 256: return GR_LOD_256;
	case 128: return GR_LOD_128;
	case 64: return GR_LOD_64;
	case 32: return GR_LOD_32;
	case 16: return GR_LOD_16;
	case 8: return GR_LOD_8;
	case 4: return GR_LOD_4;
	case 2: return GR_LOD_2;
	case 1: return GR_LOD_1;
	default:
		break;
	}
	return GR_LOD_1;
}

static int graspect(int x, int y)
{
	int steps;

	if(x == y) return GR_ASPECT_1x1;

	if(x > y) {
		steps = GR_ASPECT_1x1;
		while(y < x && steps > GR_ASPECT_8x1) {
			y <<= 1;
			steps--;
		}
	} else {
		steps = 0;
		while(x < y && steps < GR_ASPECT_1x8) {
			x <<= 1;
			steps++;
		}
	}
	return steps;
}

static int grfmt(int ifmt)
{
	switch(ifmt) {
	case GAW_LUMINANCE:
		return GR_TEXFMT_INTENSITY_8;
	case GAW_RGB:
		return GR_TEXFMT_RGB_565;
	case GAW_RGBA:
		return GR_TEXFMT_ARGB_4444;
	default:
		break;
	}
	return GR_TEXFMT_RGB_565;
}

static int texdim(int x, int lvl)
{
	while(x > 1 && lvl-- > 0) {
		x >>= 1;
	}
	return x;
}

static int texlevels(int x, int y)
{
	int lvl = 0;
	while(x > 1 || y > 1) {
		lvl++;
		x >>= 1;
		y >>= 1;
	}
	return lvl;
}

static int texsize(int x, int y, int pixsz)
{
	long sz = 0;
	while(x > 1 || y > 1) {
		sz += x * y * pixsz;
		x >>= 1;
		y >>= 1;
	}
	return sz;
}

static void halve_image_lum(uint8_t *dest, uint8_t *src, int xsz, int ysz)
{
	int i, j;
	int dest_w = xsz >> 1;
	int dest_h = ysz >> 1;

	for(i=0; i<dest_h; i++) {
		for(j=0; j<dest_w; j++) {
			dest[j] = (*src + src[1] + src[xsz] + src[xsz + 1]) >> 2;
			src += 2;
		}
		dest += dest_w;
		src += xsz;
	}
}

static void halve_image_rgb(uint8_t *dest, uint8_t *src, int xsz, int ysz)
{
	int i, j;
	int dest_w = xsz >> 1;
	int dest_h = ysz >> 1;
	int sscanlen = xsz * 3;
	uint8_t *srcodd = src + sscanlen;

	for(i=0; i<dest_h; i++) {
		for(j=0; j<dest_w; j++) {
			dest[0] = (src[0] + src[3] + srcodd[0] + srcodd[3]) >> 2;
			dest[1] = (src[1] + src[4] + srcodd[1] + srcodd[4]) >> 2;
			dest[2] = (src[2] + src[5] + srcodd[2] + srcodd[5]) >> 2;
			dest += 3;
			src += 6;
			srcodd += 6;
		}
		src += sscanlen;
		srcodd += sscanlen;
	}
}


#define PACK_RGBA(r, g, b, a) \
	(((a) << 24) | ((r) << 16) | ((g) << 8) | (b))
#define UNPACK_R(pix)	((pix) & 0xff)
#define UNPACK_G(pix)	(((pix) >> 8) & 0xff)
#define UNPACK_B(pix)	(((pix) >> 16) & 0xff)
#define UNPACK_A(pix)	((pix) >> 24)

static void halve_image_rgba(uint32_t *dest, uint32_t *src, int xsz, int ysz)
{
	int i, j, r, g, b;
	int dest_w = xsz >> 1;
	int dest_h = ysz >> 1;

	for(i=0; i<dest_h; i++) {
		for(j=0; j<dest_w; j++) {
			r = (UNPACK_R(*src) + UNPACK_R(src[1]) + UNPACK_R(src[xsz]) +
					UNPACK_R(src[xsz + 1])) >> 2;
			g = (UNPACK_G(*src) + UNPACK_G(src[1]) + UNPACK_G(src[xsz]) +
					UNPACK_G(src[xsz + 1])) >> 2;
			b = (UNPACK_B(*src) + UNPACK_B(src[1]) + UNPACK_B(src[xsz]) +
					UNPACK_B(src[xsz + 1])) >> 2;
			a = (UNPACK_A(*src) + UNPACK_A(src[1]) + UNPACK_A(src[xsz]) +
					UNPACK_A(src[xsz + 1])) >> 2;
			dest[j] = PACK_RGBA(r, g, b, a);
			src += 2;
		}
		src += xsz;
		dest += dest_w;
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
