/*!
 *  @file wizard1.c
 *  @brief ウィザードモードの処理(スポイラー出力中心) / Spoiler generation -BEN-
 *  @date 2014/02/17
 *  @author
 * Copyright (c) 1997 Ben Harrison, and others
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * 2013 Deskull rearranged comment for Doxygen.
 */

#include "angband.h"


#ifdef ALLOW_SPOILERS


/*
 * The spoiler file being created
 */
static FILE *fff = NULL;



/*!
 * @brief シンボル職の記述名を返す /
 * Extract a textual representation of an attribute
 * @param r_ptr モンスター種族の構造体ポインタ
 * @return シンボル職の記述名
 */
static cptr attr_to_text(monster_race *r_ptr)
{
#ifdef JP000
	if (r_ptr->flags1 & RF1_ATTR_CLEAR)    return "透明な";
	if (r_ptr->flags1 & RF1_ATTR_MULTI)    return "万色の";
	if (r_ptr->flags1 & RF1_ATTR_SEMIRAND) return "準ランダムな";
#else
	if (r_ptr->flags1 & RF1_ATTR_CLEAR)    return "Clear";
	if (r_ptr->flags1 & RF1_ATTR_MULTI)    return "Multi";
	if (r_ptr->flags1 & RF1_ATTR_SEMIRAND) return "S.Rand";
#endif

	switch (r_ptr->d_attr)
	{
#ifdef JP000
	case TERM_DARK:    return "XXXい";
	case TERM_WHITE:   return "白い";
	case TERM_SLATE:   return "青灰色の";
	case TERM_ORANGE:  return "オレンジの";
	case TERM_RED:     return "赤い";
	case TERM_GREEN:   return "緑の";
	case TERM_BLUE:    return "青い";
	case TERM_UMBER:   return "琥珀色の";
	case TERM_L_DARK:  return "灰色の";
	case TERM_L_WHITE: return "明青灰色の";
	case TERM_VIOLET:  return "紫の";
	case TERM_YELLOW:  return "黄色い";
	case TERM_L_RED:   return "明い赤の";
	case TERM_L_GREEN: return "明い緑の";
	case TERM_L_BLUE:  return "明い青の";
	case TERM_L_UMBER: return "明い琥珀色の";
#else
	case TERM_DARK:    return "xxx";
	case TERM_WHITE:   return "White";
	case TERM_SLATE:   return "Slate";
	case TERM_ORANGE:  return "Orange";
	case TERM_RED:     return "Red";
	case TERM_GREEN:   return "Green";
	case TERM_BLUE:    return "Blue";
	case TERM_UMBER:   return "Umber";
	case TERM_L_DARK:  return "L.Dark";
	case TERM_L_WHITE: return "L.Slate";
	case TERM_VIOLET:  return "Violet";
	case TERM_YELLOW:  return "Yellow";
	case TERM_L_RED:   return "L.Red";
	case TERM_L_GREEN: return "L.Green";
	case TERM_L_BLUE:  return "L.Blue";
	case TERM_L_UMBER: return "L.Umber";
#endif
	}

	/* Oops */
#ifdef JP000
	return "変な";
#else
	return "Icky";
#endif
}



/*
 * A tval grouper
 */
typedef struct
{
	byte tval;
	cptr name;
} grouper;



/*
 * Item Spoilers by: benh@phial.com (Ben Harrison)
 */


/*
 * The basic items categorized by type
 */
static grouper group_item[] =
{
	{ TV_SHOT,          _("射撃物", "Ammo") },
	{ TV_ARROW,         NULL },
	{ TV_BOLT,          NULL },
	{ TV_BOW,           _("弓", "Bows") },
	{ TV_DIGGING,       _("武器", "Weapons") },
	{ TV_POLEARM,       NULL },
	{ TV_HAFTED,        NULL },
	{ TV_SWORD,         NULL },
	{ TV_SOFT_ARMOR,    _("防具 (体)", "Armour (Body)") },
	{ TV_HARD_ARMOR,    NULL },
	{ TV_DRAG_ARMOR,    NULL },
	{ TV_BOOTS,         _("防具 (その他)", "Armour (Misc)") },
	{ TV_GLOVES,        NULL },
	{ TV_HELM,          NULL },
	{ TV_CROWN,         NULL },
	{ TV_SHIELD,        NULL },
	{ TV_CLOAK,         NULL },

#ifdef JP
	{ TV_LITE,          "光源" },
	{ TV_AMULET,        "アミュレット" },
	{ TV_RING,          "指輪" },
#else
	{ TV_LITE,          "Light Sources" },
	{ TV_AMULET,        "Amulets" },
	{ TV_RING,          "Rings" },
#endif

#ifdef JP
	{ TV_STAFF,         "杖" },
	{ TV_WAND,          "魔法棒" },
	{ TV_ROD,           "ロッド" },
#else
	{ TV_STAFF,         "Staffs" },
	{ TV_WAND,          "Wands" },
	{ TV_ROD,           "Rods" },
#endif

#ifdef JP
	{ TV_SCROLL,        "巻物" },
	{ TV_POTION,        "薬" },
	{ TV_FOOD,          "食料" },
#else
	{ TV_SCROLL,        "Scrolls" },
	{ TV_POTION,        "Potions" },
	{ TV_FOOD,          "Food" },
#endif

#ifdef JP
	{ TV_LIFE_BOOK,     "魔法書 (生命)" },
	{ TV_SORCERY_BOOK,  "魔法書 (仙術)" },
	{ TV_NATURE_BOOK,   "魔法書 (自然)" },
	{ TV_CHAOS_BOOK,    "魔法書 (カオス)" },
	{ TV_DEATH_BOOK,    "魔法書 (暗黒)" },
	{ TV_TRUMP_BOOK,    "魔法書 (トランプ)" },
	{ TV_ARCANE_BOOK,   "魔法書 (秘術)" },
	{ TV_CRAFT_BOOK,    "魔法書 (匠)" },
	{ TV_DAEMON_BOOK,   "魔法書 (悪魔)" },
	{ TV_CRUSADE_BOOK,  "魔法書 (破邪)" },
	{ TV_MUSIC_BOOK,    "歌集" },
	{ TV_HISSATSU_BOOK, "武芸の書" },
	{ TV_HEX_BOOK,      "魔法書 (呪術)" },
#else
	{ TV_LIFE_BOOK,     "Books (Life)" },
	{ TV_SORCERY_BOOK,  "Books (Sorcery)" },
	{ TV_NATURE_BOOK,   "Books (Nature)" },
	{ TV_CHAOS_BOOK,    "Books (Chaos)" },
	{ TV_DEATH_BOOK,    "Books (Death)" },
	{ TV_TRUMP_BOOK,    "Books (Trump)" },
	{ TV_ARCANE_BOOK,   "Books (Arcane)" },
	{ TV_CRAFT_BOOK,    "Books (Craft)" },
	{ TV_DAEMON_BOOK,   "Books (Daemon)" },
	{ TV_CRUSADE_BOOK,  "Books (Crusade)" },
	{ TV_MUSIC_BOOK,    "Song Books" },
	{ TV_HISSATSU_BOOK, "Books (Kendo)" },
	{ TV_HEX_BOOK,      "Books (Hex)" },
#endif

#ifdef JP
	{ TV_WHISTLE,       "笛" },
	{ TV_CAPTURE,       "キャプチャー・ボール" },
	{ TV_CARD,          "エクスプレスカード" },
#else
	{ TV_WHISTLE,       "Whistle" },
	{ TV_CAPTURE,       "Capture Ball" },
	{ TV_CARD,          "Express Card" },
#endif

	{ TV_CHEST,         _("箱", "Chests") },

#ifdef JP
	{ TV_FIGURINE,      "人形" },
	{ TV_STATUE,        "像" },
	{ TV_CORPSE,        "死体" },
#else
	{ TV_FIGURINE,      "Magical Figurines" },
	{ TV_STATUE,        "Statues" },
	{ TV_CORPSE,        "Corpses" },
#endif

	{ TV_SKELETON,      _("その他", "Misc") },
	{ TV_BOTTLE,        NULL },
	{ TV_JUNK,          NULL },
	{ TV_SPIKE,         NULL },
	{ TV_FLASK,         NULL },
	{ TV_PARCHMENT,     NULL },

	{ 0, "" }
};


/*!
 * @brief ベースアイテムの各情報を文字列化する /
 * Describe the kind
 * @param buf 名称を返すバッファ参照ポインタ
 * @param dam ダメージダイス記述を返すバッファ参照ポインタ
 * @param wgt 重量記述を返すバッファ参照ポインタ
 * @param lev 生成階記述を返すバッファ参照ポインタ
 * @param val 価値を返すバッファ参照ポインタ
 * @param k ベースアイテムID
 * @return なし
 */
static void kind_info(char *buf, char *dam, char *wgt, int *lev, s32b *val, int k)
{
	object_type forge;
	object_type *q_ptr;


	/* Get local object */
	q_ptr = &forge;

	/* Prepare a fake item */
	object_prep(q_ptr, k);

	/* It is known */
	q_ptr->ident |= (IDENT_KNOWN);

	/* Cancel bonuses */
	q_ptr->pval = 0;
	q_ptr->to_a = 0;
	q_ptr->to_h = 0;
	q_ptr->to_d = 0;


	/* Level */
	(*lev) = k_info[q_ptr->k_idx].level;

	/* Value */
	(*val) = object_value(q_ptr);


	/* Hack */
	if (!buf || !dam || !wgt) return;


	/* Description (too brief) */
	object_desc(buf, q_ptr, (OD_NAME_ONLY | OD_STORE));


	/* Misc info */
	strcpy(dam, "");

	/* Damage */
	switch (q_ptr->tval)
	{
		/* Bows */
		case TV_BOW:
		{
			break;
		}

		/* Ammo */
		case TV_SHOT:
		case TV_BOLT:
		case TV_ARROW:
		{
			sprintf(dam, "%dd%d", q_ptr->dd, q_ptr->ds);
			break;
		}

		/* Weapons */
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_SWORD:
		case TV_DIGGING:
		{
			sprintf(dam, "%dd%d", q_ptr->dd, q_ptr->ds);
			break;
		}

		/* Armour */
		case TV_BOOTS:
		case TV_GLOVES:
		case TV_CLOAK:
		case TV_CROWN:
		case TV_HELM:
		case TV_SHIELD:
		case TV_SOFT_ARMOR:
		case TV_HARD_ARMOR:
		case TV_DRAG_ARMOR:
		{
			sprintf(dam, "%d", q_ptr->ac);
			break;
		}
	}


	/* Weight */
	sprintf(wgt, "%3d.%d", q_ptr->weight / 10, q_ptr->weight % 10);
}


/*!
 * @brief 各ベースアイテムの情報を一行毎に記述する /
 * Create a spoiler file for items
 * @param fname ファイル名
 * @return なし
 */
