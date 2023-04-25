#ifndef OCTREE_H_
#define OCTREE_H_

#include "geom.h"

struct octnode {
	struct aabox aabb;
	struct octnode *child[8];
	struct triangle *tris;		/* darr */
};

struct octnode *oct_create(void);
void oct_free(struct octnode *tree);

int oct_addtri(struct octnode *tree, struct triangle *tri);
void oct_build(struct octnode *tree, int maxdepth, int maxnodetris);

int oct_raytest(const struct octnode *tree, const cgm_ray *ray, float tmax, struct trihit *hit);

#define oct_isleaf(n)	((n)->tris)
struct octnode *oct_find_leaf(struct octnode *tree, float x, float y, float z);

#endif	/* OCTREE_H_ */
