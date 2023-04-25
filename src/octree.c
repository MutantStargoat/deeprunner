#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include "octree.h"
#include "util.h"
#include "darray.h"

struct octnode *oct_create(void)
{
	struct octnode *node = malloc_nf(sizeof *node);
	aabox_init(&node->aabb);
	memset(node->child, 0, sizeof node->child);
	node->tris = darr_alloc(0, sizeof *node->tris);
	return node;
}

void oct_free(struct octnode *tree)
{
	int i;
	if(!tree) return;

	for(i=0; i<8; i++) {
		oct_free(tree->child[i]);
	}

	darr_free(tree->tris);
	free(tree);
}

int oct_addtri(struct octnode *tree, struct triangle *tri)
{
	if(!oct_isleaf(tree)) {
		fprintf(stderr, "oct_addtri: trying to add triangles to a constructed octree\n");
		return -1;
	}

	darr_push(tree->tris, tri);

	/* expand tree bounding box */
	aabox_union_point(&tree->aabb, tri->v);
	aabox_union_point(&tree->aabb, tri->v + 1);
	aabox_union_point(&tree->aabb, tri->v + 2);
	return 0;
}

static void subbox(struct aabox *sub, struct aabox *full, int which)
{
	*sub = *full;
	cgm_vec3 mid;

	mid.x = (full->vmin.x + full->vmax.x) * 0.5f;
	mid.y = (full->vmin.y + full->vmax.y) * 0.5f;
	mid.z = (full->vmin.z + full->vmax.z) * 0.5f;

	if((which & 1) == 0) {
		sub->vmax.x = mid.x;
	} else {
		sub->vmin.x = mid.x;
	}
	if((which & 2) == 0) {
		sub->vmax.y = mid.y;
	} else {
		sub->vmin.y = mid.y;
	}
	if((which & 4) == 0) {
		sub->vmax.z = mid.z;
	} else {
		sub->vmin.z = mid.z;
	}
}

void oct_build(struct octnode *tree, int maxdepth, int maxnodetris)
{
	int i, j, ntris;
	struct octnode *node;

	if(maxdepth <= 0) return;

	ntris = darr_size(tree->tris);
	if(ntris > maxnodetris) {
		/* split node */
		for(i=0; i<8; i++) {
			node = oct_create();
			subbox(&node->aabb, &tree->aabb, i);

			for(j=0; j<ntris; j++) {
				if(aabox_tri_test(&node->aabb, tree->tris + j)) {
					darr_push(node->tris, tree->tris + j);
				}
			}

			/*if(darr_size(node->tris) != ntris) {
				printf("subnode[%d]: added %d / %d tris\n", i, darr_size(node->tris), ntris);
			}*/

			if(darr_empty(node->tris)) {
				/* no triangles intersect this node, drop it */
				oct_free(node);
				node = 0;
			}
			tree->child[i] = node;
		}

		darr_free(tree->tris);
		tree->tris = 0;

		for(i=0; i<8; i++) {
			if(tree->child[i]) {
				oct_build(tree->child[i], maxdepth - 1, maxnodetris);
			}
		}
	}
}

int oct_raytest(const struct octnode *tree, const cgm_ray *ray, float tmax, struct trihit *hitptr)
{
	int i, count;
	struct trihit hit, hit0 = {FLT_MAX};

	if(!tree || !ray_aabox_any(ray, &tree->aabb, tmax)) {
		return 0;
	}

	if(oct_isleaf(tree)) {
		/* leaf node, find nearest intersection with the polygons */
		count = darr_size(tree->tris);
		for(i=0; i<count; i++) {
			if(ray_triangle(ray, tree->tris + i, tmax, &hit) && hit.t < hit0.t) {
				hit0 = hit;
			}
		}
	} else {
		/* recurse and find nearest intersection among the child nodes */
		for(i=0; i<8; i++) {
			if(!tree->child[i]) continue;
			if(oct_raytest(tree->child[i], ray, tmax, &hit) && hit.t < hit0.t) {
				hit0 = hit;
			}
		}
	}

	if(hit0.tri) {
		if(hitptr) *hitptr = hit0;
		return 1;
	}
	return 0;
}

struct octnode *oct_find_leaf(struct octnode *tree, float x, float y, float z)
{
	int i;
	struct octnode *par;

	while(!oct_isleaf(tree)) {
		par = tree;
		for(i=0; i<8; i++) {
			if(aabox_contains(&tree->child[i]->aabb, x, y, z)) {
				tree = tree->child[i];
				break;
			}
		}
		if(par == tree) return 0;
	}

	return tree;
}
