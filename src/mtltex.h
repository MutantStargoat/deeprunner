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
#ifndef MTLTEX_H_
#define MTLTEX_H_

#include "cgmath/cgmath.h"
#include "imago2.h"

struct texture {
	unsigned int texid;
	int tex_width, tex_height;
	int use_matrix;
	float matrix[16];
	struct img_pixmap *img;	/* no ownership */
};

struct material {
	cgm_vec4 kd, ks;
	float shin;
	int emissive;
	struct texture *texmap, *envmap;

	int uvanim;
	cgm_vec2 uvoffs;
	cgm_vec2 texvel;
};

/* texture */
struct texture *tex_load(const char *fname);
struct texture *tex_image(struct img_pixmap *img);
void tex_free(struct texture *tex);

/* material */
void mtl_init(struct material *mtl);
int mtl_apply(struct material *mtl, int pass);		/* set material and bind textures */
void mtl_end(void);

/* image manager */
int iman_init(void);
void iman_destroy(void);
void iman_clear(void);
int iman_add(struct img_pixmap *img);
struct img_pixmap *iman_find(const char *name);
struct img_pixmap *iman_get(const char *name);

int nextpow2(int x);

#endif	/* MTLTEX_H_ */
