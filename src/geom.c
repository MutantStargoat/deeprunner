#include <float.h>
#include "geom.h"

void tri_cons(struct triangle *tri, const cgm_vec3 *a, const cgm_vec3 *b, const cgm_vec3 *c)
{
	cgm_vec3 ab, ac;

	tri->v[0] = *a;
	tri->v[1] = *b;
	tri->v[2] = *c;

	ab = *b; cgm_vsub(&ab, a);
	ac = *c; cgm_vsub(&ac, c);
	cgm_vcross(&tri->norm, &ab, &ac);
	cgm_vnormalize(&tri->norm);
}

int ray_triangle(const cgm_ray *ray, const struct triangle *tri, float tmax)
{
	float t, ndotdir;
	cgm_vec3 vdir, bc, pos;

	if(fabs(ndotdir = cgm_vdot(&ray->dir, &tri->norm)) <= 1e-6) {
		return 0;
	}

	vdir = tri->v[0];
	cgm_vsub(&vdir, &ray->origin);

	if((t = cgm_vdot(&tri->norm, &vdir) / ndotdir) <= 1e-6 || t > tmax) {
		return 0;
	}

	cgm_raypos(&pos, ray, t);
	cgm_bary(&bc, &tri->v[0], &tri->v[1], &tri->v[2], &pos);

	if(bc.x < 0.0f || bc.x > 1.0f) return 0;
	if(bc.y < 0.0f || bc.y > 1.0f) return 0;
	if(bc.z < 0.0f || bc.z > 1.0f) return 0;

	return 1;
}

#define SLABCHECK(dim)	\
	do { \
		invdir = 1.0f / ray->dir.dim;	\
		t0 = (box->vmin.dim - ray->origin.dim) * invdir;	\
		t1 = (box->vmax.dim - ray->origin.dim) * invdir;	\
		if(invdir < 0.0f) {	\
			tmp = t0;	\
			t0 = t1;	\
			t1 = tmp;	\
		}	\
		tmin = t0 > tmin ? t0 : tmin;	\
		tmax = t1 < tmax ? t1 : tmax;	\
		if(tmax < tmin) return 0; \
	} while(0)


int ray_aabox_any(const cgm_ray *ray, const struct aabox *box, float tmax)
{
	float invdir, t0, t1, tmp;
	float tmin = 0.0f;

	SLABCHECK(x);
	SLABCHECK(y);
	SLABCHECK(z);

	return 1;
}

void aabox_init(struct aabox *box)
{
	box->vmin.x = box->vmin.y = box->vmin.z = FLT_MAX;
	box->vmax.x = box->vmax.y = box->vmax.z = -FLT_MAX;
}

int aabox_contains(const struct aabox *aabb, float x, float y, float z)
{
	if(x < aabb->vmin.x || x >= aabb->vmax.x) return 0;
	if(y < aabb->vmin.y || y >= aabb->vmin.y) return 0;
	if(z < aabb->vmin.z || z >= aabb->vmin.z) return 0;
	return 1;
}

void aabox_union(struct aabox *a, const struct aabox *b)
{
	if(b->vmin.x < a->vmin.x) a->vmin.x = b->vmin.x;
	if(b->vmin.y < a->vmin.y) a->vmin.y = b->vmin.y;
	if(b->vmin.z < a->vmin.z) a->vmin.z = b->vmin.z;
	if(b->vmax.x > a->vmax.x) a->vmax.x = b->vmax.x;
	if(b->vmax.y > a->vmax.y) a->vmax.y = b->vmax.y;
	if(b->vmax.z > a->vmax.z) a->vmax.z = b->vmax.z;
}

void aabox_union_point(struct aabox *box, const cgm_vec3 *pt)
{
	if(pt->x < box->vmin.x) box->vmin.x = pt->x;
	if(pt->y < box->vmin.y) box->vmin.y = pt->y;
	if(pt->z < box->vmin.z) box->vmin.z = pt->z;
	if(pt->x > box->vmax.x) box->vmax.x = pt->x;
	if(pt->y > box->vmax.y) box->vmax.y = pt->y;
	if(pt->z > box->vmax.z) box->vmax.z = pt->z;
}

int aabox_aabox_test(const struct aabox *a, const struct aabox *b)
{
	if(a->vmax.x < b->vmin.x || a->vmin.x > b->vmax.x) return 0;
	if(a->vmax.y < b->vmin.y || a->vmin.y > b->vmax.y) return 0;
	if(a->vmax.z < b->vmin.z || a->vmin.z > b->vmax.z) return 0;
	return 1;
}

/* XXX this is a rough triangle-aabox test which might miss some triangles
 * it should be fine for our use case though
 */
int aabox_tri_test(const struct aabox *box, const struct triangle *tri)
{
	int i;
	cgm_ray ray;

	for(i=0; i<3; i++) {
		if(aabox_contains(box, tri->v[i].x, tri->v[i].y, tri->v[i].z)) {
			return 1;
		}
	}

	for(i=0; i<3; i++) {
		ray.origin = tri->v[i];
		ray.dir = tri->v[(i + 1) % 3];
		cgm_vsub(&ray.dir, &ray.origin);

		if(ray_aabox_any(&ray, box, 1.0f)) {
			return 1;
		}
	}

	return 0;
}

