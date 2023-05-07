#include "config.h"

#include <string.h>
#include "game.h"
#include "enemy.h"
#include "mesh.h"
#include "geom.h"
#include "gfxutil.h"

#define MAX_HP	32
#define MAX_SP	16

void init_enemy(struct enemy *enemy)
{
	memset(enemy, 0, sizeof *enemy);

	enemy->hp = MAX_HP;
	enemy->sp = MAX_SP;

	enemy->rad = 1.0f;	/* will be overriden once we assign a mesh */
}

void destroy_enemy(struct enemy *enemy)
{
}

void enemy_addmesh(struct enemy *mob, struct mesh *mesh)
{
	mob->mesh = mesh;
	mob->rad = mesh->bsph_rad;
}

void enemy_update(struct enemy *mob)
{
	mob->aifunc(mob);

	calc_posrot_matrix(mob->matrix, &mob->pos, &mob->rot);
}

int enemy_hit_test(struct enemy *mob, const cgm_ray *ray, float *distret)
{
	if(!mob->mesh || mob->hp <= 0.0f) {
		return 0;
	}
	return ray_sphere(ray, &mob->pos, mob->rad, distret);
}

int enemy_damage(struct enemy *mob, float dmg)
{
	/* apply to shields first, then hp */
	mob->sp -= dmg;
	if(mob->sp < 0.0f) {
		mob->hp += mob->sp;
		mob->sp = 0.0f;
		if(mob->hp < 0.0f) {
			mob->hp = 0.0f;
			return 0;
		}
	}
	return 1;
}

void enemy_ai_flying1(struct enemy *mob)
{
	enemy_ai_flying2(mob);
}

void enemy_ai_flying2(struct enemy *mob)
{
	cgm_qrotate(&mob->rot, 0.01, 0, 1, 0);
}

void enemy_ai_spike(struct enemy *mob)
{
	enemy_ai_flying2(mob);
}
