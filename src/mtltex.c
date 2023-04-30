#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "opengl.h"
#include "mtltex.h"
#include "rbtree.h"

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
	struct texture *tex;
	int alpha = img_has_alpha(img);

	if(!(tex = malloc(sizeof *tex))) {
		fprintf(stderr, "failed to allocate texture\n");
		return 0;
	}
	tex->img = img;

	tex->tex_width = nextpow2(img->width);
	tex->tex_height = nextpow2(img->height);

	glGenTextures(1, &tex->texid);
	glBindTexture(GL_TEXTURE_2D, tex->texid);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if(tex->tex_width != img->width || tex->tex_height != img->height) {
		cgm_mscaling(tex->matrix, (float)img->width / (float)tex->tex_width,
				(float)img->height / (float)tex->tex_height, 1);
		tex->use_matrix = 1;

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, alpha ? 4 : 3, tex->tex_width, tex->tex_height, 0,
				GL_RGB, GL_UNSIGNED_BYTE, 0);

		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, img->width, img->height,
				alpha ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, img->pixels);
	} else {
		cgm_midentity(tex->matrix);
		tex->use_matrix = 0;

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		gluBuild2DMipmaps(GL_TEXTURE_2D, alpha ? 4 : 3, img->width, img->height,
				alpha ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, img->pixels);
	}

#ifdef DBG_NO_IMAN
	printf("FREE PIXELS! (%dx%d)\n", img->width, img->height);
	free(img->pixels);
	img->pixels = 0;
#endif
	return tex;
}

void tex_free(struct texture *tex)
{
	glDeleteTextures(1, &tex->texid);
#ifdef DBG_NO_IMAN
	img_free(tex->img);
#endif
}

void mtl_init(struct material *mtl)
{
	cgm_wcons(&mtl->kd, 1, 1, 1, 1);
	cgm_wcons(&mtl->ks, 0, 0, 0, 1);
	mtl->shin = 50.0f;
	mtl->texmap = mtl->envmap = 0;
}

static int last_envmap;

static void setup_envmap(unsigned int envmap)
{
	glBindTexture(GL_TEXTURE_2D, envmap);
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	last_envmap = 1;
}

#define stop_envmap()	\
	do { \
		if(last_envmap) { \
			glDisable(GL_TEXTURE_GEN_S); \
			glDisable(GL_TEXTURE_GEN_T); \
			last_envmap = 0; \
		} \
	} while(0)


int mtl_apply(struct material *mtl, int pass)
{
	static float black[] = {0, 0, 0, 1};
	static float white[] = {1, 1, 1, 1};

	switch(pass) {
	case 0:
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, &mtl->kd.x);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, &mtl->ks.x);
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, mtl->shin);

		if(mtl->texmap) {
			stop_envmap();
			glBindTexture(GL_TEXTURE_2D, mtl->texmap->texid);
			glEnable(GL_TEXTURE_2D);
			if(mtl->envmap) return 1;
		} else if(mtl->envmap) {
			setup_envmap(mtl->envmap->texid);
		} else {
			glDisable(GL_TEXTURE_2D);
		}
		break;

	case 1:
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, black);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, white);
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, mtl->shin);
		assert(mtl->envmap);
		setup_envmap(mtl->envmap->texid);
		break;

	default:
		break;
	}

	return 0;
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
	if(img_load(img, name) == -1) {
		fprintf(stderr, "iman_get(%s): failed to load image\n", name);
		img_free(img);
		return 0;
	}
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
