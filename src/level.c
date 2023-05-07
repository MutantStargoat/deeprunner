#include "config.h"

#include <assert.h>
#include <float.h>
#include "level.h"
#include "goat3d.h"
#include "darray.h"
#include "util.h"
#include "treestor.h"
#include "options.h"
#include "loading.h"

static int read_room(struct level *lvl, struct room *room, struct goat3d *gscn, struct goat3d_node *gnode);
static void apply_objmod(struct level *lvl, struct room *room, struct mesh *mesh, struct ts_node *tsn);
static int read_action(struct action *act, struct ts_node *tsn);
static int add_dynmesh(struct level *lvl, struct ts_node *tsn);
static int proc_dynobj(struct level *lvl, struct ts_node *tsn);
static struct room *find_portal_link(struct level *lvl, struct portal *portal);
static void build_room_octree(struct room *room);

struct room *alloc_room(void)
{
	struct room *room = calloc_nf(1, sizeof *room);
	room->meshes = darr_alloc(0, sizeof *room->meshes);
	room->colmesh = darr_alloc(0, sizeof *room->colmesh);
	room->portals = darr_alloc(0, sizeof *room->portals);
	room->triggers = darr_alloc(0, sizeof *room->triggers);
	room->objects = darr_alloc(0, sizeof *room->objects);
	room->emitters = darr_alloc(0, sizeof *room->emitters);
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

	darr_free(room->portals);

	count = darr_size(room->triggers);
	darr_free(room->triggers);

	count = darr_size(room->objects);
	for(i=0; i<count; i++) {
		free(room->objects[i]->name);
		free(room->objects[i]);
	}
	darr_free(room->objects);

	count = darr_size(room->emitters);
	for(i=0; i<count; i++) {
		psys_free(room->emitters[i]);
	}
	darr_free(room->emitters);

	free(room->name);

	oct_free(room->octree);
}


void lvl_init(struct level *lvl)
{
	memset(lvl, 0, sizeof *lvl);
	lvl->rooms = darr_alloc(0, sizeof *lvl->rooms);
	lvl->textures = darr_alloc(0, sizeof *lvl->textures);
	lvl->actions = darr_alloc(0, sizeof *lvl->actions);
	lvl->dynmeshes = darr_alloc(0, sizeof *lvl->dynmeshes);
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

	for(i=0; i<darr_size(lvl->actions); i++) {
		free(lvl->actions[i].name);
	}
	darr_free(lvl->actions);

	for(i=0; i<darr_size(lvl->dynmeshes); i++) {
		mesh_destroy(lvl->dynmeshes[i]);
		free(lvl->dynmeshes[i]);
	}
	darr_free(lvl->dynmeshes);


	free(lvl->datapath);
	free(lvl->pathbuf);
}

static struct texture *texload_wrapper(const char *fname, void *cls)
{
	return lvl_texture(cls, fname);
}

static int count_goat3d_textures(struct goat3d *gscn)
{
	int i, ntex = 0, nmtl = goat3d_get_mtl_count(gscn);
	for(i=0; i<nmtl; i++) {
		struct goat3d_material *mtl = goat3d_get_mtl(gscn, i);
		/* XXX change this if we change the number of textures we load from
		 * the material
		 */
		if(goat3d_get_mtl_attrib_map(mtl, GOAT3D_MAT_ATTR_DIFFUSE)) {
			ntex++;
		}
		if(goat3d_get_mtl_attrib_map(mtl, GOAT3D_MAT_ATTR_REFLECTION)) {
			ntex++;
		}
	}
	return ntex;
}

static int count_goat3d_trees(struct goat3d *gscn)
{
	int i, ntrees = 0, nnodes = goat3d_get_node_count(gscn);
	for(i=0; i<nnodes; i++) {
		struct goat3d_node *node = goat3d_get_node(gscn, i);
		if(!goat3d_get_node_parent(node)) {
			ntrees++;
		}
	}
	return ntrees;
}

