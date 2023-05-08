#include "config.h"

#include <string.h>
#include "game.h"
#include "enemy.h"
#include "mesh.h"
#include "geom.h"
#include "gfxutil.h"

#define MAX_MOB_HP	40
#define MAX_MOB_SP	64

void init_enemy(struct enemy *enemy)
{
	memset(enemy, 0, sizeof *enemy);

	enemy->hp = MAX_MOB_HP;
	enemy->sp = MAX_MOB_SP;

	cgm_vcons(&enemy->targ, 100, 0, 0);

	enemy->last_shield_hit = -SHIELD_OVERLAY_DUR;
	enemy->last_dmg_hit = -EXPL_DUR;

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
	cgm_vec3 up = {0, 1, 0};

	mob->prev_targ = mob->targ;

	mob->aifunc(mob);

	cgm_midentity(mob->matrix);
	cgm_mtranslate(mob->matrix, mob->pos.x, mob->pos.y, mob->pos.z);
	cgm_mlookat(mob->matrix, &mob->pos, &mob->targ, &up);

	/*calc_posrot_matrix(mob->matrix, &mob->pos, &mob->rot);*/
}

int enemy_hit_test(struct enemy *mob, const cgm_ray *ray, float *distret)
{
	if(!mob->mesh || mob->hp <= 0.0f) {
		return 0;
	}
	if(ray_sphere(ray, &mob->pos, mob->rad, distret)) {
		cgm_raypos(&mob->last_hit_pos, ray, *distret);
		return 1;
	}
	return 0;
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
		mob->last_dmg_hit = time_msec;
	} else {
		mob->last_shield_hit = time_msec;
	}
	return 1;
}

void enemy_shoot(struct enemy *mob, const cgm_vec3 *dir)
{
	cgm_vec3 pos;
	cgm_quat rot;

	pos = mob->pos;
	cgm_vadd_scaled(&pos, &mob->fwd, mob->rad * 1.2);

	cgm_mget_rotation(mob->matrix, &rot);

	if(lvl_spawn_missile(mob->lvl, mob->room, &pos, &mob->fwd, &rot) != -1) {
		mob->last_shot = time_msec;
	}
}

void enemy_ai_flying1(struct enemy *mob)
{
	enemy_ai_flying2(mob);
}

void enemy_ai_flying2(struct enemy *mob)
{
	cgm_vec3 pdir;

	if(player->room == mob->room && player->hp > 0.0f) {
		cgm_vlerp(&mob->targ, &mob->prev_targ, &player->pos, 0.1);
		mob->fwd = mob->targ; cgm_vsub(&mob->fwd, &mob->pos);
		cgm_vnormalize(&mob->fwd);

		pdir = player->pos; cgm_vsub(&pdir, &mob->pos);
		cgm_vnormalize(&pdir);

		if(time_msec - mob->last_shot > ENEMY_COOLDOWN && cgm_vdot(&mob->fwd, &pdir) > 0.99) {
			enemy_shoot(mob, &mob->fwd);
		}
	}
}

void enemy_ai_spike(struct enemy *mob)
{
	enemy_ai_flying2(mob);
}

