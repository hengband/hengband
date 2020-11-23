#pragma once

/* 人畜無害なenumヘッダを先に読み込む */
#include "player/player-classes-types.h"
#include "object/tval-types.h"
#include "realm/realm-types.h"
#include "system/angband.h"
#include "spell/technic-info-table.h"

/*
 * Information about the player's "magic"
 *
 * Note that a player with a "spell_book" of "zero" is illiterate.
 */

typedef struct player_magic {
	tval_type spell_book; /* Tval of spell books (if any) */
	int spell_xtra;		/* Something for later */

	int spell_stat;		/* Stat for spells (if any)  */
	int spell_type;		/* Spell type (mage/priest) */

	int spell_first;		/* Level of first spell */
	int spell_weight;		/* Weight that hurts spells */

	magic_type info[MAX_MAGIC][32];    /* The available spells */
} player_magic;

extern player_magic *m_info;
extern const player_magic *mp_ptr;

/*
 * Player class info
 */

typedef struct player_class player_class;

struct player_class
{
	concptr title;			/* Type of class */

#ifdef JP
	concptr E_title;		/* 英語職業 */
#endif
	s16b c_adj[6];		/* Class stat modifier */

	s16b c_dis;			/* class disarming */
	s16b c_dev;			/* class magic devices */
	s16b c_sav;			/* class saving throws */
	s16b c_stl;			/* class stealth */
	s16b c_srh;			/* class searching ability */
	s16b c_fos;			/* class searching frequency */
	s16b c_thn;			/* class to hit (normal) */
	s16b c_thb;			/* class to hit (bows) */

	s16b x_dis;			/* extra disarming */
	s16b x_dev;			/* extra magic devices */
	s16b x_sav;			/* extra saving throws */
	s16b x_stl;			/* extra stealth */
	s16b x_srh;			/* extra searching ability */
	s16b x_fos;			/* extra searching frequency */
	s16b x_thn;			/* extra to hit (normal) */
	s16b x_thb;			/* extra to hit (bows) */

	s16b c_mhp;			/* Class hit-dice adjustment */
	s16b c_exp;			/* Class experience factor */

	byte pet_upkeep_div; /* Pet upkeep divider */

    int num;
    int wgt;
    int mul;
};

extern const player_class *cp_ptr;
extern const player_class class_info[MAX_CLASS];

extern const concptr player_title[MAX_CLASS][10];
