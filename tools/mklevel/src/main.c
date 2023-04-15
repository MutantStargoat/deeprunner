#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "goat3d.h"
#include "level.h"
#include "darray.h"

struct room *create_room(struct goat3d_node *node);
int parse_args(int argc, char **argv);

const char *opt_fname;
struct goat3d *gscn;
struct level lvl;

int main(int argc, char **argv)
{
	int i;
	struct goat3d_node *node;
	struct room *room;

	if(parse_args(argc, argv) == -1) {
		return 1;
	}

	if(!(gscn = goat3d_create()) || goat3d_load(gscn, opt_fname) == -1) {
		return 1;
	}
	lvl.rooms = darr_alloc(0, sizeof *lvl.rooms);

	/* create room out of every top level node */
	for(i=0; i<goat3d_get_node_count(gscn); i++) {
		node = goat3d_get_node(gscn, i);
		if(goat3d_get_node_parent(node)) {
			continue;
		}

		room = create_room(node);
		darr_push(lvl.rooms, &room);
	}

	goat3d_free(gscn);
	return 0;
}

struct room *create_room(struct goat3d_node *node)
{
	int i;
	struct goat3d_node *child;
	struct room *room;

	room = malloc_nf(sizeof *room);
	room->portals = darr_alloc(0, sizeof *room->portals);
	room->objects = darr_alloc(0, sizeof *room->objects);

	for(i=0; i<goat3d_get_node_child_count(node); i++) {
		child = goat3d_get_node_child(node, i);

		if(match_prefix(goat3d_get_node_name(child), "portal_")) {
		}
	}
	return room;
}

int parse_args(int argc, char **argv)
{
	static const char *usage_fmt = "Usage: %s [options] <goat3d scene file>\n"
		"Options:\n"
		" -h: print usage and exit\n\n";
	int i;

	for(i=1; i<argc; i++) {
		if(argv[i][0] == '-') {
			if(argv[i][2]) {
				fprintf(stderr, "invalid option: %s\n", argv[i]);
				return -1;
			}
			switch(argv[i][1]) {
			case 'h':
				printf(usage_fmt, argv[0]);
				exit(0);

			default:
				fprintf(stderr, "invalid option: %s\n", argv[i]);
				return -1;
			}
		} else {
			if(opt_fname) {
				fprintf(stderr, "unexpected argument: %s\n", argv[i]);
				return -1;
			}
			opt_fname = argv[i];
		}
	}

	if(!opt_fname) {
		fprintf(stderr, "pass a goat3d scene file to process\n");
		return -1;
	}

	return 0;
}
