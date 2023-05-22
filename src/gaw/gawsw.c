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
#include "gaw.h"
#include "gawswtnl.h"


void gaw_swtnl_drawprim(int prim, struct vertex *v, int vnum)
{
	int i, fill_mode;
	struct pvertex pv[16];

	for(i=0; i<vnum; i++) {
		/* convert pos to 24.8 fixed point */
		pv[i].x = cround64(v[i].x * 256.0f);
		pv[i].y = cround64(v[i].y * 256.0f);

		if(st.opt & (1 << GAW_DEPTH_TEST)) {
			/* after div/w z is in [-1, 1], remap it to [0, 0xffffff] */
			pv[i].z = cround64(v[i].z * 8388607.5f + 8388607.5f);
		}

		/* convert tex coords to 16.16 fixed point */
		pv[i].u = cround64(v[i].u * 65536.0f);
		pv[i].v = cround64(v[i].v * 65536.0f);
		/* pass the color through as is */
		pv[i].r = v[i].r;
		pv[i].g = v[i].g;
		pv[i].b = v[i].b;
		pv[i].a = v[i].a;
	}

	/* backface culling */
#if 0	/* TODO fix culling */
	if(vnum > 2 && (st.opt & (1 << GAW_CULL_FACE))) {
		int32_t ax = pv[1].x - pv[0].x;
		int32_t ay = pv[1].y - pv[0].y;
		int32_t bx = pv[2].x - pv[0].x;
		int32_t by = pv[2].y - pv[0].y;
		int32_t cross_z = (ax >> 4) * (by >> 4) - (ay >> 4) * (bx >> 4);
		int sign = (cross_z >> 31) & 1;

		if(!(sign ^ st.frontface)) {
			continue;	/* back-facing */
		}
	}
#endif

	switch(prim) {
	case GAW_POINTS:
		break;

	case GAW_LINES:
		break;

	default:
		fill_mode = st.polymode;
		if(st.opt & ((1 << GAW_TEXTURE_2D) | (1 << GAW_TEXTURE_1D))) {
			fill_mode |= POLYFILL_TEX_BIT;
		}
		if((st.opt & (1 << GAW_BLEND)) && (st.bsrc == GAW_SRC_ALPHA)) {
			fill_mode |= POLYFILL_ALPHA_BIT;
		} else if((st.opt & (1 << GAW_BLEND)) && (st.bsrc == GAW_ONE)) {
			fill_mode |= POLYFILL_ADD_BIT;
		}
		if(st.opt & (1 << GAW_DEPTH_TEST)) {
			fill_mode |= POLYFILL_ZBUF_BIT;
		}
		polyfill(fill_mode, pv, vnum);
	}
}

