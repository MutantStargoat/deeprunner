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
	float rad;

	struct mesh *mesh;
	float matrix[16];

	long last_shield_hit, last_dmg_hit;
	cgm_vec3 last_hit_pos;

	void (*aifunc)(struct enemy *mob);
};

void init_enemy(struct enemy *enemy);
void destroy_enemy(struct enemy *enemy);

void enemy_addmesh(struct enemy *mob, struct mesh *mesh);

void enemy_update(struct enemy *mob);

int enemy_hit_test(struct enemy *mob, const cgm_ray *ray, float *distret);
int enemy_damage(struct enemy *mob, float dmg);

void enemy_ai_flying1(struct enemy *mob);
void enemy_ai_flying2(struct enemy *mob);
void enemy_ai_spike(struct enemy *mob);

#endif	/* ENEMY_H_ */
