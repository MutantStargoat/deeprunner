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
#include "pstrack.h"

int psys_init_track(struct psys_track *track)
{
	track->cache_tm = ANM_TIME_INVAL;

	if(anm_init_track(&track->trk) == -1) {
		return -1;
	}
	return 0;
}

void psys_destroy_track(struct psys_track *track)
{
	anm_destroy_track(&track->trk);
}

int psys_init_track3(struct psys_track3 *track)
{
	track->cache_tm = ANM_TIME_INVAL;

	if(anm_init_track(&track->x) == -1) {
		return -1;
	}
	if(anm_init_track(&track->y) == -1) {
		anm_destroy_track(&track->x);
		return -1;
	}
	if(anm_init_track(&track->z) == -1) {
		anm_destroy_track(&track->x);
		anm_destroy_track(&track->z);
		return -1;
	}
	return 0;
}

void psys_destroy_track3(struct psys_track3 *track)
{
	anm_destroy_track(&track->x);
	anm_destroy_track(&track->y);
	anm_destroy_track(&track->z);
}

void psys_copy_track(struct psys_track *dest, const struct psys_track *src)
{
	anm_copy_track(&dest->trk, &src->trk);
	dest->cache_tm = ANM_TIME_INVAL;
}

void psys_copy_track3(struct psys_track3 *dest, const struct psys_track3 *src)
{
	anm_copy_track(&dest->x, &src->x);
	anm_copy_track(&dest->y, &src->y);
	anm_copy_track(&dest->z, &src->z);

	dest->cache_tm = ANM_TIME_INVAL;
}

void psys_eval_track(struct psys_track *track, anm_time_t tm)
{
	if(track->cache_tm != tm) {
		track->cache_tm = tm;
		track->cache_val = anm_get_value(&track->trk, tm);
	}
}

void psys_set_value(struct psys_track *track, anm_time_t tm, float v)
{
	anm_set_value(&track->trk, tm, v);
	track->cache_tm = ANM_TIME_INVAL;
}

float psys_get_value(struct psys_track *track, anm_time_t tm)
{
	psys_eval_track(track, tm);
	return track->cache_val;
}

float psys_get_cur_value(struct psys_track *track)
{
	return track->cache_val;
}


void psys_eval_track3(struct psys_track3 *track, anm_time_t tm)
{
	if(track->cache_tm != tm) {
		track->cache_tm = tm;
		track->cache_vec[0] = anm_get_value(&track->x, tm);
		track->cache_vec[1] = anm_get_value(&track->y, tm);
		track->cache_vec[2] = anm_get_value(&track->z, tm);
	}
}

void psys_set_value3(struct psys_track3 *track, anm_time_t tm, float x, float y, float z)
{
	anm_set_value(&track->x, tm, x);
	anm_set_value(&track->y, tm, y);
	anm_set_value(&track->z, tm, z);
	track->cache_tm = ANM_TIME_INVAL;
}

float *psys_get_value3(struct psys_track3 *track, anm_time_t tm, float *vec)
{
	psys_eval_track3(track, tm);
	if(vec) {
		vec[0] = track->cache_vec[0];
		vec[1] = track->cache_vec[1];
		vec[2] = track->cache_vec[2];
	}
	return track->cache_vec;
}

float *psys_get_cur_value3(struct psys_track3 *track, float *vec)
{
	if(vec) {
		vec[0] = track->cache_vec[0];
		vec[1] = track->cache_vec[1];
		vec[2] = track->cache_vec[2];
	}
	return track->cache_vec;
}
