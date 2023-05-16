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
#ifndef GAW_SW_H_
#define GAW_SW_H_

void gaw_sw_init(void);
void gaw_sw_destroy(void);
void gaw_sw_reset(void);
void gaw_sw_framebuffer(int width, int height, void *pixels);
void gaw_sw_framebuffer_addr(void *pixels);

#endif	/* GAW_SW_H_ */
