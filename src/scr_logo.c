#include "miniglut.h"
#include "opengl.h"
#include "game.h"
#include "mtltex.h"
#include "gfxutil.h"
#include "mesh.h"
#include "drawtext.h"
#include "goat3d.h"

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

	glClearColor(0, 0, 0, 1);

	glDisable(GL_LIGHTING);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	if(mesh_load(&mesh_logo, "data/msglogo.g3d", 0) == -1) {
		return 0;
	}
	cgm_mrotation_x(matrix, cgm_deg_to_rad(90));
	mesh_transform(&mesh_logo, matrix);

	if(mesh_load(&mesh_sgi, "data/sgilogo.g3d", "sgilogo") == -1) {
		return 0;
	}
	mesh_compile(&mesh_sgi);

	if(!(tex_o2boot = tex_load("data/o2boot.tga"))) {
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

	glClear(GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0, 0, -10);

	glColor3f(1, 1, 1);
	mesh_draw(&mesh_logo);

	begin2d(480);

	x = (vwidth - TEXTW) / 2.0f;
	y = 360;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBindTexture(GL_TEXTURE_2D, tex_msg->texid);
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); glVertex2f(x, y);
	glTexCoord2f(1, 0); glVertex2f(x + TEXTW, y);
	glTexCoord2f(1, 1); glVertex2f(x + TEXTW, y + TEXTH);
	glTexCoord2f(0, 1); glVertex2f(x, y + TEXTH);
	glEnd();

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
	float trot, t, alpha, z;
	float tsec = (tmsec - SGISTART) / 1000.0f;

	glDisable(GL_DEPTH_TEST);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	glBegin(GL_QUADS);
	glColor3ub(95, 95, 143);
	glVertex2f(-1, -1);
	glVertex2f(1, -1);
	glColor3ub(175, 209, 254);
	glVertex2f(1, 1);
	glVertex2f(-1, 1);
	glEnd();

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex_o2boot->texid);
	glBegin(GL_QUADS);
	glColor3f(1, 1, 1);
	glTexCoord2f(0, 1); glVertex2f(-aspect, -1);
	glTexCoord2f(1, 1); glVertex2f(aspect, -1);
	glTexCoord2f(1, 0); glVertex2f(aspect, MAXV * 2.0 - 1.0);
	glTexCoord2f(0, 0); glVertex2f(-aspect, MAXV * 2.0 - 1.0);
	glEnd();

	alpha = clamp(tsec - 5.5, 0, 1);
	glTranslatef(0, 0.34, 0);
	glScalef(WAYSCALE / win_aspect, WAYSCALE, 1);
	glBindTexture(GL_TEXTURE_2D, tex_way->texid);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_QUADS);
	glColor4f(1, 1, 1, alpha);
	glTexCoord2f(0, 1); glVertex2f(-1, -1);
	glTexCoord2f(1, 1); glVertex2f(1, -1);
	glTexCoord2f(1, 0); glVertex2f(1, 1);
	glTexCoord2f(0, 0); glVertex2f(-1, 1);
	glEnd();

	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);

	glPopMatrix();

	glClear(GL_DEPTH_BUFFER_BIT);

	/* SGI logo */
	glPushAttrib(GL_ENABLE_BIT | GL_VIEWPORT_BIT);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_CULL_FACE);

	trot = cgm_smoothstep(0, 6, tsec) * 360 * 2.0f;
	//z = cgm_logerp(100, 50, clamp(tsec - 1.0f, 0, 1));
	z = cgm_lerp(50, 32, clamp(tsec - 1.0f, 0, 1));
	//z = 100.0f - cgm_smoothstep(-5, 5, tsec - 1.0f) * 50.0;
	alpha = clamp((tsec / 2) - 0.5, 0, 1);

	glViewport(win_width / 4, win_height * 0.43, win_width / 2, win_height / 2);
	glMatrixMode(GL_MODELVIEW);
	glTranslatef(0, 0, -z);

	glTranslatef(0, 0, 10);
	glRotatef(trot, 0, 1, 0);
	glRotatef(35, 1, 0, 0);
	glRotatef(-45, 0, 1, 0);

	glColor4f(1, 1, 1, alpha);
	set_mtl_diffuse(0.1, 0.1, 0.1, alpha);
	set_mtl_specular(1, 1, 1, 60.0f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	if(alpha < 0.8) {
		glDepthMask(0);
	}

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex_env->texid);
	texenv_sphmap(1);

	glCallList(mesh_sgi.dlist);

	glPopAttrib();
	glDepthMask(1);
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

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();

		glPushAttrib(GL_ENABLE_BIT);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_TEXTURE_2D);
		glColor4f(0, 0, 0, alpha);
		glRectf(-1, -1, 1, 1);

		glPopAttrib();

		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
	}
}

static void logo_reshape(int x, int y)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(50, win_aspect, 0.5, 100.0);
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
