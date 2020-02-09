#pragma once
#include "spells.h"

/*
 * 職業ごとの選択可能な魔法領域現在の所 bitrh.c でのみ使用。
 * Possible realms that can be chosen currently used only by birth.c.
 */
#define CH_NONE         0x00
#define CH_LIFE         0x01
#define CH_SORCERY      0x02
#define CH_NATURE       0x04
#define CH_CHAOS        0x08
#define CH_DEATH        0x10
#define CH_TRUMP        0x20
#define CH_ARCANE       0x40
#define CH_ENCHANT      0x80
#define CH_DAEMON       0x100
#define CH_CRUSADE      0x200

#define CH_MUSIC        0x08000	/* This is 16th bit */
#define CH_HISSATSU     0x10000
#define CH_HEX          0x20000

 /*
  * Player class constants (hard-coded by save-files, arrays, etc)
  */
#define CLASS_WARRIOR            0
#define CLASS_MAGE               1
#define CLASS_PRIEST             2
#define CLASS_ROGUE              3
#define CLASS_RANGER             4
#define CLASS_PALADIN            5
#define CLASS_WARRIOR_MAGE       6
#define CLASS_CHAOS_WARRIOR      7
#define CLASS_MONK               8
#define CLASS_MINDCRAFTER        9
#define CLASS_HIGH_MAGE         10
#define CLASS_TOURIST           11
#define CLASS_IMITATOR          12
#define CLASS_BEASTMASTER       13
#define CLASS_SORCERER          14
#define CLASS_ARCHER            15
#define CLASS_MAGIC_EATER       16
#define CLASS_BARD              17
#define CLASS_RED_MAGE          18
#define CLASS_SAMURAI           19
#define CLASS_FORCETRAINER      20
#define CLASS_BLUE_MAGE         21
#define CLASS_CAVALRY           22
#define CLASS_BERSERKER         23
#define CLASS_SMITH             24
#define CLASS_MIRROR_MASTER     25
#define CLASS_NINJA             26
#define CLASS_SNIPER            27
#define MAX_CLASS       28 /*!< 職業の最大定義数 Maximum number of player "class" types (see "table.c", etc) */

#define IS_WIZARD_CLASS(C) \
	((C)->pclass == CLASS_MAGE || (C)->pclass == CLASS_HIGH_MAGE || (C)->pclass == CLASS_SORCERER || \
	(C)->pclass == CLASS_MAGIC_EATER || (C)->pclass == CLASS_BLUE_MAGE)

/*
 * Information about the player's "magic"
 *
 * Note that a player with a "spell_book" of "zero" is illiterate.
 */

typedef struct player_magic player_magic;

struct player_magic
{
	OBJECT_TYPE_VALUE spell_book; /* Tval of spell books (if any) */
	int spell_xtra;		/* Something for later */

	int spell_stat;		/* Stat for spells (if any)  */
	int spell_type;		/* Spell type (mage/priest) */

	int spell_first;		/* Level of first spell */
	int spell_weight;		/* Weight that hurts spells */

	magic_type info[MAX_MAGIC][32];    /* The available spells */
};

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
};

extern const player_class *cp_ptr;
extern const player_class class_info[MAX_CLASS];

extern const s32b realm_choices1[];
extern const s32b realm_choices2[];
extern const concptr player_title[MAX_CLASS][PY_MAX_LEVEL / 5];
