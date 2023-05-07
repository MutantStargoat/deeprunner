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
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <assert.h>

#ifdef __MSVCRT__
#include <malloc.h>
#else
#include <alloca.h>
#endif

#include "pattr.h"
#include "psys_gl.h"

enum {
	OPT_STR,
	OPT_NUM,
	OPT_NUM_RANGE,
	OPT_VEC,
	OPT_VEC_RANGE
};

struct cfgopt {
	char *name;
	int type;
	long tm;
	char *valstr;
	float val[3], valrng[3];
};

static int init_particle_attr(struct psys_particle_attributes *pattr);
static void destroy_particle_attr(struct psys_particle_attributes *pattr);
static struct cfgopt *get_cfg_opt(const char *line);
static void release_cfg_opt(struct cfgopt *opt);
static char *stripspace(char *str);

static void *tex_cls;
static unsigned int (*load_texture)(const char*, void*) = psys_gl_load_texture;
static void (*unload_texture)(unsigned int, void*) = psys_gl_unload_texture;


void psys_texture_loader(unsigned int (*load)(const char*, void*), void (*unload)(unsigned int, void*), void *cls)
{
	load_texture = load;
	unload_texture = unload;
	tex_cls = cls;
}

struct psys_attributes *psys_create_attr(void)
{
	struct psys_attributes *attr = malloc(sizeof *attr);
	if(attr) {
		if(psys_init_attr(attr) == -1) {
			free(attr);
			attr = 0;
		}
	}
	return attr;
}

void psys_free_attr(struct psys_attributes *attr)
{
	psys_destroy_attr(attr);
	free(attr);
}

int psys_init_attr(struct psys_attributes *attr)
{
	memset(attr, 0, sizeof *attr);

	if(psys_init_track3(&attr->spawn_range) == -1)
		goto err;
	if(psys_init_track(&attr->rate) == -1)
		goto err;
	if(psys_init_anm_rnd(&attr->life) == -1)
		goto err;
	if(psys_init_anm_rnd(&attr->size) == -1)
		goto err;
	if(psys_init_anm_rnd3(&attr->dir) == -1)
		goto err;
	if(psys_init_track3(&attr->grav) == -1)
		goto err;

	if(init_particle_attr(&attr->part_attr) == -1)
		goto err;

	attr->max_particles = -1;

	anm_set_track_default(&attr->size.value.trk, 1.0);
	anm_set_track_default(&attr->life.value.trk, 1.0);

	attr->blending = PSYS_ADD;
	return 0;

err:
	psys_destroy_attr(attr);
	return -1;
}


static int init_particle_attr(struct psys_particle_attributes *pattr)
{
	if(psys_init_track3(&pattr->color) == -1) {
		return -1;
	}
	if(psys_init_track(&pattr->alpha) == -1) {
		psys_destroy_track3(&pattr->color);
		return -1;
	}
	if(psys_init_track(&pattr->size) == -1) {
		psys_destroy_track3(&pattr->color);
		psys_destroy_track(&pattr->alpha);
		return -1;
	}

	anm_set_track_default(&pattr->color.x, 1.0);
	anm_set_track_default(&pattr->color.y, 1.0);
	anm_set_track_default(&pattr->color.z, 1.0);
	anm_set_track_default(&pattr->alpha.trk, 1.0);
	anm_set_track_default(&pattr->size.trk, 1.0);
	return 0;
}


void psys_destroy_attr(struct psys_attributes *attr)
{
	psys_destroy_track3(&attr->spawn_range);
	psys_destroy_track(&attr->rate);
	psys_destroy_anm_rnd(&attr->life);
	psys_destroy_anm_rnd(&attr->size);
	psys_destroy_anm_rnd3(&attr->dir);
	psys_destroy_track3(&attr->grav);

	destroy_particle_attr(&attr->part_attr);

	if(attr->tex && unload_texture) {
		unload_texture(attr->tex, tex_cls);
	}
}

static void destroy_particle_attr(struct psys_particle_attributes *pattr)
{
	psys_destroy_track3(&pattr->color);
	psys_destroy_track(&pattr->alpha);
	psys_destroy_track(&pattr->size);
}

