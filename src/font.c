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
#include "font.h"

int load_font(struct font *font, const char *fname)
{
	if(!(font->dtxfont = dtx_open_font_glyphmap(fname))) {
		fprintf(stderr, "failed to load font: %s\n", fname);
		return -1;
	}
	font->size = dtx_get_glyphmap_ptsize(dtx_get_glyphmap(font->dtxfont, 0));
	dtx_use_font(font->dtxfont, font->size);
	font->height = dtx_line_height();
	font->baseline = font->height * 0.2;
	return 0;
}

void destroy_font(struct font *font)
{
	if(!font) return;

	dtx_close_font(font->dtxfont);
}

void use_font(struct font *font)
{
	dtx_use_font(font->dtxfont, font->size);
}

float font_strwidth(struct font *font, const char *str)
{
	dtx_use_font(font->dtxfont, font->size);
	return dtx_string_width(str);
}
