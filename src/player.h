#ifndef PLAYER_H_
#define PLAYER_H_

#include "level.h"
#include "cgmath/cgmath.h"

#define MAX_HP			256
#define MAX_SP			256
#define MAX_MISSILES	10
#define SP_REGEN		0.25

enum {
	ITEM_KEY	= 1,
	ITEM_SECRET	= 2
};

struct player {
	struct level *lvl;
	struct room *room;	/* current room the player is in */

	float hp, sp;	/* health and shields */
	unsigned int items;
	int num_missiles;

	cgm_vec3 pos, prevpos;
	cgm_quat rot;
	cgm_vec3 fwd;	/* forward vector, derived from rot */

	cgm_vec3 vel;
	float roll;

	float rotmat[16];

	cgm_vec2 mouse_input;
	cgm_vec3 sball_mov, sball_rot;

	long last_dmg, last_missile_time;
};

void init_player(struct player *p);

void player_damage(struct player *p, float dmg);

void update_player_mouse(struct player *p);
void update_player_sball(struct player *p);
void update_player(struct player *p);

void player_view_matrix(struct player *p, float *viewmat);

#endif	/* PLAYER_H_ */
