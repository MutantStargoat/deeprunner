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
#ifndef RNDVAL_H_
#define RNDVAL_H_

#include "pstrack.h"

struct psys_rnd {
	float value, range;
};

struct psys_rnd3 {
	float value[3], range[3];
};

struct psys_anm_rnd {
	struct psys_track value, range;
};

struct psys_anm_rnd3 {
	struct psys_track3 value, range;
};

#define PSYS_EVAL_CUR	ANM_TIME_INVAL

#ifdef __cplusplus
extern "C" {
#endif

int psys_init_anm_rnd(struct psys_anm_rnd *v);
void psys_destroy_anm_rnd(struct psys_anm_rnd *v);
int psys_init_anm_rnd3(struct psys_anm_rnd3 *v);
void psys_destroy_anm_rnd3(struct psys_anm_rnd3 *v);

void psys_copy_anm_rnd(struct psys_anm_rnd *dest, const struct psys_anm_rnd *src);
void psys_copy_anm_rnd3(struct psys_anm_rnd3 *dest, const struct psys_anm_rnd3 *src);

void psys_set_rnd(struct psys_rnd *r, float val, float range);
void psys_set_rnd3(struct psys_rnd3 *r, const float *val, const float *range);

void psys_set_anm_rnd(struct psys_anm_rnd *r, anm_time_t tm, float val, float range);
void psys_set_anm_rnd3(struct psys_anm_rnd3 *r, anm_time_t tm, const float *val, const float *range);

float psys_eval_rnd(struct psys_rnd *r);
void psys_eval_rnd3(struct psys_rnd3 *r, float *val);

float psys_eval_anm_rnd(struct psys_anm_rnd *r, anm_time_t tm);
void psys_eval_anm_rnd3(struct psys_anm_rnd3 *r, anm_time_t tm, float *val);

#ifdef __cplusplus
}
#endif

#endif	/* RNDVAL_H_ */
