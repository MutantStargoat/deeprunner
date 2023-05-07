/*
libpsys - reusable particle system library.
Copyright (C) 2011-2018  John Tsiombikas <nuclear@member.fsf.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef LIBPSYS_H_
#define LIBPSYS_H_

#include "rndval.h"
#include "pattr.h"

struct psys_particle;
struct psys_emitter;

typedef int (*psys_spawn_func_t)(struct psys_emitter*, struct psys_particle*, void*);
typedef void (*psys_update_func_t)(struct psys_emitter*, struct psys_particle*, long, float, void*);

typedef void (*psys_draw_func_t)(const struct psys_emitter*, const struct psys_particle*, void*);
typedef void (*psys_draw_start_func_t)(const struct psys_emitter*, void*);
typedef void (*psys_draw_end_func_t)(const struct psys_emitter*, void*);


struct psys_plane {
	float nx, ny, nz, d;
	float elasticity;
	struct psys_plane *next;
};


struct psys_emitter {
	float pos[3];

	struct psys_attributes attr;

	/* list of active particles */
	struct psys_particle *plist;
	int pcount;	/* number of active particles */

	/* list of collision planes */
	struct psys_plane *planes;

	/* custom spawn closure */
	void *spawn_cls;
	psys_spawn_func_t spawn;

	/* custom particle update closure */
	void *upd_cls;
	psys_update_func_t update;

	/* custom draw closure */
	void *draw_cls;
	psys_draw_func_t draw;
	psys_draw_start_func_t draw_start;
	psys_draw_end_func_t draw_end;

	long spawn_acc;		/* partial spawn accumulator */
	long last_update;	/* last update time (to calc dt) */
};


struct psys_particle {
	float x, y, z;
	float vx, vy, vz;
	float life, max_life;
	float base_size;

	struct psys_particle_attributes *pattr;

	/* current particle attr values calculated during update */
	float r, g, b;
	float alpha, size;

	struct psys_particle *next;
};

#ifdef __cplusplus
extern "C" {
#endif

struct psys_emitter *psys_create(void);
void psys_free(struct psys_emitter *em);

int psys_init(struct psys_emitter *em);
void psys_destroy(struct psys_emitter *em);

/* set properties */

/* set emitter position. pos should point to 3 floats (xyz) */
void psys_set_pos(struct psys_emitter *em, const float *pos, long tm);
void psys_set_pos3f(struct psys_emitter *em, float x, float y, float z, long tm);
void psys_get_pos(struct psys_emitter *em, float *pos, long tm);

/* pos should be a pointer to 3 floats (xyz) */
void psys_get_pos(struct psys_emitter *em, float *pos, long tm);
/* qrot should be a pointer to 4 floats (xyzw) */
void psys_get_rot(struct psys_emitter *em, float *qrot, long tm);
/* pivot should be a pointer to 3 floats (xyz) */
void psys_get_pivot(struct psys_emitter *em, float *pivot);

void psys_clear_collision_planes(struct psys_emitter *em);
int psys_add_collision_plane(struct psys_emitter *em, const float *plane, float elast);

void psys_add_particle(struct psys_emitter *em, struct psys_particle *p);

void psys_spawn_func(struct psys_emitter *em, psys_spawn_func_t func, void *cls);
void psys_update_func(struct psys_emitter *em, psys_update_func_t func, void *cls);
void psys_draw_func(struct psys_emitter *em, psys_draw_func_t draw,
		psys_draw_start_func_t start, psys_draw_end_func_t end, void *cls);

/* update and render */

void psys_update(struct psys_emitter *em, long tm);
void psys_draw(const struct psys_emitter *em);

#ifdef __cplusplus
}
#endif

#endif	/* LIBPSYS_H_ */
