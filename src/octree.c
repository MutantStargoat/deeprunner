#include <stdio.h>
#include <stdlib.h>
#include "octree.h"

struct octnode *oct_create(struct aabox *bounds)
{
	struct octnode *root;

	if(!(root = calloc(1, sizeof *root))) {
		fprintf(stderr, "failed to allocate octree root\n");
		return 0;
	}
	root->aabb = *bounds;
	return root;
}

void oct_free(struct octnode *tree)
{
	int i;
	if(!tree) return;

	for(i=0; i<8; i++) {
		oct_free(tree->child[i]);
	}
	free(tree);
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
