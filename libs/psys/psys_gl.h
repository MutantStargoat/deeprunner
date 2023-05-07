/*
libpsys - reusable particle system library.
Copyright (C) 2011-2014  John Tsiombikas <nuclear@member.fsf.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef PSYS_GL_H_
#define PSYS_GL_H_

struct psys_particle;
struct psys_emitter;

void psys_gl_draw_start(const struct psys_emitter *em, void *cls);
void psys_gl_draw(const struct psys_emitter *em, const struct psys_particle *p, void *cls);
void psys_gl_draw_end(const struct psys_emitter *em, void *cls);

unsigned int psys_gl_load_texture(const char *fname, void *cls);
void psys_gl_unload_texture(unsigned int tex, void *cls);

#endif	/* PSYS_GL_H_ */
