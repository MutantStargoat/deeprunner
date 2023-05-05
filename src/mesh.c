#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "opengl.h"
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
		glDeleteLists(m->dlist, 1);
	}
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

	aabox_init(&m->aabb);

	for(i=0; i<m->vcount; i++) {
		aabox_union_point(&m->aabb, m->varr + i);
	}
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

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, m->varr);
	if(m->narr) {
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, 0, m->narr);
	}
	if(m->uvarr) {
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, m->uvarr);
	}

	if(m->idxarr) {
		glDrawElements(GL_TRIANGLES, m->icount, GL_UNSIGNED_INT, m->idxarr);
	} else {
		glDrawArrays(GL_TRIANGLES, 0, m->vcount);
	}

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void mesh_compile(struct mesh *m)
{
	if(!m || !m->vcount) return;

	if(m->dlist) {
		glDeleteLists(m->dlist, 1);
	}
	m->dlist = glGenLists(1);

	glNewList(m->dlist, GL_COMPILE);
	mesh_draw(m);
	glEndList();
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


void mesh_dumpobj(struct mesh *m, const char *fname)
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