static void spoil_obj_desc(cptr fname)
{
	int i, k, s, t, n = 0, group_start = 0;

	u16b who[200];

	char buf[1024];

	char wgt[80];
	char dam[80];


	/* Build the filename */
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, fname);

	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);

	/* Open the file */
	fff = my_fopen(buf, "w");

	/* Oops */
	if (!fff)
	{
		msg_print("Cannot create spoiler file.");
		return;
	}


	/* Header */
	fprintf(fff, "Spoiler File -- Basic Items (Hengband %d.%d.%d)\n\n\n",
		FAKE_VER_MAJOR-10, FAKE_VER_MINOR, FAKE_VER_PATCH);

	/* More Header */
	fprintf(fff, "%-45s     %8s%7s%5s%9s\n",
		"Description", "Dam/AC", "Wgt", "Lev", "Cost");
	fprintf(fff, "%-45s     %8s%7s%5s%9s\n",
		"----------------------------------------",
		"------", "---", "---", "----");

	/* List the groups */
	for (i = 0; TRUE; i++)
	{
		/* Write out the group title */
		if (group_item[i].name)
		{
			if (n)
			{
				/* Hack -- bubble-sort by cost and then level */
				for (s = 0; s < n - 1; s++)
				{
					for (t = 0; t < n - 1; t++)
					{
						int i1 = t;
						int i2 = t + 1;

						int e1;
						int e2;

						s32b t1;
						s32b t2;

						kind_info(NULL, NULL, NULL, &e1, &t1, who[i1]);
						kind_info(NULL, NULL, NULL, &e2, &t2, who[i2]);

						if ((t1 > t2) || ((t1 == t2) && (e1 > e2)))
						{
							int tmp = who[i1];
							who[i1] = who[i2];
							who[i2] = tmp;
						}
					}
				}

				fprintf(fff, "\n\n%s\n\n", group_item[group_start].name);

				/* Spoil each item */
				for (s = 0; s < n; s++)
				{
					int e;
					s32b v;

					/* Describe the kind */
					kind_info(buf, dam, wgt, &e, &v, who[s]);

					/* Dump it */
					fprintf(fff, "     %-45s%8s%7s%5d%9ld\n",
						buf, dam, wgt, e, (long)(v));
				}

				/* Start a new set */
				n = 0;
			}

			/* Notice the end */
			if (!group_item[i].tval) break;

			/* Start a new set */
			group_start = i;
		}

		/* Acquire legal item types */
		for (k = 1; k < max_k_idx; k++)
		{
			object_kind *k_ptr = &k_info[k];

			/* Skip wrong tval's */
			if (k_ptr->tval != group_item[i].tval) continue;

			/* Hack -- Skip instant-artifacts */
			if (k_ptr->gen_flags & (TRG_INSTA_ART)) continue;

			/* Save the index */
			who[n++] = k;
		}
	}


	/* Check for errors */
	if (ferror(fff) || my_fclose(fff))
	{
		msg_print("Cannot close spoiler file.");
		return;
	}

	/* Message */
	msg_print("Successfully created a spoiler file.");
}


/*
 * Artifact Spoilers by: randy@PICARD.tamu.edu (Randy Hutson)
 */


/*
 * Returns a "+" string if a number is non-negative and an empty
 * string if negative
 */
#define POSITIZE(v) (((v) >= 0) ? "+" : "")

/*
 * These are used to format the artifact spoiler file. INDENT1 is used
 * to indent all but the first line of an artifact spoiler. INDENT2 is
 * used when a line "wraps". (Bladeturner's resistances cause this.)
 */
#define INDENT1 "    "
#define INDENT2 "      "

/*
 * MAX_LINE_LEN specifies when a line should wrap.
 */
#define MAX_LINE_LEN 75

/*
 * Given an array, determine how many elements are in the array
 */
#define N_ELEMENTS(a) (sizeof (a) / sizeof ((a)[0]))

/*
 * The artifacts categorized by type
 */
static grouper group_artifact[] =
{
#ifdef JP
	{ TV_SWORD,             "刀剣" },
	{ TV_POLEARM,           "槍/斧" },
	{ TV_HAFTED,            "鈍器" },
	{ TV_DIGGING,           "シャベル/つるはし" },
	{ TV_BOW,               "飛び道具" },
	{ TV_ARROW,             "矢" },
	{ TV_BOLT,              NULL },

	{ TV_SOFT_ARMOR,        "鎧" },
	{ TV_HARD_ARMOR,        NULL },
	{ TV_DRAG_ARMOR,        NULL },

	{ TV_CLOAK,             "クローク" },
	{ TV_SHIELD,            "盾" },
	{ TV_CARD,              NULL },
	{ TV_HELM,              "兜/冠" },
	{ TV_CROWN,             NULL },
	{ TV_GLOVES,            "籠手" },
	{ TV_BOOTS,             "靴" },

	{ TV_LITE,              "光源" },
	{ TV_AMULET,            "アミュレット" },
	{ TV_RING,              "指輪" },
#else
	{ TV_SWORD,             "Edged Weapons" },
	{ TV_POLEARM,           "Polearms" },
	{ TV_HAFTED,            "Hafted Weapons" },
	{ TV_DIGGING,           "Shovels/Picks" },
	{ TV_BOW,               "Bows" },
	{ TV_ARROW,             "Ammo" },
	{ TV_BOLT,              NULL },

	{ TV_SOFT_ARMOR,        "Body Armor" },
	{ TV_HARD_ARMOR,        NULL },
	{ TV_DRAG_ARMOR,        NULL },

	{ TV_CLOAK,             "Cloaks" },
	{ TV_SHIELD,            "Shields" },
	{ TV_CARD,              NULL },
	{ TV_HELM,              "Helms/Crowns" },
	{ TV_CROWN,             NULL },
	{ TV_GLOVES,            "Gloves" },
	{ TV_BOOTS,             "Boots" },

	{ TV_LITE,              "Light Sources" },
	{ TV_AMULET,            "Amulets" },
	{ TV_RING,              "Rings" },
#endif

	{ 0, NULL }
};



/*
 * Pair together a constant flag with a textual description.
 *
 * Used by both "init.c" and "wiz-spo.c".
 *
 * Note that it sometimes more efficient to actually make an array
 * of textual names, where entry 'N' is assumed to be paired with
 * the flag whose value is "1L << N", but that requires hard-coding.
 */

typedef struct flag_desc flag_desc;

struct flag_desc
{
	const int flag;
	const char *const desc;
};



/*
 * These are used for "+3 to STR, DEX", etc. These are separate from
 * the other pval affected traits to simplify the case where an object
 * affects all stats.  In this case, "All stats" is used instead of
 * listing each stat individually.
 */

static flag_desc stat_flags_desc[] =
{
#ifdef JP
	{ TR_STR,        "腕力" },
	{ TR_INT,        "知能" },
	{ TR_WIS,        "賢さ" },
	{ TR_DEX,        "器用さ" },
	{ TR_CON,        "耐久力" },
	{ TR_CHR,        "魅力" }
#else
	{ TR_STR,        "STR" },
	{ TR_INT,        "INT" },
	{ TR_WIS,        "WIS" },
	{ TR_DEX,        "DEX" },
	{ TR_CON,        "CON" },
	{ TR_CHR,        "CHR" }
#endif
};

/*
 * Besides stats, these are the other player traits
 * which may be affected by an object's pval
 */

static flag_desc pval_flags1_desc[] =
{
#ifdef JP
	{ TR_MAGIC_MASTERY,    "魔法道具使用能力" },
	{ TR_STEALTH,    "隠密" },
	{ TR_SEARCH,     "探索" },
	{ TR_INFRA,      "赤外線視力" },
	{ TR_TUNNEL,     "採掘" },
	{ TR_BLOWS,      "攻撃回数" },
	{ TR_SPEED,      "スピード" }
#else
	{ TR_STEALTH,    "Stealth" },
	{ TR_SEARCH,     "Searching" },
	{ TR_INFRA,      "Infravision" },
	{ TR_TUNNEL,     "Tunneling" },
	{ TR_BLOWS,      "Attacks" },
	{ TR_SPEED,      "Speed" }
#endif
};

/*
 * Slaying preferences for weapons
 */

static flag_desc slay_flags_desc[] =
{
#ifdef JP
	{ TR_SLAY_ANIMAL,        "動物" },
	{ TR_KILL_ANIMAL,        "*動物*" },
	{ TR_SLAY_EVIL,          "邪悪" },
	{ TR_KILL_EVIL,          "*邪悪*" },
	{ TR_SLAY_HUMAN,         "人間" },
	{ TR_KILL_HUMAN,         "*人間*" },
	{ TR_SLAY_UNDEAD,        "アンデッド" },
	{ TR_KILL_UNDEAD,        "*アンデッド*" },
	{ TR_SLAY_DEMON,         "悪魔" },
	{ TR_KILL_DEMON,         "*悪魔*" },
	{ TR_SLAY_ORC,           "オーク" },
	{ TR_KILL_ORC,           "*オーク*" },
	{ TR_SLAY_TROLL,         "トロル" },
	{ TR_KILL_TROLL,         "*トロル*" },
	{ TR_SLAY_GIANT,         "巨人" },
	{ TR_KILL_GIANT,         "*巨人*" },
	{ TR_SLAY_DRAGON,        "ドラゴン" },
	{ TR_KILL_DRAGON,        "*ドラゴン*" },
#else
	{ TR_SLAY_ANIMAL,        "Animal" },
	{ TR_KILL_ANIMAL,        "XAnimal" },
	{ TR_SLAY_EVIL,          "Evil" },
	{ TR_KILL_EVIL,          "XEvil" },
	{ TR_SLAY_HUMAN,         "Human" },
	{ TR_KILL_HUMAN,         "XHuman" },
	{ TR_SLAY_UNDEAD,        "Undead" },
	{ TR_KILL_UNDEAD,        "XUndead" },
	{ TR_SLAY_DEMON,         "Demon" },
	{ TR_KILL_DEMON,         "XDemon" },
	{ TR_SLAY_ORC,           "Orc" },
	{ TR_KILL_ORC,           "XOrc" },
	{ TR_SLAY_TROLL,         "Troll" },
	{ TR_KILL_TROLL,         "XTroll" },
	{ TR_SLAY_GIANT,         "Giant" },
	{ TR_KILL_GIANT,         "Xgiant" },
	{ TR_SLAY_DRAGON,        "Dragon" },
	{ TR_KILL_DRAGON,        "Xdragon" }
#endif
};

/*
 * Elemental brands for weapons
 *
 * Clearly, TR1_IMPACT is a bit out of place here. To simplify
 * coding, it has been included here along with the elemental
 * brands. It does seem to fit in with the brands and slaying
 * more than the miscellaneous section.
 */
