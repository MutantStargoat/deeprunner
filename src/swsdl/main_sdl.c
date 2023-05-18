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
#include <stdlib.h>
#include <SDL/SDL.h>
#include "util.h"
#include "game.h"
#include "gaw/gaw_sw.h"

static int handle_event(SDL_Event *ev);
static int translate_keysym(int sym);

static SDL_Surface *fbsurf;
static int quit;

static uint32_t *framebuf;

int main(int argc, char **argv)
{
	SDL_Event ev;

	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == -1) {
		fprintf(stderr, "failed to initialize SDL\n");
		return 1;
	}

	if(!(fbsurf = SDL_SetVideoMode(640, 480, 32, SDL_SWSURFACE | SDL_RESIZABLE))) {
		fprintf(stderr, "failed to initialize graphics\n");
		SDL_Quit();
		return 1;
	}
	SDL_WM_SetCaption("DeepRunner", 0);
	gaw_sw_init();
	game_resize(640, 480);

	if(game_init() == -1) {
		return 1;
	}

	while(!quit) {
		while(SDL_PollEvent(&ev)) {
			if(handle_event(&ev) == -1 || quit) {
				goto end;
			}
		}

		game_display();
	}

end:
	game_shutdown();
	SDL_Quit();
	return 0;
}


long game_getmsec(void)
{
	return SDL_GetTicks();
}

void game_swap_buffers(void)
{
	uint32_t *fbptr;

	if(SDL_MUSTLOCK(fbsurf)) {
		SDL_LockSurface(fbsurf);
	}

	fbptr = fbsurf->pixels;
	memcpy(fbptr, framebuf, fbsurf->w * fbsurf->h * 4);

	if(SDL_MUSTLOCK(fbsurf)) {
		SDL_UnlockSurface(fbsurf);
	}
	SDL_Flip(fbsurf);
}

void game_quit(void)
{
	quit = 1;
}

void game_resize(int x, int y)
{
	int npix, nprevpix;

	if(x == win_width && y == win_height) return;

	SDL_SetVideoMode(x, y, 32, SDL_SWSURFACE | SDL_RESIZABLE);

	npix = x * y;
	nprevpix = win_width * win_height;

	if(npix > nprevpix) {
		framebuf = realloc_nf(framebuf, npix * sizeof *framebuf);
	}
	game_reshape(x, y);

	gaw_sw_framebuffer(x, y, framebuf);
}

void game_fullscreen(int fs)
{
}

void game_grabmouse(int grab)
{
}

void game_vsync(int vsync)
{
}

static int handle_event(SDL_Event *ev)
{
	int key, bn;

	switch(ev->type) {
	case SDL_VIDEORESIZE:
		if(ev->resize.w != win_width || ev->resize.h != win_height) {
			SDL_SetVideoMode(ev->resize.w, ev->resize.h, 32, SDL_SWSURFACE | SDL_RESIZABLE);
			game_resize(ev->resize.w, ev->resize.h);
		}
		break;

	case SDL_KEYDOWN:
	case SDL_KEYUP:
		if((key = translate_keysym(ev->key.keysym.sym)) > 0) {
			game_keyboard(key, ev->key.state == SDL_PRESSED);
		}
		break;

	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
		bn = ev->button.button - SDL_BUTTON_LEFT;
		game_mouse(bn, ev->button.state == SDL_PRESSED, ev->button.x, ev->button.y);
		break;

	case SDL_MOUSEMOTION:
		game_motion(ev->motion.x, ev->motion.y);
		break;

	case SDL_QUIT:
		return -1;

	default:
		break;
	}

	return 0;
}


static int translate_keysym(int sym)
{
	switch(sym) {
	case SDLK_LEFT:
		return GKEY_LEFT;
	case SDLK_UP:
		return GKEY_UP;
	case SDLK_RIGHT:
		return GKEY_RIGHT;
	case SDLK_DOWN:
		return GKEY_DOWN;
	case SDLK_PAGEUP:
		return GKEY_PGUP;
	case SDLK_PAGEDOWN:
		return GKEY_PGDOWN;
	case SDLK_HOME:
		return GKEY_HOME;
	case SDLK_END:
		return GKEY_END;
	case SDLK_INSERT:
		return GKEY_INS;
	default:
		if(sym >= SDLK_F1 && sym <= SDLK_F12) {
			return sym - SDLK_F1 + GKEY_F1;
		} else if(sym < 128) {
			return sym;
		}
	}
	return -1;
}
