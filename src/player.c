#include "player.h"
#include "options.h"

#define MOUSE_SPEED		(opt.mouse_speed * 0.0002)
#define SBALL_RSPEED	(opt.sball_speed * 0.00002)
#define SBALL_TSPEED	(opt.sball_speed * 0.00004)

void init_player(struct player *p)
{
	cgm_vcons(&p->pos, 0, 0, 0);
	cgm_qcons(&p->rot, 0, 0, 0, 1);
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

/* this is called in the timestep update at a constant rate */
void update_player(struct player *p)
{
	float rotmat[16];
	cgm_quat rollquat;

	cgm_vec3 fwd = {0, 0, 1}, right = {1, 0, 0}, up = {0, 1, 0};

	cgm_qrotation(&rollquat, p->roll, 0, 0, 1);
	cgm_qmul(&rollquat, &p->rot);
	p->rot = rollquat;

	cgm_mrotation_quat(rotmat, &p->rot);
	cgm_vmul_v3m4(&fwd, rotmat);
	cgm_vmul_v3m4(&right, rotmat);
	cgm_vmul_v3m4(&up, rotmat);

	p->pos.x += right.x * p->vel.x + up.x * p->vel.y + fwd.x * p->vel.z;
	p->pos.y += right.y * p->vel.x + up.y * p->vel.y + fwd.y * p->vel.z;
	p->pos.z += right.z * p->vel.x + up.z * p->vel.y + fwd.z * p->vel.z;

	cgm_vcons(&p->vel, 0, 0, 0);
	p->roll = 0;
}

void player_view_matrix(struct player *p, float *view_mat)
{
	float rotmat[16];

	cgm_mtranslation(view_mat, p->pos.x, p->pos.y, p->pos.z);
	cgm_qnormalize(&p->rot);
	cgm_mrotation_quat(rotmat, &p->rot);
	cgm_mmul(view_mat, rotmat);
}
