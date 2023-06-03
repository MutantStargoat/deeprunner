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
#ifndef FONT_H_
#define FONT_H_

#include "drawtext.h"

struct font {
	struct dtx_font *dtxfont;
	int size;
	float height, baseline;
};

int load_font(struct font *font, const char *fname);
void destroy_font(struct font *font);

void use_font(struct font *font);

float font_strwidth(struct font *font, const char *str);

#endif	/* FONT_H_ */
