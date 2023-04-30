#include "config.h"

#include <assert.h>
#include <float.h>
#include "level.h"
#include "goat3d.h"
#include "darray.h"
#include "util.h"
#include "treestor.h"

static int read_room(struct level *lvl, struct room *room, struct goat3d *gscn, struct goat3d_node *gnode);
static int conv_mesh(struct level *lvl, struct mesh *mesh, struct goat3d *gscn, struct goat3d_mesh *gmesh);
static void build_room_octree(struct room *room);

struct room *alloc_room(void)
{
	struct room *room = calloc_nf(1, sizeof *room);
	room->meshes = darr_alloc(0, sizeof *room->meshes);
	room->colmesh = darr_alloc(0, sizeof *room->colmesh);
	aabox_init(&room->aabb);
	return room;
}

void free_room(struct room *room)
{
	int i, count;

	if(!room) return;

	count = darr_size(room->meshes);
	for(i=0; i<count; i++) {
		mesh_destroy(room->meshes + i);
	}
	darr_free(room->meshes);

	/* for rooms without collision meshes, colmesh just points to meshes,
	 * so don't attempt to free twice
	 */
	if(room->colmesh && room->colmesh != room->meshes) {
		count = darr_size(room->colmesh);
		for(i=0; i<count; i++) {
			mesh_destroy(room->colmesh + i);
		}
		darr_free(room->colmesh);
	}

	free(room->name);

	oct_free(room->octree);
}


void lvl_init(struct level *lvl)
{
	memset(lvl, 0, sizeof *lvl);
	lvl->rooms = darr_alloc(0, sizeof *lvl->rooms);
	lvl->textures = darr_alloc(0, sizeof *lvl->textures);
	cgm_qcons(&lvl->startrot, 0, 0, 0, 1);
}

void lvl_destroy(struct level *lvl)
{
	int i;
	for(i=0; i<darr_size(lvl->rooms); i++) {
		free_room(lvl->rooms[i]);
	}
	darr_free(lvl->rooms);

	for(i=0; i<darr_size(lvl->textures); i++) {
		tex_free(lvl->textures[i]);
	}
	darr_free(lvl->textures);

	free(lvl->datapath);
	free(lvl->pathbuf);
}

int lvl_load(struct level *lvl, const char *fname)
{
	int i, count;
	struct ts_node *ts;
	const char *scnfile, *str;
	float *vec;
	struct goat3d *gscn;
	struct goat3d_node *gnode;
	struct room *room;
	struct aabox aabb;

	aabox_init(&aabb);

	if(!(ts = ts_load(fname))) {
		fprintf(stderr, "lvl_load: failed to load level file: %s\n", fname);
		return -1;
	}

	if(!(scnfile = ts_lookup_str(ts, "level.scene", 0))) {
		fprintf(stderr, "lvl_load(%s): no scene attribute\n", fname);
		ts_free_tree(ts);
		return -1;
	}
	printf("lvl_load(%s): scene: %s\n", fname, scnfile);
	if((str = ts_lookup_str(ts, "level.datapath", 0))) {
		printf("  texture path: %s\n", str);
		lvl->datapath = strdup_nf(str);
	}

	if((vec = ts_lookup_vec(ts, "level.startpos", 0))) {
		printf("  start position: %g %g %g\n", vec[0], vec[1], vec[2]);
		cgm_vcons(&lvl->startpos, vec[0], vec[1], vec[2]);
	}
	if((vec = ts_lookup_vec(ts, "level.startrot", 0))) {
		printf("  start rotation: %g %g %g %g\n", vec[0], vec[1], vec[2], vec[3]);
		cgm_qcons(&lvl->startrot, vec[0], vec[1], vec[2], vec[3]);
		cgm_qnormalize(&lvl->startrot);
	}

	if(!(gscn = goat3d_create()) || goat3d_load(gscn, scnfile)) {
		fprintf(stderr, "lvl_load(%s): failed to load scene file: %s\n", fname, scnfile);
		ts_free_tree(ts);
		return -1;
	}

	count = goat3d_get_node_count(gscn);
	for(i=0; i<count; i++) {
		gnode = goat3d_get_node(gscn, i);
		if(goat3d_get_node_parent(gnode)) {
			continue;	/* only consider top-level nodes as rooms */
		}
		if(!(room = alloc_room())) {
			fprintf(stderr, "lvl_load(%s): failed to allocate room\n", fname);
			ts_free_tree(ts);
			return -1;
		}
		room->name = strdup_nf(goat3d_get_node_name(gnode));
		if(read_room(lvl, room, gscn, gnode) == -1) {
			fprintf(stderr, "lvl_load(%s): failed to read room\n", fname);
			free_room(room);
			continue;
		}

		aabox_union(&aabb, &room->aabb);

		/* if the room has no collision meshes, use the renderable meshes for
		 * collisions and issue a warning
		 */
		if(darr_empty(room->colmesh)) {
			fprintf(stderr, "lvl_load(%s): room %s: no collision meshes, using render meshes!\n",
					fname, room->name);
			darr_free(room->colmesh);
			room->colmesh = room->meshes;
		}

		/* construct octree for ray-tests with this room's collision mesh
		 * this also destroys the colmesh array if it's not the same as the
		 * renderable meshes array, because it's not needed any more. The octree
		 * replaces it completely.
		 */
		build_room_octree(room);

		darr_push(lvl->rooms, &room);
	}

	/* TODO read room nodes with portal links */

	ts_free_tree(ts);
	return 0;
}

