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
#include "opengl.h"
#include "gfxutil.h"
#include "mtltex.h"
#include "game.h"

void begin2d(int virt_height)
{
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, win_aspect * virt_height, virt_height, 0, -100, 100);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_LIGHTING);
}

void end2d(void)
{
	glPopAttrib();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

void blit_tex(float x, float y, struct texture *tex, float alpha)
{
	int xsz, ysz;

	xsz = tex->img->width;
	ysz = tex->img->height;

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex->texid);

	if(tex->use_matrix) {
		glMatrixMode(GL_TEXTURE);
		glLoadMatrixf(tex->matrix);
	}

	glBegin(GL_QUADS);
	glColor4f(1, 1, 1, alpha);
	glTexCoord2f(0, 1); glVertex2f(x, y + ysz);
	glTexCoord2f(1, 1); glVertex2f(x + xsz, y + ysz);
	glTexCoord2f(1, 0); glVertex2f(x + xsz, y);
	glTexCoord2f(0, 0); glVertex2f(x, y);
	glEnd();

	if(tex->use_matrix) {
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
	}
}

void blit_tex_rect(float x, float y, float xsz, float ysz, struct texture *tex,
		float alpha, float u, float v, float usz, float vsz)
{
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex->texid);

	if(tex->use_matrix) {
		glMatrixMode(GL_TEXTURE);
		glLoadMatrixf(tex->matrix);
	}

	glBegin(GL_QUADS);
	glColor4f(1, 1, 1, alpha);
	glTexCoord2f(u, v + vsz);
	glVertex2f(x, y + ysz);
	glTexCoord2f(u + usz, v + vsz);
	glVertex2f(x + xsz, y + ysz);
	glTexCoord2f(u + usz, v);
	glVertex2f(x + xsz, y);
	glTexCoord2f(u, v);
	glVertex2f(x, y);
	glEnd();

	if(tex->use_matrix) {
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
	}
}

void set_mtl_diffuse(float r, float g, float b, float a)
{
	float v[4];
	v[0] = r;
	v[1] = g;
	v[2] = b;
	v[3] = a;
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, v);
}

void set_mtl_specular(float r, float g, float b, float shin)
{
	float v[4];
	v[0] = r;
	v[1] = g;
	v[2] = b;
	v[3] = 1.0f;
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, v);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shin);
}

void texenv_sphmap(int enable)
{
	if(enable) {
		glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
		glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
		glEnable(GL_TEXTURE_GEN_S);
		glEnable(GL_TEXTURE_GEN_T);
	} else {
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
	}
}

void draw_billboard(const cgm_vec3 *p, float size, cgm_vec4 col)
{
	float m[16];

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	glTranslatef(p->x, p->y, p->z);

	glGetFloatv(GL_MODELVIEW_MATRIX, m);
	/* make the upper 3x3 part of the matrix identity */
	m[0] = m[5] = m[10] = 1.0f;
	m[1] = m[2] = m[3] = m[4] = m[6] = m[7] = m[8] = m[9] = 0.0f;
	glLoadMatrixf(m);

	glBegin(GL_QUADS);
	glColor4f(col.x, col.y, col.z, col.w);
	glTexCoord2f(0, 0);
	glVertex2f(-size, -size);
	glTexCoord2f(1, 0);
	glVertex2f(size, -size);
	glTexCoord2f(1, 1);
	glVertex2f(size, size);
	glTexCoord2f(0, 1);
	glVertex2f(-size, size);
	glEnd();

	glPopMatrix();
}


void calc_posrot_matrix(float *matrix, const cgm_vec3 *pos, const cgm_quat *rot)
{
	cgm_mrotation_quat(matrix, rot);
	matrix[12] = pos->x;
	matrix[13] = pos->y;
	matrix[14] = pos->z;
}
