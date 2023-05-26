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
#include "gaw.h"
#include "gawswtnl.h"
#include "polyfill.h"

static struct pimage textures[MAX_TEXTURES];

void gaw_sw_reset(void)
{
	gaw_swtnl_reset();

	free(pfill_zbuf);
}

void gaw_sw_init(void)
{
	gaw_swtnl_init();

	gaw_sw_reset();
}

void gaw_sw_destroy(void)
{
	gaw_swtnl_destroy();

	free(pfill_zbuf);
}

void gaw_sw_framebuffer(int width, int height, void *pixels)
{
	static int max_height;
	static int max_npixels;
	int npixels = width * height;

	if(npixels > max_npixels) {
		free(pfill_zbuf);
		pfill_zbuf = malloc_nf(npixels * sizeof *pfill_zbuf);
		max_npixels = npixels;
	}

	if(height > max_height) {
		polyfill_fbheight(height);
		max_height = height;
	}

	ST->width = width;
	ST->height = height;

	pfill_fb.pixels = pixels;
	pfill_fb.width = width;
	pfill_fb.height = height;

	gaw_viewport(0, 0, width, height);
}

/* set the framebuffer pointer, without resetting the size */
void gaw_sw_framebuffer_addr(void *pixels)
{
	pfill_fb.pixels = pixels;
}

void gaw_enable(int what)
{
	gaw_swtnl_enable(what);
}

void gaw_disable(int what)
{
	gaw_swtnl_disable(what);
}

void gaw_clear(unsigned int flags)
{
	int i, npix = pfill_fb.width * pfill_fb.height;

	if(flags & GAW_COLORBUF) {
		for(i=0; i<npix; i++) {
			pfill_fb.pixels[i] = ST->clear_color;
		}
	}

	if(flags & GAW_DEPTHBUF) {
		for(i=0; i<npix; i++) {
			pfill_zbuf[i] = ST->clear_depth;
		}
	}
}

void gaw_color_mask(int rmask, int gmask, int bmask, int amask)
{
	gaw_swtnl_color_mask(rmask, gmask, bmask, amask);
}

void gaw_depth_mask(int mask)
{
	gaw_swtnl_depth_mask(mask);
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

	free(textures[idx].pixels);
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


static __inline int calc_shift(unsigned int x)
{
	int res = -1;
	while(x) {
		x >>= 1;
		++res;
	}
	return res;
}

void gaw_tex1d(int ifmt, int xsz, int fmt, void *pix)
{
	gaw_tex2d(ifmt, xsz, 1, fmt, pix);
}

void gaw_tex2d(int ifmt, int xsz, int ysz, int fmt, void *pix)
{
	int npix;
	struct pimage *img;

	if(ST->cur_tex < 0) return;
	img = textures + ST->cur_tex;

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

	if(ST->cur_tex < 0) return;
	img = textures + ST->cur_tex;

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

void gaw_bind_tex1d(int tex)
{
	ST->cur_tex = (int)tex - 1;
	pfill_tex = textures[ST->cur_tex];
}

void gaw_bind_tex2d(int tex)
{
	ST->cur_tex = (int)tex - 1;
	pfill_tex = textures[ST->cur_tex];
}

static void dump_texture(struct pimage *img, const char *fname)
{
	int i, npix = img->width * img->height;
	FILE *fp = fopen(fname, "wb");
	if(!fp) return;

	fprintf(fp, "P6\n%d %d\n255\n", img->width, img->height);
	for(i=0; i<npix; i++) {
		int r = UNPACK_R(img->pixels[i]);
		int g = UNPACK_G(img->pixels[i]);
		int b = UNPACK_B(img->pixels[i]);
		fputc(r, fp);
		fputc(g, fp);
		fputc(b, fp);
	}
	fclose(fp);
}

void gaw_sw_dump_textures(void)
{
	int i;
	char buf[64];

	for(i=0; i<MAX_TEXTURES; i++) {
		if(ST->textypes[i] <= 0) continue;

		sprintf(buf, "tex%04d.ppm", i);
		printf("dumping %s ...\n", buf);
		dump_texture(textures + i, buf);
	}
}

void gaw_swtnl_drawprim(int prim, struct vertex *v, int vnum)
{
	int i, fill_mode;
	struct pvertex pv[16];

	for(i=0; i<vnum; i++) {
		/* viewport transformation */
		v[i].x = (v[i].x * 0.5f + 0.5f) * (float)ST->vport[2] + ST->vport[0];
		v[i].y = (v[i].y * 0.5f + 0.5f) * (float)ST->vport[3] + ST->vport[1];
		v[i].y = pfill_fb.height - v[i].y - 1;

		/* convert pos to 24.8 fixed point */
		pv[i].x = cround64(v[i].x * 256.0f);
		pv[i].y = cround64(v[i].y * 256.0f);

		if(ST->opt & (1 << GAW_DEPTH_TEST)) {
			/* after div/w z is in [-1, 1], remap it to [0, 0xffffff] */
			pv[i].z = cround64(v[i].z * 8388607.5f + 8388607.5f);
		}

		/* convert tex coords to 16.16 fixed point */
		pv[i].u = cround64(v[i].u * 65536.0f);
		pv[i].v = cround64(v[i].v * 65536.0f);
		/* pass the color through as is */
		pv[i].r = v[i].r;
		pv[i].g = v[i].g;
		pv[i].b = v[i].b;
		pv[i].a = v[i].a;
	}

	/* backface culling */
#if 0	/* TODO fix culling */
	if(vnum > 2 && (ST->opt & (1 << GAW_CULL_FACE))) {
		int32_t ax = pv[1].x - pv[0].x;
		int32_t ay = pv[1].y - pv[0].y;
		int32_t bx = pv[2].x - pv[0].x;
		int32_t by = pv[2].y - pv[0].y;
		int32_t cross_z = (ax >> 4) * (by >> 4) - (ay >> 4) * (bx >> 4);
		int sign = (cross_z >> 31) & 1;

		if(!(sign ^ ST->frontface)) {
			continue;	/* back-facing */
		}
	}
#endif

	switch(prim) {
	case GAW_POINTS:
		break;

	case GAW_LINES:
		break;

	default:
		fill_mode = ST->polymode;
		if(ST->opt & ((1 << GAW_TEXTURE_2D) | (1 << GAW_TEXTURE_1D))) {
			fill_mode |= POLYFILL_TEX_BIT;
		}
		if((ST->opt & (1 << GAW_BLEND)) && (ST->bsrc == GAW_SRC_ALPHA)) {
			fill_mode |= POLYFILL_ALPHA_BIT;
		} else if((ST->opt & (1 << GAW_BLEND)) && (ST->bsrc == GAW_ONE)) {
			fill_mode |= POLYFILL_ADD_BIT;
		}
		if(ST->opt & (1 << GAW_DEPTH_TEST)) {
			fill_mode |= POLYFILL_ZBUF_BIT;
		}
		polyfill(fill_mode, pv, vnum);
	}
}

