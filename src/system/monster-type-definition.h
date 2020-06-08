#pragma once

#include "monster/monster-timed-effect-types.h"
#include "monster-race/monster-race.h"

/*
 * Monster information, for a specific monster.
 * Note: fy, fx constrain dungeon size to 256x256
 * The "hold_o_idx" field points to the first object of a stack
 * of objects (if any) being carried by the monster (see above).
 */
typedef struct floor_type floor_type;
typedef struct monster_type {
	MONRACE_IDX r_idx;		/* Monster race index 0 = dead. */
	MONRACE_IDX ap_r_idx;	/* Monster race appearance index */
	floor_type *current_floor_ptr;

	/* Sub-alignment flags for neutral monsters */
	#define SUB_ALIGN_NEUTRAL 0x0000
	#define SUB_ALIGN_EVIL    0x0001
	#define SUB_ALIGN_GOOD    0x0002
	BIT_FLAGS8 sub_align;		/* Sub-alignment for a neutral monster */

	POSITION fy;		/* Y location on map */
	POSITION fx;		/* X location on map */
	HIT_POINT hp;		/* Current Hit points */
	HIT_POINT maxhp;		/* Max Hit points */
	HIT_POINT max_maxhp;		/* Max Max Hit points */
	HIT_POINT dealt_damage;		/* Sum of damages dealt by player */
	TIME_EFFECT mtimed[MAX_MTIMED];	/* Timed status counter */
	SPEED mspeed;	        /* Monster "speed" */
	ACTION_ENERGY energy_need;	/* Monster "energy" */
	POSITION cdis;		/* Current dis from player */
	BIT_FLAGS8 mflag;	/* Extra monster flags */
	BIT_FLAGS8 mflag2;	/* Extra monster flags */
	bool ml;		/* Monster is "visible" */
	OBJECT_IDX hold_o_idx;	/* Object being held (if any) */
	POSITION target_y;		/* Can attack !los player */
	POSITION target_x;		/* Can attack !los player */
	STR_OFFSET nickname;		/* Monster's Nickname */
	EXP exp;

	/* TODO: クローン、ペット、有効化は意義が異なるので別変数に切り離すこと。save/loadのバージョン更新が面倒そうだけど */
	BIT_FLAGS smart; /*!< Field for "smart_learn" - Some bit-flags for the "smart" field */
	MONSTER_IDX parent_m_idx;
} monster_type;
