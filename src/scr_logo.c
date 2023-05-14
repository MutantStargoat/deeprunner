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
#include "gaw/gaw.h"
#include "game.h"
#include "mtltex.h"
#include "gfxutil.h"
#include "mesh.h"
#include "drawtext.h"
#include "goat3d.h"
#include "audio.h"

static int logo_init(void);
static void logo_destroy(void);
static int logo_start(void);
static void logo_stop(void);
static void logo_display(void);
static void logo_reshape(int x, int y);
static void logo_keyb(int key, int press);
static void logo_mouse(int bn, int press, int x, int y);
static void logo_motion(int x, int y);


struct game_screen scr_logo = {
	"menu",
	logo_init, logo_destroy,
	logo_start, logo_stop,
	logo_display, logo_reshape,
	logo_keyb, logo_mouse, logo_motion
};

static struct mesh mesh_logo;
static struct mesh mesh_sgi;
static struct texture *tex_o2boot;
static struct texture *tex_env;
static struct texture *tex_way;
static struct texture *tex_msg;

static enum {ST_INVAL, ST_GOAT, ST_SGI} state;
static long tmsec, tstart;
static float tsec;


static int logo_init(void)
{
	return 0;
}

static void logo_destroy(void)
{
}

static int logo_start(void)
{
	float matrix[16];

	gaw_clear_color(0, 0, 0, 1);

	gaw_disable(GAW_LIGHTING);
	gaw_disable(GAW_CULL_FACE);
	gaw_disable(GAW_DEPTH_TEST);

	if(mesh_load(&mesh_logo, "data/msglogo.g3d", 0) == -1) {
		return 0;
	}
	cgm_mrotation_x(matrix, cgm_deg_to_rad(90));
	mesh_transform(&mesh_logo, matrix);

	if(mesh_load(&mesh_sgi, "data/sgilogo.g3d", "sgilogo") == -1) {
		return 0;
	}
	mesh_compile(&mesh_sgi);

	if(!(tex_o2boot = tex_load("data/o2boot.png"))) {
		return 0;
	}
	if(!(tex_env = tex_load("data/refmap1.jpg"))) {
		return 0;
	}
	if(!(tex_way = tex_load("data/theway.png"))) {
		return 0;
	}
	if(!(tex_msg = tex_load("data/msgtext.png"))) {
		return 0;
	}

	state = ST_GOAT;
	tstart = time_msec;
	return 0;
}

static void logo_stop(void)
{
	mesh_destroy(&mesh_logo);
	mesh_destroy(&mesh_sgi);
	tex_free(tex_o2boot);
	tex_free(tex_env);
	tex_free(tex_way);
	tex_free(tex_msg);
}

#define TEXTW	400.0f
#define TEXTH	(TEXTW / 4)
static void msglogo(void)
{
	float vwidth = win_aspect * 480.0f;
	float x, y;

	gaw_clear(GAW_COLORBUF);

	gaw_matrix_mode(GAW_MODELVIEW);
	gaw_load_identity();
	gaw_translate(0, 0, -10);

	gaw_color3f(1, 1, 1);
	mesh_draw(&mesh_logo);

	begin2d(480);

	x = (vwidth - TEXTW) / 2.0f;
	y = 360;

	gaw_enable(GAW_BLEND);
	gaw_blend_func(GAW_SRC_ALPHA, GAW_ONE_MINUS_SRC_ALPHA);

	gaw_set_tex2d(tex_msg->texid);
	gaw_begin(GAW_QUADS);
	gaw_texcoord2f(0, 0); gaw_vertex2f(x, y);
	gaw_texcoord2f(1, 0); gaw_vertex2f(x + TEXTW, y);
	gaw_texcoord2f(1, 1); gaw_vertex2f(x + TEXTW, y + TEXTH);
	gaw_texcoord2f(0, 1); gaw_vertex2f(x, y + TEXTH);
	gaw_end();

	end2d();
}

static float clamp(float x, float a, float b)
{
	return x < a ? a : (x > b ? b : x);
}

