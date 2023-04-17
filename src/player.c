#include "player.h"

void init_player(struct player *p)
{
	cgm_vcons(&p->pos, 0, 0, 0);
	cgm_qcons(&p->rot, 0, 0, 0, 1);
}
