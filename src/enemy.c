#include "config.h"

#include <string.h>
#include "game.h"
#include "enemy.h"

#define MAX_HP	32
#define MAX_SP	16

void init_enemy(struct enemy *enemy)
{
	memset(enemy, 0, sizeof *enemy);

	enemy->hp = MAX_HP;
	enemy->sp = MAX_SP;
}

void destroy_enemy(struct enemy *enemy)
{
}