static const char *find_datafile(struct level *lvl, const char *fname)
{
	int len;
	FILE *fp;

	if(!lvl->datapath) return fname;

	len = strlen(lvl->datapath) + strlen(fname) + 1;
	if(lvl->pathbuf_sz < len + 1) {
		lvl->pathbuf = realloc_nf(lvl->pathbuf, len + 1);
	}
	sprintf(lvl->pathbuf, "%s/%s", lvl->datapath, fname);

	if((fp = fopen(lvl->pathbuf, "rb"))) {
		fclose(fp);
		return lvl->pathbuf;
	}
	return fname;
}

struct texture *lvl_texture(struct level *lvl, const char *fname)
{
	struct texture *tex;
	int i, count = darr_size(lvl->textures);

	for(i=0; i<count; i++) {
		if(strcmp(lvl->textures[i]->img->name, fname) == 0) {
			return lvl->textures[i];
		}
	}

	if(!(tex = tex_load(find_datafile(lvl, fname)))) {
		return 0;
	}
	darr_push(lvl->textures, &tex);
	return tex;
}

struct room *lvl_room_at(const struct level *lvl, float x, float y, float z)
{
	/* XXX hack until we get the portals in, use 6 rays, and keep the best
	 * in case one or two happens to go through a portal
	 */
	int i, j, num_rooms;
	cgm_ray ray;
	struct room *room;
	int *roomhits, maxhits = 0;
	static const cgm_vec3 rdir[] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {-1, 0, 0},
		{0, -1, 0}, {0, 0, -1}};


	cgm_vcons(&ray.origin, x, y, z);

	num_rooms = darr_size(lvl->rooms);
	roomhits = alloca(num_rooms * sizeof *roomhits);
	memset(roomhits, 0, num_rooms * sizeof *roomhits);

	for(i=0; i<num_rooms; i++) {
		room = lvl->rooms[i];

		if(!aabox_contains(&room->aabb, x, y, z)) {
			continue;
		}

		for(j=0; j<6; j++) {
			ray.dir = rdir[j];
			if(oct_raytest(room->octree, &ray, FLT_MAX, 0)) {
				roomhits[i]++;
				if(roomhits[i] > maxhits) maxhits = roomhits[i];
			}
		}
	}

	if(maxhits) {
		for(i=0; i<num_rooms; i++) {
			if(roomhits[i] == maxhits) {
				return lvl->rooms[i];
			}
		}
	}
	return 0;
}


#ifdef DBG_SHOW_COLPOLY
extern const struct triangle *dbg_hitpoly;
#endif

int lvl_collision(const struct level *lvl, const struct room *room, const cgm_vec3 *pos,
		const cgm_vec3 *vel, struct collision *col)
{
	cgm_ray ray;
	struct trihit hit;

	if(!room) {
		if(!(room = lvl_room_at(lvl, pos->x, pos->y, pos->z))) {
			return 0;
		}
	}

	ray.origin = *pos;
	ray.dir = *vel;

	if(oct_raytest(room->octree, &ray, 1.0f, &hit)) {
#ifdef DBG_SHOW_COLPOLY
		dbg_hitpoly = hit.tri;
#endif
		cgm_raypos(&col->pos, &ray, hit.t * 0.95);
		col->norm = hit.tri->norm;
		col->depth = -tri_plane_dist(hit.tri, pos);
		return 1;
	}

	return 0;
}


#ifdef DBG_SHOW_COLPOLY
extern const struct triangle *dbg_hitpoly;
#endif

