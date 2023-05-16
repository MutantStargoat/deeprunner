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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "gaw/gaw.h"
#include "goat3d.h"
#include "util.h"
#include "mesh.h"

static mesh_tex_loader_func load_tex;
static void *load_tex_cls;


int mesh_init(struct mesh *m)
{
	memset(m, 0, sizeof *m);
	mtl_init(&m->mtl);
	return 0;
}

void mesh_destroy(struct mesh *m)
{
	if(!m) return;

	free(m->name);
	free(m->varr);
	free(m->narr);
	free(m->uvarr);
	free(m->idxarr);

	if(m->dlist) {
		gaw_free_compiled(m->dlist);
	}
}

struct mesh *mesh_alloc(void)
{
	struct mesh *m = malloc_nf(sizeof *m);
	mesh_init(m);
	return m;
}

void mesh_free(struct mesh *m)
{
	mesh_destroy(m);
	free(m);
}

void mesh_transform(struct mesh *m, const float *mat)
{
	int i;

	for(i=0; i<m->vcount; i++) {
		cgm_vmul_m4v3(m->varr + i, mat);
		if(m->narr) {
			cgm_vmul_m3v3(m->narr + i, mat);
			cgm_vnormalize(m->narr + i);
		}
	}
}

void mesh_calc_bounds(struct mesh *m)
{
	int i;
	float dsq;

	aabox_init(&m->aabb);
	cgm_vcons(&m->bsph_cent, 0, 0, 0);

	for(i=0; i<m->vcount; i++) {
		aabox_union_point(&m->aabb, m->varr + i);
		cgm_vadd(&m->bsph_cent, m->varr + i);
	}
	cgm_vscale(&m->bsph_cent, 1.0f / (float)m->vcount);

	m->bsph_rad = 0.0f;
	for(i=0; i<m->vcount; i++) {
		dsq = cgm_vdist_sq(&m->bsph_cent, m->varr + i);
		if(dsq > m->bsph_rad) {
			m->bsph_rad = dsq;
		}
	}
	m->bsph_rad = sqrt(m->bsph_rad);
}

int mesh_num_triangles(struct mesh *m)
{
	return (m->idxarr ? m->icount : m->vcount) / 3;
}

void mesh_get_triangle(struct mesh *m, int idx, struct triangle *tri)
{
	unsigned int vidx[3];

	idx *= 3;

	if(m->idxarr) {
		vidx[0] = m->idxarr[idx];
		vidx[1] = m->idxarr[idx + 1];
		vidx[2] = m->idxarr[idx + 2];
	} else {
		vidx[0] = idx;
		vidx[1] = idx + 1;
		vidx[2] = idx + 2;
	}
	tri->v[0] = m->varr[vidx[0]];
	tri->v[1] = m->varr[vidx[1]];
	tri->v[2] = m->varr[vidx[2]];

	tri_calc_normal(tri);
	tri->data = 0;
}

void mesh_draw(struct mesh *m)
{
	if(!m || !m->vcount) return;

	gaw_vertex_array(3, 0, m->varr);
	gaw_normal_array(0, m->narr);
	gaw_texcoord_array(2, 0, m->uvarr);

	if(m->idxarr) {
		gaw_draw_indexed(GAW_TRIANGLES, m->idxarr, m->icount);
	} else {
		gaw_draw(GAW_TRIANGLES, m->vcount);
	}

	gaw_vertex_array(0, 0, 0);
	gaw_normal_array(0, 0);
	gaw_texcoord_array(0, 0, 0);
}

void mesh_compile(struct mesh *m)
{
	if(!m || !m->vcount) return;

	if(m->dlist) {
		gaw_free_compiled(m->dlist);
	}
	m->dlist = gaw_compile_begin();
	mesh_draw(m);
	gaw_compile_end();
}


void mesh_tex_loader(mesh_tex_loader_func func, void *cls)
{
	load_tex = func;
	load_tex_cls = cls;
}


