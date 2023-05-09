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
#include <stdio.h>
#include <float.h>
#include "geom.h"

void tri_cons(struct triangle *tri, const cgm_vec3 *a, const cgm_vec3 *b, const cgm_vec3 *c)
{
	tri->v[0] = *a;
	tri->v[1] = *b;
	tri->v[2] = *c;

	tri_calc_normal(tri);

	tri->data = 0;
}

void tri_calc_normal(struct triangle *tri)
{
	cgm_vec3 ab, ac;

	ab = tri->v[1]; cgm_vsub(&ab, tri->v);
	ac = tri->v[2]; cgm_vsub(&ac, tri->v);
	cgm_vcross(&tri->norm, &ab, &ac);
	cgm_vnormalize(&tri->norm);
}

float tri_plane_dist(const struct triangle *tri, const cgm_vec3 *pt)
{
	float d = cgm_vdot(tri->v, &tri->norm);
	return cgm_vdot(&tri->norm, pt) - d;
}

/* Taken from "Realtime Collision Detection" by Christer Ericson.
 * ch.5.1.5 p.141
 */
void tri_proj_pt(cgm_vec3 *res, const struct triangle *tri, const cgm_vec3 *pt)
{
	cgm_vec3 ab, ac, ap, bp, cp;
	float d1, d2, d3, d4, d5, d6, d43, d56, vc, vb, va, v, w, denom;

	/* check if pt is in vertex region outside v[0] */
	ab = tri->v[1]; cgm_vsub(&ab, tri->v);
	ac = tri->v[2]; cgm_vsub(&ac, tri->v);
	ap = *pt; cgm_vsub(&ap, tri->v);
	d1 = cgm_vdot(&ab, &ap);
	d2 = cgm_vdot(&ac, &ap);
	if(d1 <= 0.0f && d2 <= 0.0f) {
		*res = tri->v[0];	/* bary (1, 0, 0) */
		return;
	}

	/* check if pt is in vertex region outside v[1] */
	bp = *pt; cgm_vsub(&bp, tri->v + 1);
	d3 = cgm_vdot(&ab, &bp);
	d4 = cgm_vdot(&ac, &bp);
	if(d3 >= 0.0f && d4 <= d3) {
		*res = tri->v[1];	/* bary (0, 1, 0) */
		return;
	}

	/* check if pt is in edge region of e01 -> return proj of pt to e01 */
	vc = d1 * d4 - d3 * d2;
	if(vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
		v = d1 / (d1 - d3);
		res->x = tri->v[0].x + ab.x * v;
		res->y = tri->v[0].y + ab.y * v;
		res->z = tri->v[0].z + ab.z * v;
		return;	/* bary (1-v, v, 0) */
	}

	/* check if pt is in vertex region outside v[2] */
	cp = *pt; cgm_vsub(&cp, tri->v + 2);
	d5 = cgm_vdot(&ab, &cp);
	d6 = cgm_vdot(&ac, &cp);
	if(d6 >= 0.0f && d5 <= d6) {
		*res = tri->v[2];	/* bary (0, 0, 1) */
		return;
	}

	/* check if pt is in edge region of e02 -> return proj of pt to e02 */
	vb = d5 * d2 - d1 * d6;
	if(vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
		w = d2 / (d2 - d6);
		res->x = tri->v[0].x + ac.x * w;
		res->y = tri->v[0].y + ac.y * w;
		res->z = tri->v[0].z + ac.z * w;
		return;	/* bary (1-w, 0, w) */
	}

	/* check if pt is in edge region of e12 -> return proj of pt to e12 */
	va = d3 * d6 - d5 * d4;
	d43 = d4 - d3;
	d56 = d5 - d6;
	if(va <= 0.0f && d43 >= 0.0f && d56 >= 0.0f) {
		w = d43 / (d43 + d56);
		res->x = tri->v[1].x + (tri->v[2].x - tri->v[1].x) * w;
		res->y = tri->v[1].y + (tri->v[2].y - tri->v[1].y) * w;
		res->z = tri->v[1].z + (tri->v[2].z - tri->v[1].z) * w;
		return;	/* bary (0, 1-w, w) */
	}

	/* pt inside face region, compute proj through barycentric coords (u,v,w) */
	denom = 1.0f / (va + vb + vc);
	v = vb * denom;
	w = vc * denom;
	res->x = tri->v[0].x + ab.x * v + ac.x * w;
	res->y = tri->v[0].y + ab.y * v + ac.y * w;
	res->z = tri->v[0].z + ab.z * v + ac.z * w;
}

