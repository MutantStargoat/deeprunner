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
	DEF_MOUSE_SPEED, DEF_SBALL_SPEED,
};

#ifndef __sgi
/* PC specific profiles (modern) */
static struct gfxoptions gfxdef_ultra = {
	1,		/* blendui */
	80,		/* draw distance */
	GFXOPT_TEX_TRILINEAR,
	GFXOPT_TEX_HIGH,
	1		/* dither */
};
#else
/* SGI specific profiles */
static struct gfxoptions gfxdef_vpro32 = {
	1,		/* blendui */
	80,		/* draw distance */
	GFXOPT_TEX_TRILINEAR,
	GFXOPT_TEX_MID,
	1		/* dither */
};
static struct gfxoptions gfxdef_vpro128 = {
	1,		/* blendui */
	80,		/* draw distance */
	GFXOPT_TEX_TRILINEAR,
	GFXOPT_TEX_HIGH,
	1		/* dither */
};
static struct gfxoptions gfxdef_o2 = {
	0,		/* blendui */
	80,		/* draw distance */
	GFXOPT_TEX_BILINEAR,
	GFXOPT_TEX_MID,
	0		/* dither */
};
#endif
static struct gfxoptions gfxdef_lowest = {
	0,		/* blendui */
	80,		/* draw distance */
	GFXOPT_TEX_NEAREST,
	GFXOPT_TEX_LOW,
	0		/* dither */
};
static struct gfxoptions gfxdefopt;

static void detect_defaults(void);


