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

void draw_billboard(const cgm_vec3 *pos, float sz, cgm_vec4 col);

void calc_posrot_matrix(float *matrix, const cgm_vec3 *pos, const cgm_quat *rot);

#endif	/* GFXUTIL_H_ */