static flag_desc brand_flags_desc[] =
{
#ifdef JP
	{ TR_BRAND_ACID,         "溶解" },
	{ TR_BRAND_ELEC,         "電撃" },
	{ TR_BRAND_FIRE,         "焼棄" },
	{ TR_BRAND_COLD,         "凍結" },
	{ TR_BRAND_POIS,         "毒殺" },

	{ TR_FORCE_WEAPON,       "理力" },
	{ TR_CHAOTIC,            "混沌" },
	{ TR_VAMPIRIC,           "吸血" },
	{ TR_IMPACT,             "地震" },
	{ TR_VORPAL,             "切れ味" },
#else
	{ TR_BRAND_ACID,         "Acid Brand" },
	{ TR_BRAND_ELEC,         "Lightning Brand" },
	{ TR_BRAND_FIRE,         "Flame Tongue" },
	{ TR_BRAND_COLD,         "Frost Brand" },
	{ TR_BRAND_POIS,         "Poisoned" },

	{ TR_FORCE_WEAPON,       "Force" },
	{ TR_CHAOTIC,            "Mark of Chaos" },
	{ TR_VAMPIRIC,           "Vampiric" },
	{ TR_IMPACT,             "Earthquake impact on hit" },
	{ TR_VORPAL,             "Very sharp" },
#endif
};


/*
 * The 15 resistables
 */
static const flag_desc resist_flags_desc[] =
{
#ifdef JP
	{ TR_RES_ACID,   "酸" },
	{ TR_RES_ELEC,   "電撃" },
	{ TR_RES_FIRE,   "火炎" },
	{ TR_RES_COLD,   "冷気" },
	{ TR_RES_POIS,   "毒" },
	{ TR_RES_FEAR,   "恐怖"},
	{ TR_RES_LITE,   "閃光" },
	{ TR_RES_DARK,   "暗黒" },
	{ TR_RES_BLIND,  "盲目" },
	{ TR_RES_CONF,   "混乱" },
	{ TR_RES_SOUND,  "轟音" },
	{ TR_RES_SHARDS, "破片" },
	{ TR_RES_NETHER, "地獄" },
	{ TR_RES_NEXUS,  "因果混乱" },
	{ TR_RES_CHAOS,  "カオス" },
	{ TR_RES_DISEN,  "劣化" },
#else
	{ TR_RES_ACID,   "Acid" },
	{ TR_RES_ELEC,   "Lightning" },
	{ TR_RES_FIRE,   "Fire" },
	{ TR_RES_COLD,   "Cold" },
	{ TR_RES_POIS,   "Poison" },
	{ TR_RES_FEAR,   "Fear"},
	{ TR_RES_LITE,   "Light" },
	{ TR_RES_DARK,   "Dark" },
	{ TR_RES_BLIND,  "Blindness" },
	{ TR_RES_CONF,   "Confusion" },
	{ TR_RES_SOUND,  "Sound" },
	{ TR_RES_SHARDS, "Shards" },
	{ TR_RES_NETHER, "Nether" },
	{ TR_RES_NEXUS,  "Nexus" },
	{ TR_RES_CHAOS,  "Chaos" },
	{ TR_RES_DISEN,  "Disenchantment" },
#endif
};

/*
 * Elemental immunities (along with poison)
 */

static const flag_desc immune_flags_desc[] =
{
#ifdef JP
	{ TR_IM_ACID,    "酸" },
	{ TR_IM_ELEC,    "電撃" },
	{ TR_IM_FIRE,    "火炎" },
	{ TR_IM_COLD,    "冷気" },
#else
	{ TR_IM_ACID,    "Acid" },
	{ TR_IM_ELEC,    "Lightning" },
	{ TR_IM_FIRE,    "Fire" },
	{ TR_IM_COLD,    "Cold" },
#endif
};

/*
 * Sustain stats -  these are given their "own" line in the
 * spoiler file, mainly for simplicity
 */
static const flag_desc sustain_flags_desc[] =
{
#ifdef JP
	{ TR_SUST_STR,   "腕力" },
	{ TR_SUST_INT,   "知能" },
	{ TR_SUST_WIS,   "賢さ" },
	{ TR_SUST_DEX,   "器用さ" },
	{ TR_SUST_CON,   "耐久力" },
	{ TR_SUST_CHR,   "魅力" },
#else
	{ TR_SUST_STR,   "STR" },
	{ TR_SUST_INT,   "INT" },
	{ TR_SUST_WIS,   "WIS" },
	{ TR_SUST_DEX,   "DEX" },
	{ TR_SUST_CON,   "CON" },
	{ TR_SUST_CHR,   "CHR" },
#endif
};

/*
 * Miscellaneous magic given by an object's "flags2" field
 */

static const flag_desc misc_flags2_desc[] =
{
#ifdef JP
	{ TR_THROW,      "投擲" },
	{ TR_REFLECT,    "反射" },
	{ TR_FREE_ACT,   "麻痺知らず" },
	{ TR_HOLD_EXP,   "経験値維持" },
#else
	{ TR_THROW,      "Throwing" },
	{ TR_REFLECT,    "Reflection" },
	{ TR_FREE_ACT,   "Free Action" },
	{ TR_HOLD_EXP,   "Hold Experience" },
#endif
};

/*
 * Miscellaneous magic given by an object's "flags3" field
 *
 * Note that cursed artifacts and objects with permanent light
 * are handled "directly" -- see analyze_misc_magic()
 */

static const flag_desc misc_flags3_desc[] =
{
#ifdef JP
	{ TR_SH_FIRE,            "火炎オーラ" },
	{ TR_SH_ELEC,            "電撃オーラ" },
	{ TR_SH_COLD,            "冷気オーラ" },
	{ TR_NO_TELE,            "反テレポート" },
	{ TR_NO_MAGIC,           "反魔法" },
	{ TR_LEVITATION,         "浮遊" },
	{ TR_SEE_INVIS,          "可視透明" },
	{ TR_TELEPATHY,          "テレパシー" },
	{ TR_ESP_ANIMAL,         "動物感知" },
	{ TR_ESP_UNDEAD,         "不死感知" },
	{ TR_ESP_DEMON,          "悪魔感知" },
	{ TR_ESP_ORC,            "オーク感知" },
	{ TR_ESP_TROLL,          "トロル感知" },
	{ TR_ESP_GIANT,          "巨人感知" },
	{ TR_ESP_DRAGON,         "ドラゴン感知" },
	{ TR_ESP_HUMAN,          "人間感知" },
	{ TR_ESP_EVIL,           "邪悪感知" },
	{ TR_ESP_GOOD,           "善良感知" },
	{ TR_ESP_NONLIVING,      "無生物感知" },
	{ TR_ESP_UNIQUE,         "ユニーク感知" },
	{ TR_SLOW_DIGEST,        "遅消化" },
	{ TR_REGEN,              "急速回復" },
	{ TR_WARNING,            "警告" },
/*	{ TR_XTRA_MIGHT,         "強力射撃" }, */
	{ TR_XTRA_SHOTS,         "追加射撃" },        /* always +1? */
	{ TR_DRAIN_EXP,          "経験値吸収" },
	{ TR_AGGRAVATE,          "反感" },
	{ TR_BLESSED,            "祝福" },
	{ TR_DEC_MANA,           "消費魔力減少" },
#else
	{ TR_SH_FIRE,            "Fiery Aura" },
	{ TR_SH_ELEC,            "Electric Aura" },
	{ TR_SH_COLD,            "Coldly Aura" },
	{ TR_NO_TELE,            "Prevent Teleportation" },
	{ TR_NO_MAGIC,           "Anti-Magic" },
	{ TR_LEVITATION,            "Levitation" },
	{ TR_SEE_INVIS,          "See Invisible" },
	{ TR_TELEPATHY,          "ESP" },
	{ TR_ESP_ANIMAL,         "Sense Animal" },
	{ TR_ESP_UNDEAD,         "Sense Undead" },
	{ TR_ESP_DEMON,          "Sense Demon" },
	{ TR_ESP_ORC,            "Sense Orc" },
	{ TR_ESP_TROLL,          "Sense Troll" },
	{ TR_ESP_GIANT,          "Sense Giant" },
	{ TR_ESP_DRAGON,         "Sense Dragon" },
	{ TR_ESP_HUMAN,          "Sense Human" },
	{ TR_ESP_EVIL,           "Sense Evil" },
	{ TR_ESP_GOOD,           "Sense Good" },
	{ TR_ESP_NONLIVING,      "Sense Nonliving" },
	{ TR_ESP_UNIQUE,         "Sense Unique" },
	{ TR_SLOW_DIGEST,        "Slow Digestion" },
	{ TR_REGEN,              "Regeneration" },
	{ TR_WARNING,            "Warning" },
/*	{ TR_XTRA_MIGHT,         "Extra Might" }, */
	{ TR_XTRA_SHOTS,         "+1 Extra Shot" },        /* always +1? */
	{ TR_DRAIN_EXP,          "Drains Experience" },
	{ TR_AGGRAVATE,          "Aggravates" },
	{ TR_BLESSED,            "Blessed Blade" },
	{ TR_DEC_MANA,           "Decrease Mana Consumption Rate" },
#endif
};


/*
 * A special type used just for deailing with pvals
 */
typedef struct
{
	/*
	 * This will contain a string such as "+2", "-10", etc.
	 */
	char pval_desc[12];

	/*
	 * A list of various player traits affected by an object's pval such
	 * as stats, speed, stealth, etc.  "Extra attacks" is NOT included in
	 * this list since it will probably be desirable to format its
	 * description differently.
	 *
	 * Note that room need only be reserved for the number of stats - 1
	 * since the description "All stats" is used if an object affects all
	 * all stats. Also, room must be reserved for a sentinel NULL pointer.
	 *
	 * This will be a list such as ["STR", "DEX", "Stealth", NULL] etc.
	 *
	 * This list includes extra attacks, for simplicity.
	 */
	cptr pval_affects[N_ELEMENTS(stat_flags_desc) - 1 +
			  N_ELEMENTS(pval_flags1_desc) + 1];

} pval_info_type;


/*
 * An "object analysis structure"
 *
 * It will be filled with descriptive strings detailing an object's
 * various magical powers. The "ignore X" traits are not noted since
 * all artifacts ignore "normal" destruction.
 */

typedef struct
{
	/* "The Longsword Dragonsmiter (6d4) (+20, +25)" */
	char description[MAX_NLEN];

	/* Description of what is affected by an object's pval */
	pval_info_type pval_info;

	/* A list of an object's slaying preferences */
	cptr slays[N_ELEMENTS(slay_flags_desc) + 1];

	/* A list if an object's elemental brands */
	cptr brands[N_ELEMENTS(brand_flags_desc) + 1];

	/* A list of immunities granted by an object */
	cptr immunities[N_ELEMENTS(immune_flags_desc) + 1];

	/* A list of resistances granted by an object */
	cptr resistances[N_ELEMENTS(resist_flags_desc) + 1];

	/* A list of stats sustained by an object */
	cptr sustains[N_ELEMENTS(sustain_flags_desc)  - 1 + 1];

	/* A list of various magical qualities an object may have */
	cptr misc_magic[N_ELEMENTS(misc_flags2_desc) + N_ELEMENTS(misc_flags3_desc)
			+ 1       /* Permanent Light */
			+ 1       /* TY curse */
			+ 1       /* type of curse */
			+ 1];     /* sentinel NULL */

	/* Additional ability or resistance */
	char addition[80];

	/* A string describing an artifact's activation */
	cptr activation;

	/* "Level 20, Rarity 30, 3.0 lbs, 20000 Gold" */
	char misc_desc[80];
} obj_desc_list;


