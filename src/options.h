#ifndef OPTIONS_H_
#define OPTIONS_H_

enum {GFXOPT_TEX_NEAREST, GFXOPT_TEX_BILINEAR, GFXOPT_TEX_TRILINEAR};

struct gfxoptions {
	int blendui;
	float drawdist;
	int texfilter;
};

struct options {
	int xres, yres;
	int vsync;
	int fullscreen;
	int vol_master, vol_mus, vol_sfx;
	int music;

	int inv_mouse_y;
	int mouse_speed, sball_speed;

	struct gfxoptions gfx;
};

extern struct options opt;

int load_options(const char *fname);
int save_options(const char *fname);

#endif	/* OPTIONS_H_ */
