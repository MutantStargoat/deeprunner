#include "config.h"

#include "opengl.h"
#include "miniglut.h"
#include "game.h"
#include "rendlvl.h"
#include "darray.h"
#include "enemy.h"
#include "gfxutil.h"

static struct level *lvl;
static struct room *cur_room;
static cgm_vec4 frust[6];	/* frustum planes */
static cgm_vec3 view_pos;

#ifdef DBG_SHOW_FRUST
#define MAX_FRUST	32
cgm_vec4 dbg_frust[MAX_FRUST][6];
int dbg_num_frust;
#endif

static unsigned int updateno;

#ifdef DBG_SHOW_CUR_ROOM
static int dbg_cur_room;
static const float red[] = {1, 0.5, 0.5, 1};
#endif
#ifdef DBG_FREEZEVIS
extern int dbg_freezevis;
#endif

static struct texture *tex_shield, *tex_expl;

static int portal_frustum_test(struct portal *portal, const cgm_vec4 *frust);
static void reduce_frustum(cgm_vec4 *np, const cgm_vec4 *p, const struct portal *portal);


int rendlvl_init(struct level *level)
{
	int i, j, nrooms, nmeshes;
	struct room *room;

	lvl = level;

	nrooms = darr_size(lvl->rooms);
	for(i=0; i<nrooms; i++) {
		room = lvl->rooms[i];

		nmeshes = darr_size(room->meshes);
		for(j=0; j<nmeshes; j++) {
			if(!room->meshes[j].dlist) {
				mesh_compile(room->meshes + j);
			}
		}
	}

	nmeshes = darr_size(level->dynmeshes);
	for(i=0; i<nmeshes; i++) {
		if(!level->dynmeshes[i]->dlist) {
			mesh_compile(level->dynmeshes[i]);
		}
	}

	if(!(tex_shield = tex_load("data/ring.png"))) {
		return -1;
	}
	if(!(tex_expl = tex_load("data/explanim.png"))) {
		return -1;
	}


	return 0;
}

void rendlvl_destroy(void)
{
	tex_free(tex_shield);
	tex_free(tex_expl);
}

void rendlvl_setup(struct room *room, const cgm_vec3 *ppos, float *vp_matrix)
{
	int i;

	if(!room) {
		room = lvl_room_at(lvl, ppos->x, ppos->y, ppos->z);
	}
	cur_room = room;
	view_pos = *ppos;

	for(i=0; i<6; i++) {
		cgm_mget_frustum_plane(vp_matrix, i, frust + i);
		cgm_normalize_plane(frust + i);
	}
}

static void update_room(struct room *room, const cgm_vec4 *frust)
{
	static float tm;
	int i, nmeshes, nobj, nportals;
	cgm_vec4 newfrust[6];

	tm += TSTEP;

	nmeshes = darr_size(room->meshes);
	for(i=0; i<nmeshes; i++) {
		struct mesh *m = room->meshes + i;

		if(m->mtl.uvanim) {
			m->mtl.uvoffs.x += m->mtl.texvel.x;
			m->mtl.uvoffs.y += m->mtl.texvel.y;
		}
	}

	/* update dynamic objects */
	nobj = darr_size(room->objects);
	for(i=0; i<nobj; i++) {
		struct object *obj = room->objects[i];

		if(obj->anim_rot) {
			cgm_qrotation(&obj->rot, tm, obj->rotaxis.x, obj->rotaxis.y, obj->rotaxis.z);
		}

		calc_posrot_matrix(obj->matrix, &obj->pos, &obj->rot);

		cgm_mcopy(obj->invmatrix, obj->matrix);
		cgm_minverse(obj->invmatrix);
	}

#if !defined(DBG_ONLY_CUR_ROOM) && !defined(DBG_ALL_ROOMS)
	/* recursively visit all rooms reachable through visible portals */
	room->vis_frm = updateno;

#ifdef DBG_SHOW_FRUST
	memcpy(dbg_frust[dbg_num_frust++], frust, 6 * sizeof(cgm_vec4));
#endif

	nportals = darr_size(room->portals);
	for(i=0; i<nportals; i++) {
		struct portal *portal = room->portals + i;

		if(!portal->link) continue;	/* unlinked portals */

		if(portal_frustum_test(portal, frust) && portal->link->vis_frm != updateno) {
			reduce_frustum(newfrust, frust, portal);
			update_room(portal->link, newfrust);
		}
	}
#endif
}

void rendlvl_update(void)
{
#ifdef DBG_ONLY_CUR_ROOM
	if(cur_room) update_room(cur_room, frust);
#elif defined(DBG_ALL_ROOMS)
	int i, nrooms;
	struct room *room;

	nrooms = darr_size(lvl->rooms);
	for(i=0; i<nrooms; i++) {
		room = lvl->rooms[i];

		update_room(room, frust);
	}
#else

#ifdef DBG_FREEZEVIS
	if(dbg_freezevis) return;
#endif

#ifdef DBG_SHOW_FRUST
	dbg_num_frust = 0;
#endif

	updateno++;
	if(cur_room) update_room(cur_room, frust);
#endif
}


