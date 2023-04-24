#ifndef LEVEL_H_
#define LEVEL_H_

#include "mesh.h"
#include "octree.h"

struct room {
	char *name;
	struct mesh *meshes;	/* darr */
	struct mesh *colmesh;	/* darr */
	struct aabox aabb;		/* axis-aligned bounding box of this room */
	struct octnode *octree;	/* octree for collision poly intersections */
};

struct portal {
	struct room *room[2];
};

struct level {
	struct room **rooms;		/* darr */
	struct texture **textures;	/* darr */
};

struct room *alloc_room(void);
void free_room(struct room *room);

void lvl_init(struct level *lvl);
void lvl_destroy(struct level *lvl);

int lvl_load(struct level *lvl, const char *fname);

struct texture *lvl_texture(struct level *lvl, const char *fname);

struct room *lvl_room_at(struct level *lvl, float x, float y, float z);

#endif	/* LEVEL_H_ */
