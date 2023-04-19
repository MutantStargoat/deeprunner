#ifndef LEVEL_H_
#define LEVEL_H_

#include "mesh.h"

struct room {
	char *name;
	struct mesh *meshes;	/* darr */
	struct mesh *colmesh;	/* darr */
};

struct portal {
	struct room *room[2];
};

struct level {
	struct room **rooms;	/* darr */
};

struct room *alloc_room(void);
void free_room(struct room *room);

void lvl_init(struct level *lvl);
void lvl_destroy(struct level *lvl);

int lvl_load(struct level *lvl, const char *fname);

#endif	/* LEVEL_H_ */
