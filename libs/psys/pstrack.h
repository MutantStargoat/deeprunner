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
#ifndef PSTRACK_H_
#define PSTRACK_H_

#include "../goat3d/src/track.h"

struct psys_track {
	struct anm_track trk;

	anm_time_t cache_tm;
	float cache_val;
};

struct psys_track3 {
	struct anm_track x, y, z;

	anm_time_t cache_tm;
	float cache_vec[3];
};

#ifdef __cplusplus
extern "C" {
#endif

int psys_init_track(struct psys_track *track);
void psys_destroy_track(struct psys_track *track);

int psys_init_track3(struct psys_track3 *track);
void psys_destroy_track3(struct psys_track3 *track);

/* XXX dest must have been initialized first */
void psys_copy_track(struct psys_track *dest, const struct psys_track *src);
void psys_copy_track3(struct psys_track3 *dest, const struct psys_track3 *src);

void psys_eval_track(struct psys_track *track, anm_time_t tm);
void psys_set_value(struct psys_track *track, anm_time_t tm, float v);
float psys_get_value(struct psys_track *track, anm_time_t tm);
float psys_get_cur_value(struct psys_track *track);

void psys_eval_track3(struct psys_track3 *track, anm_time_t tm);
void psys_set_value3(struct psys_track3 *track, anm_time_t tm, float x, float y, float z);
/* returns pointer to the internal cached value, and if vec is not null, also copies it there */
float *psys_get_value3(struct psys_track3 *track, anm_time_t tm, float *vec);
float *psys_get_cur_value3(struct psys_track3 *track, float *vec);

#ifdef __cplusplus
}
#endif

#endif	/* PSTRACK_H_ */