void psys_copy_attr(struct psys_attributes *dest, const struct psys_attributes *src)
{
	dest->tex = src->tex;

	psys_copy_track3(&dest->spawn_range, &src->spawn_range);
	psys_copy_track(&dest->rate, &src->rate);

	psys_copy_anm_rnd(&dest->life, &src->life);
	psys_copy_anm_rnd(&dest->size, &src->size);
	psys_copy_anm_rnd3(&dest->dir, &src->dir);

	psys_copy_track3(&dest->grav, &src->grav);

	dest->drag = src->drag;
	dest->max_particles = src->max_particles;

	dest->blending = src->blending;

	/* also copy the particle attributes */
	psys_copy_track3(&dest->part_attr.color, &src->part_attr.color);
	psys_copy_track(&dest->part_attr.alpha, &src->part_attr.alpha);
	psys_copy_track(&dest->part_attr.size, &src->part_attr.size);
}

void psys_eval_attr(struct psys_attributes *attr, anm_time_t tm)
{
	float tmp[3];

	psys_eval_track3(&attr->spawn_range, tm);
	psys_eval_track(&attr->rate, tm);
	psys_eval_anm_rnd(&attr->life, tm);
	psys_eval_anm_rnd(&attr->size, tm);
	psys_eval_anm_rnd3(&attr->dir, tm, tmp);
	psys_eval_track3(&attr->grav, tm);
}

int psys_load_attr(struct psys_attributes *attr, const char *fname)
{
	FILE *fp;
	int res;

	if(!fname) {
		return -1;
	}

	if(!(fp = fopen(fname, "r"))) {
		fprintf(stderr, "psys_load_attr: failed to read file: %s: %s\n", fname, strerror(errno));
		return -1;
	}
	res = psys_load_attr_stream(attr, fp);
	fclose(fp);
	return res;
}

int psys_load_attr_stream(struct psys_attributes *attr, FILE *fp)
{
	int lineno = 0;
	char buf[512];
	struct cfgopt *opt = 0;

	psys_init_attr(attr);

	while(fgets(buf, sizeof buf, fp)) {

		lineno++;

		if(!(opt = get_cfg_opt(buf))) {
			continue;
		}

		if(strcmp(opt->name, "texture") == 0) {
			if(opt->type != OPT_STR) {
				goto err;
			}
			if(!load_texture) {
				fprintf(stderr, "particle system requests a texture, but no texture loader available!\n");
				goto err;
			}
			if(!(attr->tex = load_texture(opt->valstr, tex_cls))) {
				fprintf(stderr, "failed to load texture: %s\n", opt->valstr);
				goto err;
			}

			release_cfg_opt(opt);
			continue;

		} else if(strcmp(opt->name, "blending") == 0) {
			if(opt->type != OPT_STR) {
				goto err;
			}

			/* parse blending mode */
			if(strcmp(opt->valstr, "add") == 0 || strcmp(opt->valstr, "additive") == 0) {
				attr->blending = PSYS_ADD;
			} else if(strcmp(opt->valstr, "alpha") == 0) {
				attr->blending = PSYS_ALPHA;
			} else {
				fprintf(stderr, "invalid blending mode: %s\n", opt->valstr);
				goto err;
			}

			release_cfg_opt(opt);
			continue;

		} else if(opt->type == OPT_STR) {
			fprintf(stderr, "invalid particle config: '%s'\n", opt->name);
			goto err;
		}

		if(strcmp(opt->name, "spawn_range") == 0) {
			psys_set_value3(&attr->spawn_range, opt->tm, opt->val[0], opt->val[1], opt->val[2]);
		} else if(strcmp(opt->name, "rate") == 0) {
			psys_set_value(&attr->rate, opt->tm, opt->val[0]);
		} else if(strcmp(opt->name, "life") == 0) {
			psys_set_anm_rnd(&attr->life, opt->tm, opt->val[0], opt->valrng[0]);
		} else if(strcmp(opt->name, "size") == 0) {
			psys_set_anm_rnd(&attr->size, opt->tm, opt->val[0], opt->valrng[0]);
		} else if(strcmp(opt->name, "dir") == 0) {
			psys_set_anm_rnd3(&attr->dir, opt->tm, opt->val, opt->valrng);
		} else if(strcmp(opt->name, "grav") == 0) {
			psys_set_value3(&attr->grav, opt->tm, opt->val[0], opt->val[1], opt->val[2]);
		} else if(strcmp(opt->name, "drag") == 0) {
			attr->drag = opt->val[0];
		} else if(strcmp(opt->name, "pcolor") == 0) {
			psys_set_value3(&attr->part_attr.color, opt->tm, opt->val[0], opt->val[1], opt->val[2]);
		} else if(strcmp(opt->name, "palpha") == 0) {
			psys_set_value(&attr->part_attr.alpha, opt->tm, opt->val[0]);
		} else if(strcmp(opt->name, "psize") == 0) {
			psys_set_value(&attr->part_attr.size, opt->tm, opt->val[0]);
		} else {
			fprintf(stderr, "unrecognized particle config option: %s\n", opt->name);
			goto err;
		}

		release_cfg_opt(opt);
	}

	return 0;

err:
	fprintf(stderr, "Line %d: error parsing particle definition\n", lineno);
	release_cfg_opt(opt);
	return -1;
}

