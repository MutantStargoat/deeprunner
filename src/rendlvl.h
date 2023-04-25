#ifndef RENDLVL_H_
#define RENDLVL_H_

#include "level.h"

int rendlvl_init(struct level *lvl);
void rendlvl_destroy(void);

void rendlvl_setup(const cgm_vec3 *ppos, const cgm_quat *prot);

void render_level(void);
void render_level_mesh(struct mesh *mesh);

#endif	/* RENDLVL_H_ */
