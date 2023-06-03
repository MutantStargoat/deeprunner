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
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "gaw/gaw.h"
#include "imago2.h"
#include "game.h"
#include "util.h"
#include "level.h"
#include "input.h"
#include "player.h"
#include "cgmath/cgmath.h"
#include "audio.h"
#include "font.h"
#include "options.h"
#include "rendlvl.h"
#include "gfxutil.h"
#include "drawtext.h"
#include "loading.h"
#include "enemy.h"
#include "darray.h"

static int ginit(void);
static void gdestroy(void);
static int gstart(void);
static void gstop(void);
static void gdisplay(void);
static void draw_ui(void);
static void greshape(int x, int y);
static void gkeyb(int key, int press);
static void gmouse(int bn, int press, int x, int y);
static void gmotion(int x, int y);
static void gsball_motion(int x, int y, int z);
static void gsball_rotate(int x, int y, int z);
static void gsball_button(int bn, int state);

#ifdef DBG_SHOW_FRUST
static void draw_frustum(const cgm_vec4 *frust);
#endif


struct game_screen scr_game = {
	"game",
	ginit, gdestroy,
	gstart, gstop,
	gdisplay, greshape,
	gkeyb, gmouse, gmotion,
	gsball_motion, gsball_rotate, gsball_button
};

static float view_mat[16], proj_mat[16];

struct player *player;

static struct level lvl;
static struct texture *uitex, *timertex;
static struct mesh adidome;

static struct dtx_font *font_hp;
static int font_hp_size;
static struct texture *font_hp_tex;

static struct dtx_font *font_timer;
static int font_timer_size;

static struct au_module *mod;

#ifdef DBG_FREEZEVIS
int dbg_freezevis;
#endif
#ifdef DBG_SHOW_COLPOLY
const struct triangle *dbg_hitpoly;
#endif
#ifdef DBG_SHOW_MAX_COL_ITER
int dbg_max_col_iter;
#endif
#ifdef DBG_SHOW_FRUST
extern cgm_vec4 dbg_frust[][6];
extern int dbg_num_frust;
static int dbg_frust_idx = -1;
#endif
static cgm_vec3 vispos;
static cgm_quat visrot;

static int lasers;
static unsigned int laser_tex;
static struct texture *tex_flare;
static struct collision lasers_hit, missile_hit;

static struct texture *tex_damage;
static long start_time;
static char timertext[64];

static int gameover;
static int victory;


static int ginit(void)
{
	int i;
	unsigned char pix[8 * 4];
	unsigned char *pptr;
	float matrix[16];

	pptr = pix + 7 * 4;
	for(i=0; i<4; i++) {
		int blue = ((float)i / 3.0f) * 512.0f;
		int rest = (int)(pow((float)i / 3.0f, 4) * 200.0f);
		int r = rest;
		int g = rest + blue / 4;
		int b = blue;
		int a = i * 255 / 3;
		if(r > 255) r = 255;
		if(g > 255) g = 255;
		if(b > 255) b = 255;
		pix[i * 4] = pptr[0] = r;
		pix[i * 4 + 1] = pptr[1] = g;
		pix[i * 4 + 2] = pptr[2] = b;
		pix[i * 4 + 3] = pptr[3] = a;
		pptr -= 4;
	}
	laser_tex = gaw_create_tex1d(GAW_BILINEAR);
	gaw_tex1d(GAW_RGBA, 8, GAW_RGBA, pix);


	if(!(uitex = tex_load("data/uibars.png"))) {
		return -1;
	}
	if(!(timertex = tex_load("data/timer.png"))) {
		return -1;
	}

	if(!(font_hp = dtx_open_font_glyphmap("data/hpfont.gmp"))) {
		fprintf(stderr, "failed to open glyphmap: data/hpfont.gmp\n");
		return -1;
	}
	font_hp_size = dtx_get_glyphmap_ptsize(dtx_get_glyphmap(font_hp, 0));
	if(!opt.gfx.blendui) {
		dtx_set(DTX_GL_BLEND, 0);
		dtx_set(DTX_GL_ALPHATEST, 128);
	}

	/*
	if((font_hp_tex = tex_load("data/hpfont-rgb.png"))) {
		dtxhack_replace_texture(dtx_get_glyphmap(font_hp, 0), font_hp_tex->texid);
	}
	*/

	if(!(font_timer = dtx_open_font_glyphmap("data/timefont.gmp"))) {
		fprintf(stderr, "failed to open glyphmap: data/timefont.gmp\n");
		return -1;
	}
	font_timer_size = dtx_get_glyphmap_ptsize(dtx_get_glyphmap(font_timer, 0));

	gen_geosphere(&adidome, 8, 1, 1);
	adidome.dlist = gaw_compile_begin();
	gaw_color3f(0.153, 0.153, 0.467);
	mesh_draw(&adidome);
	cgm_mrotation(matrix, cgm_deg_to_rad(180), 1, 0, 0);
	mesh_transform(&adidome, matrix);
	gaw_color3f(0.467, 0.467, 0.745);
	mesh_draw(&adidome);
	gaw_compile_end();

	if(!(tex_flare = tex_load("data/blspstar.png"))) {
		return -1;
	}
	if(!(tex_damage = tex_load("data/dmgvign.png"))) {
		return -1;
	}

	return 0;
}

