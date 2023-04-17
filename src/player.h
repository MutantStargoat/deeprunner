#ifndef PLAYER_H_
#define PLAYER_H_

#include "cgmath/cgmath.h"

struct player {
	cgm_vec3 pos;
	cgm_quat rot;
};

void init_player(struct player *p);

#endif	/* PLAYER_H_ */