/*!
 * @brief ファイルポインタ先に同じ文字を複数出力する /
 * Write out `n' of the character `c' to the spoiler file
 * @param n 出力する数
 * @param c 出力するキャラクタ
 * @return なし
 */
static void spoiler_out_n_chars(int n, char c)
{
	while (--n >= 0) fputc(c, fff);
}


/*!
 * @brief ファイルポインタ先に改行を複数出力する /
 * Write out `n' blank lines to the spoiler file
 * @param n 改行を出力する数
 * @return なし
 */
static void spoiler_blanklines(int n)
{
	spoiler_out_n_chars(n, '\n');
}


/*!
 * @brief ファイルポインタ先に複数のハイフンで装飾した文字列を出力する /
 * Write a line to the spoiler file and then "underline" it with hypens
 * @param str 出力したい文字列
 * @return なし
 */
static void spoiler_underline(cptr str)
{
	fprintf(fff, "%s\n", str);
	spoiler_out_n_chars(strlen(str), '-');
	fprintf(fff, "\n");
}



/*!
 * @brief アーティファクトの特性一覧を出力する /
 * Write a line to the spoiler file and then "underline" it with hypens
 * @param art_flags アーティファクトのフラグ群
 * @param flag_ptr フラグ記述情報の参照ポインタ
 * @param desc_ptr 記述内容を返すための文字列参照ポインタ
 * @param n_elmnts フラグの要素数
 * @return desc_ptrと同じアドレス
 * @details
 * <pre>
 * This function does most of the actual "analysis". Given a set of bit flags
 * (which will be from one of the flags fields from the object in question),
 * a "flag description structure", a "description list", and the number of
 * elements in the "flag description structure", this function sets the
 * "description list" members to the appropriate descriptions contained in
 * the "flag description structure".
 * The possibly updated description pointer is returned.
 * </pre>
 */
static cptr *spoiler_flag_aux(const u32b art_flags[TR_FLAG_SIZE],
			      const flag_desc *flag_ptr,
			      cptr *desc_ptr, const int n_elmnts)
{
	int i;

	for (i = 0; i < n_elmnts; ++i)
	{
		if (have_flag(art_flags, flag_ptr[i].flag))
		{
			*desc_ptr++ = flag_ptr[i].desc;
		}
	}

	return desc_ptr;
}


/*!
 * @brief アイテムの特定記述内容を返す /
 * Acquire a "basic" description "The Cloak of Death [1,+10]"
 * @param o_ptr 記述を得たいオブジェクトの参照ポインタ
 * @param desc_ptr 記述内容を返すための文字列参照ポインタ
 * @return なし
 */
static void analyze_general(object_type *o_ptr, char *desc_ptr)
{
	/* Get a "useful" description of the object */
	object_desc(desc_ptr, o_ptr, (OD_NAME_AND_ENCHANT | OD_STORE));
}


/*!
 * @brief アーティファクトがプレイヤーに与えるpval修正を構造体に収める /
 * List "player traits" altered by an artifact's pval. These include stats,
 * speed, infravision, tunneling, stealth, searching, and extra attacks.
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @param p_ptr pval修正構造体の参照ポインタ
 * @return なし
 */
static void analyze_pval(object_type *o_ptr, pval_info_type *p_ptr)
{
	u32b flgs[TR_FLAG_SIZE];

	cptr *affects_list;

	/* If pval == 0, there is nothing to do. */
	if (!o_ptr->pval)
	{
		/* An "empty" pval description indicates that pval == 0 */
		p_ptr->pval_desc[0] = '\0';
		return;
	}

	/* Extract the flags */
	object_flags(o_ptr, flgs);

	affects_list = p_ptr->pval_affects;

	/* Create the "+N" string */
	sprintf(p_ptr->pval_desc, "%s%d", POSITIZE(o_ptr->pval), o_ptr->pval);

	/* First, check to see if the pval affects all stats */
	if (have_flag(flgs, TR_STR) && have_flag(flgs, TR_INT) &&
	    have_flag(flgs, TR_WIS) && have_flag(flgs, TR_DEX) &&
	    have_flag(flgs, TR_CON) && have_flag(flgs, TR_CHR))
	{
		*affects_list++ = _("全能力", "All stats");
	}

	/* Are any stats affected? */
	else if (have_flag(flgs, TR_STR) || have_flag(flgs, TR_INT) ||
		 have_flag(flgs, TR_WIS) || have_flag(flgs, TR_DEX) ||
		 have_flag(flgs, TR_CON) || have_flag(flgs, TR_CHR))
	{
		affects_list = spoiler_flag_aux(flgs, stat_flags_desc,
						affects_list,
						N_ELEMENTS(stat_flags_desc));
	}

	/* And now the "rest" */
	affects_list = spoiler_flag_aux(flgs, pval_flags1_desc,
					affects_list,
					N_ELEMENTS(pval_flags1_desc));

	/* Terminate the description list */
	*affects_list = NULL;
}

/*!
 * @brief アーティファクトの種族スレイ特性を構造体に収める /
 * Note the slaying specialties of a weapon
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @param slay_list 種族スレイ構造体の参照ポインタ
 * @return なし
 */
static void analyze_slay(object_type *o_ptr, cptr *slay_list)
{
	u32b flgs[TR_FLAG_SIZE];

	object_flags(o_ptr, flgs);

	slay_list = spoiler_flag_aux(flgs, slay_flags_desc, slay_list,
				     N_ELEMENTS(slay_flags_desc));

	/* Terminate the description list */
	*slay_list = NULL;
}


/*!
 * @brief アーティファクトの属性ブランド特性を構造体に収める /
 * Note an object's elemental brands
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @param brand_list 属性ブランド構造体の参照ポインタ
 * @return なし
 */
static void analyze_brand(object_type *o_ptr, cptr *brand_list)
{
	u32b flgs[TR_FLAG_SIZE];

	object_flags(o_ptr, flgs);

	brand_list = spoiler_flag_aux(flgs, brand_flags_desc, brand_list,
				      N_ELEMENTS(brand_flags_desc));

	/* Terminate the description list */
	*brand_list = NULL;
}


/*!
 * @brief アーティファクトの通常耐性を構造体に収める /
 * Note an object's elemental brands
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @param resist_list 通常耐性構造体の参照ポインタ
 * @return なし
 */
static void analyze_resist(object_type *o_ptr, cptr *resist_list)
{
	u32b flgs[TR_FLAG_SIZE];

	object_flags(o_ptr, flgs);

	resist_list = spoiler_flag_aux(flgs, resist_flags_desc,
				       resist_list, N_ELEMENTS(resist_flags_desc));

	/* Terminate the description list */
	*resist_list = NULL;
}


/*!
 * @brief アーティファクトの免疫特性を構造体に収める /
 * Note the immunities granted by an object
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @param immune_list 免疫構造体の参照ポインタ
 * @return なし
 */
static void analyze_immune(object_type *o_ptr, cptr *immune_list)
{
	u32b flgs[TR_FLAG_SIZE];

	object_flags(o_ptr, flgs);

	immune_list = spoiler_flag_aux(flgs, immune_flags_desc,
				       immune_list, N_ELEMENTS(immune_flags_desc));

	/* Terminate the description list */
	*immune_list = NULL;
}


/*!
 * @brief アーティファクトの維持特性を構造体に収める /
 * Note which stats an object sustains
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @param sustain_list 維持特性構造体の参照ポインタ
 * @return なし
 */
static void analyze_sustains(object_type *o_ptr, cptr *sustain_list)
{
	u32b flgs[TR_FLAG_SIZE];

	object_flags(o_ptr, flgs);

	/* Simplify things if an item sustains all stats */
	if (have_flag(flgs, TR_SUST_STR) && have_flag(flgs, TR_SUST_INT) &&
	    have_flag(flgs, TR_SUST_WIS) && have_flag(flgs, TR_SUST_DEX) &&
	    have_flag(flgs, TR_SUST_CON) && have_flag(flgs, TR_SUST_CHR))
	{
		*sustain_list++ = _("全能力", "All stats");
	}

	/* Should we bother? */
	else if (have_flag(flgs, TR_SUST_STR) || have_flag(flgs, TR_SUST_INT) ||
		 have_flag(flgs, TR_SUST_WIS) || have_flag(flgs, TR_SUST_DEX) ||
		 have_flag(flgs, TR_SUST_CON) || have_flag(flgs, TR_SUST_CHR))
	{
		sustain_list = spoiler_flag_aux(flgs, sustain_flags_desc,
						sustain_list,
						N_ELEMENTS(sustain_flags_desc));
	}

	/* Terminate the description list */
	*sustain_list = NULL;
}


/*!
 * @brief アーティファクトのその他の特性を構造体に収める /
 * Note miscellaneous powers bestowed by an artifact such as see invisible,
 * free action, permanent light, etc.
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @param misc_list その他の特性構造体の参照ポインタ
 * @return なし
 */
static void analyze_misc_magic(object_type *o_ptr, cptr *misc_list)
{
	u32b flgs[TR_FLAG_SIZE];
	int rad;
	char desc[256];

	object_flags(o_ptr, flgs);

	misc_list = spoiler_flag_aux(flgs, misc_flags2_desc, misc_list,
				     N_ELEMENTS(misc_flags2_desc));

	misc_list = spoiler_flag_aux(flgs, misc_flags3_desc, misc_list,
				     N_ELEMENTS(misc_flags3_desc));

	/*
	 * Glowing artifacts -- small radius light.
	*/	
	rad = 0;
	if (have_flag(flgs, TR_LITE_1))  rad += 1;
	if (have_flag(flgs, TR_LITE_2))  rad += 2;
	if (have_flag(flgs, TR_LITE_3))  rad += 3;
	if (have_flag(flgs, TR_LITE_M1)) rad -= 1;
	if (have_flag(flgs, TR_LITE_M2)) rad -= 2;
	if (have_flag(flgs, TR_LITE_M3)) rad -= 3;
	
	if(o_ptr->name2 == EGO_LITE_SHINE) rad++;

	if (have_flag(flgs, TR_LITE_FUEL))
	{
		if(rad > 0) sprintf(desc, _("それは燃料補給によって明かり(半径 %d)を授ける。", "It provides light (radius %d) when fueled."), rad);	
	}
	else
	{
		if(rad > 0) sprintf(desc, _("永久光源(半径 %d)", "Permanent Light(radius %d)"), rad);	
		if(rad < 0) sprintf(desc, _("永久光源(半径-%d)。", "Permanent Light(radius -%d)"), -rad);
	}

	if(rad != 0) *misc_list++ = quark_str(quark_add(desc));

	/*
	 * Handle cursed objects here to avoid redundancies such as noting
	 * that a permanently cursed object is heavily cursed as well as
	 * being "lightly cursed".
	 */

/*	if (object_is_cursed(o_ptr)) */
	{
		if (have_flag(flgs, TR_TY_CURSE))
		{
			*misc_list++ = _("太古の怨念", "Ancient Curse");
		}
		if (o_ptr->curse_flags & TRC_PERMA_CURSE)
		{
			*misc_list++ = _("永遠の呪い", "Permanently Cursed");
		}
		else if (o_ptr->curse_flags & TRC_HEAVY_CURSE)
		{
			*misc_list++ = _("強力な呪い", "Heavily Cursed");
		}
/*		else */
		else if (o_ptr->curse_flags & TRC_CURSED)
		{
			*misc_list++ = _("呪い", "Cursed");
		}
		if (have_flag(flgs, TR_ADD_L_CURSE))
		{
			*misc_list++ = _("呪いを増やす", "Cursing");
		}
		if (have_flag(flgs, TR_ADD_H_CURSE))
		{
			*misc_list++ = _("強力な呪いを増やす", "Heavily Cursing");
		}
	}

	/* Terminate the description list */
	*misc_list = NULL;
}


