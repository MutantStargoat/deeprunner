/*
Deep Runner - 6dof shooter game for the SGI O2.
Copyright (C) 2023  John Tsiombikas <nuclear@mutantstargoat.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef RENDLVL_H_
#define RENDLVL_H_

#include "level.h"

struct explosion {
	long start_time, tm;
	cgm_vec3 pos;
	float sz;
};

int rendlvl_init(struct level *lvl);
void rendlvl_destroy(void);

void rendlvl_setup(struct room *room, const cgm_vec3 *ppos, float *view_matrix);

void rendlvl_update(void);

void render_level(void);
void render_level_mesh(struct mesh *mesh);
void render_dynobj(struct object *obj);
void render_enemy(struct enemy *mob);
void render_missile(struct missile *mis);
void render_explosion(struct explosion *expl);

int add_explosion(const cgm_vec3 *pos, float sz, long start_tm);

#endif	/* RENDLVL_H_ */