int tri_sphere_test(const struct triangle *tri, const cgm_vec3 *cent, float rad, float *distret)
{
	cgm_vec3 cproj;
	float dx, dy, dz, dsq;

	tri_proj_pt(&cproj, tri, cent);
	dx = cproj.x - cent->x;
	dy = cproj.y - cent->y;
	dz = cproj.z - cent->z;
	dsq = dx * dx + dy * dy + dz * dz;
	if(dsq <= rad * rad) {
		if(distret) *distret = dsq;
		return 1;
	}
	return 0;
}

int ray_triangle(const cgm_ray *ray, const struct triangle *tri, float tmax, struct trihit *hit)
{
	float t, ndotdir;
	cgm_vec3 vdir, bc, pos;

	if(fabs(ndotdir = cgm_vdot(&ray->dir, &tri->norm)) <= 1e-6) {
		return 0;
	}

	vdir = tri->v[0];
	cgm_vsub(&vdir, &ray->origin);

	if((t = cgm_vdot(&tri->norm, &vdir) / ndotdir) <= 0.0f || t > tmax) {
		return 0;
	}

	cgm_raypos(&pos, ray, t);
	cgm_bary(&bc, &tri->v[0], &tri->v[1], &tri->v[2], &pos);

	if(bc.x < 0.0f || bc.x > 1.0f) return 0;
	if(bc.y < 0.0f || bc.y > 1.0f) return 0;
	if(bc.z < 0.0f || bc.z > 1.0f) return 0;

	if(hit) {
		hit->t = t;
		cgm_raypos(&hit->pt, ray, t);
		hit->tri = tri;
	}

	return 1;
}

#define SLABCHECK(dim)	\
	do { \
		if(ray->dir.dim != 0.0f) { \
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
		} \
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
	if(y < aabb->vmin.y || y >= aabb->vmax.y) return 0;
	if(z < aabb->vmin.z || z >= aabb->vmax.z) return 0;
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

/* aabox/plane intersection test taken from "Realtime Collision Detection" by
 * Christer Ericson. ch.5.2.3, p.164.
 */
int aabox_plane_test(const struct aabox *box, const struct plane *plane)
{
	cgm_vec3 c, e;
	float r, s;

	/* compute aabox center/extents */
	c.x = (box->vmin.x + box->vmax.x) * 0.5f;
	c.y = (box->vmin.y + box->vmax.y) * 0.5f;
	c.z = (box->vmin.z + box->vmax.z) * 0.5f;
	e = box->vmax; cgm_vsub(&e, &c);

	/* compute the projection interval radius of box onto L(t) = c + norm * t */
	r = e.x * fabs(plane->norm.x) + e.y * fabs(plane->norm.y) + e.z * fabs(plane->norm.z);
	/* compute distance of box center from plane */
	s = cgm_vdot(&plane->norm, &c) - plane->d;
	/* intersects if distance s in [-r, r] */
	return fabs(s) <= r;
}

static CGM_INLINE float fltmin(float a, float b) { return a < b ? a : b; }
static CGM_INLINE float fltmax(float a, float b) { return a > b ? a : b; }
#define fltmin3(a, b, c)	fltmin(fltmin(a, b), c)
#define fltmax3(a, b, c)	fltmax(fltmax(a, b), c)

/* aabox/triangle intersection test based on algorithm from
 * "Realtime Collision Detection" by Christer Ericson. ch.5.2.9, p.171.
 */
int aabox_tri_test(const struct aabox *box, const struct triangle *tri)
{
	int i, j;
	float p0, p1, p2, r, e0, e1, e2, minp, maxp;
	cgm_vec3 c, v0, v1, v2, f[3];
	cgm_vec3 ax;
	struct plane plane;
	static const cgm_vec3 bax[] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};

	/* compute aabox center/extents */
	c.x = (box->vmin.x + box->vmax.x) * 0.5f;
	c.y = (box->vmin.y + box->vmax.y) * 0.5f;
	c.z = (box->vmin.z + box->vmax.z) * 0.5f;
	e0 = (box->vmax.x - box->vmin.x) * 0.5f;
	e1 = (box->vmax.y - box->vmin.y) * 0.5f;
	e2 = (box->vmax.z - box->vmin.z) * 0.5f;

	/* translate triangle to the coordinate system of the bounding box */
	v0 = tri->v[0]; cgm_vsub(&v0, &c);
	v1 = tri->v[1]; cgm_vsub(&v1, &c);
	v2 = tri->v[2]; cgm_vsub(&v2, &c);

	/* my own addition: this seems to fail sometimes when the triangle is
	 * entirely inside the aabox. let's add a hack to catch that...
	 */
	if(fabs(v0.x) <= e0 && fabs(v0.y) <= e1 && fabs(v0.z) <= e2) return 1;
	if(fabs(v1.x) <= e0 && fabs(v1.y) <= e1 && fabs(v1.z) <= e2) return 1;
	if(fabs(v2.x) <= e0 && fabs(v2.y) <= e1 && fabs(v2.z) <= e2) return 1;

	/* compute edge vectors for triangle */
	f[0] = v1; cgm_vsub(f, &v0);
	f[1] = v2; cgm_vsub(f + 1, &v1);
	f[2] = v0; cgm_vsub(f + 2, &v2);

	/* test axes a00..a22 */
	for(i=0; i<3; i++) {
		for(j=0; j<3; j++) {
			cgm_vcross(&ax, bax + i, f + j);
			p0 = cgm_vdot(&v0, &ax);
			p1 = cgm_vdot(&v1, &ax);
			p2 = cgm_vdot(&v2, &ax);
			r = e0 * fabs(cgm_vdot(f, &ax)) + e1 * fabs(cgm_vdot(f + 1, &ax)) +
				e2 * fabs(cgm_vdot(f + 2, &ax));

			minp = fltmin3(p0, p1, p2);
			maxp = fltmax3(p0, p1, p2);

			if(minp > r || maxp < -r) return 0;		/* found separating axis */
		}
	}

	if(fltmax3(v0.x, v1.x, v2.x) < -e0 || fltmin3(v0.x, v1.x, v2.x) > e0) return 0;
	if(fltmax3(v0.y, v1.y, v2.y) < -e1 || fltmin3(v0.y, v1.y, v2.y) > e1) return 0;
	if(fltmax3(v0.z, v1.z, v2.z) < -e2 || fltmin3(v0.z, v1.z, v2.z) > e2) return 0;

	/*plane.norm = tri->norm;*/
	cgm_vcross(&plane.norm, f, f + 1);
	/*plane.d = cgm_vdot(&tri->norm, &v0);*/
	plane.d = cgm_vdot(&plane.norm, &v0);
	return aabox_plane_test(box, &plane);
}

