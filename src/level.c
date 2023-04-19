#include "level.h"
#include "goat3d.h"
#include "darray.h"
#include "util.h"
#include "treestor.h"

static int read_room(struct room *room, struct goat3d *gscn, struct goat3d_node *gnode);
static int conv_mesh(struct mesh *mesh, struct goat3d *gscn, struct goat3d_mesh *gmesh);

struct room *alloc_room(void)
{
	struct room *room = calloc_nf(1, sizeof *room);
	room->meshes = darr_alloc(0, sizeof *room->meshes);
	room->colmesh = darr_alloc(0, sizeof *room->colmesh);
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
}

void lvl_destroy(struct level *lvl)
{
	darr_free(lvl->rooms);
}

int lvl_load(struct level *lvl, const char *fname)
{
	int i, count;
	struct ts_node *ts;
	const char *scnfile;
	struct goat3d *gscn;
	struct goat3d_node *gnode;
	struct room *room;

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
		if(read_room(room, gscn, gnode) == -1) {
			fprintf(stderr, "lvl_load(%s): failed to read room\n", fname);
			free_room(room);
			continue;
		}

		darr_push(lvl->rooms, &room);
	}

	/* TODO read room nodes with portal links */

	ts_free_tree(ts);
	return 0;
}

static int read_room(struct room *room, struct goat3d *gscn, struct goat3d_node *gnode)
{
	int i, count;
	const char *name = goat3d_get_node_name(gnode);
	enum goat3d_node_type type = goat3d_get_node_type(gnode);
	struct goat3d_mesh *gmesh;
	struct mesh mesh;

	if(match_prefix(name, "portal_")) {
		/* ignore portals at this stage, we'll connect them up later */
	} else if(match_prefix(name, "col_")) {
		if(type != GOAT3D_NODE_MESH) {
			fprintf(stderr, "ignoring non-mesh node with \"col_\" prefix: %s\n", name);
		} else {
			gmesh = goat3d_get_node_object(gnode);
			if(conv_mesh(&mesh, gscn, gmesh) != -1) {
				darr_push(room->colmesh, &mesh);
			}
		}
	} else {
		if(type == GOAT3D_NODE_MESH) {
			gmesh = goat3d_get_node_object(gnode);
			if(conv_mesh(&mesh, gscn, gmesh) != -1) {
				darr_push(room->meshes, &mesh);
			}
		}
	}

	count = goat3d_get_node_child_count(gnode);
	for(i=0; i<count; i++) {
		read_room(room, gscn, goat3d_get_node_child(gnode, i));
	}
	return 0;
}

static int conv_mesh(struct mesh *mesh, struct goat3d *gscn, struct goat3d_mesh *gmesh)
{
	return -1;	/* TODO */
}
