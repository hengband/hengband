/*!
 * @brief プレーヤーの職業に関する諸テーブル定義
 * @date 2019/04/30
 * @author deskull
 */

#include "player/player-race.h"
#include "player/player-class.h"
#include "system/object-type-definition.h"

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


/*!
 * @brief 職業とレベル毎のプレイヤー称号テーブル / Class titles for the player.
 * <pre>
 * The player gets a new title every five levels, so each class
 * needs only ten titles total.
 * </pre>
 */
#ifdef JP
const concptr player_title[MAX_CLASS][PY_MAX_LEVEL / 5] =
{
	/* Warrior */
	{
		"新参兵",
		"兵士",
		"傭兵",
		"古参兵",
		"剣士",
		"闘士",
		"英雄",
		"男爵",
		"伯爵",
		"君主",
	},

	/* Mage */
	{
		/*"見習い",*/
"練習生",                 /*丁稚、練習生 */
"奇術師",               /*詐欺師、ペテン師 */
"幻術師",
"呪術師",
"召霊師",
"召魔師",
"魔術師",
"魔道師",
"イプシシマス",
"大魔道師",
},

/* Priest */
{
	"信者",                 /*信徒 */
	"侍僧",             /*教会奉仕者、見習い僧、伴僧、従者 */
	"熟練僧",
	"聖職者",                 /*聖職者 */
	"伝道師",               /*司祭評議員、修道会会員 */
	"牧師",               /*ラマ教の僧 */
	"聖人",               /*大司教、総主教、総大司教 */
	"祭司",                 /*祭司、司祭 */
	"祭司長",               /*大祭司、祭司長 */
	"教皇",
},

/* Rogues */
{
	/* "ごろつき",*/                     /*ごろつき、風来坊、浮浪者 */
	"すり",
	"追いはぎ",                     /*追い剥ぎ、強盗、泥棒 */
	"夜盗",                         /*強盗、夜盗、泥棒 */
	"こそ泥",                       /*こそ泥、小泥棒 */
	"ペテン師",                     /*博徒、ペテン師、詐欺師 */
	"ロウシーフ",
	"ハイシーフ",
	"マスター", /* "マスターシーフ", */
	"アサシン",						/* 暗殺者 */
	"頭領", /*"ギルドマスター",*/
},

/* Rangers */
{
	"使い走り",
	"馳夫",
	"斥候",                         /*斥候、見張り、偵察兵 */
	"狩人",
	"追跡者",
	"先導者",
	"探険者",                       /*開拓者、探険者 */
	"野伏",
	"野伏頭",
	"野伏の総領",
},

/* Paladins */
{
	"勇士",                 /*色男、愛人、しゃれ者、勇敢な人 */
	"衛士",
	"保護者",
	"防衛者",
	"護衛者",
	"騎士",
	"重騎士",
	"聖騎士",
	"上級聖騎士",
	"聖騎士団長",
},

/* Warrior-Mage */
{
	"見習い",
	"徒弟",			/*丁稚、練習生 */
	"一人前",
	"古参兵",
	"魔術兵士",
	"魔術闘士",
	"魔術の英雄", /* Mage-Hero */
	"魔男爵",
	/* "魔公爵", */
	"戦闘魔術士",
	"知識の守護者", /* "ウィザードロード", */
},

/* Chaos Warrior */
{
	"新参兵",
	"兵士",
	"傭兵",
	"古参兵",
	"剣士",
	"闘士",
	"混沌の英雄",
	"混沌の男爵",
	"混沌の公爵",
	"混沌の王者",
},

/* Monk */
{
	"入門者",
	"弟子",
	"直弟子",
	"師範代",
	"師範",
	"道場主",
	"名人",
	"大名人",
	"拳聖",
	"拳神",
},

/* Mindcrafter */
{
	"練習生",
	"見習い",
	"熟練士",
	"熟達士",
	"黙想士",
	"心術士",
	"サイキック",
	"サイオニック",
	"超能力者",
	"精神の支配者", /* "マインドマスター", */
},

/* High Mage; same as Mage */
{
	/*"見習い",*/
"練習生",                 /*丁稚、練習生 */
"奇術師",               /*詐欺師、ペテン師 */
"幻術師",
"呪術師",
"召霊師",
"召魔師",
"魔術師",
"魔道師",
"イプシシマス",
"大魔道師",
},

/* Tourist */
{
	"プー太郎",
	"観光客",
	"周遊旅行者",
	"遍歴者",
	"旅行者",
	"放浪者", /* "旅人", */
	"航海者",
	"探検家",
	"冒険家",
	"スペランカー",
},

/* Imitator */
{
	"これから",
	"いまいち",
	"まだまだ",
	"ぼちぼち",
	"そこそこ",
	"まあまあ",
	"なかなか",
	"いけいけ",
	"そうとう",
	"えらい",
},

/* Beastmaster */
{
	"イモリ使い",
	"ヘビ使い",
	"クモ使い",
	"狼使い",
	"トラ使い",
	"甲虫使い",
	"ヒドラ使い",
	"ハウンド使い",
	"ムーマク使い",
	"ドラゴン使い",
},

/* Sorcerer; same as Mage */
{
	/*"見習い",*/
"練習生",                 /*丁稚、練習生 */
"奇術師",               /*詐欺師、ペテン師 */
"幻術師",
"呪術師",
"召霊師",
"召魔師",
"魔術師",
"魔道師",
"イプシシマス",
"大魔道師",
},

/* Archer */
{
	"新参兵",
	"兵士",
	"傭兵",
	"古参兵",
	"剣士",
	"闘士",
	"英雄",
	"男爵",
	"伯爵",
	"領主",
},

/* Magic eater */
{
	"無知なる者",
	"入門者",
	"奇術師",
	"秘術師",
	"秘術師",
	"熟練者",
	"達人",
	"達人",
	"魔道師",
	"全てを知る者",
},

/* Bard */
{
	"見習い",	/*"Apprentice"*/
	"作曲家",	/*"Songsmith"*/
	"吟遊詩人",	/*"Bard"*/
	"コンパニオン",	/*"Companion"*/
	"心の癒し手",	/*"Minstrel"*/
	"竪琴師",	/*"Harper"*/
	"伝承の紡ぎ手",	/*"Loreweaver"*/
	"詩神の申し子",	/*"Muse"*/
	"夢紡ぎ",	/*"Dreamweaver"*/
	"マスター",	/*"Master Harper"*/
},

/* Red Mage; same as Warrior-Mage */
{
	"見習い",
	"徒弟",			/*丁稚、練習生 */
	"一人前",
	"古参兵",
	"魔術兵士",
	"魔術闘士",
	"魔術の英雄", /* Mage-Hero */
	"魔男爵",
	/* "魔公爵", */
	"戦闘魔術士",
	"知識の守護者", /* "ウィザードロード", */
},

/* Samurai */
{
	"入門者",
	"弟子",
	"直弟子",
	"師範代",
	"師範",
	"道場主",
	"名人",
	"大名人",
	"剣聖",
	"剣神",
},

/* ForceTrainer; same as Monk(?) */
{
	"入門者",
	"弟子",
	"直弟子",
	"師範代",
	"師範",
	"道場主",
	"名人",
	"大名人",
	"拳聖",
	"拳神",
},

/* Blue Mage; same as Mage */
{
	/*"見習い",*/
"練習生",                 /*丁稚、練習生 */
"奇術師",               /*詐欺師、ペテン師 */
"幻術師",
"呪術師",
"召霊師",
"召魔師",
"魔術師",
"魔道師",
"イプシシマス",
"大魔道師",
},

/* Cavalry */
{
	"新参兵",
	"兵士",
	"傭兵",
	"古参兵",
	"剣士",
	"闘士",
	"英雄",
	"男爵",
	"伯爵",
	"領主",
},

/* Berserker */
{
	"バーサーカー",
	"バーサーカー",
	"バーサーカー",
	"怒りの公爵",
	"怒りの公爵",
	"怒りの公爵",
	"怒りの王",
	"怒りの王",
	"怒りの王",
	"怒りの化身",
},

/* Weaponsmith */
{
	"銅を鍛えし者",
	"鉄を鍛えし者",
	"鋼を鍛えし者",
	"銀を鍛えし者",
	"竜を鍛えし者",
	"霊を鍛えし者",
	"魔を鍛えし者",
	"魂を鍛えし者",
	"神を鍛えし者",
	"全を鍛えし者",
},

/* Mirror Master */
{
	"鏡を見る人",
	"鏡磨き",
	"鏡職人",
	"鏡術師",
	"鏡導師",
	"鏡の賢者",
	"鏡の王",
	"鏡の皇帝",
	"鏡の化身",
	"ラフノール王",
},
/* Ninja */
{
	"訓練生",
	"仕手",
	"熟達者",
	"短刀使い",
	"切り裂き",
	"凄腕",
	"漆黒の刃",
	"闇の一撃",
	"暗殺者",
	"死の長き腕",
},

/* Sniper */
{
	"新参兵",
	"兵士",
	"傭兵",
	"古参兵",
	"剣士",
	"闘士",
	"英雄",
	"男爵",
	"伯爵",
	"領主",
},
};

