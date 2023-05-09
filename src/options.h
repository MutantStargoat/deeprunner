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
#ifndef OPTIONS_H_
#define OPTIONS_H_

enum {GFXOPT_TEX_NEAREST, GFXOPT_TEX_BILINEAR, GFXOPT_TEX_TRILINEAR};
enum {GFXOPT_TEX_LOW, GFXOPT_TEX_MID, GFXOPT_TEX_HIGH};

struct gfxoptions {
	int blendui;
	float drawdist;
	int texfilter;
	int texsize;
	int dither;
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
