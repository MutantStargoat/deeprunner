#ifndef LEVEL_H_
#define LEVEL_H_

#include "mesh.h"
#include "octree.h"
#include "psys/psys.h"

struct portal;

enum action_type {
	ACT_DAMAGE,
	ACT_SHIELD,
	ACT_WIN
};

struct action {
	char *name;
	enum action_type type;
	float value;
};

struct trigger {
	struct aabox box;
	struct action act;
};

struct object {
	char *name;
	struct mesh *mesh;
	struct mesh *colmesh;
	cgm_vec3 pos;
	cgm_quat rot;

	float matrix[16];	/* derived from pos/rot and parent if available */

	struct object *child_list, *parent;
	struct object *next;
};

struct room {
	char *name;
	struct mesh *meshes;	/* darr */
	struct mesh *colmesh;	/* darr */
	struct aabox aabb;		/* axis-aligned bounding box of this room */
	struct octnode *octree;	/* octree for collision poly intersections */

	struct portal *portals;		/* darr */
	struct trigger *triggers;	/* darr */

	struct object **objects;	/* darr */

	struct psys_emitter **emitters;

	unsigned int vis_frm, rendered;
};

struct portal {
	char *name;
	struct room *room, *link;
	cgm_vec3 pos;
	float rad;
};

struct level {
	struct room **rooms;		/* darr */
	struct texture **textures;	/* darr */

	cgm_vec3 startpos;
	cgm_quat startrot;

	struct action *actions;		/* darr */
	struct mesh **dynmeshes;	/* darr */

	char *datapath;
	char *pathbuf;
	int pathbuf_sz;

	struct aabox aabb;			/* bounding box of the entire level */
	float maxdist;				/* maximum distance in the level */
};

struct collision {
	cgm_vec3 pos;
	cgm_vec3 norm;
	float depth;
};

struct room *alloc_room(void);
void free_room(struct room *room);

void lvl_init(struct level *lvl);
void lvl_destroy(struct level *lvl);

int lvl_load(struct level *lvl, const char *fname);

/* meshroom pointer optional, if we care which room this mesh was in */
struct room *lvl_find_room(const struct level *lvl, const char *name);
struct mesh *lvl_find_mesh(const struct level *lvl, const char *name, struct room **meshroom);
struct mesh *lvl_find_dynmesh(const struct level *lvl, const char *name);

struct texture *lvl_texture(struct level *lvl, const char *fname);

struct room *lvl_room_at(const struct level *lvl, float x, float y, float z);

int lvl_collision(const struct level *lvl, const struct room *room, const cgm_vec3 *pos,
		const cgm_vec3 *vel, struct collision *col);

int lvl_collision_rad(const struct level *lvl, const struct room *room, const cgm_vec3 *pos,
		const cgm_vec3 *vel, float rad, struct collision *col);


#endif	/* LEVEL_H_ */
