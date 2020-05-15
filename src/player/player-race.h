#pragma once

#define PRACE_IS_(C, A) (!(C)->mimic_form && ((C)->prace == A))

/*
 * Player race constants (hard-coded by save-files, arrays, etc)
 */
#define RACE_HUMAN               0
#define RACE_HALF_ELF            1
#define RACE_ELF                 2
#define RACE_HOBBIT              3
#define RACE_GNOME               4
#define RACE_DWARF               5
#define RACE_HALF_ORC            6
#define RACE_HALF_TROLL          7
#define RACE_AMBERITE            8
#define RACE_HIGH_ELF            9
#define RACE_BARBARIAN          10
#define RACE_HALF_OGRE          11
#define RACE_HALF_GIANT         12
#define RACE_HALF_TITAN         13
#define RACE_CYCLOPS            14
#define RACE_YEEK               15
#define RACE_KLACKON            16
#define RACE_KOBOLD             17
#define RACE_NIBELUNG           18
#define RACE_DARK_ELF           19
#define RACE_DRACONIAN          20
#define RACE_MIND_FLAYER        21
#define RACE_IMP                22
#define RACE_GOLEM              23
#define RACE_SKELETON           24
#define RACE_ZOMBIE             25
#define RACE_VAMPIRE            26
#define RACE_SPECTRE            27
#define RACE_SPRITE             28
#define RACE_BEASTMAN           29
#define RACE_ENT                30
#define RACE_ANGEL              31
#define RACE_DEMON              32
#define RACE_DUNADAN            33
#define RACE_S_FAIRY            34
#define RACE_KUTAR              35
#define RACE_ANDROID            36
#define RACE_MERFOLK            37

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

SYMBOL_CODE get_summon_symbol_from_player(player_type *p_ptr);