static void gdestroy(void)
{
	gaw_destroy_tex(laser_tex);
	tex_free(tex_flare);
	tex_free(tex_damage);
}

static int gstart(void)
{
	char *env;

	if(win_height) {
		greshape(win_width, win_height);
	}

	/* start with 3 items: goat3d load, renderer init, load music,
	 * the level loader will add more as soon as it can count the textures
	 */
	loading_start(3);
	loading_update();

	lvl_init(&lvl);
	if(lvl_load(&lvl, "data/level1.lvl") == -1) {
		return -1;
	}

	loading_step();

	if(rendlvl_init(&lvl)) {
		return -1;
	}

	loading_step();

	player = malloc_nf(sizeof *player);
	init_player(player);
	player->lvl = &lvl;

	lvl_spawn_enemies(&lvl);

	if((env = getenv("START_ROOM"))) {
		struct room *sr = lvl_find_room(&lvl, env);
		if(sr) {
			cgm_vlerp(&player->pos, &sr->aabb.vmin, &sr->aabb.vmax, 0.5f);
		}
	} else {
		player->pos = lvl.startpos;
		player->rot = lvl.startrot;
	}

	if(opt.music) {
		if(!(mod = au_load_module("data/ingame.it"))) {
			fprintf(stderr, "failed to open music\n");
		} else {
			au_play_module(mod);
		}
	}

	loading_step();

	gaw_lighting_fast();
	gaw_fog_fast();

	gaw_enable(GAW_DEPTH_TEST);
	gaw_enable(GAW_CULL_FACE);
	gaw_depth_func(GAW_LEQUAL);

	gaw_enable(GAW_LIGHTING);
	gaw_enable(GAW_LIGHT0);
	gaw_enable(GAW_LIGHT1);
	gaw_enable(GAW_LIGHT2);
	gaw_light_color(0, 1, 1, 1, 1);
	gaw_light_color(1, 1, 1, 1, 0.6);
	gaw_light_color(2, 1, 1, 1, 0.5);

	gaw_clear_color(0, 0, 0, 1);
	gaw_fog_color(0, 0, 0);

	start_time = time_msec;
	return 0;
}

static void gstop(void)
{
	if(mod) {
		au_stop_module(mod);
		au_free_module(mod);
		mod = 0;
	}

	rendlvl_destroy();
	lvl_destroy(&lvl);
}

#define KB_MOVE_SPEED	0.4
#define KB_SPIN_SPEED	0.075

