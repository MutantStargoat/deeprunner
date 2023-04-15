#ifndef OPTIONS_H_
#define OPTIONS_H_

struct options {
	int xres, yres;
	int vsync;
	int fullscreen;
	int vol_master, vol_mus, vol_sfx;
	int music;
};

extern struct options opt;

int load_options(const char *fname);
int save_options(const char *fname);

#endif	/* OPTIONS_H_ */