int lvl_collision_rad(const struct level *lvl, const struct room *room, const cgm_vec3 *pos,
		const cgm_vec3 *vel, float rad, struct collision *col)
{
	cgm_vec3 sphcent;
	struct trihit hit;

	if(!room) {
		if(!(room = lvl_room_at(lvl, pos->x, pos->y, pos->z))) {
			return 0;
		}
	}

	sphcent = *pos; cgm_vadd(&sphcent, vel);

	if(oct_sphtest(room->octree, &sphcent, rad, &hit)) {
#ifdef DBG_SHOW_COLPOLY
		dbg_hitpoly = hit.tri;
#endif
		col->norm = hit.tri->norm;
		return 1;
	}
	return 0;
}

static int read_room(struct level *lvl, struct room *room, struct goat3d *gscn, struct goat3d_node *gnode)
{
	int i, count;
	const char *name = goat3d_get_node_name(gnode);
	enum goat3d_node_type type = goat3d_get_node_type(gnode);
	struct goat3d_mesh *gmesh;
	struct mesh mesh;
	float xform[16];

	if(match_prefix(name, "portal_")) {
		/* ignore portals at this stage, we'll connect them up later */
	} else if(match_prefix(name, "col_")) {
		if(type != GOAT3D_NODE_MESH) {
			fprintf(stderr, "ignoring non-mesh node with \"col_\" prefix: %s\n", name);
		} else {
			gmesh = goat3d_get_node_object(gnode);
			mesh_init(&mesh);
			if(conv_mesh(lvl, &mesh, gscn, gmesh) != -1) {
				goat3d_get_matrix(gnode, xform);
				mesh_transform(&mesh, xform);
				mesh_calc_bounds(&mesh);
				aabox_union(&room->aabb, &mesh.aabb);
				darr_push(room->colmesh, &mesh);
			}
		}
	} else {
		if(type == GOAT3D_NODE_MESH) {
			gmesh = goat3d_get_node_object(gnode);
			mesh_init(&mesh);
			if(conv_mesh(lvl, &mesh, gscn, gmesh) != -1) {
				goat3d_get_matrix(gnode, xform);
				mesh_transform(&mesh, xform);
				mesh_calc_bounds(&mesh);
				aabox_union(&room->aabb, &mesh.aabb);
				darr_push(room->meshes, &mesh);
			}
		}
	}

	count = goat3d_get_node_child_count(gnode);
	for(i=0; i<count; i++) {
		read_room(lvl, room, gscn, goat3d_get_node_child(gnode, i));
	}
	return 0;
}

static int conv_mesh(struct level *lvl, struct mesh *mesh, struct goat3d *gscn, struct goat3d_mesh *gmesh)
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
		if((str = goat3d_get_mtl_attrib_map(gmtl, GOAT3D_MAT_ATTR_DIFFUSE))) {
			mesh->mtl.texmap = lvl_texture(lvl, str);
		}
		if((str = goat3d_get_mtl_attrib_map(gmtl, GOAT3D_MAT_ATTR_REFLECTION))) {
			mesh->mtl.envmap = lvl_texture(lvl, str);
		}
	}

	return 0;
}

#define MAX_OCT_DEPTH	8
#define MAX_OCT_TRIS	16

static void build_room_octree(struct room *room)
{
	int i, j, num_meshes, num_tris;
	struct triangle tri;

	if(!room->colmesh) return;

	room->octree = oct_create();

	num_meshes = darr_size(room->colmesh);
	for(i=0; i<num_meshes; i++) {
		num_tris = mesh_num_triangles(room->colmesh + i);
		for(j=0; j<num_tris; j++) {
			mesh_get_triangle(room->colmesh + i, j, &tri);
			assert(!isnan(tri.v[0].x) && !isnan(tri.v[0].y) && !isnan(tri.v[0].z));
			assert(!isnan(tri.v[1].x) && !isnan(tri.v[1].y) && !isnan(tri.v[1].z));
			assert(!isnan(tri.v[2].x) && !isnan(tri.v[2].y) && !isnan(tri.v[2].z));
			tri.data = room;
			oct_addtri(room->octree, &tri);
		}
	}

#ifndef DBG_NO_OCTREE
	oct_build(room->octree, MAX_OCT_DEPTH, MAX_OCT_TRIS);
#endif

	if(room->colmesh != room->meshes) {
		/* we don't need the collision meshes any more */
		num_meshes = darr_size(room->colmesh);
		for(i=0; i<num_meshes; i++) {
			mesh_destroy(room->colmesh + i);
		}
		darr_free(room->colmesh);
		room->colmesh = 0;
	}
}
