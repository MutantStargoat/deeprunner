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
#ifndef GUI_H_
#define GUI_H_

enum {
	GUI_ACTION,
	GUI_CHECKBOX,
	GUI_SELECTOR
};

enum {
	GUI_ALIGN_CENTER,
	GUI_ALIGN_LEFT,
	GUI_ALIGN_RIGHT,
	GUI_VEXPAND	= 4,
};
#define GUI_ALIGN_MASK	3
#define GUI_VPACK		0

struct gui_widget {
	int type;
	float x, y, width, height;
	char *text;
	void *udata;

	char **items;	/* darr */
	float max_item_width;
	int value;		/* checked/unchecked or selected item index */

	void (*cbfunc)(struct gui_widget*);
};

struct gui {
	float x, y, width, height;
	struct font *font;

	struct gui_widget *widgets;	/* darr */

	int sel;
	struct gui_widget *active, *hover;

	float padding;
};

void gui_init_widget(struct gui_widget *w);
void gui_destroy_widget(struct gui_widget *w);

void gui_init(struct gui *ui, float x, float y, float w, float h);
void gui_destroy(struct gui *ui);

void gui_clear(struct gui *ui);
void gui_add_widget(struct gui *ui, struct gui_widget *w);	/* takes ownership */
void gui_add_act(struct gui *ui, const char *str);

void gui_layout(struct gui *ui, unsigned int flags);
void gui_draw(struct gui *ui);

void gui_setsel(struct gui *ui, int sel);
void gui_activate(struct gui *ui, int which);
void gui_sel_next(struct gui_widget *w);
void gui_sel_prev(struct gui_widget *w);

void gui_evkey(struct gui *ui, int key);
void gui_evmouse(struct gui *ui, int bn, int press, int x, int y);
void gui_evmotion(struct gui *ui, int x, int y);

#endif	/* GUI_H_ */