int load_options(const char *fname)
{
	struct ts_node *cfg;

	detect_defaults();
	opt.gfx = gfxdefopt;

	if(!(cfg = ts_load(fname))) {
		return -1;
	}
	printf("loaded config: %s\n", fname);

	opt.xres = ts_lookup_int(cfg, "options.video.xres", DEF_XRES);
	opt.yres = ts_lookup_int(cfg, "options.video.yres", DEF_YRES);
	opt.vsync = ts_lookup_int(cfg, "options.video.vsync", DEF_VSYNC);
	opt.fullscreen = ts_lookup_int(cfg, "options.video.fullscreen", DEF_FULLSCR);

	opt.vol_master = ts_lookup_int(cfg, "options.audio.volmaster", DEF_VOL);
	opt.vol_mus = ts_lookup_int(cfg, "options.audio.volmusic", DEF_VOL);
	opt.vol_sfx = ts_lookup_int(cfg, "options.audio.volsfx", DEF_VOL);
	opt.music = ts_lookup_int(cfg, "options.audio.music", DEF_MUS);

	opt.inv_mouse_y = ts_lookup_int(cfg, "options.controls.invmousey", DEF_INVMOUSEY);
	opt.mouse_speed = ts_lookup_int(cfg, "options.controls.mousespeed", DEF_MOUSE_SPEED);
	opt.sball_speed = ts_lookup_int(cfg, "options.controls.sballspeed", DEF_SBALL_SPEED);

	opt.gfx.blendui = ts_lookup_int(cfg, "options.gfx.blendui", gfxdefopt.blendui);
	opt.gfx.drawdist = ts_lookup_num(cfg, "options.gfx.drawdist", gfxdefopt.drawdist);
	opt.gfx.texfilter = ts_lookup_int(cfg, "options.gfx.texfilter", gfxdefopt.texfilter);
	if(opt.gfx.texfilter < GFXOPT_TEX_NEAREST) opt.gfx.texfilter = GFXOPT_TEX_NEAREST;
	if(opt.gfx.texfilter > GFXOPT_TEX_TRILINEAR) opt.gfx.texfilter = GFXOPT_TEX_TRILINEAR;
	opt.gfx.texsize = ts_lookup_int(cfg, "options.gfx.texsize", gfxdefopt.texsize);
	if(opt.gfx.texsize < GFXOPT_TEX_LOW) opt.gfx.texsize = GFXOPT_TEX_LOW;
	if(opt.gfx.texsize > GFXOPT_TEX_HIGH) opt.gfx.texsize = GFXOPT_TEX_HIGH;

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
	fprintf(fp, "\tvideo {\n");
	WROPT(2, "xres = %d", opt.xres, DEF_XRES);
	WROPT(2, "yres = %d", opt.yres, DEF_YRES);
	WROPT(2, "vsync = %d", opt.vsync, DEF_VSYNC);
	WROPT(2, "fullscreen = %d", opt.fullscreen, DEF_FULLSCR);
	fprintf(fp, "\t}\n");

	fprintf(fp, "\tgfx {\n");
	WROPT(2, "blendui = %d", opt.gfx.blendui, gfxdefopt.blendui);
	WROPT(2, "drawdist = %f", opt.gfx.drawdist, gfxdefopt.drawdist);
	WROPT(2, "texfilter = %d", opt.gfx.texfilter, gfxdefopt.texfilter);
	WROPT(2, "texsize = %d", opt.gfx.texsize, gfxdefopt.texsize);
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

#if defined(__unix__) || defined(unix)
#include <sys/utsname.h>

#ifdef __sgi
#include <invent.h>

static const char *ipnames[] = {
	0, "IRIS 1000", "IRIS 2x00/3x00", 0, "IRIS 4D",		/* 0-4 */
	"IRIS 4D", "IRIS 4D", "IRIS 4D", 0, "IRIS 4D/210",	/* 5-9 */
	0, 0, "Indigo R3000", 0, 0,							/* 10-14 */
	0, 0, "IRIS Crimson", 0, "Onyx/Challenge",			/* 15-19 */
	"Indigo R4000", "POWER Onyx/Challenge", "Indy/Indigo2 (teal)", 0, 0, /* 20-24 */
	"Onyx/Challenge R10k", "Power Indigo2", "Onyx2/Origin2000", "Indigo2 (purple)", 0, /* 25-29 */
	"Octane", 0, "O2", 0, 0, /* 30-34*/
	"Fuel/Tezro/Onyx3k/Origin3k", 0, 0, 0	/* 35-39 */
};

struct gfxinfo {
	int gfx;
	unsigned int st;
};

static int searchfunc(inventory_t *inv, void *cls)
{
	struct gfxinfo *ginf = cls;

	if(inv->inv_class != INV_GRAPHICS) {
		return 0;
	}

	ginf->gfx = inv->inv_type;
	ginf->st = inv->inv_state;
	return 1;
}

static int hinv_gfx(struct gfxinfo *ginf)
{
	if(scaninvent(searchfunc, ginf) > 0) {
		return ginf->gfx;
	}
	return -1;
}

#endif

static void detect_defaults(void)
{
#ifdef __sgi
	struct gfxinfo ginf;
	int ip = 0;
#endif
	struct utsname uts;

	if(uname(&uts) == -1) {
		printf("uname failed: assume O2\n");
		gfxdefopt = gfxdef_lowest;	/* on failure assume the worst */
		return;
	}

#ifdef __sgi
	sscanf(uts.machine, "IP%d", &ip);
	printf("Running on IP%d machine", ip);
	if(ip < sizeof ipnames / sizeof *ipnames && ipnames[ip]) {
		printf(" (SGI %s)\n", ipnames[ip]);
	} else {
		putchar('\n');
	}

	if(ip == 32) {	/* O2 */
		printf("gfx detect: O2\n");
		gfxdefopt = gfxdef_o2;
	} else if(ip == 30) {	/* Octane */
		printf("gfx detect: ");
		if(hinv_gfx(&ginf) == INV_ODSY) {
			int arch = ginf.st & INV_ODSY_ARCHS;

			switch(ginf.st & INV_ODSY_MEMCFG) {
			case INV_ODSY_MEMCFG_32:
				printf("Odyssey V%d\n", arch == INV_ODSY_REVA_ARCH ? 6 : 10);
				gfxdefopt = gfxdef_vpro32;
				break;

			case INV_ODSY_MEMCFG_64:
				printf("Odyssey 64MB\n");
				gfxdefopt = gfxdef_vpro32;
				break;

			case INV_ODSY_MEMCFG_128:
			case INV_ODSY_MEMCFG_256:
			case INV_ODSY_MEMCFG_512:
				printf("Odyssey V%d\n", arch == INV_ODSY_REVA_ARCH ? 8 : 12);
				gfxdefopt = gfxdef_vpro128;
				break;

			default:
				printf("???\n");
				gfxdefopt = gfxdef_vpro128;
			}
		} else {
			/* TODO temp stopgap, research the octane impact options and adjust */
			printf("Impact\n");
			gfxdefopt = gfxdef_o2;
		}
	} else if(ip < 32) {
		printf("gfx detect: low end\n");
		gfxdefopt = gfxdef_lowest;
	} else if(ip > 32) {
		printf("gfx detect: high end\n");
		gfxdefopt = gfxdef_vpro128;
	}

#else
	/* TODO: improve detection, for now assume non-IRIX means modern PC */
	printf("Running on %s\n", uts.sysname);
	gfxdefopt = gfxdef_ultra;
#endif
}

#elif defined(WIN32) || defined(__WIN32)
static void detect_defaults(void)
{
	/* TODO: improve detection, for now assume windows hosts are modern PCs */
	printf("Running on windows\n");
	gfxdefopt = gfxdef_ultra;
}

#endif	/* unix/windows */
