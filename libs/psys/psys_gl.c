/*
libpsys - reusable particle system library.
Copyright (C) 2011-2018  John Tsiombikas <nuclear@member.fsf.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <string.h>
#include <errno.h>
#include <assert.h>

#ifndef __APPLE__
#ifdef WIN32
#include <windows.h>
#endif

#include <GL/gl.h>
#else
#include <OpenGL/gl.h>
#endif

#include <imago2.h>
#include "psys.h"
#include "psys_gl.h"

void psys_gl_draw_start(const struct psys_emitter *em, void *cls)
{
	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_LIGHTING);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, em->attr.blending == PSYS_ADD ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA);

	if(em->attr.tex) {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, em->attr.tex);
	}

	glDepthMask(0);
}

void psys_gl_draw(const struct psys_emitter *em, const struct psys_particle *p, void *cls)
{
	float hsz = p->size / 2.0;

	float xform[16];

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	glTranslatef(p->x, p->y, p->z);

	glGetFloatv(GL_MODELVIEW_MATRIX, xform);
	xform[0] = xform[5] = xform[10] = 1.0;
	xform[1] = xform[2] = xform[4] = xform[6] = xform[8] = xform[9] = 0.0;
	glLoadMatrixf(xform);

	glBegin(GL_QUADS);
	glColor4f(p->r, p->g, p->b, p->alpha);

	glTexCoord2f(0, 0);
	glVertex2f(-hsz, -hsz);

	glTexCoord2f(1, 0);
	glVertex2f(hsz, -hsz);

	glTexCoord2f(1, 1);
	glVertex2f(hsz, hsz);

	glTexCoord2f(0, 1);
	glVertex2f(-hsz, hsz);
	glEnd();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

void psys_gl_draw_end(const struct psys_emitter *em, void *cls)
{
	glDepthMask(1);
	glPopAttrib();
}


unsigned int psys_gl_load_texture(const char *fname, void *cls)
{
	unsigned int tex;
	void *pixels;
	int xsz, ysz;

	if(!(pixels = img_load_pixels(fname, &xsz, &ysz, IMG_FMT_RGBA32))) {
		return 0;
	}
	printf("psys_gl_load_texture: creating texture %s (%dx%d)\n",  fname, xsz, ysz);

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, xsz, ysz, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	assert(glGetError() == GL_NO_ERROR);

	img_free_pixels(pixels);
	return tex;
}

void psys_gl_unload_texture(unsigned int tex, void *cls)
{
	glDeleteTextures(1, &tex);
}