/*!
 * @brief アーティファクトの追加ランダム特性を構造体に収める /
 * Note additional ability and/or resistance of fixed artifacts
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @param addition 追加ランダム耐性構造体の参照ポインタ
 * @return なし
 */
static void analyze_addition(object_type *o_ptr, char *addition)
{
	artifact_type *a_ptr = &a_info[o_ptr->name1];

	/* Init */
	strcpy(addition, "");

#ifdef JP
	if ((a_ptr->gen_flags & TRG_XTRA_POWER) && (a_ptr->gen_flags & TRG_XTRA_H_RES)) strcat(addition, "能力and耐性");
	else if (a_ptr->gen_flags & TRG_XTRA_POWER)
	{
		strcat(addition, "能力");
		if (a_ptr->gen_flags & TRG_XTRA_RES_OR_POWER) strcat(addition, "(1/2でand耐性)");
	}
	else if (a_ptr->gen_flags & TRG_XTRA_H_RES)
	{
		strcat(addition, "耐性");
		if (a_ptr->gen_flags & TRG_XTRA_RES_OR_POWER) strcat(addition, "(1/2でand能力)");
	}
	else if (a_ptr->gen_flags & TRG_XTRA_RES_OR_POWER) strcat(addition, "能力or耐性");
#else
	if ((a_ptr->gen_flags & TRG_XTRA_POWER) && (a_ptr->gen_flags & TRG_XTRA_H_RES)) strcat(addition, "Ability and Resistance");
	else if (a_ptr->gen_flags & TRG_XTRA_POWER)
	{
		strcat(addition, "Ability");
		if (a_ptr->gen_flags & TRG_XTRA_RES_OR_POWER) strcat(addition, "(plus Resistance about 1/2)");
	}
	else if (a_ptr->gen_flags & TRG_XTRA_H_RES)
	{
		strcat(addition, "Resistance");
		if (a_ptr->gen_flags & TRG_XTRA_RES_OR_POWER) strcat(addition, "(plus Ability about 1/2)");
	}
	else if (a_ptr->gen_flags & TRG_XTRA_RES_OR_POWER) strcat(addition, "Ability or Resistance");
#endif
}


/*!
 * @brief アーティファクトの基本情報を文字列に収める /
 * Determine the minimum depth an artifact can appear, its rarity, its weight,
 * and its value in gold pieces
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @param misc_desc 基本情報を収める文字列参照ポインタ
 * @return なし
 */
static void analyze_misc(object_type *o_ptr, char *misc_desc)
{
	artifact_type *a_ptr = &a_info[o_ptr->name1];

#ifdef JP
	sprintf(misc_desc, "レベル %u, 希少度 %u, %d.%d kg, ＄%ld",
		a_ptr->level, a_ptr->rarity,
		lbtokg1(a_ptr->weight), lbtokg2(a_ptr->weight), (long int)a_ptr->cost);
#else
	sprintf(misc_desc, "Level %u, Rarity %u, %d.%d lbs, %ld Gold",
		a_ptr->level, a_ptr->rarity,
		a_ptr->weight / 10, a_ptr->weight % 10, a_ptr->cost);
#endif
}


/*!
 * @brief アーティファクトの情報全体を構造体に収める /
 * Fill in an object description structure for a given object
 * and its value in gold pieces
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @param desc_ptr 全アーティファクト情報を収める文字列参照ポインタ
 * @return なし
 */
static void object_analyze(object_type *o_ptr, obj_desc_list *desc_ptr)
{
	analyze_general(o_ptr, desc_ptr->description);
	analyze_pval(o_ptr, &desc_ptr->pval_info);
	analyze_brand(o_ptr, desc_ptr->brands);
	analyze_slay(o_ptr, desc_ptr->slays);
	analyze_immune(o_ptr, desc_ptr->immunities);
	analyze_resist(o_ptr, desc_ptr->resistances);
	analyze_sustains(o_ptr, desc_ptr->sustains);
	analyze_misc_magic(o_ptr, desc_ptr->misc_magic);
	analyze_addition(o_ptr, desc_ptr->addition);
	analyze_misc(o_ptr, desc_ptr->misc_desc);
	desc_ptr->activation = item_activation(o_ptr);
}


/*!
 * @brief バッファにアーティファクト出力情報ヘッダを収める /
 * @return なし
 */
static void print_header(void)
{
	char buf[80];

	sprintf(buf, "Artifact Spoilers for Hengband Version %d.%d.%d",
		FAKE_VER_MAJOR-10, FAKE_VER_MINOR, FAKE_VER_PATCH);
	spoiler_underline(buf);
}

/*
 * This is somewhat ugly.
 *
 * Given a header ("Resist", e.g.), a list ("Fire", "Cold", Acid", e.g.),
 * and a separator character (',', e.g.), write the list to the spoiler file
 * in a "nice" format, such as:
 *
 *      Resist Fire, Cold, Acid
 *
 * That was a simple example, but when the list is long, a line wrap
 * should occur, and this should induce a new level of indention if
 * a list is being spread across lines. So for example, Bladeturner's
 * list of resistances should look something like this
 *
 *     Resist Acid, Lightning, Fire, Cold, Poison, Light, Dark, Blindness,
 *       Confusion, Sound, Shards, Nether, Nexus, Chaos, Disenchantment
 *
 * However, the code distinguishes between a single list of many items vs.
 * many lists. (The separator is used to make this determination.) A single
 * list of many items will not cause line wrapping (since there is no
 * apparent reason to do so). So the lists of Ulmo's miscellaneous traits
 * might look like this:
 *
 *     Free Action; Hold Life; See Invisible; Slow Digestion; Regeneration
 *     Blessed Blade
 *
 * So comparing the two, "Regeneration" has no trailing separator and
 * "Blessed Blade" was not indented. (Also, Ulmo's lists have no headers,
 * but that's not relevant to line wrapping and indention.)
 */

/* ITEM_SEP separates items within a list */
#define ITEM_SEP ','


/* LIST_SEP separates lists */
#ifdef JP
#define LIST_SEP ','
#else
#define LIST_SEP ';'
#endif

/*!
 * @brief フラグ名称を出力する汎用関数
 * @param header ヘッダに出力するフラグ群の名前
 * @param list フラグ名リスト
 * @param separator フラグ表示の区切り記号
 * @return なし
 */
static void spoiler_outlist(cptr header, cptr *list, char separator)
{
	int line_len, buf_len;
	char line[MAX_LINE_LEN+1], buf[80];

	/* Ignore an empty list */
	if (*list == NULL) return;

	/* This function always indents */
	strcpy(line, INDENT1);

	/* Create header (if one was given) */
	if (header && (header[0]))
	{
		strcat(line, header);
		strcat(line, " ");
	}

	line_len = strlen(line);

	/* Now begin the tedious task */
	while (1)
	{
		/* Copy the current item to a buffer */
		strcpy(buf, *list);

		/* Note the buffer's length */
		buf_len = strlen(buf);

		/*
		 * If there is an item following this one, pad with separator and
		 * a space and adjust the buffer length
		 */

		if (list[1])
		{
			sprintf(buf + buf_len, "%c ", separator);
			buf_len += 2;
		}

		/*
		 * If the buffer will fit on the current line, just append the
		 * buffer to the line and adjust the line length accordingly.
		 */

		if (line_len + buf_len <= MAX_LINE_LEN)
		{
			strcat(line, buf);
			line_len += buf_len;
		}

		/* Apply line wrapping and indention semantics described above */
		else
		{
			/*
			 * Don't print a trailing list separator but do print a trailing
			 * item separator.
			 */
			if (line_len > 1 && line[line_len - 1] == ' '
			    && line[line_len - 2] == LIST_SEP)
			{
				/* Ignore space and separator */
				line[line_len - 2] = '\0';

				/* Write to spoiler file */
				fprintf(fff, "%s\n", line);

				/* Begin new line at primary indention level */
				sprintf(line, "%s%s", INDENT1, buf);
			}

			else
			{
				/* Write to spoiler file */
				fprintf(fff, "%s\n", line);

				/* Begin new line at secondary indention level */
				sprintf(line, "%s%s", INDENT2, buf);
			}

			line_len = strlen(line);
		}

		/* Advance, with break */
		if (!*++list) break;
	}

	/* Write what's left to the spoiler file */
	fprintf(fff, "%s\n", line);
}

/*!
 * @brief アーティファクト一件をスポイラー出力する /
 * Create a spoiler file entry for an artifact
 * @param art_ptr アーティファクト情報をまとめた構造体の参照ポインタ
 * @return なし
 */
static void spoiler_print_art(obj_desc_list *art_ptr)
{
	pval_info_type *pval_ptr = &art_ptr->pval_info;

	char buf[80];

	/* Don't indent the first line */
	fprintf(fff, "%s\n", art_ptr->description);

	/* An "empty" pval description indicates that the pval affects nothing */
	if (pval_ptr->pval_desc[0])
	{
		/* Mention the effects of pval */
		sprintf(buf, _("%sの修正:", "%s to"), pval_ptr->pval_desc);
		spoiler_outlist(buf, pval_ptr->pval_affects, ITEM_SEP);
	}

	/* Now deal with the description lists */

#ifdef JP
	spoiler_outlist("対:", art_ptr->slays, ITEM_SEP);
	spoiler_outlist("武器属性:", art_ptr->brands, LIST_SEP);
	spoiler_outlist("免疫:", art_ptr->immunities, ITEM_SEP);
	spoiler_outlist("耐性:", art_ptr->resistances, ITEM_SEP);
	spoiler_outlist("維持:", art_ptr->sustains, ITEM_SEP);
#else
	spoiler_outlist("Slay", art_ptr->slays, ITEM_SEP);
	spoiler_outlist("", art_ptr->brands, LIST_SEP);
	spoiler_outlist("Immunity to", art_ptr->immunities, ITEM_SEP);
	spoiler_outlist("Resist", art_ptr->resistances, ITEM_SEP);
	spoiler_outlist("Sustain", art_ptr->sustains, ITEM_SEP);
#endif
	spoiler_outlist("", art_ptr->misc_magic, LIST_SEP);

	if (art_ptr->addition[0])
	{
		fprintf(fff, _("%s追加: %s\n", "%sAdditional %s\n"), INDENT1, art_ptr->addition);
	}

	/* Write out the possible activation at the primary indention level */
	if (art_ptr->activation)
	{
		fprintf(fff, _("%s発動: %s\n", "%sActivates for %s\n"), INDENT1, art_ptr->activation);
	}

	/* End with the miscellaneous facts */
	fprintf(fff, "%s%s\n\n", INDENT1, art_ptr->misc_desc);
}


