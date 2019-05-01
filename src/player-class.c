
#include "angband.h"
#include "player-class.h"

/*
 * The magic info
 */
const player_magic *mp_ptr;
player_magic *m_info;


const player_class *cp_ptr;

/*!
 * @brief 職業情報 /
 * Player Classes
 * @details
 * <pre>
 *      Title,
 *      {STR,INT,WIS,DEX,CON,CHR},
 *      c_dis, c_dev, c_sav, c_stl, c_srh, c_fos, c_thn, c_thb,
 *      x_dis, x_dev, x_sav, x_stl, x_srh, x_fos, x_thn, x_thb,
 *      HD, Exp, pet_upkeep_div
 * </pre>
 */
const player_class class_info[MAX_CLASS] =
{
	{
#ifdef JP
		"戦士",
#endif
		"Warrior",

		{ 4, -2, -2, 2, 2, -1},
		25, 18, 31, 1,  14, 2, 70, 55,
		12, 7,  10, 0,  0,  0,  30, 30,
		9,  0, 40
	},

	{
#ifdef JP
		"メイジ",
#endif
		"Mage",

		{-4, 3, 0, 1, -2, 1},
		30, 40, 38, 3,  16, 20, 34, 20,
		7,  15, 11,  0,  0,  0,  6, 7,
		0, 30, 30
	},

	{
#ifdef JP
		"プリースト",
#endif
		"Priest",

		{-1, -3, 3, -1, 0, 2},
		25, 35, 40, 2,  16, 8, 48, 35,
		7,  11, 12, 0,  0,  0, 13, 11,
		2, 20, 35
	},

	{
#ifdef JP
		"盗賊",
#endif
		"Rogue",

		{ 2, 1, -2, 3, 1, -1},
		45, 37, 36, 5, 32, 24, 60, 66,
		15, 12, 10, 0,  0,  0, 21, 18,
		6, 25, 40
	},

	{
#ifdef JP
		"レンジャー",
#endif
		"Ranger",

		{ 2, 2, 0, 1, 1, 1},
		30, 37, 36, 3,  24, 16, 56, 72,
		8,  11, 10, 0,  0,  0,  18, 28,
		4, 40, 35
	},

	{
#ifdef JP
		"パラディン",
#endif
		"Paladin",

		{ 3, -3, 1, 0, 2, 2},
		20, 24, 34, 1,  12, 2, 68, 40,
		7,  10, 11, 0,  0,  0,  21, 18,
		6, 35, 40
	},

	{
#ifdef JP
		"魔法戦士",
#endif
		"Warrior-Mage",

		{ 2, 2, 0, 1, 0, 1},
		30, 35, 36, 2,  18, 16, 50, 25,
		7,  10, 10, 0,  0,  0,  15, 11,
		4, 40, 35
	},

	{
#ifdef JP
		"混沌の戦士",
#endif
		"Chaos-Warrior",

		{ 2, 1, -1, 0, 2, -2},
		20, 25, 34, 1,  14, 12, 65, 40,
		7,  11, 10, 0,  0,  0,  20, 17,
		6, 25, 40
	},

	{
#ifdef JP
		"修行僧",
#endif
		"Monk",

		{ 2, -1, 1, 3, 2, 1},
		45, 34, 36, 5, 32, 24, 64, 60,
		15, 11, 10, 0,  0,  0, 18, 18,
		6, 30, 35
	},

	{
#ifdef JP
		"超能力者",
#endif
		"Mindcrafter",

		{-1, 0, 3, -1, -1, 2},   /* note: spell stat is Wis */
		30, 33, 38, 3,  22, 16, 50, 40,
		10, 11, 10, 0,   0,  0, 14, 18,
		2, 25, 35
	},

	{
#ifdef JP
		"ハイ=メイジ",
#endif
		"High-Mage",

		{-4, 4, 0, 0, -2, 1},
		30, 40, 38, 3,  16, 20, 34, 20,
		7,  15, 11,  0,  0,  0,  6, 7,
		0, 30, 25
	},

	{
#ifdef JP
		"観光客",
#endif
		"Tourist",
		{ -1, -1, -1, -1, -1, -1},
		15, 18, 28, 1, 12, 2, 40, 20,
		5, 7, 9, 0,  0,  0,  11, 11,
		0, -30, 40
	},

	{
#ifdef JP
		"ものまね師",
#endif
		"Imitator",
		{ 0, 1, -1, 2, 0, 1},
		25, 30, 36, 2,  18, 16, 60, 50,
		7,  10,  10, 0,  0,  0,  18, 20,
		5, 10, 20
	},

	{
#ifdef JP
		"魔獣使い",
#endif
		"BeastMaster",
		{ 1, -1, -1, 1, 0, 2},
		20, 25, 32, 2,  18, 16, 52, 63,
		7,  10, 10, 0,  0,  0,  14, 25,
		3, 20, 10
	},

	{
#ifdef JP
		"スペルマスター",
#endif
		"Sorcerer",

		{-5, 6, -2, 2, 0, -2},
		30, 48, 75, 2,  12, 22,  0, 0,
		 7, 18, 13, 0,  0,  0,  0, 0,
		4, 60, 25
	},

	{
#ifdef JP
		"アーチャー",
#endif
		"Archer",

		{ 2, -1, -1, 2, 1, 0},
		38, 24, 35, 4,  24, 16, 56, 82,
		12, 10, 10, 0,  0,  0,  18, 36,
		6, 10, 40
	},

	{
#ifdef JP
		"魔道具術師",
#endif
		"Magic-Eater",

		{-1, 2, 1, 2, -2, 1},
		25, 42, 36, 2,  20, 16, 48, 35,
		7,  16, 10,  0,  0,  0, 13, 11,
		3, 30, 30
	},

	{
#ifdef JP
		"吟遊詩人",
#endif
		"Bard",              /* Note : spell stat is Charisma */
		{-2, 1, 2, -1, -2, 4},
		20, 33, 34, -5, 16, 20, 34, 20,
		8,  13, 11, 0,  0,  0,  10, 8,
		2, 40, 25
	},

	{
#ifdef JP
		"赤魔道師",
#endif
		"Red-Mage",

		{ 2, 2, -1, 1, 0, -1},
		20, 34, 34, 1,  16, 10, 56, 25,
		7,  11, 11, 0,  0,  0,  18, 11,
		4, 40, 40
	},

	{
#ifdef JP
		"剣術家",
#endif
		"Samurai",

		{ 3, -2, 1, 2, 1, 0},
		25, 18, 32, 2,  16, 6, 70, 40,
		12, 7,  10, 0,  0,  0,  23, 18,
		6,  30, 40
	},

	{
#ifdef JP
		"練気術師",
#endif
		"ForceTrainer",

		{ 0, -1, 3, 2, 1, 1},
		30, 34, 38, 4, 32, 24, 50, 40,
		10, 11, 11, 0,  0,  0, 14, 15,
		2, 35, 40
	},

	{
#ifdef JP
		"青魔道師",
#endif
		"Blue-Mage",

		{-4, 4, -1, 1, -2, -1},
		30, 40, 36, 3,  20, 16, 40, 25,
		7,  16, 11,  0,  0,  0,  6, 7,
		2, 30, 35
	},

	{
#ifdef JP
		"騎兵",
#endif
		"Cavalry",
		{ 2, -2, -2, 2, 2, 0},
		20, 18, 32, 1,  16, 10, 60, 66,
		10,  7, 10, 0,  0,  0,  22, 26,
		5, 20, 35
	},

	{
#ifdef JP
		"狂戦士",
#endif
		"Berserker",

		{ 8, -20, -20, 4, 4, -5},
		-100, -1000, -200, -100,  -100, -100, 120, -2000,
		0, 0,  0, 0,  0,  0,  50, 0,
		11,  60, 255
	},

	{
#ifdef JP
		"鍛冶師",
#endif
		"Weaponsmith",

		{ 3, -1, -1, 1, 0, -1},
		30, 28, 28, 1,  20, 10, 60, 45,
		10, 10,  10, 0,  0,  0,  21, 15,
		6,  30, 40
	},
	{
#ifdef JP
		"鏡使い",
#endif
		"Mirror-Master",

		{ -2,  3, 1, -1, -2, 1},
		30, 33, 40, 3, 14, 16, 34,30,
		10, 11, 12, 0,  0,  0,  6,10,
		2,  30, 30
	},
	{
#ifdef JP
		"忍者",
#endif
		"Ninja",

		{ 0,  -1, -1, 3, 2, -1},
		45, 24, 36, 8, 48, 32, 70,66,
		15, 10, 10, 0,  0,  0, 25,18,
		2,  20, 40
	},

	{
#ifdef JP
		"スナイパー",
#endif
		"Sniper",

		{ 2, -1, -1, 2, 1, 0},
		25, 24, 28, 5, 32, 18, 56,  72,
		12, 10, 10, 0,  0,  0, 18,  28,
		2, 20, 40,
	},
};

