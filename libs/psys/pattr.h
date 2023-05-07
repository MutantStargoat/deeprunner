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
#ifndef PATTR_H_
#define PATTR_H_

#include <stdio.h>
#include "pstrack.h"
#include "rndval.h"

/* the particle attributes vary from 0 to 1 during its lifetime */
struct psys_particle_attributes {
	struct psys_track3 color;
	struct psys_track alpha;
	struct psys_track size;
};

enum psys_blending { PSYS_ADD, PSYS_ALPHA };

struct psys_attributes {
	unsigned int tex;	/* OpenGL texture to use for the billboard */

	struct psys_track3 spawn_range;	/* radius of emmiter */
	struct psys_track rate;			/* spawn rate particles per sec */
	struct psys_anm_rnd life;		/* particle life in seconds */
	struct psys_anm_rnd size;		/* base particle size */
	struct psys_anm_rnd3 dir;		/* particle shoot direction */

	struct psys_track3 grav;		/* external force (usually gravity) */
	float drag;	/* I don't think this needs to animate */

	enum psys_blending blending;

	/* particle attributes */
	struct psys_particle_attributes part_attr;

	/* limits */
	int max_particles;
};

#ifdef __cplusplus
extern "C" {
#endif

void psys_texture_loader(unsigned int (*load)(const char*, void*), void (*unload)(unsigned int, void*), void *cls);

struct psys_attributes *psys_create_attr(void);
void psys_free_attr(struct psys_attributes *attr);

int psys_init_attr(struct psys_attributes *attr);
void psys_destroy_attr(struct psys_attributes *attr);

/* copies particle system attributes src to dest
 * XXX: dest must have been initialized first
 */
void psys_copy_attr(struct psys_attributes *dest, const struct psys_attributes *src);

void psys_eval_attr(struct psys_attributes *attr, anm_time_t tm);

int psys_load_attr(struct psys_attributes *attr, const char *fname);
int psys_load_attr_stream(struct psys_attributes *attr, FILE *fp);

int psys_save_attr(const struct psys_attributes *attr, const char *fname);
int psys_save_attr_stream(const struct psys_attributes *attr, FILE *fp);

#ifdef __cplusplus
}
#endif

#endif	/* PATTR_H_ */