static void gupdate(void)
{
	int i, count, num_enemies;
	float viewproj[16];
	cgm_ray ray;
	struct enemy *mob;
	struct room *room;
	long time_left = TIME_LIMIT - (time_msec - start_time);
	long tm_min, tm_min_rem, tm_sec, tm_msec;

	if(player->hp <= 0.0f || time_left <= 0) {
		time_left = 0;
		gameover = 1;
	}

	num_enemies = 0;
	count = darr_size(lvl.enemies);
	for(i=0; i<count; i++) {
		if(lvl.enemies[i]->hp > 0.0f) {
			num_enemies++;
			break;
		}
	}
	if(num_enemies == 0) {
		victory = 1;
	}


	tm_min = time_left / 60000;
	tm_min_rem = time_left % 60000;
	tm_sec = tm_min_rem / 1000;
	tm_msec = tm_min_rem % 1000;
	sprintf(timertext, "%02ld:%02ld:%02ld", tm_min, tm_sec, tm_msec / 10);

	if(gameover || victory) goto end;

	if(inpstate & INP_MOVE_BITS) {
		if(inpstate & INP_FWD_BIT) {
			player->vel.z -= KB_MOVE_SPEED;
		}
		if(inpstate & INP_BACK_BIT) {
			player->vel.z += KB_MOVE_SPEED;
		}
		if(inpstate & INP_RIGHT_BIT) {
			player->vel.x += KB_MOVE_SPEED;
		}
		if(inpstate & INP_LEFT_BIT) {
			player->vel.x -= KB_MOVE_SPEED;
		}
		if(inpstate & INP_UP_BIT) {
			player->vel.y += KB_MOVE_SPEED;
		}
		if(inpstate & INP_DOWN_BIT) {
			player->vel.y -= KB_MOVE_SPEED;
		}
		if(inpstate & INP_LROLL_BIT) {
			player->roll -= KB_SPIN_SPEED;
		}
		if(inpstate & INP_RROLL_BIT) {
			player->roll += KB_SPIN_SPEED;
		}
	}

	update_player_sball(player);
	update_player(player);

	lasers = 0;
	if(inpstate & INP_FIRE_BIT) {
		if(player->sp > 0.0f) {
			static long last_laser_sfx;
			lasers = 1;
			if(time_msec - last_laser_sfx > 100) {
				au_play_sample(sfx_laser, 0);
				last_laser_sfx = time_msec;
			}
		}
	}
	if(inpstate & INP_FIRE2_BIT) {
		if(player->num_missiles > 0 && time_msec - player->last_missile_time > MISSILE_COOLDOWN) {
			cgm_vec3 up = {0, 1, 0};
			cgm_vec3 pos;
			cgm_quat rot;

			rot = player->rot;
			cgm_qinvert(&rot);
			cgm_vrotate_quat(&up, &rot);

			pos = player->pos;
			cgm_vadd_scaled(&pos, &player->fwd, COL_RADIUS);
			cgm_vsub(&pos, &up);

			if(lvl_spawn_missile(player->lvl, player->room, &pos, &player->fwd, &rot) != -1) {
				player->last_missile_time = time_msec;
				/* TODO au_play_sample(sfx_missile, 0); */
			}
		}
	}

	/* update enemies */
	count = darr_size(lvl.enemies);
	for(i=0; i<count; i++) {
		struct enemy *mob = lvl.enemies[i];
		if(mob->hp > 0.0f) {
			enemy_update(mob);
		}
	}

	if(lasers) {
		cgm_vec3 dir = player->fwd;

		cgm_vscale(&dir, lvl.maxdist);

		if(!lvl_collision(&lvl, player->room, &player->pos, &dir, &lasers_hit)) {
			lasers_hit.depth = -1.0f;
			ray.dir = dir;
		} else {
			ray.dir = player->fwd;
			cgm_vscale(&ray.dir, lasers_hit.depth);	/* don't test further than the walls */
		}

		ray.origin = player->pos;
		if((mob = lvl_check_enemy_hit(&lvl, player->room, &ray))) {
			enemy_damage(mob, LASER_DAMAGE);
		}
	}

	/* update missiles */
	room = player->room;
	if(room) {
		for(i=0; i<room->num_missiles; i++) {
			struct missile *mis = room->missiles[i];
			cgm_ray ray;

			ray.origin = mis->pos;
			ray.dir = mis->vel;

			if((mob = lvl_check_enemy_hit(&lvl, player->room, &ray))) {
				enemy_damage(mob, MISSILE_DAMAGE);
				lvl_despawn_missile(mis);
				add_explosion(&mis->pos, 1, time_msec);
			}
			if(ray_sphere(&ray, &player->pos, COL_RADIUS, 0)) {
				player_damage(player, MISSILE_DAMAGE);
				lvl_despawn_missile(mis);
				add_explosion(&mis->pos, 1, time_msec);
			}
			if(lvl_collision(&lvl, player->room, &mis->pos, &mis->vel, &missile_hit)) {
				lvl_despawn_missile(mis);
				add_explosion(&mis->pos, 1, time_msec);
			}

			cgm_vadd(&mis->pos, &mis->vel);
			calc_posrot_matrix(mis->matrix, &mis->pos, &mis->rot);
		}
	}

	player_view_matrix(player, view_mat);

	cgm_mcopy(viewproj, view_mat);
	cgm_mmul(viewproj, proj_mat);

end:

#ifdef DBG_FREEZEVIS
	if(!dbg_freezevis) {
#endif
		vispos = player->pos;
		visrot = player->rot;
		rendlvl_setup(player->room, &vispos, viewproj);
#ifdef DBG_FREEZEVIS
	} else {
		rendlvl_setup(0, &vispos, viewproj);
	}
#endif

	rendlvl_update();
}

