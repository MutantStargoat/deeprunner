#include <stdio.h>
#include "mesh.h"
#include "darray.h"
#include "util.h"

/* -------- sphere -------- */

#define SURAD(u)	((u) * 2.0 * M_PI)
#define SVRAD(v)	((v) * M_PI)

static void sphvec(cgm_vec3 *v, float theta, float phi)
{
	v->x = sin(theta) * sin(phi);
	v->y = cos(phi);
	v->z = cos(theta) * sin(phi);
}

void gen_sphere(struct mesh *mesh, float rad, int usub, int vsub, float urange, float vrange)
{
	int i, j, uverts, vverts, num_verts, num_quads, num_tri, idx;
	unsigned int *idxarr;
	float u, v, du, dv, phi, theta;
	cgm_vec3 *varr, *narr, pos;
	cgm_vec2 *uvarr;

	if(urange == 0.0f || vrange == 0.0f) return;

	if(usub < 4) usub = 4;
	if(vsub < 2) vsub = 2;

	uverts = usub + 1;
	vverts = vsub + 1;

	num_verts = uverts * vverts;
	num_quads = usub * vsub;
	num_tri = num_quads * 2;

	mesh_init(mesh);
	varr = mesh->varr = malloc_nf(num_verts * sizeof *mesh->varr);
	narr = mesh->narr = malloc_nf(num_verts * sizeof *mesh->narr);
	uvarr = mesh->uvarr = malloc_nf(num_verts * sizeof *mesh->uvarr);
	idxarr = mesh->idxarr = malloc_nf(num_tri * 3 * sizeof *mesh->idxarr);
	mesh->vcount = num_verts;
	mesh->icount = num_tri * 3;

	du = urange / (float)(uverts - 1);
	dv = vrange / (float)(vverts - 1);

	u = 0.0;
	for(i=0; i<uverts; i++) {
		theta = u * 2.0 * M_PI;

		v = 0.0;
		for(j=0; j<vverts; j++) {
			phi = v * M_PI;

			sphvec(&pos, theta, phi);

			*narr++ = pos;
			cgm_vscale(&pos, rad);
			*varr++ = pos;
			uvarr->x = u / urange;
			uvarr->y = v / vrange;
			uvarr++;

			if(i < usub && j < vsub) {
				idx = i * vverts + j;
				*idxarr++ = idx;
				*idxarr++ = idx + 1;
				*idxarr++ = idx + vverts + 1;

				*idxarr++ = idx + vverts;
				*idxarr++ = idx;
				*idxarr++ = idx + vverts + 1;
			}

			v += dv;
		}
		u += du;
	}
}

/* ------ geosphere ------ */
#define PHI		1.618034

static cgm_vec3 icosa_pt[] = {
	{PHI, 1, 0},
	{-PHI, 1, 0},
	{PHI, -1, 0},
	{-PHI, -1, 0},
	{1, 0, PHI},
	{1, 0, -PHI},
	{-1, 0, PHI},
	{-1, 0, -PHI},
	{0, PHI, 1},
	{0, -PHI, 1},
	{0, PHI, -1},
	{0, -PHI, -1}
};
enum { P11, P12, P13, P14, P21, P22, P23, P24, P31, P32, P33, P34 };
static int icosa_idx[] = {
	P11, P31, P21,
	P11, P22, P33,
	P13, P21, P32,
	P13, P34, P22,
	P12, P23, P31,
	P12, P33, P24,
	P14, P32, P23,
	P14, P24, P34,

	P11, P33, P31,
	P12, P31, P33,
	P13, P32, P34,
	P14, P34, P32,

	P21, P13, P11,
	P22, P11, P13,
	P23, P12, P14,
	P24, P14, P12,

	P31, P23, P21,
	P32, P21, P23,
	P33, P22, P24,
	P34, P24, P22
};