int lvl_load(struct level *lvl, const char *fname)
{
	int i, j, count, numport;
	struct ts_node *ts, *tsnode;
	const char *scnfile, *str;
	float *vec;
	struct goat3d *gscn;
	struct goat3d_node *gnode;
	struct room *room;
	struct mesh *mesh;

	aabox_init(&lvl->aabb);

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
	if((str = ts_lookup_str(ts, "level.texpath", 0))) {
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

	if(!(gscn = goat3d_create()) || goat3d_load(gscn, scnfile) == -1) {
		fprintf(stderr, "lvl_load(%s): failed to load scene file: %s\n", fname, scnfile);
		ts_free_tree(ts);
		return -1;
	}

	/* change the amount of work expected by the loader */
	count = count_goat3d_textures(gscn) + count_goat3d_trees(gscn);
	/* +1 for the stepping we'll do immediately for having loaded the scene file */
	loading_additems(count + 1);
	loading_step();

	mesh_tex_loader(texload_wrapper, lvl);

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

		aabox_union(&lvl->aabb, &room->aabb);

		/* if the room has no collision meshes, use the renderable meshes for
		 * collisions and issue a warning
		 */
		if(darr_empty(room->colmesh)) {
			/*fprintf(stderr, "lvl_load(%s): room %s: no collision meshes, using render meshes!\n",
					fname, room->name);*/
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

		loading_step();
	}

	/* make a list of all trigger actions first, to instanciate triggers while
	 * reading the objects
	 */
	tsnode = ts->child_list;
	while(tsnode) {
		struct action act;

		if(strcmp(tsnode->name, "action") == 0) {
			if(read_action(&act, tsnode) != -1) {
				darr_push(lvl->actions, &act);
			}
		}
		tsnode = tsnode->next;
	}

	/* look for named object modifications */
	tsnode = ts->child_list;
	while(tsnode) {
		if(strcmp(tsnode->name, "object") == 0) {
			if(!(str = ts_get_attr_str(tsnode, "name", 0))) {
				fprintf(stderr, "skipping object mod without an object name\n");
				tsnode = tsnode->next;
				continue;
			}
			if(!(mesh = lvl_find_mesh(lvl, str, &room))) {
				fprintf(stderr, "skipping object mod, \"%s\" not found\n", str);
				tsnode = tsnode->next;
				continue;
			}
			apply_objmod(lvl, room, mesh, tsnode);
		}
		tsnode = tsnode->next;
	}

	/* add dynamic meshes */
	tsnode = ts->child_list;
	while(tsnode) {
		if(strcmp(tsnode->name, "dynmesh") == 0) {
			add_dynmesh(lvl, tsnode);
		}
		tsnode = tsnode->next;
	}

	/* add dynamic objects */
	tsnode = ts->child_list;
	while(tsnode) {
		if(strcmp(tsnode->name, "dynobject") == 0) {
			proc_dynobj(lvl, tsnode);
		}
		tsnode = tsnode->next;
	}

	/* link up rooms through their portals */
	count = darr_size(lvl->rooms);
	for(i=0; i<count; i++) {
		room = lvl->rooms[i];

		numport = darr_size(room->portals);
		for(j=0; j<numport; j++) {
			if(!(room->portals[j].link = find_portal_link(lvl, room->portals + j))) {
				fprintf(stderr, "warning: unlinked portal \"%s\" (room: \"%s\")\n",
						room->portals[j].name, room->name);
			}
		}
	}

	if(lvl->aabb.vmin.x >= lvl->aabb.vmax.x) {
		lvl->maxdist = 0;
	} else {
		lvl->maxdist = cgm_vdist(&lvl->aabb.vmin, &lvl->aabb.vmax);
	}
	printf("level diameter: %g\n", lvl->maxdist);

	mesh_tex_loader(0, 0);
	ts_free_tree(ts);
	return 0;
}

struct room *lvl_find_room(const struct level *lvl, const char *name)
{
	int i, count = darr_size(lvl->rooms);
	for(i=0; i<count; i++) {
		if(strcmp(lvl->rooms[i]->name, name) == 0) {
			return lvl->rooms[i];
		}
	}
	return 0;
}

struct mesh *lvl_find_mesh(const struct level *lvl, const char *name, struct room **meshroom)
{
	int i, j, nrooms, nmeshes;

	nrooms = darr_size(lvl->rooms);
	for(i=0; i<nrooms; i++) {
		nmeshes = darr_size(lvl->rooms[i]->meshes);
		for(j=0; j<nmeshes; j++) {
			struct mesh *m = lvl->rooms[i]->meshes + j;
			if(m->name && strcmp(m->name, name) == 0) {
				if(meshroom) *meshroom = lvl->rooms[i];
				return m;
			}
		}
	}
	return 0;
}

struct object *lvl_find_dynobj(const struct level *lvl, const char *name)
{
	int i, j, nrooms, nobj;

	nrooms = darr_size(lvl->rooms);
	for(i=0; i<nrooms; i++) {
		nobj = darr_size(lvl->rooms[i]->objects);
		for(j=0; j<nobj; j++) {
			struct object *obj = lvl->rooms[i]->objects[j];
			if(obj->name && strcmp(obj->name, name) == 0) {
				return obj;
			}
		}
	}
	return 0;
}


struct mesh *lvl_find_dynmesh(const struct level *lvl, const char *name)
{
	int i, count = darr_size(lvl->dynmeshes);
	for(i=0; i<count; i++) {
		if(lvl->dynmeshes[i]->name && strcmp(lvl->dynmeshes[i]->name, name) == 0) {
			return lvl->dynmeshes[i];
		}
	}
	return 0;
}

static const char *find_datafile(struct level *lvl, const char *fname)
{
	int len;
	FILE *fp;
	static const char *szdir[] = {"low", "mid", "high"};

	if(!lvl->datapath) return fname;

	len = strlen(lvl->datapath) + strlen(fname) + 16;
	if(lvl->pathbuf_sz < len + 1) {
		lvl->pathbuf = realloc_nf(lvl->pathbuf, len + 1);
	}

	/* first try outside of the level directories for non-resizable textures */
	sprintf(lvl->pathbuf, "%s/%s", lvl->datapath, fname);
	if((fp = fopen(lvl->pathbuf, "rb"))) {
		fclose(fp);
		return lvl->pathbuf;
	}

	/* try in the currently selected texture level directory */
	sprintf(lvl->pathbuf, "%s/%s/%s", lvl->datapath, szdir[opt.gfx.texsize], fname);
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
	loading_step();
	return tex;
}

#define NUM_ROOM_RAYS	3
struct room *lvl_room_at(const struct level *lvl, float x, float y, float z)
{
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

		for(j=0; j<NUM_ROOM_RAYS; j++) {
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
	int i, num_portals;

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
		cgm_raypos(&col->pos, &ray, hit.t);
		col->norm = hit.tri->norm;
		col->depth = tri_plane_dist(hit.tri, pos);
		return 1;
	}

	/* if we didn't hit anything, we probably hit a portal, test the portals and
	 * continue testing across the next room */
	num_portals = darr_size(room->portals);
	for(i=0; i<num_portals; i++) {
		struct portal *port = room->portals + i;
		if(ray_sphere(&ray, &port->pos, port->rad, 0)) {
			return lvl_collision(lvl, port->link, pos, vel, col);
		}
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

static void make_portal(struct portal *portal, struct goat3d_node *gnode)
{
	int i, vcount;
	struct goat3d_mesh *gmesh;
	cgm_vec3 *varr, v;
	float xform[16];
	float dsq, max_dsq;

	gmesh = goat3d_get_node_object(gnode);
	goat3d_get_matrix(gnode, xform);

	vcount = goat3d_get_mesh_vertex_count(gmesh);
	varr = (cgm_vec3*)goat3d_get_mesh_attribs(gmesh, GOAT3D_MESH_ATTR_VERTEX);

	cgm_vcons(&portal->pos, 0, 0, 0);
	for(i=0; i<vcount; i++) {
		v = varr[i];
		cgm_vmul_m4v3(&v, xform);
		cgm_vadd(&portal->pos, &v);
	}
	cgm_vscale(&portal->pos, 1.0f / vcount);

	max_dsq = 0;
	for(i=0; i<vcount; i++) {
		v = varr[i];
		cgm_vmul_m4v3(&v, xform);
		cgm_vsub(&v, &portal->pos);
		if((dsq = cgm_vlength_sq(&v)) > max_dsq) {
			max_dsq = dsq;
		}
	}
	portal->rad = sqrt(max_dsq);
}

static int read_room(struct level *lvl, struct room *room, struct goat3d *gscn, struct goat3d_node *gnode)
{
	int i, count;
	const char *name = goat3d_get_node_name(gnode);
	enum goat3d_node_type type = goat3d_get_node_type(gnode);
	struct goat3d_mesh *gmesh;
	struct portal portal;
	struct mesh mesh, *dynmesh;
	struct object *obj;
	float xform[16];

	if(match_prefix(name, "portal_")) {
		if(type != GOAT3D_NODE_MESH) {
			fprintf(stderr, "ignoring non-mesh node with \"portal_\" prefix: %s\n", name);
		} else {
			/* initially just create the portals and leave them unlinked */
			make_portal(&portal, gnode);
			portal.room = room;
			portal.link = 0;
			portal.name = strdup_nf(name);
			darr_push(room->portals, &portal);
		}
	} else if(match_prefix(name, "dummy_")) {
		obj = calloc_nf(1, sizeof *obj);
		obj->name = strdup_nf(name);
		goat3d_get_node_position(gnode, &obj->pos.x, &obj->pos.y, &obj->pos.z);
		goat3d_get_node_rotation(gnode, &obj->rot.x, &obj->rot.y, &obj->rot.z, &obj->rot.w);
		goat3d_get_node_matrix(gnode, obj->matrix);
		darr_push(room->objects, &obj);
	} else if(match_prefix(name, "dyn_")) {
		if(type != GOAT3D_NODE_MESH) {
			fprintf(stderr, "ignoring non-mesh with \"dyn_\" prefix: %s\n", name);
		} else {
			gmesh = goat3d_get_node_object(gnode);
			dynmesh = mesh_alloc();
			if(mesh_read_goat3d(dynmesh, gscn, gmesh) != -1) {
				free(dynmesh->name);
				dynmesh->name = strdup_nf(name);
				mesh_calc_bounds(dynmesh);
				darr_push(lvl->dynmeshes, &dynmesh);
			}

			/* add an object for this dynmesh in the room it was found in */
			obj = calloc_nf(1, sizeof *obj);
			obj->name = strdup_nf(name);
			obj->mesh = dynmesh;
			goat3d_get_node_position(gnode, &obj->pos.x, &obj->pos.y, &obj->pos.z);
			goat3d_get_node_rotation(gnode, &obj->rot.x, &obj->rot.y, &obj->rot.z, &obj->rot.w);
			goat3d_get_node_matrix(gnode, obj->matrix);
			darr_push(room->objects, &obj);
			return 0;	/* no hierarchy for dynmeshes for now */
		}
	} else if(match_prefix(name, "col_")) {
		if(type != GOAT3D_NODE_MESH) {
			fprintf(stderr, "ignoring non-mesh node with \"col_\" prefix: %s\n", name);
		} else {
			gmesh = goat3d_get_node_object(gnode);
			mesh_init(&mesh);
			if(mesh_read_goat3d(&mesh, gscn, gmesh) != -1) {
				free(mesh.name);
				mesh.name = strdup_nf(name);

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
			if(mesh_read_goat3d(&mesh, gscn, gmesh) != -1) {
				free(mesh.name);
				mesh.name = strdup_nf(name);

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

static struct action *find_action(struct level *lvl, const char *name)
{
	int i;
	for(i=0; i<darr_size(lvl->actions); i++) {
		if(strcmp(lvl->actions[i].name, name) == 0) {
			return lvl->actions + i;
		}
	}
	return 0;
}

static void apply_objmod(struct level *lvl, struct room *room, struct mesh *mesh, struct ts_node *tsn)
{
	const char *str;
	float *vec;

	if((vec = ts_lookup_vec(tsn, "object.uvanim.velocity", 0))) {
		mesh->mtl.uvanim = 1;
		mesh->mtl.texvel.x = vec[0];
		mesh->mtl.texvel.y = vec[1];
	}
	if((vec = ts_lookup_vec(tsn, "object.uvanim.offset", 0))) {
		mesh->mtl.uvanim = 1;
		mesh->mtl.uvoffs.x = vec[0];
		mesh->mtl.uvoffs.y = vec[1];
	}

	if(ts_lookup_int(tsn, "object.mtlattr.emissive", 0)) {
		mesh->mtl.emissive = 1;
	}

	/* check if this adds any triggers */
	if((str = ts_lookup_str(tsn, "object.trigger", 0))) {
		struct trigger trig;
		struct action *act = find_action(lvl, str);
		if(!act) {
			fprintf(stderr, "object \"%s\" refers to unknown trigger action \"%s\"\n",
					mesh->name, str);
		} else {
			/* assume bounding box has already been computed by now (see read_mesh) */
			trig.box = mesh->aabb;
			trig.act = *act;
			darr_push(room->triggers, &trig);
			printf("add trigger for \"%s\" to room \"%s\"\n", act->name, room->name);
		}
	}
}

static int read_action(struct action *act, struct ts_node *tsn)
{
	int i;
	const char *str;
	struct ts_attr *attr;

	static const struct {
		const char *aname;
		int ttype;
		enum action_type atype;
	} actions[] = {
		{"damage", TS_NUMBER, ACT_DAMAGE},
		{"shield", TS_NUMBER, ACT_SHIELD},
		{"win", -1, ACT_WIN}
	};

	if(!(str = ts_get_attr_str(tsn, "name", 0))) {
		fprintf(stderr, "skipping action without a name\n");
		return -1;
	}
	act->name = strdup_nf(str);

	attr = tsn->attr_list;
	while(attr) {
		for(i=0; i<sizeof actions/sizeof *actions; i++) {
			if(strcmp(attr->name, actions[i].aname) != 0) {
				continue;
			}

			if(actions[i].ttype >= 0 && actions[i].ttype != attr->val.type) {
				fprintf(stderr, "unexpected type for action: %s\n", act->name);
				free(act->name);
				return -1;
			}

			act->type = actions[i].atype;
			if(attr->val.type == TS_NUMBER) {
				act->value = attr->val.fnum;
			} else {
				act->value = 0;
			}
			return 0;
		}
		attr = attr->next;
	}
	return -1;
}


static int add_dynmesh(struct level *lvl, struct ts_node *tsn)
{
	struct mesh *mesh;
	const char *name, *fname, *meshname;

	if(!(name = ts_get_attr_str(tsn, "name", 0))) {
		fprintf(stderr, "skipping dynmesh without name\n");
		return -1;
	}
	if(!(fname = ts_get_attr_str(tsn, "file", 0))) {
		fprintf(stderr, "skipping dynmesh without filename\n");
		return -1;
	}
	meshname = ts_get_attr_str(tsn, "mesh", 0);

	mesh = mesh_alloc();
	mesh->name = strdup_nf(name);
	if(mesh_load(mesh, fname, meshname) == -1) {
		fprintf(stderr, "failed to load dynmesh \"%s\" from: %s\n", mesh->name, fname);
		mesh_destroy(mesh);
		return -1;
	}
	mesh_calc_bounds(mesh);
	darr_push(lvl->dynmeshes, &mesh);
	return 0;
}

static int proc_dynobj(struct level *lvl, struct ts_node *tsn)
{
	struct object *obj;
	struct mesh *mesh;
	const char *str, *name;
	float *vec;
	struct action *act;

	if(!(name = ts_get_attr_str(tsn, "name", 0))) {
		fprintf(stderr, "skipping dynobject without name\n");
		return -1;
	}

	if(!(obj = lvl_find_dynobj(lvl, name))) {
		fprintf(stderr, "proc_dynobj: failed to find dynobj: %s\n", name);
		return -1;
	}

	if((str = ts_get_attr_str(tsn, "mesh", 0))) {
		if(!(mesh = lvl_find_dynmesh(lvl, str))) {
			fprintf(stderr, "proc_dynobj(%s): failed to find mesh: %s\n", name, str);
			return -1;
		}
		obj->mesh = mesh;
	}
	if((str = ts_get_attr_str(tsn, "colmesh", 0))) {
		if(!(mesh = lvl_find_dynmesh(lvl, str))) {
			fprintf(stderr, "proc_dynobj(%s): failed to find collision mesh: %s\n", name, str);
			return -1;
		}
		obj->colmesh = mesh;
	}

	if((vec = ts_get_attr_vec(tsn, "rotaxis", 0))) {
		cgm_vcons(&obj->rotaxis, vec[0], vec[1], vec[2]);
		cgm_vnormalize(&obj->rotaxis);
		if((obj->rotspeed = ts_get_attr_num(tsn, "rotspeed", 0.0f)) != 0.0f) {
			obj->anim_rot = 1;
		}
	}

	if((str = ts_get_attr_str(tsn, "action", 0))) {
		if(!(act = find_action(lvl, str))) {
			fprintf(stderr, "proc_dynobj(%s): failed to find action: %s\n", name, str);
			return -1;
		}
		obj->act = *act;
	}

	return 0;
}

static struct room *find_portal_link(struct level *lvl, struct portal *portal)
{
	int i, j, nrooms, nport;
	struct room *room;
	struct portal *portb;

	nrooms = darr_size(lvl->rooms);
	for(i=0; i<nrooms; i++) {
		room = lvl->rooms[i];
		nport = darr_size(room->portals);
		for(j=0; j<nport; j++) {
			portb = room->portals + j;
			if(portb == portal) continue;
			if(sph_sph_test(&portal->pos, portal->rad, &portb->pos, portb->rad)) {
				return room;
			}
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