/*!
 * 職業毎に選択可能な第一領域魔法テーブル
 */
const s32b realm_choices1[MAX_CLASS] =
{
	(CH_NONE),				/* Warrior */
	(CH_LIFE | CH_SORCERY | CH_NATURE |
	 CH_CHAOS | CH_DEATH | CH_TRUMP |
	 CH_ARCANE | CH_ENCHANT | CH_DAEMON |
	 CH_CRUSADE),                              /* Mage */
	(CH_LIFE | CH_DEATH | CH_DAEMON |
	 CH_CRUSADE),                              /* Priest */
	(CH_SORCERY | CH_DEATH | CH_TRUMP |
	 CH_ARCANE | CH_ENCHANT),               /* Rogue */
	(CH_NATURE),                            /* Ranger */
	(CH_CRUSADE | CH_DEATH),                   /* Paladin */
	(CH_ARCANE),                            /* Warrior-Mage */
	(CH_CHAOS | CH_DAEMON),                 /* Chaos-Warrior */
	(CH_LIFE | CH_NATURE | CH_DEATH |
	 CH_ENCHANT),                           /* Monk */
	(CH_NONE),                              /* Mindcrafter */
	(CH_LIFE | CH_SORCERY | CH_NATURE |
	 CH_CHAOS | CH_DEATH | CH_TRUMP |
	 CH_ARCANE | CH_ENCHANT | CH_DAEMON |
	 CH_CRUSADE | CH_HEX),                  /* High-Mage */
	(CH_ARCANE),                            /* Tourist */
	(CH_NONE),                              /* Imitator */
	(CH_TRUMP),                             /* Beastmaster */
	(CH_NONE),                              /* Sorcerer */
	(CH_NONE),                              /* Archer */
	(CH_NONE),                              /* Magic eater */
	(CH_MUSIC),                             /* Bard */
	(CH_NONE),                              /* Red Mage */
	(CH_HISSATSU),                          /* Samurai */
	(CH_LIFE | CH_NATURE | CH_DEATH |
	 CH_ENCHANT | CH_CRUSADE),                 /* ForceTrainer */
	(CH_NONE),                              /* Blue Mage */
	(CH_NONE),				/* Cavalry */
	(CH_NONE),				/* Berserker */
	(CH_NONE),				/* Weaponsmith */
	(CH_NONE),				/* Mirror-master */
	(CH_NONE),				/* Ninja */
	(CH_NONE),				/* Sniper */
};