static struct cfgopt *get_cfg_opt(const char *line)
{
	char *buf, *tmp;
	struct cfgopt *opt;

	/* allocate a working buffer on the stack that could fit the current line */
	buf = alloca(strlen(line) + 1);

	line = stripspace((char*)line);
	if(line[0] == '#' || !line[0]) {
		return 0;	/* skip empty lines and comments */
	}

	if(!(opt = malloc(sizeof *opt))) {
		return 0;
	}
	memset(opt, 0, sizeof *opt);

	if(!(opt->valstr = strchr(line, '='))) {
		release_cfg_opt(opt);
		return 0;
	}
	*opt->valstr++ = 0;
	opt->valstr = stripspace(opt->valstr);

	strcpy(buf, line);
	buf = stripspace(buf);

	/* parse the keyframe time specifier if it exists */
	if((tmp = strchr(buf, '('))) {
		char *endp;
		float tval;

		*tmp++ = 0;
		opt->name = malloc(strlen(buf) + 1);
		strcpy(opt->name, buf);

		tval = strtod(tmp, &endp);
		if(endp == tmp) { /* nada ... */
			opt->tm = 0;
		} else if(*endp == 's') {	/* seconds suffix */
			opt->tm = (long)(tval * 1000.0f);
		} else {
			opt->tm = (long)tval;
		}
	} else {
		opt->name = malloc(strlen(buf) + 1);
		strcpy(opt->name, buf);
		opt->tm = 0;
	}

	if(sscanf(opt->valstr, "[%f %f %f] ~ [%f %f %f]", opt->val, opt->val + 1, opt->val + 2,
				opt->valrng, opt->valrng + 1, opt->valrng + 2) == 6) {
		/* value is a vector range */
		opt->type = OPT_VEC_RANGE;

	} else if(sscanf(opt->valstr, "%f ~ %f", opt->val, opt->valrng) == 2) {
		/* value is a number range */
		opt->type = OPT_NUM_RANGE;
		opt->val[1] = opt->val[2] = opt->val[0];
		opt->valrng[1] = opt->valrng[2] = opt->valrng[0];

	} else if(sscanf(opt->valstr, "[%f %f %f]", opt->val, opt->val + 1, opt->val + 2) == 3) {
		/* value is a vector */
		opt->type = OPT_VEC;
		opt->valrng[0] = opt->valrng[1] = opt->valrng[2] = 0.0f;

	} else if(sscanf(opt->valstr, "%f", opt->val) == 1) {
		/* value is a number */
		opt->type = OPT_NUM;
		opt->val[1] = opt->val[2] = opt->val[0];
		opt->valrng[0] = opt->valrng[1] = opt->valrng[2] = 0.0f;

	} else if(sscanf(opt->valstr, "\"%s\"", buf) == 1) {
		/* just a string... strip the quotes */
		if(buf[strlen(buf) - 1] == '\"') {
			buf[strlen(buf) - 1] = 0;
		}
		opt->type = OPT_STR;
		opt->valstr = malloc(strlen(buf) + 1);
		assert(opt->valstr);
		strcpy(opt->valstr, buf);
	} else {
		/* fuck it ... */
		release_cfg_opt(opt);
		return 0;
	}

	return opt;
}

static void release_cfg_opt(struct cfgopt *opt)
{
	if(opt) {
		free(opt->name);
		opt->name = 0;
	}
	opt = 0;
}


int psys_save_attr(const struct psys_attributes *attr, const char *fname)
{
	FILE *fp;
	int res;

	if(!(fp = fopen(fname, "w"))) {
		fprintf(stderr, "psys_save_attr: failed to write file: %s: %s\n", fname, strerror(errno));
		return -1;
	}
	res = psys_save_attr_stream(attr, fp);
	fclose(fp);
	return res;
}

int psys_save_attr_stream(const struct psys_attributes *attr, FILE *fp)
{
	return -1;	/* TODO */
}


static char *stripspace(char *str)
{
	char *end;

	while(*str && isspace(*str)) {
		str++;
	}

	end = str + strlen(str) - 1;
	while(end >= str && isspace(*end)) {
		*end-- = 0;
	}
	return str;
}