static cgm_vec3 *geosphere(cgm_vec3 *verts, cgm_vec3 *v1, cgm_vec3 *v2, cgm_vec3 *v3, int iter, int hemi)
{
	cgm_vec3 v12, v23, v31;

	if(!iter) {
		if(hemi && (v1->y < 0.01 && v2->y < 0.01 && v3->y < 0.01)) {
			return verts;
		}
		darr_push(verts, v1);
		darr_push(verts, v2);
		darr_push(verts, v3);
		return verts;
	}

	v12 = *v1;
	cgm_vadd(&v12, v2);
	cgm_vnormalize(&v12);
	v23 = *v2;
	cgm_vadd(&v23, v3);
	cgm_vnormalize(&v23);
	v31 = *v3;
	cgm_vadd(&v31, v1);
	cgm_vnormalize(&v31);

	verts = geosphere(verts, v1, &v12, &v31, iter - 1, hemi);
	verts = geosphere(verts, v2, &v23, &v12, iter - 1, hemi);
	verts = geosphere(verts, v3, &v31, &v23, iter - 1, hemi);
	verts = geosphere(verts, &v12, &v23, &v31, iter - 1, hemi);
	return verts;
}

void gen_geosphere(struct mesh *mesh, float rad, int subdiv, int hemi)
{
	int i, j, num_verts, num_tri, vidx;
	cgm_vec3 v[3], *verts;
	cgm_vec3 *varr, *narr;
	cgm_vec2 *uvarr;
	float theta, phi;
	float mat[16];

	cgm_mrotation_x(mat, cgm_deg_to_rad(31.7175));

	num_tri = (sizeof icosa_idx / sizeof *icosa_idx) / 3;

	verts = darr_alloc(0, sizeof *verts);
	for(i=0; i<num_tri; i++) {
		for(j=0; j<3; j++) {
			vidx = icosa_idx[i * 3 + j];
			v[j] = icosa_pt[vidx];
			cgm_vmul_m4v3(v + j, mat);
			cgm_vnormalize(v + j);
		}

		verts = geosphere(verts, v, v + 1, v + 2, subdiv, hemi);
	}

	num_verts = darr_size(verts);

	mesh_init(mesh);
	varr = mesh->varr = malloc_nf(num_verts * sizeof *mesh->varr);
	narr = mesh->narr = malloc_nf(num_verts * sizeof *mesh->narr);
	uvarr = mesh->uvarr = malloc_nf(num_verts * sizeof *mesh->uvarr);
	mesh->vcount = num_verts;

	for(i=0; i<num_verts; i++) {
		*varr = verts[i];
		cgm_vscale(varr++, rad);
		*narr++ = verts[i];

		theta = atan2(verts[i].z, verts[i].x);
		phi = acos(verts[i].y);

		uvarr->x = 0.5 * theta / M_PI + 0.5;
		uvarr->y = phi / M_PI;
		uvarr++;
	}

	darr_free(verts);
}

/* -------- torus ----------- */
static void torusvec(cgm_vec3 *v, float theta, float phi, float mr, float rr)
{
	float rx, ry, rz;

	theta = -theta;

	rx = -cos(phi) * rr + mr;
	ry = sin(phi) * rr;
	rz = 0.0;

	v->x = rx * sin(theta) + rz * cos(theta);
	v->y = ry;
	v->z = -rx * cos(theta) + rz * sin(theta);
}