static void gdisplay(void)
{
	static long prev_msec;
	static float tm_acc = TSTEP;
	long msec, upd_iter;

#ifdef DBG_FREEZEVIS
	if(dbg_freezevis) {
		gaw_clear(GAW_COLORBUF | GAW_DEPTHBUF);
	} else
#endif
	gaw_clear(GAW_DEPTHBUF);

	msec = game_getmsec();
	tm_acc += (float)(msec - prev_msec) / 1000.0f;
	prev_msec = msec;

	/* updating mouse input every frame feels more fluid */
	update_player_mouse(player);

	/* update all other game logic once per timestep */
	upd_iter = 16;
	while(tm_acc >= TSTEP && --upd_iter > 0) {
		gupdate();
		tm_acc -= TSTEP;
	}

	gaw_matrix_mode(GAW_MODELVIEW);
	gaw_load_matrix(view_mat);

	gaw_light_dir(0, -1, 1, 5);
	gaw_light_dir(1, 5, 0, 3);
	gaw_light_dir(2, -0.5, -2, -3);

	gaw_enable(GAW_FOG);

	render_level();

	gaw_disable(GAW_FOG);

	gaw_save();
	gaw_disable(GAW_LIGHTING);
	gaw_set_tex2d(0);

	if(lasers) {
		int i;
		float s = 1.0 + sin(time_msec / 100.0f) * 0.01;
		float sz = 0.18 + sin(time_msec / 50.0f) * 0.008;

		gaw_enable(GAW_BLEND);
		gaw_disable(GAW_DEPTH_TEST);

		if(lasers_hit.depth >= 0.0f) {
			gaw_blend_func(GAW_SRC_ALPHA, GAW_ONE_MINUS_SRC_ALPHA);
			gaw_set_tex2d(tex_flare->texid);
			draw_billboard(&lasers_hit.pos, 3, cgm_wvec(0.7, 0.8, 1, 1));
			gaw_set_tex2d(0);
		}

		gaw_blend_func(GAW_ONE, GAW_ONE);
		/*gaw_enable(GAW_ALPHA_TEST);
		gaw_alpha_func(GAW_GREATER, 0.5);*/
		gaw_set_tex1d(laser_tex);
		gaw_load_identity();
		gaw_color3f(1, 1, 1);
		gaw_begin(GAW_QUADS);
		for(i=0; i<2; i++) {
			float x = (i ? -1 : 1) * s;
			gaw_texcoord1f(0); gaw_vertex3f(x - sz, -1, -0.05);
			gaw_texcoord1f(1); gaw_vertex3f(x + sz, -1, -0.05);
			gaw_texcoord1f(1); gaw_vertex3f(x + sz, -1, -opt.gfx.drawdist);
			gaw_texcoord1f(0); gaw_vertex3f(x - sz, -1, -opt.gfx.drawdist);
		}
		gaw_end();
		gaw_set_tex1d(0);
		/*gaw_disable(GAW_ALPHA_TEST);*/
	}

#ifdef DBG_SHOW_COLPOLY
	if(dbg_hitpoly) {
		gaw_enable(GAW_BLEND);
		gaw_blend_func(GAW_ONE, GAW_ONE);

		gaw_begin(GAW_TRIANGLES);
		gaw_color3f(0.1, 0.1, 0.4);
		gaw_vertex3fv(&dbg_hitpoly->v[0].x);
		gaw_vertex3fv(&dbg_hitpoly->v[1].x);
		gaw_vertex3fv(&dbg_hitpoly->v[2].x);
		gaw_end();

		gaw_disable(GAW_BLEND);
	}
#endif

#ifdef DBG_FREEZEVIS
	if(dbg_freezevis) {
		gaw_begin(GAW_LINES);
		gaw_color3f(0, 1, 0);
		gaw_vertex3f(vispos.x - 100, vispos.y, vispos.z);
		gaw_vertex3f(vispos.x + 100, vispos.y, vispos.z);
		gaw_vertex3f(vispos.x, vispos.y - 100, vispos.z);
		gaw_vertex3f(vispos.x, vispos.y + 100, vispos.z);
		gaw_vertex3f(vispos.x, vispos.y, vispos.z - 100);
		gaw_vertex3f(vispos.x, vispos.y, vispos.z + 100);
		gaw_end();

		/*
		gaw_push_matrix();
		gaw_color3f(0.2, 0.2, 0.2);
		gaw_translate(vispos.x, vispos.y, vispos.z);
		glutSolidSphere(COL_RADIUS, 10, 5);
		gaw_pop_matrix();
		*/
	}
#endif

#ifdef DBG_SHOW_FRUST
	if(dbg_num_frust > 0) {
		if(dbg_frust_idx >= dbg_num_frust) {
			dbg_frust_idx = 0;
		}
		if(dbg_frust_idx >= 0) {
			draw_frustum(dbg_frust[dbg_frust_idx]);
		}
	}
#endif

	gaw_restore();

#ifdef DBG_FREEZEVIS
	if(!dbg_freezevis)
#endif
		draw_ui();
}

