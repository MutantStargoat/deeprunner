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
#ifndef AUDIO_H_
#define AUDIO_H_

enum { AU_STOPPED, AU_PLAYING };
enum { AU_NORMAL, AU_CRITICAL };	/* sample playback priority */

enum {
	AU_CUR = 0x7000,
	AU_VOLUP = 0x7100,
	AU_VOLDN = 0x7200
};

struct au_module {
	char *name;
	void *impl;
};

struct au_sample {
	char *name;
	void *impl;
};

int au_init(void);
void au_shutdown(void);

struct au_module *au_load_module(const char *fname);
void au_free_module(struct au_module *mod);

int au_play_module(struct au_module *mod);
void au_update(void);
int au_stop_module(struct au_module *mod);
int au_module_state(struct au_module *mod);

struct au_sample *au_load_sample(const char *fname);
void au_free_sample(struct au_sample *sfx);
void au_play_sample(struct au_sample *sfx, int prio);


int au_volume(int vol);
int au_sfx_volume(int vol);
int au_music_volume(int vol);

/* pay no attention to the man behind the curtain */
#define AU_VOLADJ(vol, newvol) \
	do { \
		int d; \
		switch(newvol & 0xff00) { \
		case AU_CUR: \
			return (vol); \
		case AU_VOLUP: \
			d = newvol & 0xff; \
			(newvol) = (vol) + (d ? d : 16); \
			if((newvol) >= 256) (newvol) = 255; \
			break; \
		case AU_VOLDN: \
			d = newvol & 0xff; \
			(newvol) = (vol) - (d ? d : 16); \
			if((newvol) < 0) (newvol) = 0; \
			break; \
		default: \
			break; \
		} \
	} while(0)

#endif	/* AUDIO_H_ */
