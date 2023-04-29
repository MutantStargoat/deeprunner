#ifndef GFXUTIL_H_
#define GFXUTIL_H_

struct texture;

void begin2d(int virt_height);
void end2d(void);

void blit_tex(float x, float y, struct texture *tex, float alpha);

#endif	/* GFXUTIL_H_ */