void gen_torus(struct mesh *mesh, float mainrad, float ringrad, int usub, int vsub, float urange, float vrange)
{
	int i, j, uverts, vverts, num_verts, num_quads, num_tri, idx;
	unsigned int *idxarr;
	cgm_vec3 *varr, *narr, pos, cent;
	cgm_vec2 *uvarr;
	float u, v, du, dv, theta, phi;

	if(usub < 4) usub = 4;
	if(vsub < 2) vsub = 2;

	uverts = usub + 1;
	vverts = vsub + 1;

	num_verts = uverts * vverts;
	num_quads = usub * vsub;
	num_tri = num_quads * 2;

	mesh_init(mesh);
	varr = mesh->varr = malloc_nf(num_verts * sizeof *mesh->varr);
	narr = mesh->narr = malloc_nf(num_verts * sizeof *mesh->narr);
	uvarr = mesh->uvarr = malloc_nf(num_verts * sizeof *mesh->uvarr);
	idxarr = mesh->idxarr = malloc_nf(num_tri * 3 * sizeof *mesh->idxarr);
	mesh->vcount = num_verts;
	mesh->icount = num_tri * 3;

	du = urange / (float)(uverts - 1);
	dv = vrange / (float)(vverts - 1);

	u = 0.0;
	for(i=0; i<uverts; i++) {
		theta = u * 2.0 * M_PI;

		v = 0.0;
		for(j=0; j<vverts; j++) {
			phi = v * 2.0 * M_PI;

			torusvec(&pos, theta, phi, mainrad, ringrad);
			torusvec(&cent, theta, phi, mainrad, 0.0);

			*varr++ = pos;
			*narr = pos;
			cgm_vsub(narr, &cent);
			cgm_vscale(narr, 1.0f / ringrad);
			narr++;

			uvarr->x = u * urange;
			uvarr->y = v * vrange;
			uvarr++;

			if(i < usub && j < vsub) {
				idx = i * vverts + j;
				*idxarr++ = idx;
				*idxarr++ = idx + 1;
				*idxarr++ = idx + vverts + 1;

				*idxarr++ = idx + vverts;
				*idxarr++ = idx;
				*idxarr++ = idx + vverts + 1;
			}

			v += dv;
		}
		u += du;
	}
}

/* -------- cylinder -------- */

static void cylvec(cgm_vec3 *v, float theta, float height)
{
	v->x = sin(theta);
	v->y = height;
	v->z = cos(theta);
}

void gen_cylinder(struct mesh *mesh, float rad, float height, int usub, int vsub, int capsub, float urange, float vrange)
{
	int i, j, uverts, vverts, num_body_verts, num_body_quads, num_body_tri, idx;
	int capvverts, num_cap_verts, num_cap_quads, num_cap_tri, num_verts, num_tri;
	cgm_vec3 *varr, *narr, pos, vprev, tang;
	cgm_vec2 *uvarr;
	float y, u, v, du, dv, theta, r;
	unsigned int *idxarr, vidx[4];

	if(usub < 4) usub = 4;
	if(vsub < 1) vsub = 1;

	uverts = usub + 1;
	vverts = vsub + 1;

	num_body_verts = uverts * vverts;
	num_body_quads = usub * vsub;
	num_body_tri = num_body_quads * 2;

	capvverts = capsub ? capsub + 1 : 0;
	num_cap_verts = uverts * capvverts;
	num_cap_quads = usub * capsub;
	num_cap_tri = num_cap_quads * 2;

	num_verts = num_body_verts + num_cap_verts * 2;
	num_tri = num_body_tri + num_cap_tri * 2;

	mesh_init(mesh);
	varr = mesh->varr = malloc_nf(num_verts * sizeof *mesh->varr);
	narr = mesh->narr = malloc_nf(num_verts * sizeof *mesh->narr);
	uvarr = mesh->uvarr = malloc_nf(num_verts * sizeof *mesh->uvarr);
	idxarr = mesh->idxarr = malloc_nf(num_tri * 3 * sizeof *mesh->idxarr);
	mesh->vcount = num_verts;
	mesh->icount = num_tri * 3;

	du = urange / (float)(uverts - 1);
	dv = vrange / (float)(vverts - 1);

	u = 0.0f;
	for(i=0; i<uverts; i++) {
		theta = SURAD(u);

		v = 0.0f;
		for(j=0; j<vverts; j++) {
			y = (v - 0.5) * height;
			cylvec(&pos, theta, y);

			cgm_vcons(varr++, pos.x * rad, pos.y, pos.z * rad);
			cgm_vcons(narr++, pos.x, 0.0f, pos.z);
			uvarr->x = u * urange;
			uvarr->y = v * vrange;
			uvarr++;

			if(i < usub && j < vsub) {
				idx = i * vverts + j;

				*idxarr++ = idx;
				*idxarr++ = idx + vverts + 1;
				*idxarr++ = idx + 1;

				*idxarr++ = idx;
				*idxarr++ = idx + vverts;
				*idxarr++ = idx + vverts + 1;
			}

			v += dv;
		}
		u += du;
	}


	/* now the cap! */
	if(!capsub) {
		return;
	}

	dv = 1.0 / (float)(capvverts - 1);

	u = 0.0;
	for(i=0; i<uverts; i++) {
		theta = SURAD(u);

		v = 0.0;
		for(j=0; j<capvverts; j++) {
			r = v * rad;

			cylvec(&pos, theta, height / 2.0f);
			cgm_vscale(&pos, r);
			pos.y = height / 2.0;
			cylvec(&vprev, theta - 0.1f, 0.0f);
			cylvec(&tang, theta + 0.1f, 0.0f);
			cgm_vsub(&tang, &vprev);
			cgm_vnormalize(&tang);

			*varr++ = pos;
			cgm_vcons(narr++, 0, 1, 0);
			uvarr->x = u * urange;
			uvarr->y = v;
			uvarr++;

			pos.y = -height / 2.0;
			*varr++ = pos;
			cgm_vcons(narr++, 0, -1, 0);
			uvarr->x = u * urange;
			uvarr->y = v;
			uvarr++;

			if(i < usub && j < capsub) {
				idx = num_body_verts + (i * capvverts + j) * 2;

				vidx[0] = idx;
				vidx[1] = idx + capvverts * 2;
				vidx[2] = idx + (capvverts + 1) * 2;
				vidx[3] = idx + 2;

				*idxarr++ = vidx[0];
				*idxarr++ = vidx[2];
				*idxarr++ = vidx[1];
				*idxarr++ = vidx[0];
				*idxarr++ = vidx[3];
				*idxarr++ = vidx[2];

				*idxarr++ = vidx[0] + 1;
				*idxarr++ = vidx[1] + 1;
				*idxarr++ = vidx[2] + 1;
				*idxarr++ = vidx[0] + 1;
				*idxarr++ = vidx[2] + 1;
				*idxarr++ = vidx[3] + 1;
			}

			v += dv;
		}
		u += du;
	}
}

