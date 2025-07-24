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
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "gaw/gaw.h"
#include "mtltex.h"
#include "rbtree.h"
#include "util.h"
#include "options.h"
#include "gfxutil.h"

struct texture *tex_load(const char *fname)
{
	struct img_pixmap *img;

	if(!(img = iman_get(fname))) {
		return 0;
	}

	return tex_image(img);
}

struct texture *tex_image(struct img_pixmap *img)
{
	int i;
	struct texture *tex;
	int alpha = img_has_alpha(img);
	int ifmt, fmt, pixsz, sz;
	static void *zerobuf;
	static int zerobuf_size;
	unsigned char *sptr, *dptr;

	if(!(tex = malloc(sizeof *tex))) {
		fprintf(stderr, "failed to allocate texture\n");
		return 0;
	}
	tex->img = img;

	tex->tex_width = nextpow2(img->width);
	tex->tex_height = nextpow2(img->height);

	if(alpha) {
		ifmt = GAW_RGBA;
		fmt = GAW_RGBA;
		pixsz = 4;
	} else {
		ifmt = GAW_RGBA;
		fmt = GAW_RGB;
		pixsz = 3;
	}

	tex->texid = gaw_create_tex2d(opt.gfx.texfilter);

	if(tex->tex_width != img->width || tex->tex_height != img->height) {
		cgm_mscaling(tex->matrix, (float)img->width / (float)tex->tex_width,
				(float)img->height / (float)tex->tex_height, 1);
		tex->use_matrix = 1;

		sz = tex->tex_width * tex->tex_height * pixsz;
		if(zerobuf_size < sz) {
			zerobuf = realloc_nf(zerobuf, sz);
			zerobuf_size = sz;
		}
		memset(zerobuf, 0, zerobuf_size);

		sptr = img->pixels;
		dptr = zerobuf;
		for(i=0; i<img->height; i++) {
			memcpy(dptr, sptr, img->width * pixsz);
			sptr += img->width * pixsz;
			dptr += tex->tex_width * pixsz;
		}

		/* avoid using mipmaps */
		if(opt.gfx.texfilter == GFXOPT_TEX_TRILINEAR) {
			gaw_texfilter2d(GAW_BILINEAR);
		}
		gaw_tex2d(ifmt, tex->tex_width, tex->tex_height, fmt, zerobuf);
	} else {
		cgm_midentity(tex->matrix);
		tex->use_matrix = 0;

		gaw_texfilter2d(opt.gfx.texfilter);
		gaw_tex2d(ifmt, img->width, img->height, fmt,img->pixels);
	}

#ifdef DBG_NO_IMAN
	free(img->pixels);
	img->pixels = 0;
#endif
	return tex;
}

void tex_free(struct texture *tex)
{
	if(!tex) return;

	if(tex->texid) {
		gaw_destroy_tex(tex->texid);
	}
#ifdef DBG_NO_IMAN
	img_free(tex->img);
#endif
}

void mtl_init(struct material *mtl)
{
	memset(mtl, 0, sizeof *mtl);
	cgm_wcons(&mtl->kd, 1, 1, 1, 1);
	cgm_wcons(&mtl->ks, 0, 0, 0, 1);
	mtl->shin = 50.0f;
}

static int last_envmap;

static void setup_envmap(unsigned int envmap)
{
	gaw_set_tex2d(envmap);
	gaw_texenv_sphmap(1);
	last_envmap = 1;
}

#define stop_envmap()	\
	do { \
		if(last_envmap) { \
			gaw_texenv_sphmap(0); \
			last_envmap = 0; \
		} \
	} while(0)


int mtl_apply(struct material *mtl, int pass)
{
	switch(pass) {
	case 0:
		gaw_mtl_diffuse(mtl->kd.x, mtl->kd.y, mtl->kd.z, mtl->kd.w);
		gaw_mtl_specular(mtl->ks.x, mtl->ks.y, mtl->ks.z, mtl->shin);
		if(mtl->emissive) {
			gaw_mtl_emission(mtl->kd.x, mtl->kd.y, mtl->kd.z);
		} else {
			gaw_mtl_emission(0, 0, 0);
		}

		if(mtl->texmap) {
			stop_envmap();
			gaw_set_tex2d(mtl->texmap->texid);

			gaw_matrix_mode(GAW_TEXTURE);
			gaw_load_identity();
			if(mtl->uvanim) {
				gaw_translate(mtl->uvoffs.x, mtl->uvoffs.y, 0);
			}
			gaw_matrix_mode(GAW_MODELVIEW);

			if(mtl->envmap) return 1;
		} else if(mtl->envmap) {
			setup_envmap(mtl->envmap->texid);
		} else {
			gaw_set_tex2d(0);
		}
		break;

	case 1:
		if(mtl->uvanim) {
			gaw_matrix_mode(GAW_TEXTURE);
			gaw_load_identity();
			gaw_matrix_mode(GAW_MODELVIEW);
		}
		gaw_mtl_diffuse(0, 0, 0, 1);
		gaw_mtl_specular(1, 1, 1, mtl->shin);
		assert(mtl->envmap);
		setup_envmap(mtl->envmap->texid);
		break;

	default:
		break;
	}

	return 0;
}

void mtl_end(void)
{
	stop_envmap();
}

/* ------------- image manager ------------- */
struct rbtree *imandb;

static void iman_delfunc(struct rbnode *rbn, void *cls)
{
	img_free(rbn->data);
}

int iman_init(void)
{
	if(!(imandb = rb_create(RB_KEY_STRING))) {
		fprintf(stderr, "iman: failed to create image database\n");
		return -1;
	}
	rb_set_delete_func(imandb, iman_delfunc, 0);
	return 0;
}

void iman_destroy(void)
{
	rb_free(imandb);
	imandb = 0;
}

void iman_clear(void)
{
	rb_clear(imandb);
}

int iman_add(struct img_pixmap *img)
{
	return rb_insert(imandb, img->name, img);
}

struct img_pixmap *iman_find(const char *name)
{
	struct rbnode *rbn = rb_find(imandb, (char*)name);
	return rbn ? rbn->data : 0;
}

struct img_pixmap *iman_get(const char *name)
{
	struct img_pixmap *img;

	if((img = iman_find(name))) {
		return img;
	}

	if(!(img = img_create())) {
		fprintf(stderr, "iman_get(%s): failed to allocate image\n", name);
		return 0;
	}
	printf("loading image %s ... ", name); fflush(stdout);
	if(img_load(img, name) == -1) {
		printf("failed!\n");
		fprintf(stderr, "iman_get(%s): failed to load image\n", name);
		img_free(img);
		return 0;
	}
	printf("OK.\n");
	img_convert(img, img_has_alpha(img) ? IMG_FMT_RGBA32 : IMG_FMT_RGB24);
#ifndef DBG_NO_IMAN
	iman_add(img);
#endif
	return img;
}

int nextpow2(int x)
{
	if(x <= 0) return 0;

	x--;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return x + 1;
}
