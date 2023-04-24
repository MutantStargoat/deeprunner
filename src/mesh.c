#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "opengl.h"
#include "util.h"
#include "mesh.h"

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

void mesh_compile(struct mesh *m)
{
	if(!m || !m->vcount) return;

	if(m->dlist) {
		glDeleteLists(m->dlist, 1);
	}
	m->dlist = glGenLists(1);

	glNewList(m->dlist, GL_COMPILE);

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
	glEndList();
}
