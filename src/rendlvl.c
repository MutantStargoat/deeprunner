#include "config.h"

#include "opengl.h"
#include "rendlvl.h"
#include "darray.h"

static struct level *lvl;
static struct room *cur_room;

#ifdef DBG_SHOW_CUR_ROOM
static int dbg_cur_room;
static const float red[] = {1, 0.5, 0.5, 1};
#endif

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

void rendlvl_setup(const cgm_vec3 *ppos, const cgm_quat *prot)
{
	cur_room = lvl_room_at(lvl, ppos->x, ppos->y, ppos->z);
}

static void render_room(struct room *room)
{
	int i, nmeshes;

	nmeshes = darr_size(room->meshes);
	for(i=0; i<nmeshes; i++) {
		render_level_mesh(room->meshes + i);
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

		render_room(room);
	}
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
