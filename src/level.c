#include "config.h"

#include <assert.h>
#include <float.h>
#include "level.h"
#include "goat3d.h"
#include "darray.h"
#include "util.h"
#include "treestor.h"
#include "options.h"

static int read_room(struct level *lvl, struct room *room, struct goat3d *gscn, struct goat3d_node *gnode);
static int conv_mesh(struct level *lvl, struct mesh *mesh, struct goat3d *gscn, struct goat3d_mesh *gmesh);
static void apply_objmod(struct level *lvl, struct room *room, struct mesh *mesh, struct ts_node *tsn);
static int read_action(struct action *act, struct ts_node *tsn);
static struct room *find_portal_link(struct level *lvl, struct portal *portal);
static void build_room_octree(struct room *room);

struct room *alloc_room(void)
{
	struct room *room = calloc_nf(1, sizeof *room);
	room->meshes = darr_alloc(0, sizeof *room->meshes);
	room->colmesh = darr_alloc(0, sizeof *room->colmesh);
	room->portals = darr_alloc(0, sizeof *room->portals);
	room->triggers = darr_alloc(0, sizeof *room->triggers);
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

	free(room->name);

	oct_free(room->octree);
}


void lvl_init(struct level *lvl)
{
	memset(lvl, 0, sizeof *lvl);
	lvl->rooms = darr_alloc(0, sizeof *lvl->rooms);
	lvl->textures = darr_alloc(0, sizeof *lvl->textures);
	lvl->actions = darr_alloc(0, sizeof *lvl->actions);
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

	free(lvl->datapath);
	free(lvl->pathbuf);
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
	struct aabox aabb;
	struct mesh *mesh;

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

	ts_free_tree(ts);
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

	/* first try outside of the level diretories for non-resizable textures */
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
	return tex;
}

#define NUM_ROOM_RAYS	3
struct room *lvl_room_at(const struct level *lvl, float x, float y, float z)
{
	/* XXX hack until we get the portals in, use N rays, and keep the best
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
	struct mesh mesh;
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
	} else if(match_prefix(name, "col_")) {
		if(type != GOAT3D_NODE_MESH) {
			fprintf(stderr, "ignoring non-mesh node with \"col_\" prefix: %s\n", name);
		} else {
			gmesh = goat3d_get_node_object(gnode);
			mesh_init(&mesh);
			if(conv_mesh(lvl, &mesh, gscn, gmesh) != -1) {
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
			if(conv_mesh(lvl, &mesh, gscn, gmesh) != -1) {
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
