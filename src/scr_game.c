#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "opengl.h"
#include "imago2.h"
#include "miniglut.h"
#include "game.h"
#include "util.h"
#include "level.h"
#include "input.h"
#include "player.h"
#include "cgmath/cgmath.h"
#include "audio.h"
#include "options.h"
#include "player.h"
#include "rendlvl.h"
#include "gfxutil.h"
#include "drawtext.h"

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

static void set_light_dir(int idx, float x, float y, float z);
static void set_light_color(int idx, float r, float g, float b, float s);

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

static struct player player;

static struct level lvl;
static unsigned int dbgtex;
static struct texture *uitex;
static struct mesh adidome;

static struct dtx_font *font_hp;
static int font_hp_size;
static struct texture *font_hp_tex;

extern struct dtx_font *font_menu;
extern int font_menu_sz;

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
static int laser_dlist;
static unsigned int laser_tex;

static int ginit(void)
{
	int i;

	lvl_init(&lvl);
	if(lvl_load(&lvl, "data/level1.lvl") == -1) {
		return -1;
	}

	{
		int j;
		unsigned char *pix, *pptr;

		pptr = pix = malloc(256 * 256);

		for(i=0; i<256; i++) {
			for(j=0; j<256; j++) {
				*pptr++ = (i ^ j) | 0x80;
			}
		}

		glGenTextures(1, &dbgtex);
		glBindTexture(GL_TEXTURE_2D, dbgtex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		gluBuild2DMipmaps(GL_TEXTURE_2D, 1, 256, 256, GL_LUMINANCE, GL_UNSIGNED_BYTE, pix);

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
		glGenTextures(1, &laser_tex);
		glBindTexture(GL_TEXTURE_1D, laser_tex);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, pix);

		free(pix);
	}

	if(!(uitex = tex_load("data/uibars.png"))) {
		return -1;
	}

	if(!(font_hp = dtx_open_font_glyphmap("data/hpfont.gmp"))) {
		fprintf(stderr, "failed to open glyphmap: data/impact14.gmp\n");
		return -1;
	}
	font_hp_size = dtx_get_glyphmap_ptsize(dtx_get_glyphmap(font_hp, 0));
	if(!opt.gfx.blendui) {
		dtx_set(DTX_GL_BLEND, 0);
		dtx_set(DTX_GL_ALPHATEST, 128);
	}

	if((font_hp_tex = tex_load("data/hpfont-rgb.png"))) {
		dtxhack_replace_texture(dtx_get_glyphmap(font_hp, 0), font_hp_tex->texid);
		printf("replacement texture: %d\n", font_hp_tex->texid);
	}

	gen_geosphere(&adidome, 8, 1, 1);
	adidome.dlist = glGenLists(1);
	glNewList(adidome.dlist, GL_COMPILE);
	glColor3f(0.153, 0.153, 0.467);
	mesh_draw(&adidome);
	glRotatef(180, 1, 0, 0);
	glColor3f(0.467, 0.467, 0.745);
	mesh_draw(&adidome);
	glEndList();

	laser_dlist = glGenLists(1);
	glNewList(laser_dlist, GL_COMPILE);
	glBegin(GL_QUADS);
	for(i=0; i<2; i++) {
		float x = i ? -1 : 1;
		glTexCoord1f(0); glVertex3f(x - 0.25, -1, 0);
		glTexCoord1f(1); glVertex3f(x + 0.25, -1, 0);
		glTexCoord1f(1); glVertex3f(x + 0.25, -1, -200);
		glTexCoord1f(0); glVertex3f(x - 0.25, -1, -200);
	}
	glEnd();
	glEndList();

	return 0;
}

static void gdestroy(void)
{
	glDeleteTextures(1, &laser_tex);
	lvl_destroy(&lvl);
}

static int gstart(void)
{
	float amb[] = {0.25, 0.25, 0.25, 1};
	float zero[] = {0, 0, 0, 1};

	if(win_height) {
		greshape(win_width, win_height);
	}

	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb);
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 0);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);

	glHint(GL_FOG_HINT, GL_FASTEST);

	glDepthFunc(GL_LEQUAL);

	glEnable(GL_LIGHTING);
	set_light_color(0, 1, 1, 1, 1);
	glEnable(GL_LIGHT0);
	set_light_color(1, 1, 1, 1, 0.6);
	glEnable(GL_LIGHT1);
	set_light_color(2, 1, 1, 1, 0.5);
	glEnable(GL_LIGHT2);

	glClearColor(0, 0, 0, 1);
	glFogfv(GL_FOG_COLOR, zero);

	if(rendlvl_init(&lvl)) {
		return -1;
	}

	init_player(&player);
	player.pos = lvl.startpos;
	player.rot = lvl.startrot;
	player.lvl = &lvl;

	if(opt.music) {
		if(!(mod = au_load_module("data/ingame.it"))) {
			fprintf(stderr, "failed to open music\n");
		} else {
			au_play_module(mod);
		}
	}
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
}

#define KB_MOVE_SPEED	0.4
#define KB_SPIN_SPEED	0.075

