#ifndef GEOM_H_
#define GEOM_H_

#include "cgmath/cgmath.h"

struct aabox {
	cgm_vec3 vmin, vmax;
};

struct plane {
	cgm_vec3 norm;
	float d;
};

struct triangle {
	cgm_vec3 v[3];
	cgm_vec3 norm;
	void *data;
};

struct trihit {
	float t;
	cgm_vec3 pt;
	const struct triangle *tri;
};

void tri_cons(struct triangle *tri, const cgm_vec3 *a, const cgm_vec3 *b, const cgm_vec3 *c);
void tri_calc_normal(struct triangle *tri);

int ray_triangle(const cgm_ray *ray, const struct triangle *tri, float tmax, struct trihit *hit);
int ray_aabox_any(const cgm_ray *ray, const struct aabox *box, float tmax);

void aabox_init(struct aabox *box);
int aabox_contains(const struct aabox *bb, float x, float y, float z);
void aabox_union(struct aabox *a, const struct aabox *b);
void aabox_union_point(struct aabox *bb, const cgm_vec3 *pt);
int aabox_aabox_test(const struct aabox *a, const struct aabox *b);
int aabox_tri_test(const struct aabox *box, const struct triangle *tri);

#endif	/* GEOM_H_ */