/*!
 * @brief アーティファクト情報を出力するためにダミー生成を行う /
 * Hack -- Create a "forged" artifact
 * @param o_ptr 一時生成先を保管するオブジェクト構造体
 * @param name1 生成するアーティファクトID
 * @return 生成が成功した場合TRUEを返す
 */
static bool make_fake_artifact(object_type *o_ptr, int name1)
{
	int i;

	artifact_type *a_ptr = &a_info[name1];


	/* Ignore "empty" artifacts */
	if (!a_ptr->name) return FALSE;

	/* Acquire the "kind" index */
	i = lookup_kind(a_ptr->tval, a_ptr->sval);

	/* Oops */
	if (!i) return (FALSE);

	/* Create the artifact */
	object_prep(o_ptr, i);

	/* Save the name */
	o_ptr->name1 = name1;

	/* Extract the fields */
	o_ptr->pval = a_ptr->pval;
	o_ptr->ac = a_ptr->ac;
	o_ptr->dd = a_ptr->dd;
	o_ptr->ds = a_ptr->ds;
	o_ptr->to_a = a_ptr->to_a;
	o_ptr->to_h = a_ptr->to_h;
	o_ptr->to_d = a_ptr->to_d;
	o_ptr->weight = a_ptr->weight;

	/* Success */
	return (TRUE);
}


/*!
 * @brief アーティファクト情報のスポイラー出力を行うメインルーチン /
 * Create a spoiler file for artifacts
 * @param fname 生成ファイル名
 * @return なし
 */
static void spoil_artifact(cptr fname)
{
	int i, j;

	object_type forge;
	object_type *q_ptr;

	obj_desc_list artifact;

	char buf[1024];


	/* Build the filename */
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, fname);

	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);

	/* Open the file */
	fff = my_fopen(buf, "w");

	/* Oops */
	if (!fff)
	{
		msg_print("Cannot create spoiler file.");
		return;
	}

	/* Dump the header */
	print_header();

	/* List the artifacts by tval */
	for (i = 0; group_artifact[i].tval; i++)
	{
		/* Write out the group title */
		if (group_artifact[i].name)
		{
			spoiler_blanklines(2);
			spoiler_underline(group_artifact[i].name);
			spoiler_blanklines(1);
		}

		/* Now search through all of the artifacts */
		for (j = 1; j < max_a_idx; ++j)
		{
			artifact_type *a_ptr = &a_info[j];

			/* We only want objects in the current group */
			if (a_ptr->tval != group_artifact[i].tval) continue;

			/* Get local object */
			q_ptr = &forge;

			/* Wipe the object */
			object_wipe(q_ptr);

			/* Attempt to "forge" the artifact */
			if (!make_fake_artifact(q_ptr, j)) continue;

			/* Analyze the artifact */
			object_analyze(q_ptr, &artifact);

			/* Write out the artifact description to the spoiler file */
			spoiler_print_art(&artifact);
		}
	}

	/* Check for errors */
	if (ferror(fff) || my_fclose(fff))
	{
		msg_print("Cannot close spoiler file.");
		return;
	}

	/* Message */
	msg_print("Successfully created a spoiler file.");
}


/*!
 * @brief モンスター簡易情報のスポイラー出力を行うメインルーチン /
 * Create a spoiler file for monsters   -BEN-
 * @param fname 生成ファイル名
 * @return なし
 */
static void spoil_mon_desc(cptr fname)
{
	int i, n = 0;

	u16b why = 2;
	s16b *who;

	char buf[1024];

	char nam[80];
	char lev[80];
	char rar[80];
	char spd[80];
	char ac[80];
	char hp[80];
	char exp[80];

	/* Build the filename */
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, fname);

	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);

	/* Open the file */
	fff = my_fopen(buf, "w");

	/* Oops */
	if (!fff)
	{
		msg_print("Cannot create spoiler file.");
		return;
	}

	/* Allocate the "who" array */
	C_MAKE(who, max_r_idx, s16b);

	/* Dump the header */
	fprintf(fff, "Monster Spoilers for Hengband Version %d.%d.%d\n",
		FAKE_VER_MAJOR-10, FAKE_VER_MINOR, FAKE_VER_PATCH);
	fprintf(fff, "------------------------------------------\n\n");

	/* Dump the header */
	fprintf(fff, "    %-38.38s%4s%4s%4s%7s%5s  %11.11s\n",
		"Name", "Lev", "Rar", "Spd", "Hp", "Ac", "Visual Info");
	fprintf(fff, "%-42.42s%4s%4s%4s%7s%5s  %11.11s\n",
		"--------", "---", "---", "---", "--", "--", "-----------");


	/* Scan the monsters */
	for (i = 1; i < max_r_idx; i++)
	{
		monster_race *r_ptr = &r_info[i];

		/* Use that monster */
		if (r_ptr->name) who[n++] = i;
	}

	/* Select the sort method */
	ang_sort_comp = ang_sort_comp_hook;
	ang_sort_swap = ang_sort_swap_hook;

	/* Sort the array by dungeon depth of monsters */
	ang_sort(who, &why, n);

	/* Scan again */
	for (i = 0; i < n; i++)
	{
		monster_race *r_ptr = &r_info[who[i]];

		cptr name = (r_name + r_ptr->name);
		if (r_ptr->flags7 & (RF7_KAGE)) continue;

		/* Get the "name" */
		/*
		else if (r_ptr->flags1 & (RF1_QUESTOR))
		{
			sprintf(nam, "[Q] %s", name);
		}
		*/
		else if (r_ptr->flags1 & (RF1_UNIQUE))
		{
			sprintf(nam, "[U] %s", name);
		}
		else
		{
			sprintf(nam, _("    %s", "The %s"), name);
		}


		/* Level */
		sprintf(lev, "%d", r_ptr->level);

		/* Rarity */
		sprintf(rar, "%d", r_ptr->rarity);

		/* Speed */
		if (r_ptr->speed >= 110)
		{
			sprintf(spd, "+%d", (r_ptr->speed - 110));
		}
		else
		{
			sprintf(spd, "-%d", (110 - r_ptr->speed));
		}

		/* Armor Class */
		sprintf(ac, "%d", r_ptr->ac);

		/* Hitpoints */
		if ((r_ptr->flags1 & (RF1_FORCE_MAXHP)) || (r_ptr->hside == 1))
		{
			sprintf(hp, "%d", r_ptr->hdice * r_ptr->hside);
		}
		else
		{
			sprintf(hp, "%dd%d", r_ptr->hdice, r_ptr->hside);
		}


		/* Experience */
		sprintf(exp, "%ld", (long)(r_ptr->mexp));

		/* Hack -- use visual instead */
		sprintf(exp, "%s '%c'", attr_to_text(r_ptr), r_ptr->d_char);

		/* Dump the info */
		fprintf(fff, "%-42.42s%4s%4s%4s%7s%5s  %11.11s\n",
			nam, lev, rar, spd, hp, ac, exp);
	}

	/* End it */
	fprintf(fff, "\n");


	/* Free the "who" array */
	C_KILL(who, max_r_idx, s16b);

	/* Check for errors */
	if (ferror(fff) || my_fclose(fff))
	{
		msg_print("Cannot close spoiler file.");
		return;
	}

	/* Worked */
	msg_print("Successfully created a spoiler file.");
}




/*
 * Monster spoilers by: smchorse@ringer.cs.utsa.edu (Shawn McHorse)
 *
 * Primarily based on code already in mon-desc.c, mostly by -BEN-
 */



/*!
 * @brief 文字列をファイルポインタに出力する /
 * Buffer text to the given file. (-SHAWN-)
 * This is basically c_roff() from mon-desc.c with a few changes.
 * @param str 文字列参照ポインタ
 * @return なし
 */
static void spoil_out(cptr str)
{
	cptr r;

	/* Line buffer */
	static char roff_buf[256];

	/* Delay buffer */
	static char roff_waiting_buf[256];

#ifdef JP
	bool iskanji_flag = FALSE;
#endif
	/* Current pointer into line roff_buf */
	static char *roff_p = roff_buf;

	/* Last space saved into roff_buf */
	static char *roff_s = NULL;

	/* Mega-Hack -- Delayed output */
	static bool waiting_output = FALSE;

	/* Special handling for "new sequence" */
	if (!str)
	{
		if (waiting_output)
		{
			fputs(roff_waiting_buf, fff);
			waiting_output = FALSE;
		}

		if (roff_p != roff_buf) roff_p--;
		while (*roff_p == ' ' && roff_p != roff_buf) roff_p--;

		if (roff_p == roff_buf) fprintf(fff, "\n");
		else
		{
			*(roff_p + 1) = '\0';
			fprintf(fff, "%s\n\n", roff_buf);
		}

		roff_p = roff_buf;
		roff_s = NULL;
		roff_buf[0] = '\0';
		return;
	}

	/* Scan the given string, character at a time */
	for (; *str; str++)
	{
#ifdef JP
		char cbak;
		bool k_flag = iskanji((unsigned char)(*str));
#endif
		char ch = *str;
		bool wrap = (ch == '\n');

#ifdef JP
		if (!isprint(ch) && !k_flag && !iskanji_flag) ch = ' ';
		iskanji_flag = k_flag && !iskanji_flag;
#else
		if (!isprint(ch)) ch = ' ';
#endif

		if (waiting_output)
		{
			fputs(roff_waiting_buf, fff);
			if (!wrap) fputc('\n', fff);
			waiting_output = FALSE;
		}

		if (!wrap)
		{
#ifdef JP
			if (roff_p >= roff_buf + (k_flag ? 74 : 75)) wrap = TRUE;
			else if ((ch == ' ') && (roff_p >= roff_buf + (k_flag ? 72 : 73))) wrap = TRUE;
#else
			if (roff_p >= roff_buf + 75) wrap = TRUE;
			else if ((ch == ' ') && (roff_p >= roff_buf + 73)) wrap = TRUE;
#endif

			if (wrap)
			{
#ifdef JP
				bool k_flag_local;
				bool iskanji_flag_local = FALSE;
				cptr tail = str + (k_flag ? 2 : 1);
#else
				cptr tail = str + 1;
#endif

				for (; *tail; tail++)
				{
					if (*tail == ' ') continue;

#ifdef JP
					k_flag_local = iskanji((unsigned char)(*tail));
					if (isprint(*tail) || k_flag_local || iskanji_flag_local) break;
					iskanji_flag_local = k_flag_local && !iskanji_flag_local;
#else
					if (isprint(*tail)) break;
#endif
				}

				if (!*tail) waiting_output = TRUE;
			}
		}

		/* Handle line-wrap */
		if (wrap)
		{
			*roff_p = '\0';
			r = roff_p;
#ifdef JP
			cbak = ' ';
#endif
			if (roff_s && (ch != ' '))
			{
#ifdef JP
				cbak = *roff_s;
#endif
				*roff_s = '\0';
				r = roff_s + 1;
			}
			if (!waiting_output) fprintf(fff, "%s\n", roff_buf);
			else strcpy(roff_waiting_buf, roff_buf);
			roff_s = NULL;
			roff_p = roff_buf;
#ifdef JP
			if (cbak != ' ') *roff_p++ = cbak;
#endif
			while (*r) *roff_p++ = *r++;
		}

		/* Save the char */
		if ((roff_p > roff_buf) || (ch != ' '))
		{
#ifdef JP
			if (!k_flag)
			{
				if ((ch == ' ') || (ch == '(')) roff_s = roff_p;
			}
			else
			{
				if (iskanji_flag &&
				    strncmp(str, "。", 2) != 0 &&
				    strncmp(str, "、", 2) != 0 &&
				    strncmp(str, "ィ", 2) != 0 &&
				    strncmp(str, "ー", 2) != 0) roff_s = roff_p;
			}
#else
			if (ch == ' ') roff_s = roff_p;
#endif

			*roff_p++ = ch;
		}
	}
}



