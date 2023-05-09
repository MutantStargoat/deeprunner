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
#include "config.h"

#include <string.h>
#include <float.h>
#include <limits.h>
#include "game.h"
#include "player.h"
#include "options.h"
#include "geom.h"
#include "darray.h"

#define MOUSE_SPEED		(opt.mouse_speed * 0.0002)
#define SBALL_RSPEED	(opt.sball_speed * 0.00002)
#define SBALL_TSPEED	(opt.sball_speed * 0.00004)

static void activate(struct player *p, struct action *act);
static int check_collision(struct player *p, const cgm_vec3 *vel, struct collision *col);

void init_player(struct player *p)
{
	memset(p, 0, sizeof *p);

	cgm_vcons(&p->pos, 0, 0, 0);
	cgm_qcons(&p->rot, 0, 0, 0, 1);

	p->hp = MAX_HP;
	p->sp = MAX_SP;
	p->num_missiles = MAX_MISSILES;

	p->last_dmg = -DMG_OVERLAY_DUR;
	p->last_missile_time = -MISSILE_COOLDOWN;
}

void player_damage(struct player *p, float dmg)
{
	p->sp -= dmg;
	if(p->sp < 0.0f) {
		p->hp += p->sp;
		p->sp = 0.0f;
		if(p->hp < 0.0f) p->hp = 0.0f;
		if(p->hp > MAX_HP) p->hp = MAX_HP;

		p->last_dmg = time_msec;
	} else if(p->sp > MAX_SP) {
		p->sp = MAX_SP;
	}
}

void update_player_mouse(struct player *p)
{
	float len, theta, phi;
	cgm_quat rot;

	theta = p->mouse_input.x * MOUSE_SPEED;
	phi = p->mouse_input.y * MOUSE_SPEED;

	if((len = sqrt(theta * theta + phi * phi)) != 0.0f) {
		cgm_qrotation(&rot, len, phi / len, theta / len, 0);
		cgm_qmul(&rot, &p->rot);
		p->rot = rot;
		p->mouse_input.x = p->mouse_input.y = 0.0f;
	}
}

void update_player_sball(struct player *p)
{
	float len;
	cgm_quat rot;

	if((len = cgm_vlength(&p->sball_rot)) != 0.0f) {
		cgm_qrotation(&rot, len * SBALL_RSPEED, p->sball_rot.x / len,
				p->sball_rot.y / len, p->sball_rot.z / len);
		cgm_qmul(&rot, &p->rot);
		p->rot = rot;
		cgm_vcons(&p->sball_rot, 0, 0, 0);
	}

	cgm_vadd_scaled(&p->vel, &p->sball_mov, SBALL_TSPEED);
	cgm_vcons(&p->sball_mov, 0, 0, 0);
}

#ifdef DBG_SHOW_COLPOLY
extern const struct triangle *dbg_hitpoly;
#endif
#ifdef DBG_FREEZEVIS
extern int dbg_freezevis;
#endif
#ifdef DBG_SHOW_MAX_COL_ITER
extern int dbg_max_col_iter;
#endif

