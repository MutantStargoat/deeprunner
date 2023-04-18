#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "options.h"
#include "treestor.h"

#define DEF_XRES		640
#define DEF_YRES		480
#define DEF_VSYNC		1
#define DEF_VOL			255
#define DEF_MUS			1
#define DEF_FULLSCR		1
#define DEF_INVMOUSEY	0
#define DEF_MOUSE_SPEED	50
#define DEF_SBALL_SPEED	50

struct options opt = {
	DEF_XRES, DEF_YRES,
	DEF_VSYNC,
	DEF_FULLSCR,
	DEF_VOL, DEF_VOL, DEF_VOL,
	DEF_MUS,
	DEF_INVMOUSEY,
	DEF_MOUSE_SPEED, DEF_SBALL_SPEED
};

int load_options(const char *fname)
{
	struct ts_node *cfg;

	if(!(cfg = ts_load(fname))) {
		return -1;
	}
	printf("loaded config: %s\n", fname);

	opt.xres = ts_lookup_int(cfg, "options.gfx.xres", DEF_XRES);
	opt.yres = ts_lookup_int(cfg, "options.gfx.yres", DEF_YRES);
	opt.vsync = ts_lookup_int(cfg, "options.gfx.vsync", DEF_VSYNC);
	opt.fullscreen = ts_lookup_int(cfg, "options.gfx.fullscreen", DEF_FULLSCR);

	opt.vol_master = ts_lookup_int(cfg, "options.audio.volmaster", DEF_VOL);
	opt.vol_mus = ts_lookup_int(cfg, "options.audio.volmusic", DEF_VOL);
	opt.vol_sfx = ts_lookup_int(cfg, "options.audio.volsfx", DEF_VOL);
	opt.music = ts_lookup_int(cfg, "options.audio.music", DEF_MUS);

	opt.inv_mouse_y = ts_lookup_int(cfg, "options.ctl.invmousey", DEF_INVMOUSEY);
	opt.mouse_speed = ts_lookup_int(cfg, "options.ctl.mousespeed", DEF_MOUSE_SPEED);
	opt.sball_speed = ts_lookup_int(cfg, "options.ctl.sballspeed", DEF_SBALL_SPEED);

	ts_free_tree(cfg);
	return 0;
}

#define WROPT(lvl, fmt, val, defval) \
	do { \
		int i; \
		for(i=0; i<lvl; i++) fputc('\t', fp); \
		if((val) == (defval)) fputc('#', fp); \
		fprintf(fp, fmt "\n", val); \
	} while(0)

int save_options(const char *fname)
{
	FILE *fp;

	printf("writing config: %s\n", fname);

	if(!(fp = fopen(fname, "wb"))) {
		fprintf(stderr, "failed to save options (%s): %s\n", fname, strerror(errno));
	}
	fprintf(fp, "options {\n");
	fprintf(fp, "\tgfx {\n");
	WROPT(2, "xres = %d", opt.xres, DEF_XRES);
	WROPT(2, "yres = %d", opt.yres, DEF_YRES);
	WROPT(2, "vsync = %d", opt.vsync, DEF_VSYNC);
	WROPT(2, "fullscreen = %d", opt.fullscreen, DEF_FULLSCR);
	fprintf(fp, "\t}\n");

	fprintf(fp, "\taudio {\n");
	WROPT(2, "volmaster = %d", opt.vol_master, DEF_VOL);
	WROPT(2, "volmusic = %d", opt.vol_mus, DEF_VOL);
	WROPT(2, "volsfx = %d", opt.vol_sfx, DEF_VOL);
	WROPT(2, "music = %d", opt.music ? 1 : 0, DEF_MUS);
	fprintf(fp, "\t}\n");

	fprintf(fp, "\tcontrols {\n");
	WROPT(2, "invmousey = %d", opt.inv_mouse_y, DEF_INVMOUSEY);
	WROPT(2, "mousespeed = %d", opt.mouse_speed, DEF_MOUSE_SPEED);
	WROPT(2, "sballspeed = %d", opt.sball_speed, DEF_SBALL_SPEED);
	fprintf(fp, "\t}\n");

	fprintf(fp, "}\n");
	fprintf(fp, "# v" "i:ts=4 sts=4 sw=4 noexpandtab:\n");

	fclose(fp);
	return 0;
}