#else
const concptr player_title[MAX_CLASS][PY_MAX_LEVEL / 5] =
{
	/* Warrior */
	{
		"Rookie",
		"Soldier",
		"Mercenary",
		"Veteran",
		"Swordsman",
		"Champion",
		"Hero",
		"Baron",
		"Duke",
		"Lord",
	},

	/* Mage */
	{
		"Apprentice",
		"Trickster",
		"Illusionist",
		"Spellbinder",
		"Evoker",
		"Conjurer",
		"Warlock",
		"Sorcerer",
		"Ipsissimus",
		"Archimage",
	},

	/* Priest */
	{
		"Believer",
		"Acolyte",
		"Adept",
		"Curate",
		"Canon",
		"Priest",
		"High Priest",
		"Cardinal",
		"Inquisitor",
		"Pope",
	},

	/* Rogues */
	{
		"Cutpurse",
		"Robber",
		"Burglar",
		"Filcher",
		"Sharper",
		"Low Thief",
		"High Thief",
		"Master Thief",
		"Assassin",
		"Guildmaster",
	},

	/* Rangers */
	{
		"Runner",
		"Strider",
		"Scout",
		"Courser",
		"Tracker",
		"Guide",
		"Pathfinder",
		"Low Ranger",
		"High Ranger",
		"Ranger Lord",
	},

	/* Paladins */
	{
		"Gallant",
		"Keeper",
		"Protector",
		"Defender",
		"Warder",
		"Knight",
		"Guardian",
		"Low Paladin",
		"High Paladin",
		"Paladin Lord",
	},

	/* Warrior-Mage */
	{
		"Novice",
		"Apprentice",
		"Journeyman",
		"Veteran",
		"Enchanter",
		"Champion",
		"Mage-Hero",
		"Baron Mage",
		"Battlemage",
		"Wizard Lord",
	},

	/* Chaos Warrior */
	{
		"Rookie",
		"Soldier",
		"Mercenary",
		"Veteran",
		"Swordsman",
		"Champion",
		"Chaos Hero",
		"Chaos Baron",
		"Chaos Duke",
		"Chaos Lord",
	},

	/* Monk */
	{
		"Initiate",
		"Brother",
		"Disciple",
		"Immaculate",
		"Master",
		"Soft Master",
		"Hard Master",
		"Flower Master",
		"Dragon Master",
		"Grand Master",
	},

	/* Mindcrafter */
	{
		"Trainee",
		"Acolyte",
		"Adept",
		"Immaculate",
		"Contemplator",
		"Mentalist",
		"Psychic",
		"Psionicist",
		"Esper",
		"Mindmaster",
	},

	/* High Mage; same as Mage */
	{
		"Apprentice",
		"Trickster",
		"Illusionist",
		"Spellbinder",
		"Evoker",
		"Conjurer",
		"Warlock",
		"Sorcerer",
		"Ipsissimus",
		"Archimage",
	},

	/* Tourist */
	{
		"Rambler",
		"Sightseer",
		"Excursionist",
		"Peregrinator",
		"Traveler",
		"Journeyer",
		"Voyager",
		"Explorer",
		"Adventurer",
		"Spelunker",
	},

	/* Imitator */
	{
		"Yet",
		"Lacks",
		"Still more",
		"So so",
		"All right",
		"Not bad",
		"Considerable",
		"Go go",
		"Sizable",
		"Great man",
	},

	/* BeastMaster */
	{
		"Newt Master",
		"Snake Master",
		"Spider Master",
		"Wolf Master",
		"Tiger Master",
		"Beetle Master",
		"Hydra Master",
		"Hound Master",
		"Mumak Master",
		"Dragon Master",
	},

	/* Sorcerer */
	{
		"Apprentice",
		"Trickster",
		"Illusionist",
		"Spellbinder",
		"Evoker",
		"Conjurer",
		"Warlock",
		"Sorcerer",
		"Ipsissimus",
		"Archimage",
	},

	/* Archer */
	{
		"Rookie",
		"Soldier",
		"Mercenary",
		"Veteran",
		"Bowman",
		"Champion",
		"Hero",
		"Baron",
		"Duke",
		"Lord",
	},

	/* Magic eater */
	{
		"Apprentice",
		"Beginner",
		"Jagguler",
		"Skilled",
		"Conjurer",
		"Magician",
		"Master",
		"Master",
		"Wizard",
		"Almighty",
	},

	/* Bard */
	{
		"Apprentice",	/*"Apprentice"*/
		"Songsmith",	/*"Songsmith"*/
		"Bard",	/*"Bard"*/
		"Companion",	/*"Companion"*/
		"Minstrel",	/*"Minstrel"*/
		"Harper",	/*"Harper"*/
		"Loreweaver",	/*"Loreweaver"*/
		"Muse",	/*"Muse"*/
		"Dreamweaver",	/*"Dreamweaver"*/
		"Master Harper",	/*"Master Harper"*/
	},

	/* Red Mage */
	{
		"Novice",
		"Apprentice",
		"Journeyman",
		"Veteran",
		"Enchanter",
		"Champion",
		"Mage-Hero",
		"Baron Mage",
		"Battlemage",
		"Wizard Lord",
	},

	/* Samurai */
	{
		"Initiate",
		"Brother",
		"Disciple",
		"Immaculate",
		"Master",
		"Soft Master",
		"Hard Master",
		"Flower Master",
		"Dragon Master",
		"Grand Master",
	},

	/* ForceTrainer */
	{
		"Initiate",
		"Brother",
		"Disciple",
		"Immaculate",
		"Master",
		"Soft Master",
		"Hard Master",
		"Flower Master",
		"Dragon Master",
		"Grand Master",
	},

	/* Blue Mage */
	{
		"Apprentice",
		"Trickster",
		"Illusionist",
		"Spellbinder",
		"Evoker",
		"Conjurer",
		"Warlock",
		"Sorcerer",
		"Ipsissimus",
		"Archimage",
	},

	/* Warrior */
	{
		"Rookie",
		"Soldier",
		"Mercenary",
		"Veteran",
		"Swordsman",
		"Champion",
		"Hero",
		"Baron",
		"Duke",
		"Lord",
	},

	/* Berserker */
	{
			"Berserker",
			"Berserker",
			"Berserker",
			"Rage Prince",
			"Rage Prince",
			"Rage Prince",
			"Rage King",
			"Rage King",
			"Rage King",
			"God of Rage",
	},

	/* Weaponsmith */
	{
		"Copper smith",
		"Iron smith",
		"Steel smith",
		"Silver smith",
		"Dragon smith",
		"Spirit smith",
		"Magic smith",
		"Soul smith",
		"God smith",
		"AlmightySmith",
	},

	/* Mirror Master */
	{
		"Mirrorstarer",
		"Mirrorcleaner",
		"Mirrormaker",
		"Mirrormagician",
		"Mirror Guru",
		"Mirror Mage",
		"Mirror King",
		"Mirror Emperor",
		"Mirror Avatar",
		"Ruffnor King",
	},

	/* Ninja */
	{
		"Trainee",
		"Myrmidon",
		"Initiate",
		"Knifer",
		"Bladesman",
		"Hashishin",
		"Black Dagger",
		"Shadowstrike",
		"Assassinator",
		"Death Lord",
	},

	/* Sniper */
	{
		"Rookie",
		"Soldier",
		"Mercenary",
		"Veteran",
		"Swordsman",
		"Champion",
		"Hero",
		"Baron",
		"Duke",
		"Lord",
	},
};
#endif

