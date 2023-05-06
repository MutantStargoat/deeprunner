#ifndef GFXUTIL_H_
#define GFXUTIL_H_

#include "cgmath/cgmath.h"

struct rect {
	float x, y, width, height;
};

struct texture;

void begin2d(int virt_height);
void end2d(void);

void blit_tex(float x, float y, struct texture *tex, float alpha);
void blit_tex_rect(float x, float y, float xsz, float ysz, struct texture *tex,
		float alpha, float u, float v, float usz, float vsz);

void set_mtl_diffuse(float r, float g, float b, float a);
void set_mtl_specular(float r, float g, float b, float shin);

void texenv_sphmap(int enable);

void draw_billboard(const cgm_vec3 *pos, float sz, cgm_vec4 col);

#endif	/* GFXUTIL_H_ */