/*!
 * 職業毎に選択可能な第二領域魔法テーブル
 */
const s32b realm_choices2[MAX_CLASS] =
{
	(CH_NONE),                              /* Warrior */
	(CH_LIFE | CH_SORCERY | CH_NATURE |
	 CH_CHAOS | CH_DEATH | CH_TRUMP |
	 CH_ARCANE | CH_ENCHANT | CH_DAEMON |
	 CH_CRUSADE),                              /* Mage */
	(CH_LIFE | CH_SORCERY | CH_NATURE |
	 CH_CHAOS | CH_DEATH | CH_TRUMP |
	 CH_ARCANE | CH_ENCHANT | CH_DAEMON |
	 CH_CRUSADE),                              /* Priest */
	(CH_NONE),                              /* Rogue */
	(CH_SORCERY | CH_CHAOS | CH_DEATH |
	 CH_TRUMP | CH_ARCANE | CH_DAEMON),     /* Ranger */
	(CH_NONE),                              /* Paladin */
	(CH_LIFE | CH_NATURE | CH_CHAOS |
	 CH_DEATH | CH_TRUMP | CH_ARCANE |
	 CH_SORCERY | CH_ENCHANT | CH_DAEMON |
	 CH_CRUSADE),                              /* Warrior-Mage */
	(CH_NONE),                              /* Chaos-Warrior */
	(CH_NONE),                              /* Monk */
	(CH_NONE),                              /* Mindcrafter */
	(CH_NONE),                              /* High-Mage */
	(CH_NONE),                              /* Tourist */
	(CH_NONE),                              /* Imitator */
	(CH_NONE),                              /* Beastmanster */
	(CH_NONE),                              /* Sorcerer */
	(CH_NONE),                              /* Archer */
	(CH_NONE),                              /* Magic eater */
	(CH_NONE),                              /* Bard */
	(CH_NONE),                              /* Red Mage */
	(CH_NONE),                              /* Samurai */
	(CH_NONE),                              /* ForceTrainer */
	(CH_NONE),                              /* Blue Mage */
	(CH_NONE),				/* Cavalry */
	(CH_NONE),				/* Berserker */
	(CH_NONE),				/* Weaponsmith */
	(CH_NONE),				/* Mirror-master */
	(CH_NONE),				/* Ninja */
	(CH_NONE),				/* Sniper */
};