int mesh_read_goat3d(struct mesh *mesh, struct goat3d *gscn, struct goat3d_mesh *gmesh)
{
	int i, nfaces;
	void *data;
	cgm_vec2 *uvdata;
	struct goat3d_material *gmtl;
	const float *mattr;
	const char *str;

	mesh->name = strdup_nf(goat3d_get_mesh_name(gmesh));

	mesh->vcount = goat3d_get_mesh_vertex_count(gmesh);
	nfaces = goat3d_get_mesh_face_count(gmesh);
	mesh->icount = nfaces * 3;

	data = goat3d_get_mesh_attribs(gmesh, GOAT3D_MESH_ATTR_VERTEX);
	mesh->varr = malloc_nf(mesh->vcount * sizeof *mesh->varr);
	memcpy(mesh->varr, data, mesh->vcount * sizeof *mesh->varr);

	if((data = goat3d_get_mesh_attribs(gmesh, GOAT3D_MESH_ATTR_NORMAL))) {
		mesh->narr = malloc_nf(mesh->vcount * sizeof *mesh->narr);
		memcpy(mesh->narr, data, mesh->vcount * sizeof *mesh->narr);
	}

	if((data = goat3d_get_mesh_attribs(gmesh, GOAT3D_MESH_ATTR_TEXCOORD))) {
		uvdata = data;
		mesh->uvarr = malloc_nf(mesh->vcount * sizeof *mesh->uvarr);
		for(i=0; i<mesh->vcount; i++) {
			mesh->uvarr[i].x = uvdata[i].x;
			mesh->uvarr[i].y = 1.0f - uvdata[i].y;
		}
	}

	data = goat3d_get_mesh_faces(gmesh);
	mesh->idxarr = malloc_nf(mesh->icount * sizeof *mesh->idxarr);
	memcpy(mesh->idxarr, data, mesh->icount * sizeof *mesh->idxarr);

	mtl_init(&mesh->mtl);

	if((gmtl = goat3d_get_mesh_mtl(gmesh))) {
		if((mattr = goat3d_get_mtl_attrib(gmtl, GOAT3D_MAT_ATTR_DIFFUSE))) {
			cgm_wcons(&mesh->mtl.kd, mattr[0], mattr[1], mattr[2], 1);
		}
		if((mattr = goat3d_get_mtl_attrib(gmtl, GOAT3D_MAT_ATTR_SPECULAR))) {
			cgm_wcons(&mesh->mtl.ks, mattr[0], mattr[1], mattr[2], 1);
		}
		if((mattr = goat3d_get_mtl_attrib(gmtl, GOAT3D_MAT_ATTR_SHININESS))) {
			mesh->mtl.shin = mattr[0];
		}
		if((mattr = goat3d_get_mtl_attrib(gmtl, GOAT3D_MAT_ATTR_ALPHA))) {
			mesh->mtl.kd.w = mattr[0];
		}

		if(load_tex) {
			if((str = goat3d_get_mtl_attrib_map(gmtl, GOAT3D_MAT_ATTR_DIFFUSE))) {
				mesh->mtl.texmap = load_tex(str, load_tex_cls);
			}
			if((str = goat3d_get_mtl_attrib_map(gmtl, GOAT3D_MAT_ATTR_REFLECTION))) {
				mesh->mtl.envmap = load_tex(str, load_tex_cls);
			}
		}
	}

	return 0;
}

int mesh_load(struct mesh *m, const char *fname, const char *mname)
{
	struct goat3d *gscn;
	struct goat3d_mesh *gmesh;

	if(!(gscn = goat3d_create()) || goat3d_load(gscn, fname) == -1) {
		fprintf(stderr, "failed to load mesh: %s\n", fname);
		goat3d_free(gscn);
		return -1;
	}

	if(mname) {
		if(!(gmesh = goat3d_get_mesh_by_name(gscn, mname))) {
			fprintf(stderr, "mesh_load: failed to find mesh named \"%s\" in \"%s\"\n",
					mname, fname);
			goat3d_free(gscn);
			return -1;
		}
	} else {
		if(!(gmesh = goat3d_get_mesh(gscn, 0))) {
			fprintf(stderr, "mesh_load: %s contains no meshes\n", fname);
			goat3d_free(gscn);
			return -1;
		}
	}

	mesh_init(m);
	if(mesh_read_goat3d(m, gscn, gmesh) == -1) {
		fprintf(stderr, "mesh_load(%s): failed to convert mesh \"%s\"\n",
				fname, goat3d_get_mesh_name(gmesh));
		goat3d_free(gscn);
		return -1;
	}
	goat3d_free(gscn);
	return 0;
}

void mesh_dumpobj(const struct mesh *m, const char *fname)
{
	static const char *fmtstr[] = {" %u", " %u//%u", " %u/%u", " %u/%u/%u"};
	int i, j;
	unsigned int aflags = 0;
	cgm_vec3 *vptr;
	cgm_vec2 *uvptr;
	FILE *fp;

	if(!(fp = fopen(fname, "wb"))) {
		fprintf(stderr, "mesh_dumpobj: failed to open %s: %s\n", fname, strerror(errno));
		return;
	}
	if(m->name) {
		fprintf(fp, "# mesh: %s\n", m->name);
	}

	vptr = m->varr;
	for(i=0; i<m->vcount; i++) {
		fprintf(fp, "v %f %f %f\n", vptr->x, vptr->y, vptr->z);
		vptr++;
	}

	if(m->narr) {
		vptr = m->narr;
		for(i=0; i<m->vcount; i++) {
			fprintf(fp, "vn %f %f %f\n", vptr->x, vptr->y, vptr->z);
			vptr++;
		}
	}

	if(m->uvarr) {
		uvptr = m->uvarr;
		for(i=0; i<m->vcount; i++) {
			fprintf(fp, "vt %f %f\n", uvptr->x, uvptr->y);
			uvptr++;
		}
	}

	if(m->idxarr) {
		unsigned int *idxptr = m->idxarr;
		int numtri = m->icount / 3;

		for(i=0; i<numtri; i++) {
			fputc('f', fp);
			for(j=0; j<3; j++) {
				unsigned int idx = *idxptr++ + 1;
				fprintf(fp, fmtstr[aflags], idx, idx, idx);
			}
			fputc('\n', fp);
		}
	} else {
		int numtri = m->vcount / 3;
		unsigned int idx = 1;
		for(i=0; i<numtri; i++) {
			fputc('f', fp);
			for(j=0; j<3; j++) {
				fprintf(fp, fmtstr[aflags], idx, idx, idx);
				++idx;
			}
			fputc('\n', fp);
		}
	}

	fclose(fp);
}