/* -------- cone -------- */

static void conevec(cgm_vec3 *v, float theta, float y, float height)
{
	float scale = 1.0f - y / height;
	v->x = sin(theta) * scale;
	v->y = y;
	v->z = cos(theta) * scale;
}

void gen_cone(struct mesh *mesh, float rad, float height, int usub, int vsub, int capsub, float urange, float vrange)
{
	int i, j, uverts, vverts, num_body_verts, num_body_quads, num_body_tri, idx;
	int capvverts, num_cap_verts, num_cap_quads, num_cap_tri, num_verts, num_tri;
	cgm_vec3 *varr, *narr, pos, vprev, tang, bitang;
	cgm_vec2 *uvarr;
	unsigned int *idxarr, vidx[4];
	float u, v, du, dv, theta, y, r;

	if(usub < 4) usub = 4;
	if(vsub < 1) vsub = 1;

	uverts = usub + 1;
	vverts = vsub + 1;

	num_body_verts = uverts * vverts;
	num_body_quads = usub * vsub;
	num_body_tri = num_body_quads * 2;

	capvverts = capsub ? capsub + 1 : 0;
	num_cap_verts = uverts * capvverts;
	num_cap_quads = usub * capsub;
	num_cap_tri = num_cap_quads * 2;

	num_verts = num_body_verts + num_cap_verts;
	num_tri = num_body_tri + num_cap_tri;

	mesh_init(mesh);
	varr = mesh->varr = malloc_nf(num_verts * sizeof *mesh->varr);
	narr = mesh->narr = malloc_nf(num_verts * sizeof *mesh->narr);
	uvarr = mesh->uvarr = malloc_nf(num_verts * sizeof *mesh->uvarr);
	idxarr = mesh->idxarr = malloc_nf(num_tri * 3 * sizeof *mesh->idxarr);
	mesh->vcount = num_verts;
	mesh->icount = num_tri * 3;

	du = urange / (float)(uverts - 1);
	dv = vrange / (float)(vverts - 1);

	u = 0.0;
	for(i=0; i<uverts; i++) {
		theta = SURAD(u);

		v = 0.0;
		for(j=0; j<vverts; j++) {
			y = v * height;
			conevec(&pos, theta, y, height);

			conevec(&vprev, theta - 0.1f, 0.0f, height);
			conevec(&tang, theta + 0.1f, 0.0f, height);
			cgm_vsub(&tang, &vprev);
			cgm_vnormalize(&tang);
			conevec(&bitang, theta, y + 0.1f, height);
			cgm_vsub(&bitang, &pos);
			cgm_vnormalize(&bitang);

			cgm_vcons(varr++, pos.x * rad, pos.y, pos.z * rad);
			cgm_vcross(narr++, &tang, &bitang);
			uvarr->x = u * urange;
			uvarr->y = v * vrange;
			uvarr++;

			if(i < usub && j < vsub) {
				idx = i * vverts + j;

				*idxarr++ = idx;
				*idxarr++ = idx + vverts + 1;
				*idxarr++ = idx + 1;

				*idxarr++ = idx;
				*idxarr++ = idx + vverts;
				*idxarr++ = idx + vverts + 1;
			}

			v += dv;
		}
		u += du;
	}


	/* now the bottom cap! */
	if(!capsub) {
		return;
	}

	dv = 1.0 / (float)(capvverts - 1);

	u = 0.0;
	for(i=0; i<uverts; i++) {
		theta = SURAD(u);

		v = 0.0;
		for(j=0; j<capvverts; j++) {
			r = v * rad;

			conevec(&pos, theta, 0.0f, height);
			cgm_vscale(&pos, r);
			cylvec(&vprev, theta - 0.1f, 0.0f);

			*varr++ = pos;
			cgm_vcons(narr++, 0, -1, 0);
			uvarr->x = u * urange;
			uvarr->y = v;
			uvarr++;

			if(i < usub && j < capsub) {
				idx = num_body_verts + i * capvverts + j;

				vidx[0] = idx;
				vidx[1] = idx + capvverts;
				vidx[2] = idx + (capvverts + 1);
				vidx[3] = idx + 1;

				*idxarr++ = vidx[0];
				*idxarr++ = vidx[1];
				*idxarr++ = vidx[2];
				*idxarr++ = vidx[0];
				*idxarr++ = vidx[2];
				*idxarr++ = vidx[3];
			}

			v += dv;
		}
		u += du;
	}
}


