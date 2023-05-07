#ifndef RENDLVL_H_
#define RENDLVL_H_

#include "level.h"

int rendlvl_init(struct level *lvl);
void rendlvl_destroy(void);

void rendlvl_setup(struct room *room, const cgm_vec3 *ppos, float *view_matrix);

void rendlvl_update(void);

void render_level(void);
void render_level_mesh(struct mesh *mesh);
void render_dynobj(struct object *obj);
void render_enemy(struct enemy *mob);

#endif	/* RENDLVL_H_ */
