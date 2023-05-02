#ifndef OPTIONS_H_
#define OPTIONS_H_

struct gfxoptions {
	int blendui;
	float drawdist;
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