/* -------- plane -------- */

void gen_plane(struct mesh *mesh, float width, float height, int usub, int vsub)
{
	gen_heightmap(mesh, width, height, usub, vsub, 0, 0);
}


/* ----- heightmap ------ */

void gen_heightmap(struct mesh *mesh, float width, float height, int usub, int vsub, float (*hf)(float, float, void*), void *hfdata)
{
	int i, j, uverts, vverts, num_verts, num_quads, num_tri, idx;
	cgm_vec3 *varr, *narr, normal, tang, bitan;
	cgm_vec2 *uvarr;
	unsigned int *idxarr;
	float u, v, du, dv, x, y, z, u1z, v1z;

	if(usub < 1) usub = 1;
	if(vsub < 1) vsub = 1;

	uverts = usub + 1;
	vverts = vsub + 1;
	num_verts = uverts * vverts;

	num_quads = usub * vsub;
	num_tri = num_quads * 2;

	mesh_init(mesh);
	varr = mesh->varr = malloc_nf(num_verts * sizeof *mesh->varr);
	narr = mesh->narr = malloc_nf(num_verts * sizeof *mesh->narr);
	uvarr = mesh->uvarr = malloc_nf(num_verts * sizeof *mesh->uvarr);
	idxarr = mesh->idxarr = malloc_nf(num_tri * 3 * sizeof *mesh->idxarr);
	mesh->vcount = num_verts;
	mesh->icount = num_tri * 3;

	du = 1.0f / (float)usub;
	dv = 1.0f / (float)vsub;

	u = 0.0f;
	for(i=0; i<uverts; i++) {
		v = 0.0;
		for(j=0; j<vverts; j++) {
			x = (u - 0.5) * width;
			y = (v - 0.5) * height;
			z = hf ? hf(u, v, hfdata) : 0.0;

			cgm_vcons(&normal, 0, 0, 1);
			if(hf) {
				u1z = hf(u + du, v, hfdata);
				v1z = hf(u, v + dv, hfdata);

				cgm_vcons(&tang, du * width, 0, u1z - z);
				cgm_vcons(&bitan, 0, dv * height, v1z - z);
				cgm_vcross(&normal, &tang, &bitan);
				cgm_vnormalize(&normal);
			}

			cgm_vcons(varr++, x, y, z);
			*narr++ = normal;
			uvarr->x = u;
			uvarr->y = v;
			uvarr++;

			if(i < usub && j < vsub) {
				idx = i * vverts + j;

				*idxarr++ = idx;
				*idxarr++ = idx + vverts + 1;
				*idxarr++ = idx + 1;

				*idxarr++ = idx;
				*idxarr++ = idx + vverts;
				*idxarr++ = idx + vverts + 1;
			}

			v += dv;
		}
		u += du;
	}
}


