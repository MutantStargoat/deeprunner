#ifndef POLYCLIP_H_
#define POLYCLIP_H_

#include "polyfill.h"

struct cplane {
	float x, y, z;
	float nx, ny, nz;
};

enum {
	CLIP_LEFT, CLIP_RIGHT,
	CLIP_BOTTOM, CLIP_TOP,
	CLIP_NEAR, CLIP_FAR
};

/* Generic polygon clipper
 * returns:
 *  1 -> fully inside, not clipped
 *  0 -> straddling the plane and clipped
 * -1 -> fully outside, not clipped
 * in all cases, vertices are copied to vout, and the vertex count is written
 * to wherever voutnum is pointing
 */
int clip_poly(struct vertex *vout, int *voutnum,
		const struct vertex *vin, int vnum, struct cplane *plane);

/* only checks if the polygon would be clipped by the plane, and classifies it
 * as inside/outside/straddling, without actually producing a clipped polygon.
 * return values are the same as clip_poly.
 */
int check_clip_poly(const struct vertex *v, int vnum, struct cplane *plane);

/* Special-case frustum clipper (might be slightly faster) */
int clip_frustum(struct vertex *vout, int *voutnum,
		const struct vertex *vin, int vnum, int fplane);

#endif	/* POLYCLIP_H_ */