static void gupdate(void)
{
	float viewproj[16];

	if(inpstate & INP_MOVE_BITS) {
		if(inpstate & INP_FWD_BIT) {
			player.vel.z -= KB_MOVE_SPEED;
		}
		if(inpstate & INP_BACK_BIT) {
			player.vel.z += KB_MOVE_SPEED;
		}
		if(inpstate & INP_RIGHT_BIT) {
			player.vel.x += KB_MOVE_SPEED;
		}
		if(inpstate & INP_LEFT_BIT) {
			player.vel.x -= KB_MOVE_SPEED;
		}
		if(inpstate & INP_UP_BIT) {
			player.vel.y += KB_MOVE_SPEED;
		}
		if(inpstate & INP_DOWN_BIT) {
			player.vel.y -= KB_MOVE_SPEED;
		}
		if(inpstate & INP_LROLL_BIT) {
			player.roll -= KB_SPIN_SPEED;
		}
		if(inpstate & INP_RROLL_BIT) {
			player.roll += KB_SPIN_SPEED;
		}
	}

	lasers = 0;
	if(inpstate & INP_FIRE_BIT) {
		if(player.hp >= 0.0f && player.sp >= 0.0f) {
			lasers = 1;
		}
	}

	if(player.hp > 0) {
		update_player_sball(&player);
		update_player(&player);
	}

	player_view_matrix(&player, view_mat);

	cgm_mcopy(viewproj, view_mat);
	cgm_mmul(viewproj, proj_mat);

#ifdef DBG_FREEZEVIS
	if(!dbg_freezevis) {
#endif
		vispos = player.pos;
		visrot = player.rot;
		rendlvl_setup(player.room, &vispos, viewproj);
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
	long msec;

	glClear(GL_DEPTH_BUFFER_BIT);

	msec = glutGet(GLUT_ELAPSED_TIME);
	tm_acc += (float)(msec - prev_msec) / 1000.0f;
	prev_msec = msec;

	/* updating mouse input every frame feels more fluid */
	update_player_mouse(&player);

	/* update all other game logic once per timestep */
	while(tm_acc >= TSTEP) {
		gupdate();
		tm_acc -= TSTEP;
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(view_mat);

	set_light_dir(0, -1, 1, 5);
	set_light_dir(1, 5, 0, 3);
	set_light_dir(2, -0.5, -2, -3);

	glEnable(GL_FOG);

	render_level();
	/* TODO: render non-static meshes */

	glDisable(GL_FOG);

	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);

	if(lasers) {
		int i;
		float s = 1.0 + sin(time_msec / 100.0f) * 0.01;
		float sz = 0.18 + sin(time_msec / 50.0f) * 0.008;
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		/*glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.5);*/
		glEnable(GL_TEXTURE_1D);
		glBindTexture(GL_TEXTURE_1D, laser_tex);
		glLoadIdentity();
		glColor3f(1, 1, 1);
		glBegin(GL_QUADS);
		for(i=0; i<2; i++) {
			float x = (i ? -1 : 1) * s;
			glTexCoord1f(0); glVertex3f(x - sz, -1, 0);
			glTexCoord1f(1); glVertex3f(x + sz, -1, 0);
			glTexCoord1f(1); glVertex3f(x + sz, -1, -400);
			glTexCoord1f(0); glVertex3f(x - sz, -1, -400);
		}
		glEnd();
		glDisable(GL_TEXTURE_1D);
		/*glDisable(GL_ALPHA_TEST);*/
	}

#ifdef DBG_SHOW_COLPOLY
	if(dbg_hitpoly) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);

		glBegin(GL_TRIANGLES);
		glColor3f(0.1, 0.1, 0.4);
		glVertex3fv(&dbg_hitpoly->v[0].x);
		glVertex3fv(&dbg_hitpoly->v[1].x);
		glVertex3fv(&dbg_hitpoly->v[2].x);
		glEnd();

		glDisable(GL_BLEND);
	}
#endif

