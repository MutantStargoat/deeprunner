#ifndef PLAYER_H_
#define PLAYER_H_

#include "level.h"
#include "cgmath/cgmath.h"

#define MAX_HP	256
#define MAX_SP	256

struct player {
	struct level *lvl;

	int hp, sp;	/* health and shields */

	cgm_vec3 pos, prevpos;
	cgm_quat rot;

	cgm_vec3 vel;
	float roll;

	float rotmat[16];

	cgm_vec2 mouse_input;
	cgm_vec3 sball_mov, sball_rot;
};

void init_player(struct player *p);

void update_player_mouse(struct player *p);
void update_player_sball(struct player *p);
void update_player(struct player *p);

void player_view_matrix(struct player *p, float *viewmat);

#endif	/* PLAYER_H_ */