static const cgm_vec3 barcover[] = {
	{68,35, 0}, {48,13, 0},
	{84,35, 0.125}, {69,13, 0.125},
	{105,35, 0.25}, {95,23, 0.25},
	{129,35, 0.5}, {125,28, 0.5},
	{191,35, 1}, {186,28, 1}
};

static void draw_ui(void)
{
	int i, j;
	float yoffs, yscale;
	float xform[16], *ptr;
	float x, vwidth = win_aspect * 480.0f;
	float timer_xoffs = vwidth - 125;

	begin2d(480);

	if(opt.gfx.blendui) {
		gaw_enable(GAW_BLEND);
		gaw_blend_func(GAW_SRC_ALPHA, GAW_ONE_MINUS_SRC_ALPHA);
		gaw_alpha_func(GAW_GREATER, 0.01);
	} else {
		gaw_enable(GAW_ALPHA_TEST);
		gaw_alpha_func(GAW_GREATER, 0.25);
	}

	blit_tex(0, 0, uitex, 1);
	blit_tex(timer_xoffs, 0, timertex, 1);

	gaw_disable(GAW_BLEND);
	gaw_enable(GAW_ALPHA_TEST);
	gaw_set_tex2d(0);

	gaw_alpha_func(GAW_GREATER, (float)player->sp / MAX_SP);

	yoffs = 0.0f;
	yscale = 1.0f;
	for(j=0; j<2; j++) {
		gaw_begin(GAW_QUAD_STRIP);
		for(i=0; i<sizeof barcover / sizeof barcover[0]; i++) {
			gaw_color4f(0.075, 0.075, 0.149, barcover[i].z);
			gaw_vertex2f(barcover[i].x, barcover[i].y * yscale + yoffs);
		}
		gaw_end();
		yoffs = 71;
		yscale = -1;
		gaw_alpha_func(GAW_GREATER, player->hp / MAX_HP);
	}
	gaw_disable(GAW_ALPHA_TEST);

	dtx_use_font(font_hp, font_hp_size);
	gaw_push_matrix();
	gaw_translate(162, 26, 0);
	gaw_scale(0.4, -0.4, 0.4);
	gaw_color3f(0.008, 0.396, 0.678);
	dtx_printf("%d", (int)player->sp * 100 / MAX_SP);

	gaw_translate(0, -85, 0);
	gaw_color3f(0.725, 0.075, 0.173);
	dtx_printf("%d", (int)player->hp * 100 / MAX_HP);
	gaw_pop_matrix();

	/* draw timer */
	dtx_use_font(font_timer, font_timer_size);
	gaw_push_matrix();
	gaw_translate(timer_xoffs + 10.5, 24.5, 0);
	gaw_scale(0.3535, -0.3535, 0.3535);
	gaw_color3f(0.8, 0.8, 1);
	dtx_string(timertext);
	dtx_flush();
	gaw_pop_matrix();


	/* draw ADI */
	gaw_enable(GAW_CULL_FACE);
	gaw_push_matrix();
	gaw_translate(41, 36, 0);
	ptr = player->rotmat;
	for(i=0; i<4; i++) {
		for(j=0; j<4; j++) {
			xform[(j << 2) + i] = *ptr++;
		}
	}
	gaw_mult_matrix(xform);

	gaw_set_tex2d(0);
	gaw_draw_compiled(adidome.dlist);

	gaw_pop_matrix();

	/* crosshair */
	x = vwidth * 0.5f;
	gaw_begin(GAW_LINES);
	gaw_color3f(0.5, 0.8, 0.5);
	gaw_vertex2f(x - 6, 240);
	gaw_vertex2f(x - 2, 240);
	gaw_vertex2f(x + 2, 240);
	gaw_vertex2f(x + 6, 240);
	gaw_vertex2f(x, 240 - 6);
	gaw_vertex2f(x, 240 - 2);
	gaw_vertex2f(x, 240 + 6);
	gaw_vertex2f(x, 240 + 2);
	gaw_end();

	if(time_msec - player->last_dmg < DMG_OVERLAY_DUR) {
		gaw_disable(GAW_CULL_FACE);
		gaw_set_tex2d(tex_damage->texid);
		gaw_enable(GAW_BLEND);
		gaw_blend_func(GAW_SRC_ALPHA, GAW_ONE_MINUS_SRC_ALPHA);

		gaw_begin(GAW_QUADS);
		gaw_color3f(1, 1, 1);
		gaw_texcoord2f(0, 0); gaw_vertex2f(0, 0);
		gaw_texcoord2f(1, 0); gaw_vertex2f(vwidth, 0);
		gaw_texcoord2f(1, 1); gaw_vertex2f(vwidth, 480);
		gaw_texcoord2f(0, 1); gaw_vertex2f(0, 480);
		gaw_end();
	}

	if(gameover) {
		gaw_push_matrix();
		use_font(font_menu);
		gaw_translate(x - font_strwidth(font_menu, "GAME OVER!") / 2, 240, 0);
		gaw_scale(1, -1, 1);
		gaw_color3f(1, 1, 1);
		dtx_printf("GAME OVER!");
		gaw_pop_matrix();
	}

	if(victory) {
		gaw_push_matrix();
		use_font(font_menu);
		gaw_translate(x - font_strwidth(font_menu, "VICTORY!") / 2, 240, 0);
		gaw_scale(1, -1, 1);
		gaw_color3f(1, 1, 1);
		dtx_printf("VICTORY!");
		gaw_translate(-80, -50, 0);
		dtx_printf("SECRET: %s", player->items & ITEM_SECRET ? "FOUND" : "NOT FOUND");
		gaw_pop_matrix();
	}

	end2d();
}

