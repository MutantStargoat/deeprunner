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

#include <string.h>
#include "gaw/gaw.h"
#include "gui.h"
#include "game.h"
#include "font.h"
#include "util.h"
#include "gfxutil.h"
#include "darray.h"

void gui_init_widget(struct gui_widget *w)
{
	memset(w, 0, sizeof *w);
	w->items = darr_alloc(0, sizeof *w->items);
}

void gui_destroy_widget(struct gui_widget *w)
{
	int i, num;

	if(!w) return;

	free(w->text);

	num = darr_size(w->items);
	for(i=0; i<num; i++) {
		free(w->items[i]);
	}
	darr_free(w->items);
}

void gui_init(struct gui *ui, float x, float y, float w, float h)
{
	memset(ui, 0, sizeof *ui);

	ui->x = x;
	ui->y = y;
	ui->width = w;
	ui->height = h;

	ui->widgets = darr_alloc(0, sizeof *ui->widgets);
	ui->sel = -1;
}

void gui_destroy(struct gui *ui)
{
	int i, num = darr_size(ui->widgets);

	for(i=0; i<num; i++) {
		gui_destroy_widget(ui->widgets + i);
	}
	darr_free(ui->widgets);
}

void gui_clear(struct gui *ui)
{
	int i, num = darr_size(ui->widgets);
	for(i=0; i<num; i++) {
		gui_destroy_widget(ui->widgets + i);
	}
	darr_clear(ui->widgets);
}

void gui_add_widget(struct gui *ui, struct gui_widget *w)
{
	darr_push(ui->widgets, w);
}

void gui_add_act(struct gui *ui, const char *str)
{
	struct gui_widget *w;

	w = calloc_nf(1, sizeof *w);
	gui_init_widget(w);
	w->type = GUI_ACTION;

	w->text = strdup_nf(str);
	w->width = font_strwidth(ui->font, str);
	w->height = ui->font->height;

	gui_add_widget(ui, w);
}

void gui_layout(struct gui *ui, unsigned int flags)
{
	int i, num;
	float y = ui->padding;
	float cx = ui->width * 0.5f;
	float dy;

	num = darr_size(ui->widgets);

	if(flags & GUI_VEXPAND) {
		dy = (ui->height - ui->font->height - ui->padding * 2.0f) / (num - 1);
	} else {
		dy = ui->font->height + ui->padding;
	}

	for(i=0; i<num; i++) {
		switch(ui->widgets[i].type) {
		case GUI_ACTION:
		default:
			switch(flags & GUI_ALIGN_MASK) {
			case GUI_ALIGN_LEFT:
				ui->widgets[i].x = cx;
				break;
			case GUI_ALIGN_RIGHT:
				ui->widgets[i].x = cx - ui->widgets[i].width;
				break;
			case GUI_ALIGN_CENTER:
			default:
				ui->widgets[i].x = cx - ui->widgets[i].width * 0.5f;
			}
			ui->widgets[i].y = y;
		}

		y += dy;
	}
}

static void draw_action(struct gui *ui, struct gui_widget *w)
{
	gaw_push_matrix();
	gaw_translate(w->x, w->y + w->height - ui->font->baseline, 0);
	gaw_scale(1, -1, 1);

	gaw_color3f(1, 1, 1);
	dtx_printf(w->text);

	gaw_pop_matrix();

#ifdef DBG_GUI_FRAMES
	gaw_poly_wire();
	gaw_begin(GAW_QUADS);
	gaw_color3f(0.1, 0.4, 0.1);
	gaw_vertex2f(w->x, w->y);
	gaw_vertex2f(w->x + w->width, w->y);
	gaw_vertex2f(w->x + w->width, w->y + w->height);
	gaw_vertex2f(w->x, w->y + w->height);
	gaw_end();
	gaw_poly_gouraud();
#endif
}

void gui_draw(struct gui *ui)
{
	int i, num;

	begin2d(480);

#ifdef DBG_GUI_FRAMES
	gaw_poly_wire();
	gaw_begin(GAW_QUADS);
	gaw_color3f(0.4, 0.4, 0.1);
	gaw_vertex2f(ui->x, ui->y);
	gaw_vertex2f(ui->x + ui->width, ui->y);
	gaw_vertex2f(ui->x + ui->width, ui->y + ui->height);
	gaw_vertex2f(ui->x, ui->y + ui->height);
	gaw_end();
	gaw_poly_gouraud();
#endif

	use_font(font_menu);

	num = darr_size(ui->widgets);
	for(i=0; i<num; i++) {
		switch(ui->widgets[i].type) {
		case GUI_ACTION:
		default:
			draw_action(ui, ui->widgets + i);
		}
	}

	end2d();
}

void gui_setsel(struct gui *ui, int sel)
{
	if(sel < 0 || sel >= darr_size(ui->widgets)) {
		return;
	}
	ui->sel = sel;
	ui->active = ui->widgets + sel;
}

void gui_activate(struct gui *ui, int which)
{
	struct gui_widget *w;

	if(which < 0 || which >= darr_size(ui->widgets)) {
		w = ui->active;
	} else {
		w = ui->widgets + which;
	}

	if(w->cbfunc) w->cbfunc(w);
}

void gui_sel_next(struct gui_widget *w)
{
}

void gui_sel_prev(struct gui_widget *w)
{
}


void gui_evkey(struct gui *ui, int key)
{
	switch(key) {
	case GKEY_HOME:
		gui_setsel(ui, 0);
		break;

	case GKEY_END:
		gui_setsel(ui, darr_size(ui->widgets) - 1);
		break;

	case GKEY_UP:
		gui_setsel(ui, ui->sel - 1);
		break;

	case GKEY_DOWN:
		gui_setsel(ui, ui->sel + 1);
		break;

	case '\r':
		gui_activate(ui, -1);
		break;

	case GKEY_LEFT:
		gui_sel_prev(ui->active);
		break;

	case GKEY_RIGHT:
		gui_sel_next(ui->active);
		break;
	}
}

void gui_evmouse(struct gui *ui, int bn, int press, int x, int y)
{
}

void gui_evmotion(struct gui *ui, int x, int y)
{
}
