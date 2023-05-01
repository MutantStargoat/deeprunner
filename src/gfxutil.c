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
	glTexCoord2f(0, 1); glVertex2f(x, ysz);
	glTexCoord2f(1, 1); glVertex2f(x + xsz, ysz);
	glTexCoord2f(1, 0); glVertex2f(x + xsz, 0);
	glTexCoord2f(0, 0); glVertex2f(x, 0);
	glEnd();

	if(tex->use_matrix) {
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
	}
}
