#ifndef MESH_H_
#define MESH_H_

#include "cgmath/cgmath.h"
#include "mtltex.h"

struct mesh {
	char *name;
	cgm_vec3 *varr, *narr;
	cgm_vec2 *uvarr;
	unsigned int *idxarr;
	long vcount, icount;
	int dlist;

	struct material mtl;
};

int mesh_init(struct mesh *m);
void mesh_destroy(struct mesh *m);

void mesh_transform(struct mesh *m, const float *mat);

void mesh_compile(struct mesh *m);

#endif	/* MESH_H_ */
