#ifndef MESH_H_
#define MESH_H_

#include "cgmath/cgmath.h"
#include "mtltex.h"
#include "geom.h"

struct goat3d;
struct goat3d_mesh;

struct mesh {
	char *name;
	cgm_vec3 *varr, *narr;
	cgm_vec2 *uvarr;
	unsigned int *idxarr;
	long vcount, icount;
	int dlist;

	struct material mtl;
	struct aabox aabb;
};

typedef struct texture *(*mesh_tex_loader_func)(const char *fname, void *cls);

int mesh_init(struct mesh *m);
void mesh_destroy(struct mesh *m);

struct mesh *mesh_alloc(void);
void mesh_free(struct mesh *m);

void mesh_transform(struct mesh *m, const float *mat);
void mesh_calc_bounds(struct mesh *m);

int mesh_num_triangles(struct mesh *m);
void mesh_get_triangle(struct mesh *m, int idx, struct triangle *tri);

void mesh_draw(struct mesh *m);
void mesh_compile(struct mesh *m);

void mesh_tex_loader(mesh_tex_loader_func func, void *cls);
int mesh_read_goat3d(struct mesh *m, struct goat3d *gscn, struct goat3d_mesh *gmesh);
int mesh_load(struct mesh *m, const char *fname, const char *mname);
void mesh_dumpobj(const struct mesh *m, const char *fname);

/* --- mesh generation --- */

void gen_sphere(struct mesh *mesh, float rad, int usub, int vsub, float urange, float vrange);
void gen_geosphere(struct mesh *mesh, float rad, int subdiv, int hemi);
void gen_torus(struct mesh *mesh, float mainrad, float ringrad, int usub, int vsub,
		float urange, float vrange);
void gen_cylinder(struct mesh *mesh, float rad, float height, int usub, int vsub,
		int capsub, float urange, float vrange);
void gen_cone(struct mesh *mesh, float rad, float height, int usub, int vsub,
		int capsub, float urange, float vrange);
void gen_plane(struct mesh *mesh, float width, float height, int usub, int vsub);
void gen_heightmap(struct mesh *mesh, float width, float height, int usub, int vsub,
		float (*hf)(float, float, void*), void *hfdata);
void gen_box(struct mesh *mesh, float xsz, float ysz, float zsz, int usub, int vsub);

void gen_revol(struct mesh *mesh, int usub, int vsub, cgm_vec2 (*rfunc)(float, float, void*),
		cgm_vec2 (*nfunc)(float, float, void*), void *cls);

/* callback args: (float u, float v, void *cls) -> Vec2 XZ offset u,v in [0, 1] */
void gen_sweep(struct mesh *mesh, float height, int usub, int vsub,
		cgm_vec2 (*sfunc)(float, float, void*), void *cls);


#endif	/* MESH_H_ */
