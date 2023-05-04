#ifndef LEVEL_H_
#define LEVEL_H_

#include "mesh.h"
#include "octree.h"

struct portal;

struct room {
	char *name;
	struct mesh *meshes;	/* darr */
	struct mesh *colmesh;	/* darr */
	struct aabox aabb;		/* axis-aligned bounding box of this room */
	struct octnode *octree;	/* octree for collision poly intersections */

	struct portal *portals;	/* darr */

	unsigned int vis_frm, rendered;
};

struct portal {
	char *name;
	struct room *room, *link;
	cgm_vec3 pos;
	float rad;
};

enum trigger_type {
	TRIG_DAMAGE,
	TRIG_SHIELD,
	TRIG_WIN
};

struct trigger {
	char *name;
	struct aabox box;

	enum trigger_type type;
	union {
		float damage;
		float shield;
		int win;
	} act;
};

struct level {
	struct room **rooms;		/* darr */
	struct texture **textures;	/* darr */

	cgm_vec3 startpos;
	cgm_quat startrot;

	char *datapath;
	char *pathbuf;
	int pathbuf_sz;
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

struct mesh *lvl_find_mesh(const struct level *lvl, const char *name);

struct texture *lvl_texture(struct level *lvl, const char *fname);

struct room *lvl_room_at(const struct level *lvl, float x, float y, float z);

int lvl_collision(const struct level *lvl, const struct room *room, const cgm_vec3 *pos,
		const cgm_vec3 *vel, struct collision *col);

int lvl_collision_rad(const struct level *lvl, const struct room *room, const cgm_vec3 *pos,
		const cgm_vec3 *vel, float rad, struct collision *col);

#endif	/* LEVEL_H_ */
