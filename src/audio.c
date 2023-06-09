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
#include <string.h>
#include <ctype.h>
#if defined(__WATCOMC__) || defined(_WIN32) || defined(__DJGPP__)
#include <malloc.h>
#else
#include <alloca.h>
#endif
#include "mikmod.h"
#include "audio.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif

#define NUM_SFX_CHAN	16

#define SET_SFX_VOL(vol) \
	do { \
		int mv = (vol) * vol_master >> 9; \
		md_sndfxvolume = mv ? mv + 1 : 0; \
	} while(0)

#define SET_MUS_VOL(vol) \
	do { \
		int mv = (vol) * vol_master >> 9; \
		Player_SetVolume((SWORD)(mv ? mv + 1 : 0)); \
	} while(0)

static struct au_module *curmod;
static int vol_master, vol_mus, vol_sfx;

#ifdef _WIN32
static DWORD WINAPI update(void *cls);
#else
static void *update(void *cls);
#endif

int au_init(void)
{
	curmod = 0;
	vol_master = vol_mus = vol_sfx = 255;

#if defined(__linux__)
	MikMod_RegisterDriver(&drv_pulseaudio);
	MikMod_RegisterDriver(&drv_alsa);
	MikMod_RegisterDriver(&drv_oss);
#elif defined(__FreeBSD__)
	MikMod_RegisterDriver(&drv_oss);
#elif defined(__sgi)
	MikMod_RegisterDriver(&drv_sgi);
#elif defined(_WIN32)
	MikMod_RegisterDriver(&drv_win);
	/*MikMod_RegisterDriver(&drv_ds);*/
	/*MikMod_RegisterDriver(&drv_xaudio2);*/
#else
	MikMod_RegisterDriver(&drv_nos);
#endif

	MikMod_RegisterLoader(&load_it);
	MikMod_RegisterLoader(&load_mod);
	MikMod_RegisterLoader(&load_s3m);
	MikMod_RegisterLoader(&load_xm);

	md_mode |= DMODE_SOFT_SNDFX;
	if(MikMod_Init("")) {
		fprintf(stderr, "failed ot initialize mikmod: %s\n", MikMod_strerror(MikMod_errno));
		return -1;
	}
	MikMod_InitThreads();

	MikMod_SetNumVoices(-1, NUM_SFX_CHAN);

	{
#ifdef _WIN32
		HANDLE thr;
		if((thr = CreateThread(0, 0, update, 0, 0, 0))) {
			CloseHandle(thr);
		}
#else
		pthread_t upd_thread;
		if(pthread_create(&upd_thread, 0, update, 0) == 0) {
			pthread_detach(upd_thread);
		}
#endif
	}

	printf("Available MikMod drivers:\n%s\n", MikMod_InfoDriver());
	printf("Using MikMod driver: %s\n", md_driver->Name);
	MikMod_EnableOutput();
	return 0;
}

void au_shutdown(void)
{
	curmod = 0;
	MikMod_DisableOutput();
	MikMod_Exit();
}

struct au_module *au_load_module(const char *fname)
{
	struct au_module *mod;
	MODULE *mikmod;
	char *name = 0, *end;

	if(!(mod = malloc(sizeof *mod))) {
		fprintf(stderr, "au_load_module: failed to allocate module\n");
		return 0;
	}

	if(!(mikmod = Player_Load(fname, 64, 0))) {
		fprintf(stderr, "au_load_module: failed to load module: %s: %s\n",
				fname, MikMod_strerror(MikMod_errno));
		free(mod);
		return 0;
	}
	mod->impl = mikmod;

	if(mikmod->songname && *mikmod->songname) {
		name = alloca(strlen(mikmod->songname) + 1);
		strcpy(name, mikmod->songname);

		end = name + strlen(name) - 1;
		while(end >= name && isspace(*end)) {
			*end-- = 0;
		}
		if(!*name) name = 0;
	}

	if(!name) {
		/* fallback to using the filename */
		if((name = strrchr(fname, '/')) || (name = strrchr(fname, '\\'))) {
			name++;
		} else {
			name = (char*)fname;
		}
	}

	if(!(mod->name = malloc(strlen(name) + 1))) {
		fprintf(stderr, "au_load_module: mod->name malloc failed\n");
		Player_Free(mod->impl);
		free(mod);
		return 0;
	}
	strcpy(mod->name, name);

	printf("loaded module \"%s\" (%s)\n", name, fname);
	return mod;
}

void au_free_module(struct au_module *mod)
{
	if(!mod) return;

	if(mod == curmod) {
		au_stop_module(curmod);
	}
	Player_Free(mod->impl);
	free(mod->name);
	free(mod);
}

int au_play_module(struct au_module *mod)
{
	if(curmod) {
		if(curmod == mod) return 0;
		au_stop_module(curmod);
	}

	((MODULE*)mod->impl)->wrap = 1;
	Player_Start(mod->impl);
	SET_MUS_VOL(vol_mus);
	curmod = mod;
	return 0;
}

void au_update(void)
{
	if(!curmod) return;

	if(!Player_Active()) {
		Player_Stop();
		curmod = 0;
	}
}

#ifdef _WIN32
static DWORD WINAPI update(void *cls)
#else
static void *update(void *cls)
#endif
{
	for(;;) {
		MikMod_Update();
#ifdef _WIN32
		Sleep(10);
#else
		usleep(10000);
#endif
	}
	return 0;
}

int au_stop_module(struct au_module *mod)
{
	if(mod && curmod != mod) return -1;
	if(!curmod) return -1;

	Player_Stop();
	curmod = 0;
	return 0;
}

int au_module_state(struct au_module *mod)
{
	if(mod) {
		return curmod == mod ? AU_PLAYING : AU_STOPPED;
	}
	return curmod ? AU_PLAYING : AU_STOPPED;
}

struct au_sample *au_load_sample(const char *fname)
{
	struct au_sample *sfx;
	SAMPLE *mmsfx;

	printf("loading sfx: %s\n", fname);
	if(!(mmsfx = Sample_Load(fname))) {
		fprintf(stderr, "failed to load sample: %s: %s\n", fname,
				MikMod_strerror(MikMod_errno));
		return 0;
	}
	mmsfx->panning = PAN_CENTER;

	if(!(sfx = malloc(sizeof *sfx))) {
		fprintf(stderr, "failed to allocate sample\n");
		Sample_Free(mmsfx);
		return 0;
	}
	sfx->name = strdup(fname);
	sfx->impl = mmsfx;
	return sfx;
}

void au_free_sample(struct au_sample *sfx)
{
	if(!sfx) return;

	Sample_Free(sfx->impl);
	free(sfx->name);
	free(sfx);
}

void au_play_sample(struct au_sample *sfx, int prio)
{
	Sample_Play(sfx->impl, 0, prio == AU_CRITICAL ? SFX_CRITICAL : 0);
}

int au_volume(int vol)
{
	AU_VOLADJ(vol_master, vol);
	if(vol != vol_master) {
		vol_master = vol;

		au_sfx_volume(vol_sfx);
		au_music_volume(vol_mus);
	}
	return vol_master;
}

int au_sfx_volume(int vol)
{
	AU_VOLADJ(vol_sfx, vol);
	vol_sfx = vol;
	SET_SFX_VOL(vol);
	return vol_sfx;
}

int au_music_volume(int vol)
{
	AU_VOLADJ(vol_mus, vol);
	vol_mus = vol;

	if(curmod) {
		SET_MUS_VOL(vol);
	}
	return vol_mus;
}
