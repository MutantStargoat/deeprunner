#include "level.h"
#include "goat3d.h"
#include "darray.h"
#include "util.h"
#include "treestor.h"

static int read_room(struct level *lvl, struct room *room, struct goat3d *gscn, struct goat3d_node *gnode);
static int conv_mesh(struct level *lvl, struct mesh *mesh, struct goat3d *gscn, struct goat3d_mesh *gmesh);
static void build_room_octree(struct octnode *octn, struct level *lvl);

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

	count = darr_size(room->colmesh);
	for(i=0; i<count; i++) {
		mesh_destroy(room->colmesh + i);
	}
	darr_free(room->colmesh);

	free(room->name);
}


void lvl_init(struct level *lvl)
{
	lvl->rooms = darr_alloc(0, sizeof *lvl->rooms);
	lvl->textures = darr_alloc(0, sizeof *lvl->textures);
	lvl->roomtree = 0;
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

	oct_free(lvl->roomtree);
}

int lvl_load(struct level *lvl, const char *fname)
{
	int i, count;
	struct ts_node *ts;
	const char *scnfile;
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

		darr_push(lvl->rooms, &room);
	}

	/* TODO read room nodes with portal links */

	/* construct octree for mapping regions of space to rooms containing them */
	printf("level %s bounds: %g,%g,%g -> %g,%g,%g\n", fname, aabb.vmin.x,
			aabb.vmin.y, aabb.vmin.z, aabb.vmax.x, aabb.vmax.y, aabb.vmax.z);
	if(!(lvl->roomtree = oct_create(&aabb))) {
		ts_free_tree(ts);
		return -1;
	}

	build_room_octree(lvl->roomtree, lvl);

	ts_free_tree(ts);
	return 0;
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

	if(!(tex = tex_load(fname))) {
		return 0;
	}
	darr_push(lvl->textures, &tex);
	return tex;
}

struct room *lvl_room_at(struct level *lvl, float x, float y, float z)
{
	struct octnode *octn;

	if(!(octn = oct_find_leaf(lvl->roomtree, x, y, z))) {
		return 0;
	}
	return octn->data;
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
				goat3d_get_node_matrix(gnode, xform);
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
				goat3d_get_node_matrix(gnode, xform);
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
	int nfaces;
	void *data;
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
		mesh->uvarr = malloc_nf(mesh->vcount * sizeof *mesh->uvarr);
		memcpy(mesh->uvarr, data, mesh->vcount * sizeof *mesh->uvarr);
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

static void build_room_octree(struct octnode *octn, struct level *lvl)
{
	int i, j, nrooms, nmeshes;
	int num_cont = 0;
	struct room *cont_room = 0;

	/* find rooms intersecting with this node, by checking with the room
	 * bounding box first, and then all vertices of the room's collision
	 * geometry
	 */
	for(i=0; i<nrooms; i++) {

	}
}