/*!
 * @brief 関数ポインタ用の出力関数 /
 * Hook function used in spoil_mon_info()
 * @param attr 未使用
 * @param str 文字列参照ポインタ
 * @return なし
 */
static void roff_func(byte attr, cptr str)
{
	/* Unused */
	(void)attr;

	spoil_out(str);
}


/*!
 * @brief モンスター詳細情報をスポイラー出力するメインルーチン /
 * Create a spoiler file for monsters (-SHAWN-)
 * @param fname ファイル名
 * @return なし
 */
static void spoil_mon_info(cptr fname)
{
	char buf[1024];
	int i, l, n = 0;
	u32b flags1;

	u16b why = 2;
	s16b *who;

	/* Build the filename */
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, fname);

	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);

	/* Open the file */
	fff = my_fopen(buf, "w");

	/* Oops */
	if (!fff)
	{
		msg_print("Cannot create spoiler file.");
		return;
	}


	/* Dump the header */
	sprintf(buf, "Monster Spoilers for Hengband Version %d.%d.%d\n",
	     FAKE_VER_MAJOR-10, FAKE_VER_MINOR, FAKE_VER_PATCH);

	spoil_out(buf);
	spoil_out("------------------------------------------\n\n");

	/* Allocate the "who" array */
	C_MAKE(who, max_r_idx, s16b);

	/* Scan the monsters */
	for (i = 1; i < max_r_idx; i++)
	{
		monster_race *r_ptr = &r_info[i];

		/* Use that monster */
		if (r_ptr->name) who[n++] = i;
	}

	/* Select the sort method */
	ang_sort_comp = ang_sort_comp_hook;
	ang_sort_swap = ang_sort_swap_hook;

	/* Sort the array by dungeon depth of monsters */
	ang_sort(who, &why, n);


	/*
	 * List all monsters in order
	 */
	for (l = 0; l < n; l++)
	{
		monster_race *r_ptr = &r_info[who[l]];

		/* Extract the flags */
		flags1 = r_ptr->flags1;

		/* Prefix */
		/*
		if (flags1 & (RF1_QUESTOR))
		{
			spoil_out("[Q] ");
		}
		else
		*/
		if (flags1 & (RF1_UNIQUE))
		{
			spoil_out("[U] ");
		}
		else
		{
#ifndef JP
			spoil_out("The ");
#endif
		}

		/* Name */
		sprintf(buf, _("%s/%s  (", "%s  ("), (r_name + r_ptr->name), _(r_name+r_ptr->E_name, ""));  /* ---)--- */

		spoil_out(buf);

		/* Color */
		spoil_out(attr_to_text(r_ptr));

		/* Symbol --(-- */
		sprintf(buf, " '%c')\n", r_ptr->d_char);
		spoil_out(buf);


		/* Indent */
		sprintf(buf, "=== ");
		spoil_out(buf);

		/* Number */
		sprintf(buf, "Num:%d  ", who[l]);
		spoil_out(buf);

		/* Level */
		sprintf(buf, "Lev:%d  ", r_ptr->level);
		spoil_out(buf);

		/* Rarity */
		sprintf(buf, "Rar:%d  ", r_ptr->rarity);
		spoil_out(buf);

		/* Speed */
		if (r_ptr->speed >= 110)
		{
			sprintf(buf, "Spd:+%d  ", (r_ptr->speed - 110));
		}
		else
		{
			sprintf(buf, "Spd:-%d  ", (110 - r_ptr->speed));
		}
		spoil_out(buf);

		/* Hitpoints */
		if ((flags1 & (RF1_FORCE_MAXHP)) || (r_ptr->hside == 1))
		{
			sprintf(buf, "Hp:%d  ", r_ptr->hdice * r_ptr->hside);
		}
		else
		{
			sprintf(buf, "Hp:%dd%d  ", r_ptr->hdice, r_ptr->hside);
		}
		spoil_out(buf);

		/* Armor Class */
		sprintf(buf, "Ac:%d  ", r_ptr->ac);
		spoil_out(buf);

		/* Experience */
		sprintf(buf, "Exp:%ld\n", (long)(r_ptr->mexp));
		spoil_out(buf);

		/* Reuse the code of monster recall. */
		output_monster_spoiler(who[l], roff_func);

		spoil_out(NULL);
	}

	/* Free the "who" array */
	C_KILL(who, max_r_idx, s16b);

	/* Check for errors */
	if (ferror(fff) || my_fclose(fff))
	{
		msg_print("Cannot close spoiler file.");
		return;
	}

	msg_print("Successfully created a spoiler file.");
}



#define MAX_EVOL_DEPTH 64


/*!
 * @brief int配列でstrncmp()と似た比較処理を行う / 
 * Compare two int-type array like strncmp() and return TRUE if equals
 * @param a 比較するint配列1
 * @param b 比較するint配列2
 * @param length 
 * @return 両者の値が等しければTRUEを返す
 */
static bool int_n_cmp(int *a, int *b, int length)
{
	/* Null-string comparation is always TRUE */
	if (!length) return TRUE;

	do
	{
		if (*a != *(b++)) return FALSE;
		if (!(*(a++))) break;
	}
	while (--length);

	return TRUE;
}


/*!
 * @brief ある木が指定された木の部分木かどうかを返す / 
 * Returns TRUE if an evolution tree is "partial tree"
 * @param tree 元となる木構造リスト
 * @param partial_tree 部分木かどうか判定したい木構造リスト
 * @return 部分木ならばTRUEを返す
 */
static bool is_partial_tree(int *tree, int *partial_tree)
{
	int pt_head = *(partial_tree++);
	int pt_len = 0;

	while (partial_tree[pt_len]) pt_len++;

	while (*tree)
	{
		if (*(tree++) == pt_head)
		{
			if (int_n_cmp(tree, partial_tree, pt_len)) return TRUE;
		}
	}

	return FALSE;
}


/*!
 * @brief 進化ツリーをソートするためモンスター種族の判定関数 / 
 * Sorting hook -- Comp function
 * @param u 進化木構造データ
 * @param v 未使用
 * @param a 比較したいモンスター種族ID1
 * @param b 比較したいモンスター種族ID2
 * @return 2が大きければTRUEを返す
 */
static bool ang_sort_comp_evol_tree(vptr u, vptr v, int a, int b)
{
	int **evol_tree = (int **)u;

	int w1 = evol_tree[a][0];
	int w2 = evol_tree[b][0];
	monster_race *r1_ptr = &r_info[w1];
	monster_race *r2_ptr = &r_info[w2];

	/* Unused */
	(void)v;

	/* Used tree first */
	if (w1 && !w2) return TRUE;
	if (!w1 && w2) return FALSE;

	/* Sort by monster level */
	if (r1_ptr->level < r2_ptr->level) return TRUE;
	if (r1_ptr->level > r2_ptr->level) return FALSE;

	/* Sort by monster experience */
	if (r1_ptr->mexp < r2_ptr->mexp) return TRUE;
	if (r1_ptr->mexp > r2_ptr->mexp) return FALSE;

	/* Compare indexes */
	return w1 <= w2;
}

/*!
 * @brief 進化ツリーをソートするため木構造のスワップ関数 / 
 * Sorting hook -- Swap function
 * @param u 進化木構造データ
 * @param v 未使用
 * @param a スワップしたい木構造1
 * @param b スワップしたい木構造2
 * @return 2が大きければTRUEを返す
 */
static void ang_sort_swap_evol_tree(vptr u, vptr v, int a, int b)
{
	int **evol_tree = (int **)u;
	int *holder;

	/* Unused */
	(void)v;

	/* Swap */
	holder = evol_tree[a];
	evol_tree[a] = evol_tree[b];
	evol_tree[b] = holder;
}

/*!
 * @brief 進化ツリーをスポイラー出力するメインルーチン / 
 * Print monsters' evolution information to file
 * @param fname 出力ファイル名
 * @return なし
 */