static inline void rev_vert(cgm_vec3 *res, float u, float v, cgm_vec2 (*rf)(float, float, void*), void *cls)
{
	cgm_vec2 pos = rf(u, v, cls);

	float angle = u * 2.0 * M_PI;
	res->x = pos.x * cos(angle);
	res->y = pos.y;
	res->z = pos.x * sin(angle);
}

/* ------ surface of revolution ------- */
void gen_revol(struct mesh *mesh, int usub, int vsub, cgm_vec2 (*rfunc)(float, float, void*),
		cgm_vec2 (*nfunc)(float, float, void*), void *cls)
{
	int i, j, uverts, vverts, num_verts, num_quads, num_tri, idx;
	cgm_vec3 *varr, *narr, pos, nextu, nextv, tang, normal, bitan;
	cgm_vec2 *uvarr;
	unsigned int *idxarr;
	float u, v, du, dv, new_v;

	if(!rfunc) return;
	if(usub < 3) usub = 3;
	if(vsub < 1) vsub = 1;

	uverts = usub + 1;
	vverts = vsub + 1;
	num_verts = uverts * vverts;

	num_quads = usub * vsub;
	num_tri = num_quads * 2;

	mesh_init(mesh);
	varr = mesh->varr = malloc_nf(num_verts * sizeof *mesh->varr);
	narr = mesh->narr = malloc_nf(num_verts * sizeof *mesh->narr);
	uvarr = mesh->uvarr = malloc_nf(num_verts * sizeof *mesh->uvarr);
	idxarr = mesh->idxarr = malloc_nf(num_tri * 3 * sizeof *mesh->idxarr);
	mesh->vcount = num_verts;
	mesh->icount = num_tri * 3;

	du = 1.0f / (float)(uverts - 1);
	dv = 1.0f / (float)(vverts - 1);

	u = 0.0f;
	for(i=0; i<uverts; i++) {
		v = 0.0f;
		for(j=0; j<vverts; j++) {
			rev_vert(&pos, u, v, rfunc, cls);

			rev_vert(&nextu, fmod(u + du, 1.0), v, rfunc, cls);
			tang = nextu;
			cgm_vsub(&tang, &pos);
			if(cgm_vlength_sq(&tang) < 1e-6) {
				new_v = v > 0.5f ? v - dv * 0.25f : v + dv * 0.25f;
				rev_vert(&nextu, fmod(u + du, 1.0f), new_v, rfunc, cls);
				tang = nextu;
				cgm_vsub(&tang, &pos);
			}

			if(nfunc) {
				rev_vert(&normal, u, v, nfunc, cls);
			} else {
				rev_vert(&nextv, u, v + dv, rfunc, cls);
				bitan = nextv;
				cgm_vsub(&bitan, &pos);
				if(cgm_vlength_sq(&bitan) < 1e-6f) {
					rev_vert(&nextv, u, v - dv, rfunc, cls);
					bitan = pos;
					cgm_vsub(&bitan, &nextv);
				}

				cgm_vcross(&normal, &tang, &bitan);
			}
			cgm_vnormalize(&normal);
			cgm_vnormalize(&tang);

			*varr++ = pos;
			*narr++ = normal;
			uvarr->x = u;
			uvarr->y = v;
			uvarr++;

			if(i < usub && j < vsub) {
				idx = i * vverts + j;

				*idxarr++ = idx;
				*idxarr++ = idx + vverts + 1;
				*idxarr++ = idx + 1;

				*idxarr++ = idx;
				*idxarr++ = idx + vverts;
				*idxarr++ = idx + vverts + 1;
			}

			v += dv;
		}
		u += du;
	}
}

