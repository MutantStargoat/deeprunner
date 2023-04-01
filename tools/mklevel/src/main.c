#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "goat3d.h"

int parse_args(int argc, char **argv);

const char *opt_fname;
struct goat3d *gscn;

int main(int argc, char **argv)
{
	if(parse_args(argc, argv) == -1) {
		return 1;
	}

	if(!(gscn = goat3d_create()) || goat3d_load(gscn, opt_fname) == -1) {
		return 1;
	}

	goat3d_free(gscn);
	return 0;
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
