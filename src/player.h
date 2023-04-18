#ifndef PLAYER_H_
#define PLAYER_H_

#include "cgmath/cgmath.h"

struct player {
	cgm_vec3 pos;
	cgm_quat rot;

	cgm_vec3 vel;

	cgm_vec2 mouse_input;
	cgm_vec3 sball_mov, sball_rot;
};

void init_player(struct player *p);

void update_player_mouse(struct player *p);
void update_player_sball(struct player *p);
void update_player(struct player *p);

void player_view_matrix(struct player *p, float *viewmat);

#endif	/* PLAYER_H_ */
