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
#include "gaw/gaw.h"
#include "gfxutil.h"
#include "mtltex.h"
#include "game.h"

void begin2d(int virt_height)
{
	gaw_matrix_mode(GAW_PROJECTION);
	gaw_push_matrix();
	gaw_load_identity();
	gaw_ortho(0, win_aspect * virt_height, virt_height, 0, -100, 100);

	gaw_matrix_mode(GAW_MODELVIEW);
	gaw_push_matrix();
	gaw_load_identity();

	gaw_save();
	gaw_disable(GAW_DEPTH_TEST);
	gaw_disable(GAW_CULL_FACE);
	gaw_disable(GAW_LIGHTING);
}

void end2d(void)
{
	gaw_restore();
	gaw_matrix_mode(GAW_PROJECTION);
	gaw_pop_matrix();
	gaw_matrix_mode(GAW_MODELVIEW);
	gaw_pop_matrix();
}

void blit_tex(float x, float y, struct texture *tex, float alpha)
{
	int xsz, ysz;

	xsz = tex->img->width;
	ysz = tex->img->height;

	gaw_set_tex2d(tex->texid);

	if(tex->use_matrix) {
		gaw_matrix_mode(GAW_TEXTURE);
		gaw_load_matrix(tex->matrix);
	}

	gaw_begin(GAW_QUADS);
	gaw_color4f(1, 1, 1, alpha);
	gaw_texcoord2f(0, 1); gaw_vertex2f(x, y + ysz);
	gaw_texcoord2f(1, 1); gaw_vertex2f(x + xsz, y + ysz);
	gaw_texcoord2f(1, 0); gaw_vertex2f(x + xsz, y);
	gaw_texcoord2f(0, 0); gaw_vertex2f(x, y);
	gaw_end();

	if(tex->use_matrix) {
		gaw_load_identity();
		gaw_matrix_mode(GAW_MODELVIEW);
	}
}

void blit_tex_rect(float x, float y, float xsz, float ysz, struct texture *tex,
		float alpha, float u, float v, float usz, float vsz)
{
	gaw_set_tex2d(tex->texid);

	if(tex->use_matrix) {
		gaw_matrix_mode(GAW_TEXTURE);
		gaw_load_matrix(tex->matrix);
	}

	gaw_begin(GAW_QUADS);
	gaw_color4f(1, 1, 1, alpha);
	gaw_texcoord2f(u, v + vsz);
	gaw_vertex2f(x, y + ysz);
	gaw_texcoord2f(u + usz, v + vsz);
	gaw_vertex2f(x + xsz, y + ysz);
	gaw_texcoord2f(u + usz, v);
	gaw_vertex2f(x + xsz, y);
	gaw_texcoord2f(u, v);
	gaw_vertex2f(x, y);
	gaw_end();

	if(tex->use_matrix) {
		gaw_load_identity();
		gaw_matrix_mode(GAW_MODELVIEW);
	}
}

void draw_billboard(const cgm_vec3 *p, float size, cgm_vec4 col)
{
	float m[16];

	gaw_matrix_mode(GAW_MODELVIEW);
	gaw_push_matrix();

	gaw_translate(p->x, p->y, p->z);

	gaw_get_modelview(m);
	/* make the upper 3x3 part of the matrix identity */
	m[0] = m[5] = m[10] = 1.0f;
	m[1] = m[2] = m[3] = m[4] = m[6] = m[7] = m[8] = m[9] = 0.0f;
	gaw_load_matrix(m);

	gaw_begin(GAW_QUADS);
	gaw_color4f(col.x, col.y, col.z, col.w);
	gaw_texcoord2f(0, 0);
	gaw_vertex2f(-size, -size);
	gaw_texcoord2f(1, 0);
	gaw_vertex2f(size, -size);
	gaw_texcoord2f(1, 1);
	gaw_vertex2f(size, size);
	gaw_texcoord2f(0, 1);
	gaw_vertex2f(-size, size);
	gaw_end();

	gaw_pop_matrix();
}


void calc_posrot_matrix(float *matrix, const cgm_vec3 *pos, const cgm_quat *rot)
{
	cgm_mrotation_quat(matrix, rot);
	matrix[12] = pos->x;
	matrix[13] = pos->y;
	matrix[14] = pos->z;
}
