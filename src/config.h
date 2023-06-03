#ifndef CONFIG_H_
#define CONFIG_H_

#define TIME_LIMIT	240000

#define COL_RADIUS	0.25f
#define LASER_DAMAGE	8
#define MISSILE_DAMAGE	64
#define MISSILE_COOLDOWN	250
#define MISSILE_SPEED	1.0f
#define DMG_OVERLAY_DUR	128
#define SHIELD_OVERLAY_DUR	80
#define EXPL_FRAME_DUR	32
#define NUM_EXPL_FRAMES	8
#define EXPL_DUR		(EXPL_FRAME_DUR * NUM_EXPL_FRAMES)
#define ENEMY_COOLDOWN	700
#define MAX_ROOM_MISSILES	32
#define FLYER_SPEED			0.2
#define SPIKEMOB_SPEED		0.1
#define SPIKEMOB_DAMAGE		128
#define CLOSE_DIST			8.0f

#undef DBG_NOSEED
#undef DBG_ESCQUIT
#undef DBG_FPEXCEPT

#define DBG_NOPSYS
#define DBG_NO_OCTREE
#undef DBG_SHOW_COLPOLY
#define DBG_FREEZEVIS
#undef DBG_SHOW_CUR_ROOM
#undef DBG_ONLY_CUR_ROOM
#undef DBG_ALL_ROOMS
#undef DBG_SHOW_MAX_COL_ITER
#define DBG_NO_IMAN
#undef DBG_SHOW_PORTALS
#undef DBG_SHOW_FRUST

#define DBG_GUI_FRAMES

#endif	/* CONFIG_H_ */