#ifdef DBG_FREEZEVIS
	if(dbg_freezevis) {
		glBegin(GL_LINES);
		glColor3f(0, 1, 0);
		glVertex3f(vispos.x - 100, vispos.y, vispos.z);
		glVertex3f(vispos.x + 100, vispos.y, vispos.z);
		glVertex3f(vispos.x, vispos.y - 100, vispos.z);
		glVertex3f(vispos.x, vispos.y + 100, vispos.z);
		glVertex3f(vispos.x, vispos.y, vispos.z - 100);
		glVertex3f(vispos.x, vispos.y, vispos.z + 100);
		glEnd();

		glPushMatrix();
		glColor3f(0.2, 0.2, 0.2);
		glTranslatef(vispos.x, vispos.y, vispos.z);
		glutSolidSphere(COL_RADIUS, 10, 5);
		glPopMatrix();
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

	glPopAttrib();

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

	begin2d(480);

	if(opt.gfx.blendui) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glAlphaFunc(GL_GREATER, 0.01);
	} else {
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.25);
	}

	blit_tex(0, 0, uitex, 1);

	glDisable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_2D);

	glAlphaFunc(GL_GREATER, (float)player.sp / MAX_SP);

	yoffs = 0.0f;
	yscale = 1.0f;
	for(j=0; j<2; j++) {
		glBegin(GL_QUAD_STRIP);
		for(i=0; i<sizeof barcover / sizeof barcover[0]; i++) {
			glColor4f(0.075, 0.075, 0.149, barcover[i].z);
			glVertex2f(barcover[i].x, barcover[i].y * yscale + yoffs);
		}
		glEnd();
		yoffs = 71;
		yscale = -1;
		glAlphaFunc(GL_GREATER, player.hp / MAX_HP);
	}
	glDisable(GL_ALPHA_TEST);

	dtx_use_font(font_hp, font_hp_size);
	glPushMatrix();
	glTranslatef(162, 26, 0);
	glScalef(0.4, -0.4, 0.4);
	glColor3f(0.008, 0.396, 0.678);
	dtx_printf("%d", (int)player.sp * 100 / MAX_SP);

	glTranslatef(0, -85, 0);
	glColor3f(0.725, 0.075, 0.173);
	dtx_printf("%d", (int)player.hp * 100 / MAX_HP);
	glPopMatrix();

	/* draw ADI */
	glEnable(GL_CULL_FACE);
	glPushMatrix();
	glTranslatef(41, 36, 0);
	ptr = player.rotmat;
	for(i=0; i<4; i++) {
		for(j=0; j<4; j++) {
			xform[(j << 2) + i] = *ptr++;
		}
	}
	glMultMatrixf(xform);

	glDisable(GL_TEXTURE_2D);
	glCallList(adidome.dlist);

	glPopMatrix();

	/* crosshair */
	x = vwidth * 0.5f;
	glBegin(GL_LINES);
	glColor3f(0.5, 0.8, 0.5);
	glVertex2f(x - 6, 240);
	glVertex2f(x - 2, 240);
	glVertex2f(x + 2, 240);
	glVertex2f(x + 6, 240);
	glVertex2f(x, 240 - 6);
	glVertex2f(x, 240 - 2);
	glVertex2f(x, 240 + 6);
	glVertex2f(x, 240 + 2);
	glEnd();

	if(player.hp <= 0.0f) {
		glPushMatrix();
		dtx_use_font(font_menu, font_menu_sz);
		glTranslatef(x - dtx_string_width("GAME OVER!") / 2, 240, 0);
		glScalef(1, -1, 1);
		glColor3f(1, 1, 1);
		dtx_printf("GAME OVER!");
		glPopMatrix();
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
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(proj_mat);

	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogf(GL_FOG_START, zfar * 0.75);
	glFogf(GL_FOG_END, zfar);
}

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

		case GKEY_F2:
			dbg_freezevis ^= 1;
			if(!dbg_freezevis) {
				player.pos = vispos;
				player.rot = visrot;
			}
			greshape(win_width, win_height);	/* to change the far clip */
			break;

		case ';':
			player.hp -= 8;
			if(player.hp < 0) player.hp = 0;
			break;
		case '\'':
			player.hp += 8;
			if(player.hp > MAX_HP) player.hp = MAX_HP;
			break;
		case '[':
			player.sp -= 8;
			if(player.sp < 0) player.sp = 0;
			break;
		case ']':
			player.sp += 8;
			if(player.sp > MAX_SP) player.sp = MAX_SP;
			break;

		case GKEY_F1:
			printf("player: %g %g %g\n", player.pos.x, player.pos.y, player.pos.z);
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

	if(mouse_state[0] || mouse_grabbed) {
		player.mouse_input.x += dx;
		player.mouse_input.y += opt.inv_mouse_y ? -dy : dy;
	}
}

static void gsball_motion(int x, int y, int z)
{
	cgm_vcons(&player.sball_mov, x, y, -z);
}

static void gsball_rotate(int x, int y, int z)
{
	cgm_vcons(&player.sball_rot, -x, -y, z);
}

static void gsball_button(int bn, int press)
{
	printf("pos: %g %g %g\n", player.pos.x, player.pos.y, player.pos.z);
	printf("rot: %g %g %g %g\n", player.rot.x, player.rot.y, player.rot.z, player.rot.w);
}

static void set_light_dir(int idx, float x, float y, float z)
{
	float pos[4];
	pos[0] = x;
	pos[1] = y;
	pos[2] = z;
	pos[3] = 0;
	glLightfv(GL_LIGHT0 + idx, GL_POSITION, pos);
}

static void set_light_color(int idx, float r, float g, float b, float s)
{
	float color[4];
	color[0] = r * s;
	color[1] = g * s;
	color[2] = b * s;
	color[3] = 1;
	glLightfv(GL_LIGHT0 + idx, GL_DIFFUSE, color);
	glLightfv(GL_LIGHT0 + idx, GL_SPECULAR, color);
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
		glPointSize(5);
		glBegin(GL_LINES);
		glColor3fv(col[i]);
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

			glVertex3f(vispos.x, vispos.y, vispos.z);
			glVertex3f(pt.x, pt.y, pt.z);
		}
		glEnd();
	}
}
#endif