/* this is called in the timestep update at a constant rate */
void update_player(struct player *p)
{
	int i, iter, count;
	cgm_quat rollquat;
	cgm_vec3 right, up;
	cgm_vec3 vel;
	struct collision col;
	float vnlen;

	if(p->roll != 0.0f) {
		cgm_qrotation(&rollquat, p->roll, 0, 0, 1);
		cgm_qmul(&rollquat, &p->rot);
		p->rot = rollquat;
	}

	cgm_mrotation_quat(p->rotmat, &p->rot);
	cgm_vcons(&right, p->rotmat[0], p->rotmat[4], p->rotmat[8]);
	cgm_vcons(&up, p->rotmat[1], p->rotmat[5], p->rotmat[9]);
	cgm_vcons(&p->fwd, -p->rotmat[2], -p->rotmat[6], -p->rotmat[10]);

	p->prevpos = p->pos;

	vel.x = right.x * p->vel.x + up.x * p->vel.y - p->fwd.x * p->vel.z;
	vel.y = right.y * p->vel.x + up.y * p->vel.y - p->fwd.y * p->vel.z;
	vel.z = right.z * p->vel.x + up.z * p->vel.y - p->fwd.z * p->vel.z;

	cgm_vcons(&p->vel, 0, 0, 0);
	p->roll = 0;

	/* collision detection */
#ifdef DBG_FREEZEVIS
	if(dbg_freezevis) {
		cgm_vadd(&p->pos, &vel);
		return;
	}
#endif

	if(!(p->room = lvl_room_at(p->lvl, p->pos.x, p->pos.y, p->pos.z))) {
		cgm_vadd(&p->pos, &vel);
		return;
	}

	iter = 0;
	while(cgm_vlength_sq(&vel) > 1e-5 && iter++ < 16 && check_collision(p, &vel, &col)) {
		vnlen = -cgm_vdot(&vel, &col.norm);
		cgm_vadd_scaled(&vel, &col.norm, vnlen);	/* vel += norm * vnlen */
	}

#ifdef DBG_SHOW_MAX_COL_ITER
	if(iter > dbg_max_col_iter) {
		dbg_max_col_iter = iter;
		printf("max col iter: %d\n", iter);
	}
#endif

	if(iter < 16) {
		cgm_vadd(&p->pos, &vel);
	}

	/* regenerate energy */
	p->sp += SP_REGEN;
	if(p->sp > MAX_SP) p->sp = MAX_SP;

	/* check for pickups and triggers */
	count = darr_size(p->room->triggers);
	for(i=0; i<count; i++) {
		struct trigger *trig = p->room->triggers + i;
		if(trig->act.type == ACT_NONE) continue;
		if(aabox_sph_test(&trig->box, &p->pos, COL_RADIUS)) {
			activate(p, &trig->act);
		}
	}

	count = darr_size(p->room->objects);
	for(i=0; i<count; i++) {
		cgm_vec3 localpos;
		struct object *obj = p->room->objects[i];
		if(obj->act.type == ACT_NONE) continue;

		localpos = p->pos;
		cgm_vmul_m4v3(&localpos, obj->invmatrix);

		if(aabox_sph_test(&obj->aabb, &localpos, COL_RADIUS)) {
			if(obj->octree && !oct_sphtest(obj->octree, &localpos, COL_RADIUS, 0)) {
				continue;
			}
			if(obj->act.type == ACT_PICKUP) {
				obj->mesh = 0;
			}
			activate(p, &obj->act);
		}
	}
}

void player_view_matrix(struct player *p, float *view_mat)
{
	float rotmat[16];

	cgm_mtranslation(view_mat, -p->pos.x, -p->pos.y, -p->pos.z);
	cgm_qnormalize(&p->rot);
	cgm_mrotation_quat(rotmat, &p->rot);
	cgm_mmul(view_mat, rotmat);
}

void dbg_getkey(void)
{
	struct action act = {"key", ACT_PICKUP, 0};
	activate(player, &act);
}

static void activate(struct player *p, struct action *act)
{
	struct object *obj;

	switch(act->type) {
	case ACT_DAMAGE:
		player_damage(p, act->value);
		break;

	case ACT_HPDAMAGE:
		p->hp -= act->value;
		if(p->hp < 0.0f) p->hp = 0.0f;
		if(p->hp > MAX_HP) p->hp = MAX_HP;

		p->last_dmg = time_msec;
		break;

	case ACT_SHIELD:
		p->sp -= act->value;
		if(p->sp <= 0.0f) p->sp = 0.0f;
		if(p->sp >= MAX_SP) p->sp = MAX_SP;
		break;

	case ACT_PICKUP:
		if(strcmp(act->name, "secret") == 0) {
			p->items |= ITEM_SECRET;
			au_play_sample(sfx_o2chime, AU_CRITICAL);
		} else if(strcmp(act->name, "key") == 0) {
			p->items |= ITEM_KEY;
			au_play_sample(sfx_gling1, AU_CRITICAL);

			if((obj = lvl_find_dynobj(player->lvl, "dyn_slide_l"))) {
				cgm_vcons(&obj->pos, 0, -10000, 0);
			}
			if((obj = lvl_find_dynobj(player->lvl, "dyn_slide_r"))) {
				cgm_vcons(&obj->pos, 0, -10000, 0);
			}
		}
		act->type = ACT_NONE;
		break;

	case ACT_WIN:
		/* TODO */
		break;

	default:
		break;
	}
}

static int check_collision(struct player *p, const cgm_vec3 *vel, struct collision *col)
{
	/*int i, nobj;*/

	if(lvl_collision_rad(p->lvl, p->room, &p->pos, vel, COL_RADIUS, col)) {
		return 1;
	}

#if 0
	nobj = darr_size(p->room->objects);
	for(i=0; i<nobj; i++) {
		struct object *obj = p->room->objects[i];
		if(obj->colmesh) {
			if(!aabox_sph_test(&obj->colmesh->aabb, &p->pos, COL_RADIUS)) {
				continue;
			}
			cgm_vcons(&col->norm, 0, 0, 0);
			return 1;
			/* TODO check triangles */
		} else if(obj->mesh) {
			if(aabox_sph_test(&obj->mesh->aabb, &p->pos, COL_RADIUS)) {
				cgm_vcons(&col->norm, 0, 0, 0);
				return 1;
			}
		}
	}
#endif

	return 0;
}