#define SGISTART		4000
#define FADEOUT_START	(SGISTART + 7000)
#define FADEOUT_DUR		1000
#define END_TIME		FADEOUT_START + FADEOUT_DUR
#define MAXV			(1.25 * 256.0 / 1024.0)
#define O2ASPECT		(1280.0 / 1024.0)
#define WAYSCALE		0.75f
static void sgiscr(void)
{
	float aspect = O2ASPECT / win_aspect;
	float trot, alpha, z;
	float tsec = (tmsec - SGISTART) / 1000.0f;
	static int played_chime;

	if(!played_chime && tsec >= 1.0f) {
		au_play_sample(sfx_o2chime, 0);
		played_chime = 1;
	}

	gaw_disable(GAW_DEPTH_TEST);

	gaw_matrix_mode(GAW_MODELVIEW);
	gaw_load_identity();
	gaw_matrix_mode(GAW_PROJECTION);
	gaw_push_matrix();
	gaw_load_identity();

	gaw_begin(GAW_QUADS);
	gaw_color3ub(95, 95, 143);
	gaw_vertex2f(-1, -1);
	gaw_vertex2f(1, -1);
	gaw_color3ub(175, 209, 254);
	gaw_vertex2f(1, 1);
	gaw_vertex2f(-1, 1);
	gaw_end();

	gaw_set_tex2d(tex_o2boot->texid);
	gaw_begin(GAW_QUADS);
	gaw_color3f(1, 1, 1);
	gaw_texcoord2f(0, 1); gaw_vertex2f(-aspect, -1);
	gaw_texcoord2f(1, 1); gaw_vertex2f(aspect, -1);
	gaw_texcoord2f(1, 0); gaw_vertex2f(aspect, MAXV * 2.0 - 1.0);
	gaw_texcoord2f(0, 0); gaw_vertex2f(-aspect, MAXV * 2.0 - 1.0);
	gaw_end();

	alpha = clamp(tsec - 5.5, 0, 1);
	gaw_translate(0, 0.34, 0);
	gaw_scale(WAYSCALE / win_aspect, WAYSCALE, 1);
	gaw_set_tex2d(tex_way->texid);
	gaw_enable(GAW_BLEND);
	gaw_blend_func(GAW_SRC_ALPHA, GAW_ONE_MINUS_SRC_ALPHA);
	gaw_begin(GAW_QUADS);
	gaw_color4f(1, 1, 1, alpha);
	gaw_texcoord2f(0, 1); gaw_vertex2f(-1, -1);
	gaw_texcoord2f(1, 1); gaw_vertex2f(1, -1);
	gaw_texcoord2f(1, 0); gaw_vertex2f(1, 1);
	gaw_texcoord2f(0, 0); gaw_vertex2f(-1, 1);
	gaw_end();

	gaw_disable(GAW_BLEND);
	gaw_set_tex2d(0);

	gaw_pop_matrix();

	gaw_clear(GAW_DEPTHBUF);

	/* SGI logo */
	gaw_save();	/* XXX had viewport bit here too, is it necessary? */
	gaw_enable(GAW_DEPTH_TEST);
	gaw_disable(GAW_LIGHTING);
	gaw_enable(GAW_LIGHT0);
	gaw_enable(GAW_CULL_FACE);

	trot = cgm_smoothstep(0, 6, tsec) * 360 * 2.0f;
	//z = cgm_logerp(100, 50, clamp(tsec - 1.0f, 0, 1));
	z = cgm_lerp(50, 32, clamp(tsec - 1.0f, 0, 1));
	//z = 100.0f - cgm_smoothstep(-5, 5, tsec - 1.0f) * 50.0;
	alpha = clamp((tsec / 2) - 0.5, 0, 1);

	gaw_viewport(win_width / 4, win_height * 0.43, win_width / 2, win_height / 2);
	gaw_matrix_mode(GAW_MODELVIEW);
	gaw_translate(0, 0, -z);

	gaw_translate(0, 0, 10);
	gaw_rotate(trot, 0, 1, 0);
	gaw_rotate(35, 1, 0, 0);
	gaw_rotate(-45, 0, 1, 0);

	gaw_color4f(1, 1, 1, alpha);
	gaw_mtl_diffuse(0.1, 0.1, 0.1, alpha);
	gaw_mtl_specular(1, 1, 1, 60.0f);
	gaw_enable(GAW_BLEND);
	gaw_blend_func(GAW_SRC_ALPHA, GAW_ONE_MINUS_SRC_ALPHA);
	if(alpha < 0.8) {
		gaw_depth_mask(0);
	}

	gaw_set_tex2d(tex_env->texid);
	gaw_texenv_sphmap(1);

	gaw_draw_compiled(mesh_sgi.dlist);

	gaw_viewport(0, 0, win_width, win_height);
	gaw_restore();
	gaw_depth_mask(1);
}

static void logo_display(void)
{
	tmsec = time_msec - tstart;
	tsec = (float)tmsec / 1000.0f;

	if(tmsec >= END_TIME) {
		game_chscr(&scr_menu);
	}

	switch(state) {
	case ST_GOAT:
		msglogo();
		if(tmsec >= SGISTART) {
			state = ST_SGI;
		}
		break;

	case ST_SGI:
		sgiscr();
		break;

	default:
		game_chscr(&scr_menu);
		break;
	}

	if(tmsec >= FADEOUT_START) {
		float alpha = clamp((tmsec - FADEOUT_START) / (float)FADEOUT_DUR, 0, 1);

		gaw_matrix_mode(GAW_MODELVIEW);
		gaw_load_identity();
		gaw_matrix_mode(GAW_PROJECTION);
		gaw_push_matrix();
		gaw_load_identity();

		gaw_save();
		gaw_disable(GAW_DEPTH_TEST);
		gaw_enable(GAW_BLEND);
		gaw_blend_func(GAW_SRC_ALPHA, GAW_ONE_MINUS_SRC_ALPHA);
		gaw_set_tex2d(0);
		gaw_color4f(0, 0, 0, alpha);
		gaw_rect(-1, -1, 1, 1);

		gaw_restore();

		gaw_pop_matrix();
		gaw_matrix_mode(GAW_MODELVIEW);
	}
}

static void logo_reshape(int x, int y)
{
	gaw_matrix_mode(GAW_PROJECTION);
	gaw_load_identity();
	gaw_perspective(50, win_aspect, 0.5, 100.0);
}

static void logo_keyb(int key, int press)
{
	if(press) {
		game_chscr(&scr_menu);
	}
}

static void logo_mouse(int bn, int press, int x, int y)
{
	if(press) {
		game_chscr(&scr_menu);
	}
}

static void logo_motion(int x, int y)
{
}