void calc_class_status(player_type *creature_ptr)
{
    switch (creature_ptr->pclass) {
    case CLASS_WARRIOR:
        if (creature_ptr->lev > 29)
            creature_ptr->resist_fear = TRUE;
        if (creature_ptr->lev > 44)
            creature_ptr->regenerate = TRUE;
        break;
    case CLASS_PALADIN:
        if (creature_ptr->lev > 39)
            creature_ptr->resist_fear = TRUE;
        break;
    case CLASS_CHAOS_WARRIOR:
        if (creature_ptr->lev > 29)
            creature_ptr->resist_chaos = TRUE;
        if (creature_ptr->lev > 39)
            creature_ptr->resist_fear = TRUE;
        break;
    case CLASS_MINDCRAFTER:
        if (creature_ptr->lev > 9)
            creature_ptr->resist_fear = TRUE;
        if (creature_ptr->lev > 19)
            creature_ptr->sustain_wis = TRUE;
        if (creature_ptr->lev > 29)
            creature_ptr->resist_conf = TRUE;
        if (creature_ptr->lev > 39)
            creature_ptr->telepathy = TRUE;
        break;
    case CLASS_MONK:
    case CLASS_FORCETRAINER:
        if (!(heavy_armor(creature_ptr))) {
            if (creature_ptr->lev > 24)
                creature_ptr->free_act = TRUE;
        }

        break;
    case CLASS_BARD:
        creature_ptr->resist_sound = TRUE;
        break;
    case CLASS_SAMURAI:
        if (creature_ptr->lev > 29)
            creature_ptr->resist_fear = TRUE;
        break;
    case CLASS_BERSERKER:
        creature_ptr->shero = 1;
        creature_ptr->sustain_str = TRUE;
        creature_ptr->sustain_dex = TRUE;
        creature_ptr->sustain_con = TRUE;
        creature_ptr->regenerate = TRUE;
        creature_ptr->free_act = TRUE;
        if (creature_ptr->lev > 39)
            creature_ptr->reflect = TRUE;
        creature_ptr->redraw |= PR_STATUS;
        break;
    case CLASS_MIRROR_MASTER:
        if (creature_ptr->lev > 39)
            creature_ptr->reflect = TRUE;
        break;
    case CLASS_NINJA:
        if (heavy_armor(creature_ptr)) {
            creature_ptr->skill_stl -= (creature_ptr->lev) / 10;
        } else if ((!creature_ptr->inventory_list[INVEN_RARM].k_idx || creature_ptr->migite)
            && (!creature_ptr->inventory_list[INVEN_LARM].k_idx || creature_ptr->hidarite)) {
            creature_ptr->skill_stl += (creature_ptr->lev) / 10;
            if (creature_ptr->lev > 24)
                creature_ptr->free_act = TRUE;
        }

        if ((!creature_ptr->inventory_list[INVEN_RARM].k_idx || creature_ptr->migite)
            && (!creature_ptr->inventory_list[INVEN_LARM].k_idx || creature_ptr->hidarite)) {
            creature_ptr->to_a += creature_ptr->lev / 2 + 5;
            creature_ptr->dis_to_a += creature_ptr->lev / 2 + 5;
        }

        creature_ptr->slow_digest = TRUE;
        creature_ptr->resist_fear = TRUE;
        if (creature_ptr->lev > 19)
            creature_ptr->resist_pois = TRUE;
        if (creature_ptr->lev > 24)
            creature_ptr->sustain_dex = TRUE;
        if (creature_ptr->lev > 29)
            creature_ptr->see_inv = TRUE;
        if (creature_ptr->lev > 44) {
            creature_ptr->oppose_pois = 1;
            creature_ptr->redraw |= PR_STATUS;
        }

        creature_ptr->see_nocto = TRUE;
        break;
    }
}