static void greshape(int x, int y)
{
	float zfar = opt.gfx.drawdist;
#ifdef DBG_FREEZEVIS
	if(dbg_freezevis) {
		zfar = 500;
	}
#endif
	cgm_mperspective(proj_mat, cgm_deg_to_rad(60), win_aspect, 0.1, zfar);
	gaw_matrix_mode(GAW_PROJECTION);
	gaw_load_matrix(proj_mat);

	gaw_fog_linear(zfar * 0.75, zfar);
}

void dbg_getkey();

static void gkeyb(int key, int press)
{
	int i;

	for(i=0; i<MAX_INPUTS; i++) {
		if(inpmap[i].key == key) {
			if(press) {
				inpstate |= 1 << inpmap[i].inp;
			} else {
				inpstate &= ~(1 << inpmap[i].inp);
			}
			break;
		}
	}

	if(press) {
		switch(key) {
		case 27:
			game_chscr(&scr_menu);
			break;

		case '`':
			if(!fullscr) {
				game_grabmouse(-1);	/* toggle */
			}
			break;

		case GKEY_F5:
			dbg_getkey();
			break;

		case GKEY_F2:
			dbg_freezevis ^= 1;
			if(!dbg_freezevis) {
				player->pos = vispos;
				player->rot = visrot;
			}
			greshape(win_width, win_height);	/* to change the far clip */
			break;

		case GKEY_F1:
			printf("player: %g %g %g\n", player->pos.x, player->pos.y, player->pos.z);
			break;

#ifdef DBG_SHOW_FRUST
		case GKEY_F3:
			if(dbg_frust_idx >= 0) dbg_frust_idx--;
			printf("frustum: %d\n", dbg_frust_idx);
			break;
		case GKEY_F4:
			if(dbg_frust_idx < dbg_num_frust - 1) dbg_frust_idx++;
			printf("frustum: %d\n", dbg_frust_idx);
			break;
#endif
		}
	}
}

