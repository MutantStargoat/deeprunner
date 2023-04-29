#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
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
