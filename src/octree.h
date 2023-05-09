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
int oct_sphtest(const struct octnode *tree, const cgm_vec3 *pt, float rad, struct trihit *hit);

#define oct_isleaf(n)	((n)->tris)
struct octnode *oct_find_leaf(struct octnode *tree, float x, float y, float z);

#endif	/* OCTREE_H_ */
