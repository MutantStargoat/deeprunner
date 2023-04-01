#ifndef INPUT_H_
#define INPUT_H_

/* game input actions */
enum {
	INP_FWD,
	INP_BACK,
	INP_LEFT,
	INP_RIGHT,
	INP_FIRE,
	INP_LROLL,
	INP_RROLL,

	MAX_INPUTS
};

#define INP_FWD_BIT		(1 << INP_FWD)
#define INP_BACK_BIT	(1 << INP_BACK)
#define INP_LEFT_BIT	(1 << INP_LEFT)
#define INP_RIGHT_BIT	(1 << INP_RIGHT)
#define INP_FIRE_BIT	(1 << INP_FIRE)
#define INP_LROLL_BIT	(1 << INP_LROLL)
#define INP_RROLL_BIT	(1 << INP_RROLL)

#define INP_MOVE_BITS	\
	(INP_FWD_BIT | INP_BACK_BIT | INP_LEFT_BIT | INP_RIGHT_BIT)

struct input_map {
	int inp, key, mbn;
};
extern struct input_map inpmap[MAX_INPUTS];

extern unsigned int inpstate;

void init_input(void);

#endif	/* INPUT_H_ */
