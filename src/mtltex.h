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
	struct texture *texmap, *envmap;
};

/* texture */
struct texture *tex_load(const char *fname);
struct texture *tex_image(struct img_pixmap *img);
void tex_free(struct texture *tex);

/* material */
void mtl_init(struct material *mtl);
int mtl_apply(struct material *mtl, int pass);		/* set material and bind textures */

/* image manager */
int iman_init(void);
void iman_destroy(void);
void iman_clear(void);
int iman_add(struct img_pixmap *img);
struct img_pixmap *iman_find(const char *name);
struct img_pixmap *iman_get(const char *name);

int nextpow2(int x);

#endif	/* MTLTEX_H_ */
