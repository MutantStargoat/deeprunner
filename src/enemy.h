#ifndef ENEMY_H_
#define ENEMY_H_

#include "cgmath/cgmath.h"

struct enemy {
	struct level *lvl;
	struct room *room;

	float hp, sp;

	cgm_vec3 pos;
	cgm_quat rot;
	cgm_vec3 fwd;

	struct mesh *mesh;
};

void init_enemy(struct enemy *enemy);
void destroy_enemy(struct enemy *enemy);

#endif	/* ENEMY_H_ */
