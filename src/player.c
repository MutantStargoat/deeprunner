#include "config.h"

#include <float.h>
#include "player.h"
#include "options.h"
#include "geom.h"
#include "darray.h"

#define MOUSE_SPEED		(opt.mouse_speed * 0.0002)
#define SBALL_RSPEED	(opt.sball_speed * 0.00002)
#define SBALL_TSPEED	(opt.sball_speed * 0.00004)

void init_player(struct player *p)
{
	cgm_vcons(&p->pos, 0, 0, 0);
	cgm_qcons(&p->rot, 0, 0, 0, 1);

	p->hp = MAX_HP;
	p->sp = MAX_SP;
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
	while(cgm_vlength_sq(&vel) > 1e-5 && iter++ < 16 &&
			lvl_collision_rad(p->lvl, p->room, &p->pos, &vel, COL_RADIUS, &col)) {
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

	/* check for pickups and triggers */
	count = darr_size(p->room->triggers);
	for(i=0; i<count; i++) {
		struct trigger *trig = p->room->triggers + i;
		if(aabox_sph_test(&trig->box, &p->pos, COL_RADIUS)) {
			switch(trig->act.type) {
			case ACT_DAMAGE:
				p->hp -= trig->act.value;
				if(p->hp <= 0.0f) p->hp = 0.0f;
				if(p->hp >= MAX_HP) p->hp = MAX_HP;
				break;

			case ACT_SHIELD:
				p->sp -= trig->act.value;
				if(p->sp <= 0.0f) p->sp = 0.0f;
				if(p->sp >= MAX_SP) p->sp = MAX_SP;
				break;

			case ACT_WIN:
				/* TODO */
				break;
			}
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