static void gmouse(int bn, int press, int x, int y)
{
	int i;

	for(i=0; i<MAX_INPUTS; i++) {
		if(inpmap[i].mbn == bn) {
			if(press) {
				inpstate |= 1 << inpmap[i].inp;
			} else {
				inpstate &= ~(1 << inpmap[i].inp);
			}
			break;
		}
	}
}

#define PIHALF	(M_PI / 2.0)

static void gmotion(int x, int y)
{
	int dx, dy;

	if(mouse_grabbed) {
		dx = x - win_width / 2;
		dy = y - win_height / 2;
	} else {
		dx = x - mouse_x;
		dy = y - mouse_y;
	}

	if(!(dx | dy)) return;

	if(mouse_state[0] || mouse_state[1] || mouse_grabbed) {
		player->mouse_input.x += dx;
		player->mouse_input.y += opt.inv_mouse_y ? -dy : dy;
	}
}

static void gsball_motion(int x, int y, int z)
{
	cgm_vcons(&player->sball_mov, x, y, -z);
}

static void gsball_rotate(int x, int y, int z)
{
	cgm_vcons(&player->sball_rot, -x, -y, z);
}

static void gsball_button(int bn, int press)
{
	printf("pos: %g %g %g\n", player->pos.x, player->pos.y, player->pos.z);
	printf("rot: %g %g %g %g\n", player->rot.x, player->rot.y, player->rot.z, player->rot.w);
}

#ifdef DBG_SHOW_FRUST
static void draw_frustum(const cgm_vec4 *frust)
{
	int i, j, k, tries;
	float u, v;
	cgm_vec3 up, pt, vdir, norm;
	static const float col[][4] = {
		{0.5, 0, 0}, {0.5, 0, 0.5}, {0, 0.5, 0}, {0, 0.5, 0.5}};

	for(i=0; i<4; i++) {
		gaw_pointsize(5);
		gaw_begin(GAW_LINES);
		gaw_color3fv(col[i]);
		for(j=0; j<128; j++) {
			tries = 0;
reject:
			if(tries++ >= 1024) break;
			vdir = *(cgm_vec3*)(frust + 4);
			norm = *(cgm_vec3*)(frust + i);

			cgm_vcross(&up, &vdir, &norm);
			cgm_vnormalize(&up);
			cgm_vcross(&vdir, &norm, &up);

			u = (float)rand() / (float)RAND_MAX * opt.gfx.drawdist * 2 - opt.gfx.drawdist;
			v = (float)rand() / (float)RAND_MAX * opt.gfx.drawdist * 2 - opt.gfx.drawdist;

			pt.x = up.x * u + vdir.x * v;
			pt.y = up.y * u + vdir.y * v;
			pt.z = up.z * u + vdir.z * v;

			if(cgm_vdot(&pt, &vdir) < 0.0f) {
				cgm_vcons(&pt, -pt.x, -pt.y, -pt.z);
			}
			cgm_vadd(&pt, &vispos);

			for(k=0; k<6; k++) {
				if(i == k) continue;
				if(plane_point_sdist(frust + k, &pt) < 0) goto reject;
			}

			gaw_vertex3f(vispos.x, vispos.y, vispos.z);
			gaw_vertex3f(pt.x, pt.y, pt.z);
		}
		gaw_end();
	}
}
#endif
