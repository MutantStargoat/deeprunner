#include "config.h"

#include "opengl.h"
#include "miniglut.h"
#include "rendlvl.h"
#include "darray.h"

static struct level *lvl;
static struct room *cur_room;
static cgm_vec4 frust[6];	/* frustum planes */

static unsigned int updateno;

#ifdef DBG_SHOW_CUR_ROOM
static int dbg_cur_room;
static const float red[] = {1, 0.5, 0.5, 1};
#endif
#ifdef DBG_FREEZEVIS
extern int dbg_freezevis;
#endif

static int portal_frustum_test(struct portal *portal);


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
	return 0;
}

void rendlvl_destroy(void)
{
}

void rendlvl_setup(struct room *room, const cgm_vec3 *ppos, float *view_matrix)
{
	int i;

	if(!room) {
		room = lvl_room_at(lvl, ppos->x, ppos->y, ppos->z);
	}
	cur_room = room;

	for(i=0; i<6; i++) {
		cgm_mget_frustum_plane(view_matrix, i, frust + i);
	}
}

static void update_room(struct room *room)
{
	int i, nmeshes, nportals;

	nmeshes = darr_size(room->meshes);
	for(i=0; i<nmeshes; i++) {
		struct mesh *m = room->meshes + i;

		if(m->mtl.uvanim) {
			m->mtl.uvoffs.x += m->mtl.texvel.x;
			m->mtl.uvoffs.y += m->mtl.texvel.y;
		}
	}

#if !defined(DBG_ONLY_CUR_ROOM) && !defined(DBG_ALL_ROOMS)
	/* recursively visit all rooms reachable through visible portals */
	room->vis_frm = updateno;

	nportals = darr_size(room->portals);
	for(i=0; i<nportals; i++) {
		struct portal *portal = room->portals + i;

		if(!portal->link) continue;	/* unlinked portals */

		if(portal_frustum_test(portal) && portal->link->vis_frm != updateno) {
			update_room(portal->link);
		}
	}
#endif
}

void rendlvl_update(void)
{
#ifdef DBG_ONLY_CUR_ROOM
	if(cur_room) update_room(cur_room);
#elif defined(DBG_ALL_ROOMS)
	int i, nrooms;
	struct room *room;

	nrooms = darr_size(lvl->rooms);
	for(i=0; i<nrooms; i++) {
		room = lvl->rooms[i];

		update_room(room);
	}
#else

#ifdef DBG_FREEZEVIS
	if(dbg_freezevis) return;
#endif

	updateno++;
	if(cur_room) update_room(cur_room);
#endif
}


static void render_room(struct room *room)
{
	int i, nmeshes, nportals;

	nmeshes = darr_size(room->meshes);
	for(i=0; i<nmeshes; i++) {
		render_level_mesh(room->meshes + i);
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
#endif
#endif
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
}


static int portal_frustum_test(struct portal *portal)
{
	int i;

	for(i=0; i<6; i++) {
		if(plane_point_sdist(frust + i, &portal->pos) > portal->rad) {
			return 0;
		}
	}
	return 1;
}