static void spoil_mon_evol(cptr fname)
{
	char buf[1024];
	monster_race *r_ptr;
	int **evol_tree, i, j, n, r_idx;
	int *evol_tree_zero; /* For C_KILL() */

	/* Build the filename */
	path_build(buf, sizeof buf, ANGBAND_DIR_USER, fname);

	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);

	/* Open the file */
	fff = my_fopen(buf, "w");

	/* Oops */
	if (!fff)
	{
		msg_print("Cannot create spoiler file.");
	    return;
	}

	/* Dump the header */
	sprintf(buf, "Monster Spoilers for Hengband Version %d.%d.%d\n",
	     FAKE_VER_MAJOR-10, FAKE_VER_MINOR, FAKE_VER_PATCH);

	spoil_out(buf);
	spoil_out("------------------------------------------\n\n");

	/* Allocate the "evol_tree" array (2-dimension) */
	C_MAKE(evol_tree, max_r_idx, int *);
	C_MAKE(*evol_tree, max_r_idx * (MAX_EVOL_DEPTH + 1), int);
	for (i = 1; i < max_r_idx; i++) evol_tree[i] = *evol_tree + i * (MAX_EVOL_DEPTH + 1);
	evol_tree_zero = *evol_tree;

	/* Step 1: Build the evolution tree */
	for (i = 1; i < max_r_idx; i++)
	{
		r_ptr = &r_info[i];

		/* No evolution */
		if (!r_ptr->next_exp) continue;

		/* Trace evolution */
		n = 0;
		evol_tree[i][n++] = i;
		do
		{
			evol_tree[i][n++] = r_ptr->next_r_idx;
			r_ptr = &r_info[r_ptr->next_r_idx];
		}
		while (r_ptr->next_exp && (n < MAX_EVOL_DEPTH));
	}

	/* Step 2: Scan the evolution trees and remove "partial tree" */
	for (i = 1; i < max_r_idx; i++)
	{
		/* Not evolution tree */
		if (!evol_tree[i][0]) continue;

		for (j = 1; j < max_r_idx; j++)
		{
			/* Same tree */
			if (i == j) continue;

			/* Not evolution tree */
			if (!evol_tree[j][0]) continue;

			/* Is evolution tree[i] is part of [j]? */
			if (is_partial_tree(evol_tree[j], evol_tree[i]))
			{
				/* Remove this evolution tree */
				evol_tree[i][0] = 0;
				break;
			}
		}
	}

	/* Step 3: Sort the evolution trees */

	/* Select the sort method */
	ang_sort_comp = ang_sort_comp_evol_tree;
	ang_sort_swap = ang_sort_swap_evol_tree;

	/* Sort the array */
	ang_sort(evol_tree, NULL, max_r_idx);

	/* Step 4: Print the evolution trees */
	for (i = 0; i < max_r_idx; i++)
	{
		r_idx = evol_tree[i][0];

		/* No evolution or removed evolution tree */
		if (!r_idx) continue;

		/* Trace the evolution tree */
		r_ptr = &r_info[r_idx];
#ifdef JP
		fprintf(fff, "[%d]: %s (レベル%d, '%c')\n", r_idx,
			r_name + r_ptr->name, r_ptr->level, r_ptr->d_char);
#else
		fprintf(fff, "[%d]: %s (Level %d, '%c')\n", r_idx,
			r_name + r_ptr->name, r_ptr->level, r_ptr->d_char);
#endif
		for (n = 1; r_ptr->next_exp; n++)
		{
			fprintf(fff, "%*s-(%ld)-> ", n * 2, "", (long int)r_ptr->next_exp);
			fprintf(fff, "[%d]: ", r_ptr->next_r_idx);
			r_ptr = &r_info[r_ptr->next_r_idx];
#ifdef JP
			fprintf(fff, "%s (レベル%d, '%c')\n",
				r_name + r_ptr->name, r_ptr->level, r_ptr->d_char);
#else
			fprintf(fff, "%s (Level %d, '%c')\n",
				r_name + r_ptr->name, r_ptr->level, r_ptr->d_char);
#endif
		}

		/* End of evolution tree */
		fputc('\n', fff);
	}

	/* Free the "evol_tree" array (2-dimension) */
	C_KILL(evol_tree_zero, max_r_idx * (MAX_EVOL_DEPTH + 1), int);
	C_KILL(evol_tree, max_r_idx, int *);

	/* Check for errors */
	if (ferror(fff) || my_fclose(fff))
	{
		msg_print("Cannot close spoiler file.");
		return;
	}

	/* Message */
	msg_print("Successfully created a spoiler file.");
}



/*
 * Forward declare
 */
extern void do_cmd_spoilers(void);

/*!
 * @brief スポイラー出力を行うコマンドのメインルーチン / 
 * Create Spoiler files -BEN-
 * @return なし
 */
void do_cmd_spoilers(void)
{
	/* Save the screen */
	screen_save();

	/* Interact */
	while (1)
	{
		/* Clear screen */
		Term_clear();

		/* Info */
		prt("Create a spoiler file.", 2, 0);

		/* Prompt for a file */
		prt("(1) Brief Object Info (obj-desc.spo)", 5, 5);
		prt("(2) Brief Artifact Info (artifact.spo)", 6, 5);
		prt("(3) Brief Monster Info (mon-desc.spo)", 7, 5);
		prt("(4) Full Monster Info (mon-info.spo)", 8, 5);
		prt("(5) Monster Evolution Info (mon-evol.spo)", 9, 5);

		/* Prompt */
		prt(_("コマンド:", "Command: "), _(18, 12), 0);

		/* Get a choice */
		switch (inkey())
		{
		/* Escape */
		case ESCAPE:
			/* Restore the screen */
			screen_load();
			return;

		/* Option (1) */
		case '1':
			spoil_obj_desc("obj-desc.spo");
			break;

		/* Option (2) */
		case '2':
			spoil_artifact("artifact.spo");
			break;

		/* Option (3) */
		case '3':
			spoil_mon_desc("mon-desc.spo");
			break;

		/* Option (4) */
		case '4':
			spoil_mon_info("mon-info.spo");
			break;

		/* Option (5) */
		case '5':
			spoil_mon_evol("mon-evol.spo");
			break;

		/* Oops */
		default:
			bell();
			break;
		}

		/* Flush messages */
		msg_print(NULL);
	}
}

/*!
 * @brief ランダムアーティファクト１件を解析する / 
 * Fill in an object description structure for a given object
 * @param o_ptr ランダムアーティファクトのオブジェクト構造体参照ポインタ
 * @param desc_ptr 記述内容を収める構造体参照ポインタ
 * @return なし
 */
static void random_artifact_analyze(object_type *o_ptr, obj_desc_list *desc_ptr)
{
	analyze_general(o_ptr, desc_ptr->description);
	analyze_pval(o_ptr, &desc_ptr->pval_info);
	analyze_brand(o_ptr, desc_ptr->brands);
	analyze_slay(o_ptr, desc_ptr->slays);
	analyze_immune(o_ptr, desc_ptr->immunities);
	analyze_resist(o_ptr, desc_ptr->resistances);
	analyze_sustains(o_ptr, desc_ptr->sustains);
	analyze_misc_magic(o_ptr, desc_ptr->misc_magic);
	desc_ptr->activation = item_activation(o_ptr);
#ifdef JP
	sprintf(desc_ptr->misc_desc, "重さ %d.%d kg",
		lbtokg1(o_ptr->weight), lbtokg2(o_ptr->weight));
#else
	sprintf(desc_ptr->misc_desc, "Weight %d.%d lbs",
		o_ptr->weight / 10, o_ptr->weight % 10);
#endif
}

/*!
 * @brief ランダムアーティファクト１件をスポイラー出力する / 
 * Create a spoiler file entry for an artifact
 * @param o_ptr ランダムアーティファクトのオブジェクト構造体参照ポインタ
 * @param art_ptr 記述内容を収めた構造体参照ポインタ
 * Fill in an object description structure for a given object
 * @return なし
 */
static void spoiler_print_randart(object_type *o_ptr, obj_desc_list *art_ptr)
{
	pval_info_type *pval_ptr = &art_ptr->pval_info;

	char buf[80];

	/* Don't indent the first line */
	fprintf(fff, "%s\n", art_ptr->description);
	
	/* unidentified */
	if (!(o_ptr->ident & (IDENT_MENTAL)))
	{
		fprintf(fff, _("%s不明\n", "%sUnknown\n"),INDENT1);
	}
	else {
		/* An "empty" pval description indicates that the pval affects nothing */
		if (pval_ptr->pval_desc[0])
		{
			/* Mention the effects of pval */
			sprintf(buf, _("%sの修正:", "%s to"), pval_ptr->pval_desc);
			spoiler_outlist(buf, pval_ptr->pval_affects, ITEM_SEP);
		}

		/* Now deal with the description lists */

#ifdef JP
		spoiler_outlist("対:", art_ptr->slays, ITEM_SEP);
		spoiler_outlist("武器属性:", art_ptr->brands, LIST_SEP);
		spoiler_outlist("免疫:", art_ptr->immunities, ITEM_SEP);
		spoiler_outlist("耐性:", art_ptr->resistances, ITEM_SEP);
		spoiler_outlist("維持:", art_ptr->sustains, ITEM_SEP);
#else
		spoiler_outlist("Slay", art_ptr->slays, ITEM_SEP);
		spoiler_outlist("", art_ptr->brands, LIST_SEP);
		spoiler_outlist("Immunity to", art_ptr->immunities, ITEM_SEP);
		spoiler_outlist("Resist", art_ptr->resistances, ITEM_SEP);
		spoiler_outlist("Sustain", art_ptr->sustains, ITEM_SEP);
#endif
		spoiler_outlist("", art_ptr->misc_magic, LIST_SEP);

		/* Write out the possible activation at the primary indention level */
		if (art_ptr->activation)
		{
			fprintf(fff, _("%s発動: %s\n", "%sActivates for %s\n"), INDENT1, art_ptr->activation);
		}
	}
	/* End with the miscellaneous facts */
	fprintf(fff, "%s%s\n\n", INDENT1, art_ptr->misc_desc);
}


/*!
 * @brief ランダムアーティファクト内容をスポイラー出力するサブルーチン / 
 * @param o_ptr ランダムアーティファクトのオブジェクト構造体参照ポインタ
 * @param i 出力したい記録ランダムアーティファクトID
 * @return なし
 */
static void spoil_random_artifact_aux(object_type *o_ptr, int i)
{
	obj_desc_list artifact;

	if (!object_is_known(o_ptr) || !o_ptr->art_name
		|| o_ptr->tval != group_artifact[i].tval)
		return;

	/* Analyze the artifact */
	random_artifact_analyze(o_ptr, &artifact);

	/* Write out the artifact description to the spoiler file */
	spoiler_print_randart(o_ptr, &artifact);
}

/*!
 * @brief ランダムアーティファクト内容をスポイラー出力するメインルーチン / 
 * Create a list file for random artifacts
 * @param fname 出力ファイル名
 * @return なし
 */
void spoil_random_artifact(cptr fname)
{
	int i,j;

	store_type  *st_ptr;
	object_type *q_ptr;

	char buf[1024];


	/* Build the filename */
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, fname);

	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);

	/* Open the file */
	fff = my_fopen(buf, "w");

	/* Oops */
	if (!fff)
	{
		msg_print("Cannot create list file.");
		return;
	}

	/* Dump the header */
	sprintf(buf, "Random artifacts list.\r");
	spoiler_underline(buf);

	/* List the artifacts by tval */
	for (j = 0; group_artifact[j].tval; j++)
	{
		/* random artifacts wielding */
		for (i = INVEN_RARM; i < INVEN_TOTAL; i++)
		{
			q_ptr = &inventory[i];
			spoil_random_artifact_aux(q_ptr, j);
		}

		/* random artifacts in inventory */
		for (i = 0; i < INVEN_PACK; i++)
		{
			q_ptr = &inventory[i];
			spoil_random_artifact_aux(q_ptr, j);
		}

		/* random artifacts in home */
		st_ptr = &town[1].store[STORE_HOME];
		for (i = 0; i < st_ptr->stock_num; i++)
		{
			q_ptr = &st_ptr->stock[i];
			spoil_random_artifact_aux(q_ptr, j);
		}

		/* random artifacts in museum */
		st_ptr = &town[1].store[STORE_MUSEUM];
		for (i = 0; i < st_ptr->stock_num; i++)
		{
			q_ptr = &st_ptr->stock[i];
			spoil_random_artifact_aux(q_ptr, j);
		}
	}

	/* Check for errors */
	if (ferror(fff) || my_fclose(fff))
	{
		msg_print("Cannot close list file.");
		return;
	}

	/* Message */
	msg_print("Successfully created a list file.");
}

#else

#ifdef MACINTOSH
static int i = 0;
#endif /* MACINTOSH */

#endif