static void render_room(struct room *room)
{
	int i, nmeshes, nportals, nobj;
	struct enemy *mob;

	nmeshes = darr_size(room->meshes);
	for(i=0; i<nmeshes; i++) {
		render_level_mesh(room->meshes + i);
	}

	/* render dynamic objects */
	nobj = darr_size(room->objects);
	for(i=0; i<nobj; i++) {
		if(room->objects[i]->mesh) {
			render_dynobj(room->objects[i]);
		}
	}

	/* render enemies */
	nobj = darr_size(room->enemies);
	for(i=0; i<nobj; i++) {
		mob = room->enemies[i];
		if(mob->hp > 0.0f) {
			render_enemy(mob);
		}
	}

	/* render missiles */
	for(i=0; i<room->num_missiles; i++) {
		render_missile(room->missiles[i]);
	}

	/* mark this room as visited in the current frame */
	room->rendered = 1;

	nportals = darr_size(room->portals);
	for(i=0; i<nportals; i++) {
#if !defined(DBG_ONLY_CUR_ROOM) && !defined(DBG_ALL_ROOMS)
		/* recursively draw visible rooms */
		struct room *link = room->portals[i].link;
		/* skip unlinked and those rendered this frame */
		if(!link || link->rendered) continue;

		/* recurse if the next room is visible in the current frame */
		if(link->vis_frm == updateno) {
			render_room(link);
		}
#endif
#ifdef DBG_SHOW_PORTALS
		glPushAttrib(GL_ENABLE_BIT);
		glDisable(GL_LIGHTING);
		glDisable(GL_TEXTURE_2D);
		glPushMatrix();
		glTranslatef(room->portals[i].pos.x, room->portals[i].pos.y, room->portals[i].pos.z);
		glutWireSphere(room->portals[i].rad, 10, 5);
		glPopMatrix();
		glPopAttrib();
#endif
	}
}

void render_level(void)
{
#ifdef DBG_ONLY_CUR_ROOM
	render_room(cur_room);
#else
	int i, nrooms;
	struct room *room;

	nrooms = darr_size(lvl->rooms);
	for(i=0; i<nrooms; i++) {
		room = lvl->rooms[i];

#ifdef DBG_SHOW_CUR_ROOM
		dbg_cur_room = room == cur_room;
#endif

#ifdef DBG_ALL_ROOMS
		render_room(room);
#else
		room->rendered = 0;
#endif
	}

#ifndef DBG_ALL_ROOMS
	/* render only the current room, and those linked by visible portals */
	render_room(cur_room);
#endif	/* !def DBG_ALL_ROOMS */
#endif	/* else of DBG_ONLY_CUR_ROOM */

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
}

void render_level_mesh(struct mesh *mesh)
{
	int pass, more;

	/* TODO handle multipass logic here */
	pass = 0;
	for(;;) {
		more = mtl_apply(&mesh->mtl, pass++);

#ifdef DBG_SHOW_CUR_ROOM
		if(dbg_cur_room) {
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, red);
		}
#endif

		glCallList(mesh->dlist);

		if(more) {
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		} else {
			if(pass > 0) glDisable(GL_BLEND);
			break;
		}
	}

	mtl_end();
}

void render_dynobj(struct object *obj)
{
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glMultMatrixf(obj->matrix);

	render_level_mesh(obj->mesh);

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

void render_enemy(struct enemy *mob)
{
	long expl_time;

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glMultMatrixf(mob->matrix);

	render_level_mesh(mob->mesh);

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	if((expl_time = time_msec - mob->last_dmg_hit) < EXPL_DUR) {
		int frm = expl_time / EXPL_FRAME_DUR;
		float tx = (float)frm / NUM_EXPL_FRAMES;

		glPushAttrib(GL_ENABLE_BIT);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_LIGHTING);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, tex_expl->texid);

		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glTranslatef(tx, 0, 0);
		glScalef(1.0 / NUM_EXPL_FRAMES, 1, 1);

		draw_billboard(&mob->last_hit_pos, mob->rad * 0.5, cgm_wvec(1, 1, 1, 1));

		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);

		glPopAttrib();

	} else if(time_msec - mob->last_shield_hit < SHIELD_OVERLAY_DUR) {
		glPushAttrib(GL_ENABLE_BIT);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_LIGHTING);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, tex_shield->texid);
		draw_billboard(&mob->pos, mob->rad, cgm_wvec(0.2, 0.4, 1, 0.8));

		glPopAttrib();
	}
}

void render_missile(struct missile *mis)
{
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glMultMatrixf(mis->matrix);

	render_level_mesh(mis->mesh);

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

static int portal_frustum_test(struct portal *portal, const cgm_vec4 *frust)
{
	int i;

	for(i=0; i<6; i++) {
		if(plane_point_sdist(frust + i, &portal->pos) < -portal->rad) {
			return 0;
		}
	}
	return 1;
}

static void reduce_frustum(cgm_vec4 *np, const cgm_vec4 *p, const struct portal *portal)
{
	int i;
	cgm_vec3 pt, ptdir, n, axis;
	const cgm_vec3 *norm;
	/*static const cgm_vec3 axis[] = {{0, -1, 0}, {0, 1, 0}, {1, 0, 0}, {-1, 0, 0}};*/

	/* near and far planes stay the same */
	np[4] = p[4];
	np[5] = p[5];

	for(i=0; i<4; i++) {
		if(plane_point_sdist(p + i, &portal->pos) <= portal->rad) {
			/* portal already touches this plane, no need to reduce */
			np[i] = p[i];
			continue;
		}

		/* find point on the sphere nearest the plane */
		norm = (const cgm_vec3*)(p + i);
		pt = portal->pos; cgm_vsub_scaled(&pt, norm, portal->rad);

		ptdir = pt; cgm_vsub(&ptdir, &view_pos);
		cgm_vnormalize(&ptdir);

		/* construct plane passing through the viewpos and that point */
		cgm_vcross(&axis, &ptdir, norm);
		cgm_vcross(&n, &axis, &ptdir);

		cgm_wcons(np + i, n.x, n.y, n.z, -cgm_vdot(&pt, &n));
		cgm_normalize_plane(np + i);
	}
}