static inline void sweep_vert(cgm_vec3 *res, float u, float v, float height,
		cgm_vec2 (*sf)(float, float, void*), void *cls)
{
	cgm_vec2 pos = sf(u, v, cls);

	res->x = pos.x;
	res->y = v * height;
	res->z = pos.y;
}

/* ---- sweep shape along a path ---- */
void gen_sweep(struct mesh *mesh, float height, int usub, int vsub,
		cgm_vec2 (*sfunc)(float, float, void*), void *cls)
{
	int i, j, uverts, vverts, num_verts, num_quads, num_tri, idx;
	cgm_vec3 *varr, *narr, pos, nextu, nextv, tang, bitan, normal;
	cgm_vec2 *uvarr;
	unsigned int *idxarr;
	float u, v, du, dv, new_v;

	if(!sfunc) return;
	if(usub < 3) usub = 3;
	if(vsub < 1) vsub = 1;

	uverts = usub + 1;
	vverts = vsub + 1;
	num_verts = uverts * vverts;

	num_quads = usub * vsub;
	num_tri = num_quads * 2;

	mesh_init(mesh);
	varr = mesh->varr = malloc_nf(num_verts * sizeof *mesh->varr);
	narr = mesh->narr = malloc_nf(num_verts * sizeof *mesh->narr);
	uvarr = mesh->uvarr = malloc_nf(num_verts * sizeof *mesh->uvarr);
	idxarr = mesh->idxarr = malloc_nf(num_tri * 3 * sizeof *mesh->idxarr);
	mesh->vcount = num_verts;
	mesh->icount = num_tri * 3;

	du = 1.0f / (float)(uverts - 1);
	dv = 1.0f / (float)(vverts - 1);

	u = 0.0f;
	for(i=0; i<uverts; i++) {
		v = 0.0f;
		for(j=0; j<vverts; j++) {
			sweep_vert(&pos, u, v, height, sfunc, cls);

			sweep_vert(&nextu, fmod(u + du, 1.0), v, height, sfunc, cls);
			tang = nextu;
			cgm_vsub(&tang, &pos);
			if(cgm_vlength_sq(&tang) < 1e-6f) {
				new_v = v > 0.5f ? v - dv * 0.25f : v + dv * 0.25f;
				sweep_vert(&nextu, fmod(u + du, 1.0f), new_v, height, sfunc, cls);
				tang = nextu;
				cgm_vsub(&tang, &pos);
			}

			sweep_vert(&nextv, u, v + dv, height, sfunc, cls);
			bitan = nextv;
			cgm_vsub(&bitan, &pos);
			if(cgm_vlength_sq(&bitan) < 1e-6f) {
				sweep_vert(&nextv, u, v - dv, height, sfunc, cls);
				bitan = pos;
				cgm_vsub(&bitan, &nextv);
			}

			cgm_vcross(&normal, &tang, &bitan);
			cgm_vnormalize(&normal);
			cgm_vnormalize(&tang);

			*varr++ = pos;
			*narr++ = normal;
			uvarr->x = u;
			uvarr->y = v;
			uvarr++;

			if(i < usub && j < vsub) {
				idx = i * vverts + j;

				*idxarr++ = idx;
				*idxarr++ = idx + vverts + 1;
				*idxarr++ = idx + 1;

				*idxarr++ = idx;
				*idxarr++ = idx + vverts;
				*idxarr++ = idx + vverts + 1;
			}

			v += dv;
		}
		u += du;
	}
}
