/*
Deep Runner - 6dof shooter game for the SGI O2.
Copyright (C) 2023  John Tsiombikas <nuclear@mutantstargoat.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef ENEMY_H_
#define ENEMY_H_

#include "cgmath/cgmath.h"

struct enemy {
	struct level *lvl;
	struct room *room;

	float hp, sp;

	cgm_vec3 pos, targ, prev_targ;
	cgm_quat rot;
	cgm_vec3 fwd;
	float rad;

	struct mesh *mesh;
	float matrix[16];

	long last_shield_hit, last_dmg_hit, last_shot;
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
