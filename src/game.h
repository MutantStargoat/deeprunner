#ifndef GAME_H_
#define GAME_H_

#define GAME_CFG_FILE	"game.cfg"

enum {
	GKEY_ESC	= 27,
	GKEY_DEL	= 127,
	GKEY_F1		= 256,
	GKEY_F2, GKEY_F3, GKEY_F4, GKEY_F5, GKEY_F6, GKEY_F7,
	GKEY_F8, GKEY_F9, GKEY_F10, GKEY_F11, GKEY_F12,
	GKEY_UP, GKEY_DOWN, GKEY_LEFT, GKEY_RIGHT,
	GKEY_PGUP, GKEY_PGDOWN,
	GKEY_HOME, GKEY_END,
	GKEY_INS
};

enum {
	GKEY_MOD_SHIFT	= 1,
	GKEY_MOD_CTRL	= 4,
	GKEY_MOD_ALT	= 8
};


struct game_screen {
	const char *name;

	int (*init)(void);
	void (*destroy)(void);
	int (*start)(void);
	void (*stop)(void);
	void (*display)(void);
	void (*reshape)(int, int);
	void (*keyboard)(int, int);
	void (*mouse)(int, int, int, int);
	void (*motion)(int, int);
};

extern int mouse_x, mouse_y, mouse_state[3];
extern int mouse_grabbed;
extern unsigned int modkeys;
extern int win_width, win_height;
extern float win_aspect;
extern int fullscr;

extern long time_msec;
extern struct game_screen *cur_scr;


int game_init(void);
void game_shutdown(void);

void game_display(void);
void game_reshape(int x, int y);
void game_keyboard(int key, int press);
void game_mouse(int bn, int st, int x, int y);
void game_motion(int x, int y);

void game_chscr(struct game_screen *scr);

/* defined in main.c */
void game_swap_buffers(void);
void game_quit(void);
void game_fullscreen(int fs);
void game_grabmouse(int grab);

#endif	/* GAME_H_ */
