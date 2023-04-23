#ifndef OCTREE_H_
#define OCTREE_H_

#include "geom.h"

struct octnode {
	struct aabox aabb;
	struct octnode *child[8];
	void *data;
};

struct octnode *oct_create(struct aabox *bounds);
void oct_free(struct octnode *tree);

#define oct_isleaf(n)	((n)->child[0] == 0)
struct octnode *oct_find_leaf(struct octnode *tree, float x, float y, float z);

#endif	/* OCTREE_H_ */
