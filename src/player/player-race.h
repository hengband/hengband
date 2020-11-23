#pragma once

#include "system/angband.h"

/*
 * Constant for kinds of mimic
 */
#define MIMIC_NONE       0
#define MIMIC_DEMON      1
#define MIMIC_DEMON_LORD 2
#define MIMIC_VAMPIRE    3

#define MIMIC_FLAGS choice
#define MIMIC_IS_NONLIVING 0x00000001
#define MIMIC_IS_DEMON     0x00000002
#define MIMIC_IS_UNDEAD    0x00000004

/*
 * Player racial info
 */

typedef struct player_race player_race;

struct player_race
{
	concptr title;			/* Type of race */

#ifdef JP
	concptr E_title;		/* 英語種族 */
#endif
	s16b r_adj[6];		/* Racial stat bonuses */

	s16b r_dis;			/* disarming */
	s16b r_dev;			/* magic devices */
	s16b r_sav;			/* saving throw */
	s16b r_stl;			/* stealth */
	s16b r_srh;			/* search ability */
	s16b r_fos;			/* search frequency */
	s16b r_thn;			/* combat (normal) */
	s16b r_thb;			/* combat (shooting) */

	byte r_mhp;			/* Race hit-dice modifier */
	byte r_exp;			/* Race experience factor */

	byte b_age;			/* base age */
	byte m_age;			/* mod age */

	byte m_b_ht;		/* base height (males) */
	byte m_m_ht;		/* mod height (males) */
	byte m_b_wt;		/* base weight (males) */
	byte m_m_wt;		/* mod weight (males) */

	byte f_b_ht;		/* base height (females) */
	byte f_m_ht;		/* mod height (females)	  */
	byte f_b_wt;		/* base weight (females) */
	byte f_m_wt;		/* mod weight (females) */

	byte infra;			/* Infra-vision	range */

	u32b choice;        /* Legal class choices */
/*    byte choice_xtra;   */
};

extern const player_race *rp_ptr;

typedef struct player_type player_type;
SYMBOL_CODE get_summon_symbol_from_player(player_type *creature_ptr);
bool is_specific_player_race(player_type *creature_ptr, player_race_type prace);
