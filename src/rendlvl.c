#include "opengl.h"
#include "rendlvl.h"
#include "darray.h"

static struct level *lvl;

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

void render_level(void)
{
	int i, j, nrooms, nmeshes;
	struct room *room;

	nrooms = darr_size(lvl->rooms);
	for(i=0; i<nrooms; i++) {
		room = lvl->rooms[i];

		nmeshes = darr_size(room->meshes);
		for(j=0; j<nmeshes; j++) {
			render_level_mesh(room->meshes + j);
		}
	}
}

void render_level_mesh(struct mesh *mesh)
{
	int pass, more;

	/* TODO handle multipass logic here */
	pass = 0;
	for(;;) {
		more = mtl_apply(&mesh->mtl, pass++);

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