float aabox_distsq(const struct aabox *box, const cgm_vec3 *pt)
{
	float dmin, dmax, dsq = 0.0f;

	dmin = box->vmin.x - pt->x;
	dmax = pt->x - box->vmax.x;
	if(dmin > 0) dsq += dmin * dmin;
	if(dmax > 0) dsq += dmax * dmax;

	dmin = box->vmin.y - pt->y;
	dmax = pt->y - box->vmax.y;
	if(dmin > 0) dsq += dmin * dmin;
	if(dmax > 0) dsq += dmax * dmax;

	dmin = box->vmin.z - pt->z;
	dmax = pt->z - box->vmax.z;
	if(dmin > 0) dsq += dmin * dmin;
	if(dmax > 0) dsq += dmax * dmax;

	return dsq;
}

int aabox_sph_test(const struct aabox *box, const cgm_vec3 *pt, float rad)
{
	float dsq = aabox_distsq(box, pt);
	return dsq <= rad * rad;
}

int ray_sphere(const cgm_ray *ray, const cgm_vec3 *cent, float rad, float *distret)
{
	float a, b, c, d, sqrt_d, t1, t2;

	a = cgm_vdot(&ray->dir, &ray->dir);
	b = 2.0f * ray->dir.x * (ray->origin.x - cent->x) +
		2.0f * ray->dir.y * (ray->origin.y - cent->y) +
		2.0f * ray->dir.z * (ray->origin.z - cent->z);
	c = cgm_vdot(cent, cent) + cgm_vdot(&ray->origin, &ray->origin) +
		cgm_vdot(cent, &ray->origin) * -2.0f - rad * rad;

	if((d = b * b - 4.0 * a * c) < 0.0) return 0;

	sqrt_d = sqrt(d);
	t1 = (-b + sqrt_d) / (2.0 * a);
	t2 = (-b - sqrt_d) / (2.0 * a);

	if((t1 < 1e-6f && t2 < 1e-6f) || (t1 > 1.0f && t2 > 1.0f)) {
		return 0;
	}

	if(distret) {
		if(t2 < t1) t1 = t2;
		if(t1 < 1e-6f || t1 > 1.0f) {
			*distret = t2;
		} else {
			*distret = t1;
		}
	}
	return 1;
}

int sph_sph_test(const cgm_vec3 *apos, float arad, const cgm_vec3 *bpos, float brad)
{
	float radsum = arad + brad;
	cgm_vec3 dir = *bpos;
	cgm_vsub(&dir, apos);

	if(cgm_vlength_sq(&dir) < radsum * radsum) {
		return 1;
	}
	return 0;
}

float plane_point_sdist(const cgm_vec4 *plane, const cgm_vec3 *pt)
{
	return pt->x * plane->x + pt->y * plane->y + pt->z * plane->z + plane->w;
}
