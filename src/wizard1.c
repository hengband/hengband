/* File: wizard1.c */

/* Purpose: Spoiler generation -BEN- */

#include "angband.h"


#ifdef ALLOW_SPOILERS


/*
 * The spoiler file being created
 */
static FILE *fff = NULL;



/*
 * Extract a textual representation of an attribute
 */
static cptr attr_to_text(byte a)
{
	switch (a)
	{
#ifdef JP000
case TERM_DARK:    return ("XXXい");
case TERM_WHITE:   return ("白い");
case TERM_SLATE:   return ("青灰色の");
case TERM_ORANGE:  return ("オレンジの");
case TERM_RED:     return ("赤い");
case TERM_GREEN:   return ("緑の");
case TERM_BLUE:    return ("青い");
case TERM_UMBER:   return ("琥珀色の");
case TERM_L_DARK:  return ("灰色の");
case TERM_L_WHITE: return ("明青灰色の");
case TERM_VIOLET:  return ("紫の");
case TERM_YELLOW:  return ("黄色い");
case TERM_L_RED:   return ("明い赤の");
case TERM_L_GREEN: return ("明い緑の");
case TERM_L_BLUE:  return ("明い青の");
case TERM_L_UMBER: return ("明い琥珀色の");
#else
		case TERM_DARK:    return ("xxx");
		case TERM_WHITE:   return ("White");
		case TERM_SLATE:   return ("Slate");
		case TERM_ORANGE:  return ("Orange");
		case TERM_RED:     return ("Red");
		case TERM_GREEN:   return ("Green");
		case TERM_BLUE:    return ("Blue");
		case TERM_UMBER:   return ("Umber");
		case TERM_L_DARK:  return ("L.Dark");
		case TERM_L_WHITE: return ("L.Slate");
		case TERM_VIOLET:  return ("Violet");
		case TERM_YELLOW:  return ("Yellow");
		case TERM_L_RED:   return ("L.Red");
		case TERM_L_GREEN: return ("L.Green");
		case TERM_L_BLUE:  return ("L.Blue");
		case TERM_L_UMBER: return ("L.Umber");
#endif

	}

	/* Oops */
#ifdef JP000
return ("変な");
#else
	return ("Icky");
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
#ifdef JP
{ TV_SHOT,          "射撃物" },
#else
	{ TV_SHOT,          "Ammo" },
#endif

	{ TV_ARROW,         NULL },
	{ TV_BOLT,          NULL },

#ifdef JP
{ TV_BOW,           "弓" },
#else
	{ TV_BOW,           "Bows" },
#endif


#ifdef JP
{ TV_SWORD,         "武器" },
#else
	{ TV_SWORD,         "Weapons" },
#endif

	{ TV_POLEARM,       NULL },
	{ TV_HAFTED,        NULL },
	{ TV_DIGGING,       NULL },

#ifdef JP
{ TV_SOFT_ARMOR,    "防具（体）" },
#else
	{ TV_SOFT_ARMOR,    "Armour (Body)" },
#endif

	{ TV_HARD_ARMOR,    NULL },
	{ TV_DRAG_ARMOR,    NULL },

#ifdef JP
{ TV_CLOAK,         "防具（その他）" },
#else
	{ TV_CLOAK,         "Armour (Misc)" },
#endif

	{ TV_SHIELD,        NULL },
	{ TV_HELM,          NULL },
	{ TV_CROWN,         NULL },
	{ TV_GLOVES,        NULL },
	{ TV_BOOTS,         NULL },

#ifdef JP
{ TV_AMULET,        "アミュレット" },
{ TV_RING,          "指輪" },
#else
	{ TV_AMULET,        "Amulets" },
	{ TV_RING,          "Rings" },
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
{ TV_ROD,           "ロッド" },
{ TV_WAND,          "魔法棒" },
{ TV_STAFF,         "杖" },
#else
	{ TV_ROD,           "Rods" },
	{ TV_WAND,          "Wands" },
	{ TV_STAFF,         "Staffs" },
#endif


#ifdef JP
{ TV_LIFE_BOOK,     "魔法書（生命）" },
{ TV_SORCERY_BOOK,  "魔法書（仙術）" },
{ TV_NATURE_BOOK,   "魔法書（自然）" },
{ TV_CHAOS_BOOK,    "魔法書（カオス）" },
{ TV_DEATH_BOOK,    "魔法書（暗黒）" },
{ TV_TRUMP_BOOK,    "魔法書（トランプ）" },
{ TV_ARCANE_BOOK,   "魔法書（秘術）" },
{ TV_ENCHANT_BOOK,  "魔法書（匠）" },
{ TV_DAEMON_BOOK,   "魔法書（悪魔）" },
{ TV_MUSIC_BOOK,    "歌集" },
{ TV_HISSATSU_BOOK, "武芸の書" },
#else
	{ TV_LIFE_BOOK,     "Books (Life)" },
	{ TV_SORCERY_BOOK,  "Books (Sorcery)" },
	{ TV_NATURE_BOOK,   "Books (Nature)" },
	{ TV_CHAOS_BOOK,    "Books (Chaos)" },
	{ TV_DEATH_BOOK,    "Books (Death)" },
	{ TV_TRUMP_BOOK,    "Books (Trump)" },
	{ TV_ARCANE_BOOK,   "Books (Arcane)" },
	{ TV_ENCHANT_BOOK,  "Books (Craft)" },
	{ TV_DAEMON_BOOK,   "Books (Daemon)" },
	{ TV_MUSIC_BOOK,    "Song Books" },
	{ TV_HISSATSU_BOOK, "Books (Kendo)" },
#endif

#ifdef JP
{ TV_PARCHEMENT,    "羊皮紙" },
#else
{ TV_PARCHEMENT,    "Parchement" },
#endif

#ifdef JP
{ TV_CHEST,         "箱" },
#else
	{ TV_CHEST,         "Chests" },
#endif

#ifdef JP
{ TV_CAPTURE,         "キャプチャー・ボール" },
#else
	{ TV_CAPTURE,         "Capture Ball" },
#endif

#ifdef JP
{ TV_CARD,         "エクスプレスカード" },
#else
	{ TV_CARD,         "Express Card" },
#endif

	{ TV_FIGURINE,      "Magical Figurines" },
	{ TV_STATUE,        "Statues" },
	{ TV_CORPSE,        "Corpses" },

#ifdef JP
{ TV_WHISTLE,         "笛" },
#else
	{ TV_WHISTLE,         "Whistle" },
#endif

#ifdef JP
{ TV_SPIKE,         "くさび" },
#else
	{ TV_SPIKE,         "Spike" },
#endif

	{ TV_LITE,          NULL },
	{ TV_FLASK,         NULL },
	{ TV_JUNK,          NULL },
	{ TV_BOTTLE,        NULL },
	{ TV_SKELETON,      NULL },

	{ 0, "" }
};


/*
 * Describe the kind
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
	(*lev) = get_object_level(q_ptr);

	/* Value */
	(*val) = object_value(q_ptr);


	/* Hack */
	if (!buf || !dam || !wgt) return;


	/* Description (too brief) */
	object_desc_store(buf, q_ptr, FALSE, 0);


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


/*
 * Create a spoiler file for items
 */
static void spoil_obj_desc(cptr fname)
{
	int i, k, s, t, n = 0;

	u16b who[200];

	char buf[1024];

	char wgt[80];
	char dam[80];


	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_USER, fname);

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

			/* Notice the end */
			if (!group_item[i].tval) break;

			/* Start a new set */
			fprintf(fff, "\n\n%s\n\n", group_item[i].name);
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
	const u32b flag;
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
	{ TR1_STR,        "腕力" },
	{ TR1_INT,        "知能" },
	{ TR1_WIS,        "賢さ" },
	{ TR1_DEX,        "器用さ" },
	{ TR1_CON,        "耐久力" },
	{ TR1_CHR,        "魅力" }
#else
	{ TR1_STR,        "STR" },
	{ TR1_INT,        "INT" },
	{ TR1_WIS,        "WIS" },
	{ TR1_DEX,        "DEX" },
	{ TR1_CON,        "CON" },
	{ TR1_CHR,        "CHR" }
#endif
};

/*
 * Besides stats, these are the other player traits
 * which may be affected by an object's pval
 */

static flag_desc pval_flags1_desc[] =
{
#ifdef JP
	{ TR1_MAGIC_MASTERY,    "魔法道具使用能力" },
	{ TR1_STEALTH,    "隠密" },
	{ TR1_SEARCH,     "探索" },
	{ TR1_INFRA,      "赤外線視力" },
	{ TR1_TUNNEL,     "採掘" },
	{ TR1_BLOWS,      "攻撃回数" },
	{ TR1_SPEED,      "スピード" }
#else
	{ TR1_STEALTH,    "Stealth" },
	{ TR1_SEARCH,     "Searching" },
	{ TR1_INFRA,      "Infravision" },
	{ TR1_TUNNEL,     "Tunneling" },
	{ TR1_BLOWS,      "Attacks" },
	{ TR1_SPEED,      "Speed" }
#endif
};

/*
 * Slaying preferences for weapons
 */

static flag_desc slay_flags_desc[] =
{
#ifdef JP
	{ TR1_SLAY_ANIMAL,        "動物" },
	{ TR1_SLAY_EVIL,          "邪悪" },
	{ TR1_SLAY_UNDEAD,        "アンデッド" },
	{ TR1_SLAY_DEMON,         "悪魔" },
	{ TR1_SLAY_ORC,           "オーク" },
	{ TR1_SLAY_TROLL,         "トロル" },
	{ TR1_SLAY_GIANT,         "巨人" },
	{ TR1_SLAY_DRAGON,        "ドラゴン" },
	{ TR1_KILL_DRAGON,        "*ドラゴン*" },
#else
	{ TR1_SLAY_ANIMAL,        "Animal" },
	{ TR1_SLAY_EVIL,          "Evil" },
	{ TR1_SLAY_UNDEAD,        "Undead" },
	{ TR1_SLAY_DEMON,         "Demon" },
	{ TR1_SLAY_ORC,           "Orc" },
	{ TR1_SLAY_TROLL,         "Troll" },
	{ TR1_SLAY_GIANT,         "Giant" },
	{ TR1_SLAY_DRAGON,        "Dragon" },
	{ TR1_KILL_DRAGON,        "Xdragon" }
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
	{ TR1_BRAND_ACID,         "溶解" },
	{ TR1_BRAND_ELEC,         "電撃" },
	{ TR1_BRAND_FIRE,         "焼棄" },
	{ TR1_BRAND_COLD,         "凍結" },
	{ TR1_BRAND_POIS,         "毒殺" },

	{ TR1_FORCE_WEAPON,       "理力" },
	{ TR1_CHAOTIC,            "混沌" },
	{ TR1_VAMPIRIC,           "吸血" },
	{ TR1_IMPACT,             "地震" },
	{ TR1_VORPAL,             "切れ味" },
#else
	{ TR1_BRAND_ACID,         "Acid Brand" },
	{ TR1_BRAND_ELEC,         "Lightning Brand" },
	{ TR1_BRAND_FIRE,         "Flame Tongue" },
	{ TR1_BRAND_COLD,         "Frost Brand" },
	{ TR1_BRAND_POIS,         "Poisoned" },

	{ TR1_FORCE_WEAPON,       "Force" },
	{ TR1_CHAOTIC,            "Mark of Chaos" },
	{ TR1_VAMPIRIC,           "Vampiric" },
	{ TR1_IMPACT,             "Earthquake impact on hit" },
	{ TR1_VORPAL,             "Very sharp" },
#endif
};


/*
 * The 15 resistables
 */
static const flag_desc resist_flags_desc[] =
{
#ifdef JP
	{ TR2_RES_ACID,   "酸" },
	{ TR2_RES_ELEC,   "電撃" },
	{ TR2_RES_FIRE,   "火炎" },
	{ TR2_RES_COLD,   "冷気" },
	{ TR2_RES_POIS,   "毒" },
	{ TR2_RES_FEAR,   "恐怖"},
	{ TR2_RES_LITE,   "閃光" },
	{ TR2_RES_DARK,   "暗黒" },
	{ TR2_RES_BLIND,  "盲目" },
	{ TR2_RES_CONF,   "混乱" },
	{ TR2_RES_SOUND,  "轟音" },
	{ TR2_RES_SHARDS, "破片" },
	{ TR2_RES_NETHER, "地獄" },
	{ TR2_RES_NEXUS,  "因果混乱" },
	{ TR2_RES_CHAOS,  "カオス" },
	{ TR2_RES_DISEN,  "劣化" },
#else
	{ TR2_RES_ACID,   "Acid" },
	{ TR2_RES_ELEC,   "Lightning" },
	{ TR2_RES_FIRE,   "Fire" },
	{ TR2_RES_COLD,   "Cold" },
	{ TR2_RES_POIS,   "Poison" },
	{ TR2_RES_FEAR,   "Fear"},
	{ TR2_RES_LITE,   "Light" },
	{ TR2_RES_DARK,   "Dark" },
	{ TR2_RES_BLIND,  "Blindness" },
	{ TR2_RES_CONF,   "Confusion" },
	{ TR2_RES_SOUND,  "Sound" },
	{ TR2_RES_SHARDS, "Shards" },
	{ TR2_RES_NETHER, "Nether" },
	{ TR2_RES_NEXUS,  "Nexus" },
	{ TR2_RES_CHAOS,  "Chaos" },
	{ TR2_RES_DISEN,  "Disenchantment" },
#endif
};

/*
 * Elemental immunities (along with poison)
 */

static const flag_desc immune_flags_desc[] =
{
#ifdef JP
	{ TR2_IM_ACID,    "酸" },
	{ TR2_IM_ELEC,    "電撃" },
	{ TR2_IM_FIRE,    "火炎" },
	{ TR2_IM_COLD,    "冷気" },
#else
	{ TR2_IM_ACID,    "Acid" },
	{ TR2_IM_ELEC,    "Lightning" },
	{ TR2_IM_FIRE,    "Fire" },
	{ TR2_IM_COLD,    "Cold" },
#endif
};

/*
 * Sustain stats -  these are given their "own" line in the
 * spoiler file, mainly for simplicity
 */
static const flag_desc sustain_flags_desc[] =
{
#ifdef JP
	{ TR2_SUST_STR,   "腕力" },
	{ TR2_SUST_INT,   "知能" },
	{ TR2_SUST_WIS,   "賢さ" },
	{ TR2_SUST_DEX,   "器用さ" },
	{ TR2_SUST_CON,   "耐久力" },
	{ TR2_SUST_CHR,   "魅力" },
#else
	{ TR2_SUST_STR,   "STR" },
	{ TR2_SUST_INT,   "INT" },
	{ TR2_SUST_WIS,   "WIS" },
	{ TR2_SUST_DEX,   "DEX" },
	{ TR2_SUST_CON,   "CON" },
	{ TR2_SUST_CHR,   "CHR" },
#endif
};

/*
 * Miscellaneous magic given by an object's "flags2" field
 */

static const flag_desc misc_flags2_desc[] =
{
#ifdef JP
	{ TR2_THROW,      "投擲" },
	{ TR2_REFLECT,    "反射" },
	{ TR2_FREE_ACT,   "麻痺知らず" },
	{ TR2_HOLD_LIFE,  "生命力維持" },
#else
	{ TR2_THROW,      "Throwing" },
	{ TR2_REFLECT,    "Reflection" },
	{ TR2_FREE_ACT,   "Free Action" },
	{ TR2_HOLD_LIFE,  "Hold Life" },
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
	{ TR3_SH_FIRE,            "火炎オーラ" },
	{ TR3_SH_ELEC,            "電撃オーラ" },
	{ TR3_SH_COLD,            "冷気オーラ" },
	{ TR3_NO_TELE,            "反テレポート" },
	{ TR3_NO_MAGIC,           "反魔法" },
	{ TR3_FEATHER,            "浮遊" },
	{ TR3_SEE_INVIS,          "可視透明" },
	{ TR3_TELEPATHY,          "テレパシー" },
	{ TR3_SLOW_DIGEST,        "遅消化" },
	{ TR3_REGEN,              "急速回復" },
	{ TR3_WARNING,            "警告" },
/*	{ TR3_XTRA_MIGHT,         "強力射撃" }, */
	{ TR3_XTRA_SHOTS,         "追加射撃" },        /* always +1? */
	{ TR3_DRAIN_EXP,          "経験値吸収" },
	{ TR3_AGGRAVATE,          "反感" },
	{ TR3_BLESSED,            "祝福" },
	{ TR3_DEC_MANA,           "消費魔力減少" },
#else
	{ TR3_SH_FIRE,            "Fiery Aura" },
	{ TR3_SH_ELEC,            "Electric Aura" },
	{ TR3_SH_COLD,            "Coldly Aura" },
	{ TR3_NO_TELE,            "Prevent Teleportation" },
	{ TR3_NO_MAGIC,           "Anti-Magic" },
	{ TR3_FEATHER,            "Levitation" },
	{ TR3_SEE_INVIS,          "See Invisible" },
	{ TR3_TELEPATHY,          "ESP" },
	{ TR3_SLOW_DIGEST,        "Slow Digestion" },
	{ TR3_REGEN,              "Regeneration" },
	{ TR3_WARNING,            "Warning" },
/*	{ TR3_XTRA_MIGHT,         "Extra Might" }, */
	{ TR3_XTRA_SHOTS,         "+1 Extra Shot" },        /* always +1? */
	{ TR3_DRAIN_EXP,          "Drains Experience" },
	{ TR3_AGGRAVATE,          "Aggravates" },
	{ TR3_BLESSED,            "Blessed Blade" },
	{ TR3_DEC_MANA,           "Decrease Shouhi Mana" },
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

	/* A string describing an artifact's activation */
	cptr activation;

	/* "Level 20, Rarity 30, 3.0 lbs, 20000 Gold" */
	char misc_desc[80];
} obj_desc_list;


/*
 * Write out `n' of the character `c' to the spoiler file
 */
static void spoiler_out_n_chars(int n, char c)
{
	while (--n >= 0) fputc(c, fff);
}


/*
 * Write out `n' blank lines to the spoiler file
 */
static void spoiler_blanklines(int n)
{
	spoiler_out_n_chars(n, '\n');
}


/*
 * Write a line to the spoiler file and then "underline" it with hypens
 */
static void spoiler_underline(cptr str)
{
	fprintf(fff, "%s\n", str);
	spoiler_out_n_chars(strlen(str), '-');
	fprintf(fff, "\n");
}



/*
 * This function does most of the actual "analysis". Given a set of bit flags
 * (which will be from one of the flags fields from the object in question),
 * a "flag description structure", a "description list", and the number of
 * elements in the "flag description structure", this function sets the
 * "description list" members to the appropriate descriptions contained in
 * the "flag description structure".
 *
 * The possibly updated description pointer is returned.
 */
static cptr *spoiler_flag_aux(const u32b art_flags, const flag_desc *flag_ptr,
			      cptr *desc_ptr, const int n_elmnts)
{
	int i;

	for (i = 0; i < n_elmnts; ++i)
	{
		if (art_flags & flag_ptr[i].flag)
		{
			*desc_ptr++ = flag_ptr[i].desc;
		}
	}

	return desc_ptr;
}


/*
 * Acquire a "basic" description "The Cloak of Death [1,+10]"
 */
static void analyze_general (object_type *o_ptr, char *desc_ptr)
{
	/* Get a "useful" description of the object */
	object_desc_store(desc_ptr, o_ptr, TRUE, 1);
}


/*
 * List "player traits" altered by an artifact's pval. These include stats,
 * speed, infravision, tunneling, stealth, searching, and extra attacks.
 */
static void analyze_pval (object_type *o_ptr, pval_info_type *p_ptr)
{
	const u32b all_stats = (TR1_STR | TR1_INT | TR1_WIS |
				TR1_DEX | TR1_CON | TR1_CHR);

	u32b f1, f2, f3;

	cptr *affects_list;

	/* If pval == 0, there is nothing to do. */
	if (!o_ptr->pval)
	{
		/* An "empty" pval description indicates that pval == 0 */
		p_ptr->pval_desc[0] = '\0';
		return;
	}

	/* Extract the flags */
	object_flags(o_ptr, &f1, &f2, &f3);

	affects_list = p_ptr->pval_affects;

	/* Create the "+N" string */
	sprintf(p_ptr->pval_desc, "%s%d", POSITIZE(o_ptr->pval), o_ptr->pval);

	/* First, check to see if the pval affects all stats */
	if ((f1 & all_stats) == all_stats)
	{
#ifdef JP
		*affects_list++ = "全能力";
#else
		*affects_list++ = "All stats";
#endif
	}

	/* Are any stats affected? */
	else if (f1 & all_stats)
	{
		affects_list = spoiler_flag_aux(f1, stat_flags_desc,
						affects_list,
						N_ELEMENTS(stat_flags_desc));
	}

	/* And now the "rest" */
	affects_list = spoiler_flag_aux(f1, pval_flags1_desc,
					affects_list,
					N_ELEMENTS(pval_flags1_desc));

	/* Terminate the description list */
	*affects_list = NULL;
}


/* Note the slaying specialties of a weapon */
static void analyze_slay (object_type *o_ptr, cptr *slay_list)
{
	u32b f1, f2, f3;

	object_flags(o_ptr, &f1, &f2, &f3);

	slay_list = spoiler_flag_aux(f1, slay_flags_desc, slay_list,
				     N_ELEMENTS(slay_flags_desc));

	/* Terminate the description list */
	*slay_list = NULL;
}

/* Note an object's elemental brands */
static void analyze_brand (object_type *o_ptr, cptr *brand_list)
{
	u32b f1, f2, f3;

	object_flags(o_ptr, &f1, &f2, &f3);

	brand_list = spoiler_flag_aux(f1, brand_flags_desc, brand_list,
				      N_ELEMENTS(brand_flags_desc));

	/* Terminate the description list */
	*brand_list = NULL;
}


/* Note the resistances granted by an object */
static void analyze_resist (object_type *o_ptr, cptr *resist_list)
{
	u32b f1, f2, f3;

	object_flags(o_ptr, &f1, &f2, &f3);

	resist_list = spoiler_flag_aux(f2, resist_flags_desc,
				       resist_list, N_ELEMENTS(resist_flags_desc));

	/* Terminate the description list */
	*resist_list = NULL;
}


/* Note the immunities granted by an object */
static void analyze_immune (object_type *o_ptr, cptr *immune_list)
{
	u32b f1, f2, f3;

	object_flags(o_ptr, &f1, &f2, &f3);

	immune_list = spoiler_flag_aux(f2, immune_flags_desc,
				       immune_list, N_ELEMENTS(immune_flags_desc));

	/* Terminate the description list */
	*immune_list = NULL;
}

/* Note which stats an object sustains */

static void analyze_sustains (object_type *o_ptr, cptr *sustain_list)
{
	const u32b all_sustains = (TR2_SUST_STR | TR2_SUST_INT | TR2_SUST_WIS |
				   TR2_SUST_DEX | TR2_SUST_CON | TR2_SUST_CHR);

	u32b f1, f2, f3;

	object_flags(o_ptr, &f1, &f2, &f3);

	/* Simplify things if an item sustains all stats */
	if ((f2 & all_sustains) == all_sustains)
	{
#ifdef JP
		*sustain_list++ = "全能力";
#else
		*sustain_list++ = "All stats";
#endif
	}

	/* Should we bother? */
	else if ((f2 & all_sustains))
	{
		sustain_list = spoiler_flag_aux(f2, sustain_flags_desc,
						sustain_list,
						N_ELEMENTS(sustain_flags_desc));
	}

	/* Terminate the description list */
	*sustain_list = NULL;
}


/*
 * Note miscellaneous powers bestowed by an artifact such as see invisible,
 * free action, permanent light, etc.
 */
static void analyze_misc_magic (object_type *o_ptr, cptr *misc_list)
{
	u32b f1, f2, f3;

	object_flags(o_ptr, &f1, &f2, &f3);

	misc_list = spoiler_flag_aux(f2, misc_flags2_desc, misc_list,
				     N_ELEMENTS(misc_flags2_desc));

	misc_list = spoiler_flag_aux(f3, misc_flags3_desc, misc_list,
				     N_ELEMENTS(misc_flags3_desc));

	/*
	 * Artifact lights -- large radius light.
	 */
	if ((o_ptr->tval == TV_LITE) && artifact_p(o_ptr))
	{
#ifdef JP
		*misc_list++ = "永久光源(半径3)";
#else
		*misc_list++ = "Permanent Light(3)";
#endif
	}

	/*
	 * Glowing artifacts -- small radius light.
	 */
	if (f3 & (TR3_LITE))
	{
#ifdef JP
		*misc_list++ = "永久光源(半径1)";
#else
		*misc_list++ = "Permanent Light(1)";
#endif
	}

	/*
	 * Handle cursed objects here to avoid redundancies such as noting
	 * that a permanently cursed object is heavily cursed as well as
	 * being "lightly cursed".
	 */

/*	if (cursed_p(o_ptr)) */
	if (1)
	{
		if (f3 & TR3_TY_CURSE)
		{
#ifdef JP
			*misc_list++ = "太古の怨念";
#else
			*misc_list++ = "Ancient Curse";
#endif
		}
		if (o_ptr->curse_flags & TRC_PERMA_CURSE)
		{
#ifdef JP
			*misc_list++ = "永遠の呪い";
#else
			*misc_list++ = "Permanently Cursed";
#endif
		}
		else if (o_ptr->curse_flags & TRC_HEAVY_CURSE)
		{
#ifdef JP
			*misc_list++ = "強力な呪い";
#else
			*misc_list++ = "Heavily Cursed";
#endif
		}
/*		else */
		else if (o_ptr->curse_flags & TRC_CURSED)
		{
#ifdef JP
			*misc_list++ = "呪い";
#else
			*misc_list++ = "Cursed";
#endif
		}
	}

	/* Terminate the description list */
	*misc_list = NULL;
}




/*
 * Determine the minimum depth an artifact can appear, its rarity, its weight,
 * and its value in gold pieces
 */
static void analyze_misc (object_type *o_ptr, char *misc_desc)
{
	artifact_type *a_ptr = &a_info[o_ptr->name1];

#ifdef JP
	sprintf(misc_desc, "レベル %u, 希少度 %u, %d.%d kg, ＄%ld",
		a_ptr->level, a_ptr->rarity,
		lbtokg1(a_ptr->weight), lbtokg2(a_ptr->weight), a_ptr->cost);
#else
	sprintf(misc_desc, "Level %u, Rarity %u, %d.%d lbs, %ld Gold",
		a_ptr->level, a_ptr->rarity,
		a_ptr->weight / 10, a_ptr->weight % 10, a_ptr->cost);
#endif
}


/*
 * Fill in an object description structure for a given object
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

	analyze_misc(o_ptr, desc_ptr->misc_desc);

	desc_ptr->activation = item_activation(o_ptr);
}


static void print_header(void)
{
	char buf[80];

#ifndef FAKE_VERSION
	sprintf(buf, "Artifact Spoilers for Angband Version %d.%d.%d",
		VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
	spoiler_underline(buf);
#else /* FAKE_VERSION */
	sprintf(buf, "Artifact Spoilers for Hengband Version %d.%d.%d",
	        FAKE_VER_MAJOR-10, FAKE_VER_MINOR, FAKE_VER_PATCH);
	spoiler_underline(buf);
#endif /* FAKE_VERSION */

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


/* Create a spoiler file entry for an artifact */

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
#ifdef JP
		sprintf(buf, "%sの修正:", pval_ptr->pval_desc);
#else
		sprintf(buf, "%s to", pval_ptr->pval_desc);
#endif
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
#ifdef JP
		fprintf(fff, "%s発動: %s\n", INDENT1, art_ptr->activation);
#else
		fprintf(fff, "%sActivates for %s\n", INDENT1, art_ptr->activation);
#endif
	}

	/* End with the miscellaneous facts */
	fprintf(fff, "%s%s\n\n", INDENT1, art_ptr->misc_desc);
}


/*
 * Hack -- Create a "forged" artifact
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


/*
 * Create a spoiler file for artifacts
 */
static void spoil_artifact(cptr fname)
{
	int i, j;

	object_type forge;
	object_type *q_ptr;

	obj_desc_list artifact;

	char buf[1024];


	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_USER, fname);

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





/*
 * Create a spoiler file for monsters   -BEN-
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
	path_build(buf, 1024, ANGBAND_DIR_USER, fname);

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

#ifndef FAKE_VERSION
	fprintf(fff, "Monster Spoilers for Angband Version %d.%d.%d\n",
		VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
	fprintf(fff, "------------------------------------------\n\n");
#else
	fprintf(fff, "Monster Spoilers for Hengband Version %d.%d.%d\n",
	        FAKE_VER_MAJOR-10, FAKE_VER_MINOR, FAKE_VER_PATCH);
	fprintf(fff, "------------------------------------------\n\n");
#endif

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
/*		if (r_ptr->flags1 & (RF1_QUESTOR)) */
		if (0)
		{
			sprintf(nam, "[Q] %s", name);
		}
		else if (r_ptr->flags1 & (RF1_UNIQUE))
		{
			sprintf(nam, "[U] %s", name);
		}
		else
		{
#ifdef JP
			sprintf(nam, "    %s", name);
#else
			sprintf(nam, "The %s", name);
#endif
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
		sprintf(exp, "%s '%c'", attr_to_text(r_ptr->d_attr), r_ptr->d_char);

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

/*
 * Pronoun arrays
 */
static cptr wd_che[3] =
#ifdef JP
{ "それ", "彼", "彼女" };
#else
{ "It", "He", "She" };
#endif

#ifndef JP
static cptr wd_lhe[3] =
{ "it", "he", "she" };
#endif


/*
 * Buffer text to the given file. (-SHAWN-)
 * This is basically c_roff() from mon-desc.c with a few changes.
 */
static void spoil_out(cptr str)
{
	cptr r;

	/* Line buffer */
	static char roff_buf[256];

#ifdef JP
	char iskanji2=0;
#endif
	/* Current pointer into line roff_buf */
	static char *roff_p = roff_buf;

	/* Last space saved into roff_buf */
	static char *roff_s = NULL;

	/* Special handling for "new sequence" */
	if (!str)
	{
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
                int k_flag = iskanji((unsigned char)(*str));
#endif
		char ch = *str;
		int wrap = (ch == '\n');

#ifdef JP
                if (!isprint(ch) && !k_flag && !iskanji2) ch = ' ';
		if(k_flag && !iskanji2)iskanji2=1;else iskanji2=0;
#else
		if (!isprint(ch)) ch = ' ';
#endif

#ifdef JP
                if ( roff_p >= roff_buf+( (k_flag) ? 74 : 75) ) wrap=1;
		if ((ch == ' ') && (roff_p + 2 >= roff_buf + ((k_flag) ? 74 : 75))) wrap = 1;
#else
		if (roff_p >= roff_buf + 75) wrap = 1;
		if ((ch == ' ') && (roff_p + 2 >= roff_buf + 75)) wrap = 1;
#endif


		/* Handle line-wrap */
		if (wrap)
		{
			*roff_p = '\0';
			r = roff_p;
#ifdef JP
                                cbak=' ';
#endif
			if (roff_s && (ch != ' '))
			{
#ifdef JP
			        cbak=*roff_s;
#endif
				*roff_s = '\0';
				r = roff_s + 1;
			}
			fprintf(fff, "%s\n", roff_buf);
			roff_s = NULL;
			roff_p = roff_buf;
#ifdef JP
			if(cbak != ' ') *roff_p++ = cbak; 
#endif
			while (*r) *roff_p++ = *r++;
		}

		/* Save the char */
		if ((roff_p > roff_buf) || (ch != ' '))
		{
#ifdef JP
		  if( !k_flag ){
			if (ch == ' ' || ch == '(' ) roff_s = roff_p;
		  }
		  else{
		    if( iskanji2 && 
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


/*
 * Create a spoiler file for monsters (-SHAWN-)
 */
static void spoil_mon_info(cptr fname)
{
	char buf[1024];
	int msex, vn, i, j, k, l, n=0;
	bool breath, magic, sin;
	cptr p, q;
	cptr vp[64];
	u32b flags1, flags2, flags3, flags4, flags5, flags6, flags7;

	u16b why = 2;
	s16b *who;


#ifdef JP
        char            jverb_buf[64];
#endif
	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_USER, fname);

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
#ifndef FAKE_VERSION
	sprintf(buf, "Monster Spoilers for Angband Version %d.%d.%d\n",
		VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
#else
	sprintf(buf, "Monster Spoilers for Hengband Version %d.%d.%d\n",
	     FAKE_VER_MAJOR-10, FAKE_VER_MINOR, FAKE_VER_PATCH);
#endif

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
		flags2 = r_ptr->flags2;
		flags3 = r_ptr->flags3;
		flags4 = r_ptr->flags4;
		flags5 = r_ptr->flags5;
		flags6 = r_ptr->flags6;
		flags7 = r_ptr->flags7;
		breath = FALSE;
		magic = FALSE;

		/* Extract a gender (if applicable) */
		if (flags1 & (RF1_FEMALE)) msex = 2;
		else if (flags1 & (RF1_MALE)) msex = 1;
		else msex = 0;


		/* Prefix */
		if (flags1 & (RF1_QUESTOR))
		{
			spoil_out("[Q] ");
		}
		else if (flags1 & (RF1_UNIQUE))
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
#ifdef JP
		sprintf(buf, "%s/%s  (", (r_name + r_ptr->name),(r_name+r_ptr->E_name));  /* ---)--- */
#else
		sprintf(buf, "%s  (", (r_name + r_ptr->name));  /* ---)--- */
#endif

		spoil_out(buf);

		/* Color */
		spoil_out(attr_to_text(r_ptr->d_attr));

		/* Symbol --(-- */
		sprintf(buf, " '%c')\n", r_ptr->d_char);
		spoil_out(buf);


		/* Indent */
		sprintf(buf, "=== ");
		spoil_out(buf);

		/* Number */
		sprintf(buf, "Num:%d  ", n);
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


		/* Describe */
		spoil_out(r_text + r_ptr->text);
#ifdef JP
		spoil_out("。 ");
#else
		spoil_out("  ");
#endif



#ifdef JP
                spoil_out("この");

if (flags2 & RF2_ELDRITCH_HORROR) spoil_out("狂気を誘う");/*nuke me*/
if (flags3 & RF3_ANIMAL)          spoil_out("自然界の");
if (flags3 & RF3_EVIL)            spoil_out("邪悪なる");
if (flags3 & RF3_GOOD)            spoil_out("善良な");
if (flags3 & RF3_UNDEAD)          spoil_out("アンデッドの");

if ((flags3 & (RF3_DRAGON | RF3_DEMON | RF3_GIANT | RF3_TROLL | RF3_ORC | RF3_AMBERITE)) || (flags2 & RF2_QUANTUM))
{
     if (flags3 & RF3_DRAGON)   spoil_out("ドラゴン");
     if (flags3 & RF3_DEMON)    spoil_out("デーモン");
     if (flags3 & RF3_GIANT)    spoil_out("ジャイアント");
     if (flags3 & RF3_TROLL)    spoil_out("トロル");
     if (flags3 & RF3_ORC)      spoil_out("オーク");
     if (flags3 & RF3_AMBERITE) spoil_out("アンバーの王族");/*nuke me*/
     if (flags2 & RF2_QUANTUM)  spoil_out("量子生物");
}
else                            spoil_out("モンスター");
#else
		spoil_out("This");

		if (flags2 & (RF2_ELDRITCH_HORROR)) spoil_out (" sanity-blasting");
		if (flags3 & (RF3_ANIMAL)) spoil_out(" natural");
		if (flags3 & (RF3_EVIL)) spoil_out(" evil");
		if (flags3 & (RF3_GOOD)) spoil_out(" good");
		if (flags3 & (RF3_UNDEAD)) spoil_out(" undead");

		if (flags3 & (RF3_DRAGON)) spoil_out(" dragon");
		else if (flags3 & (RF3_DEMON)) spoil_out(" demon");
		else if (flags3 & (RF3_GIANT)) spoil_out(" giant");
		else if (flags3 & (RF3_TROLL)) spoil_out(" troll");
		else if (flags3 & (RF3_ORC)) spoil_out(" orc");
		else if (flags3 & (RF3_AMBERITE)) spoil_out (" Amberite");
		else spoil_out(" creature");
#endif


#ifdef JP
		spoil_out("は");
#else
		spoil_out(" moves");
#endif


		if ((flags1 & (RF1_RAND_50)) && (flags1 & (RF1_RAND_25)))
		{
#ifdef JP
spoil_out("かなり不規則に");
#else
			spoil_out(" extremely erratically");
#endif

		}
		else if (flags1 & (RF1_RAND_50))
		{
#ifdef JP
spoil_out("幾分不規則に");
#else
			spoil_out(" somewhat erratically");
#endif

		}
		else if (flags1 & (RF1_RAND_25))
		{
#ifdef JP
spoil_out("少々不規則に");
#else
			spoil_out(" a bit erratically");
#endif

		}
		else
		{
#ifdef JP
spoil_out("普通に");
#else
			spoil_out(" normally");
#endif

		}

#ifdef JP
spoil_out("動いている");
#endif
		if (flags1 & (RF1_NEVER_MOVE))
		{
#ifdef JP
spoil_out("が、侵入者を追跡しない");
#else
			spoil_out(", but does not deign to chase intruders");
#endif

		}

#ifdef JP
			spoil_out("ことがある。");
#else
		spoil_out(".  ");
#endif


		if (!r_ptr->level || (flags1 & (RF1_FORCE_DEPTH)))
		{
			sprintf(buf, "%s is never found out of depth.  ", wd_che[msex]);
			spoil_out(buf);
		}

		if (flags1 & (RF1_FORCE_SLEEP))
		{
			sprintf(buf, "%s is always created sluggish.  ", wd_che[msex]);
			spoil_out(buf);
		}

		if (flags2 & (RF2_AURA_FIRE))
		{
			sprintf(buf, "%s is surrounded by flames.  ", wd_che[msex]);
			spoil_out(buf);
		}

		if (flags3 & (RF3_AURA_COLD))
		{
			sprintf(buf, "%s is surrounded by ice.  ", wd_che[msex]);
			spoil_out(buf);
		}

		if (flags2 & (RF2_AURA_ELEC))
		{
			sprintf(buf, "%s is surrounded by electricity.  ", wd_che[msex]);
			spoil_out(buf);
		}

		if (flags2 & (RF2_REFLECTING))
		{
			sprintf(buf, "%s reflects bolt spells.  ", wd_che[msex]);
			spoil_out(buf);
		}

		if (flags1 & (RF1_ESCORT))
		{
			sprintf(buf, "%s usually appears with ", wd_che[msex]);
			spoil_out(buf);
			if (flags1 & (RF1_ESCORTS)) spoil_out("escorts.  ");
			else spoil_out("an escort.  ");
		}

		if (flags1 & RF1_FRIENDS)
		{
			sprintf(buf, "%s usually appears in groups.  ", wd_che[msex]);
			spoil_out(buf);
		}

		/* Collect inate attacks */
		vn = 0;
#ifdef JP
if (flags4 & RF4_SHRIEK)  vp[vn++] = "悲鳴で助けを求める";
if (flags4 & RF4_ROCKET)  vp[vn++] = "ロケットを発射する";
if (flags4 & RF4_SHOOT) vp[vn++] = "射撃をする";
if (flags6 & (RF6_SPECIAL)) vp[vn++] = "特別な行動をする";
#else
		if (flags4 & RF4_SHRIEK)  vp[vn++] = "shriek for help";
		if (flags4 & RF4_ROCKET)  vp[vn++] = "shoot a rocket";
		if (flags4 & RF4_SHOOT) vp[vn++] = "fire missiles";
		if (flags6 & (RF6_SPECIAL)) vp[vn++] = "do something";
#endif


		if (vn)
		{
#ifdef JP
		        spoil_out(wd_che[msex]);
                        spoil_out("は");
#else
			spoil_out(wd_che[msex]);
#endif

			for (i = 0; i < vn; i++)
			{
#ifdef JP
                        if(i!=vn-1){
                          jverb3(vp[i],jverb_buf);
                          spoil_out(jverb_buf);
                          spoil_out("り、");
                        }
                        else  spoil_out(vp[i]);
#else
				if (!i) spoil_out(" may ");
				else if (i < vn-1) spoil_out(", ");
				else spoil_out(" or ");
				spoil_out(vp[i]);
#endif

			}
#ifdef JP
			spoil_out("ことができる。");
#else
			spoil_out(".  ");
#endif

		}

		/* Collect breaths */
		vn = 0;
#ifdef JP
if (flags4 & (RF4_BR_ACID))		vp[vn++] = "酸";
if (flags4 & (RF4_BR_ELEC))		vp[vn++] = "稲妻";
if (flags4 & (RF4_BR_FIRE))		vp[vn++] = "火炎";
if (flags4 & (RF4_BR_COLD))		vp[vn++] = "冷気";
if (flags4 & (RF4_BR_POIS))		vp[vn++] = "毒";
if (flags4 & (RF4_BR_NETH))		vp[vn++] = "地獄";
if (flags4 & (RF4_BR_LITE))		vp[vn++] = "閃光";
if (flags4 & (RF4_BR_DARK))		vp[vn++] = "暗黒";
if (flags4 & (RF4_BR_CONF))		vp[vn++] = "混乱";
if (flags4 & (RF4_BR_SOUN))		vp[vn++] = "轟音";
if (flags4 & (RF4_BR_CHAO))		vp[vn++] = "カオス";
if (flags4 & (RF4_BR_DISE))		vp[vn++] = "劣化";
if (flags4 & (RF4_BR_NEXU))		vp[vn++] = "因果混乱";
if (flags4 & (RF4_BR_TIME))		vp[vn++] = "時間逆転";
if (flags4 & (RF4_BR_INER))		vp[vn++] = "遅鈍";
if (flags4 & (RF4_BR_GRAV))		vp[vn++] = "重力";
if (flags4 & (RF4_BR_SHAR))		vp[vn++] = "破片";
if (flags4 & (RF4_BR_PLAS))		vp[vn++] = "プラズマ";
if (flags4 & (RF4_BR_WALL))		vp[vn++] = "フォース";
if (flags4 & (RF4_BR_MANA))		vp[vn++] = "魔力";
if (flags4 & (RF4_BR_NUKE))		vp[vn++] = "放射性廃棄物";
if (flags4 & (RF4_BR_DISI))		vp[vn++] = "分解";
#else
		if (flags4 & (RF4_BR_ACID)) vp[vn++] = "acid";
		if (flags4 & (RF4_BR_ELEC)) vp[vn++] = "lightning";
		if (flags4 & (RF4_BR_FIRE)) vp[vn++] = "fire";
		if (flags4 & (RF4_BR_COLD)) vp[vn++] = "frost";
		if (flags4 & (RF4_BR_POIS)) vp[vn++] = "poison";
		if (flags4 & (RF4_BR_NETH)) vp[vn++] = "nether";
		if (flags4 & (RF4_BR_LITE)) vp[vn++] = "light";
		if (flags4 & (RF4_BR_DARK)) vp[vn++] = "darkness";
		if (flags4 & (RF4_BR_CONF)) vp[vn++] = "confusion";
		if (flags4 & (RF4_BR_SOUN)) vp[vn++] = "sound";
		if (flags4 & (RF4_BR_CHAO)) vp[vn++] = "chaos";
		if (flags4 & (RF4_BR_DISE)) vp[vn++] = "disenchantment";
		if (flags4 & (RF4_BR_NEXU)) vp[vn++] = "nexus";
		if (flags4 & (RF4_BR_TIME)) vp[vn++] = "time";
		if (flags4 & (RF4_BR_INER)) vp[vn++] = "inertia";
		if (flags4 & (RF4_BR_GRAV)) vp[vn++] = "gravity";
		if (flags4 & (RF4_BR_SHAR)) vp[vn++] = "shards";
		if (flags4 & (RF4_BR_PLAS)) vp[vn++] = "plasma";
		if (flags4 & (RF4_BR_WALL)) vp[vn++] = "force";
		if (flags4 & (RF4_BR_MANA)) vp[vn++] = "mana";
		if (flags4 & (RF4_BR_NUKE)) vp[vn++] = "toxic waste";
		if (flags4 & (RF4_BR_DISI)) vp[vn++] = "disintegration";
#endif


		if (vn)
		{
			breath = TRUE;
#ifdef JP
			spoil_out(wd_che[msex]);
			spoil_out("は");
#else
			spoil_out(wd_che[msex]);
#endif

#ifdef JP
			spoil_out("は");
#endif
			for (i = 0; i < vn; i++)
			{
#ifdef JP
                        if ( i != 0 ) spoil_out("や");
#else
				if (!i) spoil_out(" may breathe ");
				else if (i < vn-1) spoil_out(", ");
				else spoil_out(" or ");
#endif

				spoil_out(vp[i]);
			}
#ifdef JP
			spoil_out("のブレスを");
			if (flags2 & (RF2_POWERFUL)) spoil_out("強力に");
			spoil_out("吐くことがあ");
#else
			if (flags2 & (RF2_POWERFUL)) spoil_out(" powerfully");
#endif

		}

		/* Collect spells */
		vn = 0;
#ifdef JP
if (flags5 & (RF5_BA_ACID))         vp[vn++] = "アシッド・ボールの";
if (flags5 & (RF5_BA_ELEC))         vp[vn++] = "サンダー・ボールの";
if (flags5 & (RF5_BA_FIRE))         vp[vn++] = "ファイア・ボールの";
if (flags5 & (RF5_BA_COLD))         vp[vn++] = "アイス・ボールの";
if (flags5 & (RF5_BA_POIS))         vp[vn++] = "悪臭雲の";
if (flags5 & (RF5_BA_NETH))         vp[vn++] = "地獄球の";
if (flags5 & (RF5_BA_WATE))         vp[vn++] = "ウォーター・ボールの";
if (flags4 & (RF4_BA_NUKE))         vp[vn++] = "放射能球の";
#else
		if (flags5 & (RF5_BA_ACID))           vp[vn++] = "produce acid balls";
		if (flags5 & (RF5_BA_ELEC))           vp[vn++] = "produce lightning balls";
		if (flags5 & (RF5_BA_FIRE))           vp[vn++] = "produce fire balls";
		if (flags5 & (RF5_BA_COLD))           vp[vn++] = "produce frost balls";
		if (flags5 & (RF5_BA_POIS))           vp[vn++] = "produce poison balls";
		if (flags5 & (RF5_BA_NETH))           vp[vn++] = "produce nether balls";
		if (flags5 & (RF5_BA_WATE))           vp[vn++] = "produce water balls";
		if (flags4 & (RF4_BA_NUKE))           vp[vn++] = "produce balls of radiation";
#endif

#ifdef JP
if (flags5 & (RF5_BA_MANA))         vp[vn++] = "魔力の嵐の";
if (flags5 & (RF5_BA_DARK))         vp[vn++] = "暗黒の嵐の";
if (flags5 & (RF5_BA_LITE))         vp[vn++] = "スターバーストの";
if (flags4 & (RF4_BA_CHAO))         vp[vn++] = "純ログルスの";
#else
		if (flags5 & (RF5_BA_MANA))           vp[vn++] = "produce mana storms";
		if (flags5 & (RF5_BA_DARK))           vp[vn++] = "produce darkness storms";
		if (flags4 & (RF5_BA_LITE))           vp[vn++] = "produce starburst";
		if (flags4 & (RF4_BA_CHAO))           vp[vn++] = "invoke raw Logrus";
#endif
#ifdef JP
if (flags6 & (RF6_HAND_DOOM))       vp[vn++] = "破滅の手の";
if (flags6 & (RF6_PSY_SPEAR))       vp[vn++] = "光の剣の";
if (flags5 & (RF5_DRAIN_MANA))      vp[vn++] = "魔力を吸い取る";
if (flags5 & (RF5_MIND_BLAST))      vp[vn++] = "精神を攻撃する";
if (flags5 & (RF5_BRAIN_SMASH))     vp[vn++] = "脳を攻撃する";
if (flags5 & (RF5_CAUSE_1))         vp[vn++] = "軽傷を引き起こして呪いをかける";
if (flags5 & (RF5_CAUSE_2))         vp[vn++] = "重傷を引き起こして呪いをかける";
if (flags5 & (RF5_CAUSE_3))         vp[vn++] = "致命傷を引き起こして呪いをかける";
if (flags5 & (RF5_CAUSE_4))         vp[vn++] = "秘孔を突く";
if (flags5 & (RF5_BO_ACID))         vp[vn++] = "アシッド・ボルトの";
if (flags5 & (RF5_BO_ELEC))         vp[vn++] = "サンダー・ボルトの";
if (flags5 & (RF5_BO_FIRE))         vp[vn++] = "ファイア・ボルトの";
if (flags5 & (RF5_BO_COLD))         vp[vn++] = "アイス・ボルトの";
if (flags5 & (RF5_BO_NETH))         vp[vn++] = "地獄の矢の";
if (flags5 & (RF5_BO_WATE))         vp[vn++] = "ウォーター・ボルトの";
if (flags5 & (RF5_BO_MANA))         vp[vn++] = "魔力の矢の";
if (flags5 & (RF5_BO_PLAS))         vp[vn++] = "プラズマ・ボルトの";
if (flags5 & (RF5_BO_ICEE))         vp[vn++] = "極寒の矢の";
if (flags5 & (RF5_MISSILE))         vp[vn++] = "マジックミサイルの";
if (flags5 & (RF5_SCARE))           vp[vn++] = "恐怖を呼び起こす";
if (flags5 & (RF5_BLIND))           vp[vn++] = "目をくらませる";
if (flags5 & (RF5_CONF))            vp[vn++] = "混乱させる";
if (flags5 & (RF5_SLOW))            vp[vn++] = "減速させる";
if (flags5 & (RF5_HOLD))            vp[vn++] = "麻痺させる";
if (flags6 & (RF6_HASTE))           vp[vn++] = "自分を加速する";
if (flags6 & (RF6_HEAL))            vp[vn++] = "自分を治癒する";
if (flags6 & (RF6_INVULNER))        vp[vn++] = "無傷の球の";
if (flags4 & (RF4_DISPEL))          vp[vn++] = "魔力を消し去る";
if (flags6 & (RF6_BLINK))           vp[vn++] = "瞬時に消える";
if (flags6 & (RF6_TPORT))           vp[vn++] = "テレポートする";
if (flags6 & (RF6_WORLD))           vp[vn++] = "時を止める";
if (flags6 & (RF6_TELE_TO))         vp[vn++] = "テレポートで引き戻す";
if (flags6 & (RF6_TELE_AWAY))       vp[vn++] = "テレポートさせる";
if (flags6 & (RF6_TELE_LEVEL))      vp[vn++] = "テレポート・レベルさせる";
if (flags6 & (RF6_DARKNESS))        vp[vn++] = "暗闇を作る";
if (flags6 & (RF6_TRAPS))           vp[vn++] = "トラップを作る";
if (flags6 & (RF6_FORGET))          vp[vn++] = "記憶を消去する";
if (flags6 & (RF6_RAISE_DEAD))      vp[vn++] = "死者を甦らせる";
if (flags6 & (RF6_S_MONSTER))       vp[vn++] = "一体のモンスターを召喚する";
if (flags6 & (RF6_S_MONSTERS))      vp[vn++] = "複数のモンスターを召喚する";
if (flags6 & (RF6_S_KIN))           vp[vn++] = "救援を召喚する";
if (flags6 & (RF6_S_ANT))           vp[vn++] = "アリを召喚する";
if (flags6 & (RF6_S_SPIDER))        vp[vn++] = "クモを召喚する";
if (flags6 & (RF6_S_HOUND))         vp[vn++] = "ハウンドを召喚する";
if (flags6 & (RF6_S_HYDRA))         vp[vn++] = "ヒドラを召喚する";
if (flags6 & (RF6_S_ANGEL))         vp[vn++] = "一体の天使を召喚する";
if (flags6 & (RF6_S_DEMON))         vp[vn++] = "一体のデーモンを召喚する";
if (flags6 & (RF6_S_UNDEAD))        vp[vn++] = "一体のアンデッドを召喚する";
if (flags6 & (RF6_S_DRAGON))        vp[vn++] = "一体のドラゴンを召喚する";
if (flags6 & (RF6_S_HI_UNDEAD))     vp[vn++] = "強力なアンデッドを召喚する";
if (flags6 & (RF6_S_HI_DRAGON))     vp[vn++] = "古代ドラゴンを召喚する";
if (flags6 & (RF6_S_CYBER))         vp[vn++] = "サイバーデーモンを召喚する";
if (flags6 & (RF6_S_AMBERITES))     vp[vn++] = "アンバーの王族を召喚する";
if (flags6 & (RF6_S_UNIQUE))        vp[vn++] = "ユニーク・モンスターを召喚する";
#else
		if (flags6 & (RF6_HAND_DOOM))         vp[vn++] = "invoke the Hand of Doom";
		if (flags6 & (RF6_PSY_SPEAR))         vp[vn++] = "psycho-spear";
		if (flags5 & (RF5_DRAIN_MANA))        vp[vn++] = "drain mana";
		if (flags5 & (RF5_MIND_BLAST))        vp[vn++] = "cause mind blasting";
		if (flags5 & (RF5_BRAIN_SMASH))       vp[vn++] = "cause brain smashing";
		if (flags5 & (RF5_CAUSE_1))           vp[vn++] = "cause light wounds and cursing";
		if (flags5 & (RF5_CAUSE_2))           vp[vn++] = "cause serious wounds and cursing";
		if (flags5 & (RF5_CAUSE_3))           vp[vn++] = "cause critical wounds and cursing";
		if (flags5 & (RF5_CAUSE_4))           vp[vn++] = "cause mortal wounds";
		if (flags5 & (RF5_BO_ACID))           vp[vn++] = "produce acid bolts";
		if (flags5 & (RF5_BO_ELEC))           vp[vn++] = "produce lightning bolts";
		if (flags5 & (RF5_BO_FIRE))           vp[vn++] = "produce fire bolts";
		if (flags5 & (RF5_BO_COLD))           vp[vn++] = "produce frost bolts";
		if (flags5 & (RF5_BO_NETH))           vp[vn++] = "produce nether bolts";
		if (flags5 & (RF5_BO_WATE))           vp[vn++] = "produce water bolts";
		if (flags5 & (RF5_BO_MANA))           vp[vn++] = "produce mana bolts";
		if (flags5 & (RF5_BO_PLAS))           vp[vn++] = "produce plasma bolts";
		if (flags5 & (RF5_BO_ICEE))           vp[vn++] = "produce ice bolts";
		if (flags5 & (RF5_MISSILE))           vp[vn++] = "produce magic missiles";
		if (flags5 & (RF5_SCARE))             vp[vn++] = "terrify";
		if (flags5 & (RF5_BLIND))             vp[vn++] = "blind";
		if (flags5 & (RF5_CONF))              vp[vn++] = "confuse";
		if (flags5 & (RF5_SLOW))              vp[vn++] = "slow";
		if (flags5 & (RF5_HOLD))              vp[vn++] = "paralyze";
		if (flags6 & (RF6_HASTE))             vp[vn++] = "haste-self";
		if (flags6 & (RF6_HEAL))              vp[vn++] = "heal-self";
		if (flags6 & (RF6_INVULNER))          vp[vn++] = "make invulnerable";
		if (flags6 & (RF6_BLINK))             vp[vn++] = "blink-self";
		if (flags6 & (RF6_TPORT))             vp[vn++] = "teleport-self";
		if (flags6 & (RF6_WORLD))             vp[vn++] = "stop the time";
		if (flags6 & (RF6_TELE_TO))           vp[vn++] = "teleport to";
		if (flags6 & (RF6_TELE_AWAY))         vp[vn++] = "teleport away";
		if (flags6 & (RF6_TELE_LEVEL))        vp[vn++] = "teleport level";
		if (flags6 & (RF6_DARKNESS))          vp[vn++] = "create darkness";
		if (flags6 & (RF6_TRAPS))             vp[vn++] = "create traps";
		if (flags6 & (RF6_FORGET))            vp[vn++] = "cause amnesia";
		if (flags6 & (RF6_RAISE_DEAD))        vp[vn++] = "raise dead";
		if (flags6 & (RF6_S_MONSTER))         vp[vn++] = "summon a monster";
		if (flags6 & (RF6_S_MONSTERS))        vp[vn++] = "summon monsters";
		if (flags6 & (RF6_S_KIN))             vp[vn++] = "summon aid";
		if (flags6 & (RF6_S_ANT))             vp[vn++] = "summon ants";
		if (flags6 & (RF6_S_SPIDER))          vp[vn++] = "summon spiders";
		if (flags6 & (RF6_S_HOUND))           vp[vn++] = "summon hounds";
		if (flags6 & (RF6_S_HYDRA))           vp[vn++] = "summon hydras";
		if (flags6 & (RF6_S_ANGEL))           vp[vn++] = "summon an angel";
		if (flags6 & (RF6_S_DEMON))           vp[vn++] = "summon a demon";
		if (flags6 & (RF6_S_UNDEAD))          vp[vn++] = "summon an undead";
		if (flags6 & (RF6_S_DRAGON))          vp[vn++] = "summon a dragon";
		if (flags6 & (RF6_S_HI_UNDEAD))       vp[vn++] = "summon greater undead";
		if (flags6 & (RF6_S_HI_DRAGON))       vp[vn++] = "summon ancient dragons";
		if (flags6 & (RF6_S_CYBER))           vp[vn++] = "summon Cyberdemons";
		if (flags6 & (RF6_S_AMBERITES))       vp[vn++] = "summon Lords of Amber";
		if (flags6 & (RF6_S_UNIQUE))          vp[vn++] = "summon unique monsters";
#endif


		if (vn)
		{
			magic = TRUE;
			if (breath)
			{
#ifdef JP
				spoil_out("り、なおかつ");
#else
				spoil_out(", and is also");
#endif

			}
			else
			{
#ifdef JP
				spoil_out(wd_che[msex]);
				spoil_out("は");
#else
				spoil_out(wd_che[msex]);
				spoil_out(" is");
#endif

			}

#ifdef JP
			/* Adverb */
			if (flags2 & (RF2_SMART)) spoil_out("的確に");

			/* Verb Phrase */
			spoil_out("魔法を使うことができ、");
#else
			spoil_out(" magical, casting spells");
			if (flags2 & (RF2_SMART)) spoil_out(" intelligently");
#endif


			for (i = 0; i < vn; i++)
			{
#ifdef JP
                        if ( i != 0 ) spoil_out("呪文、");
#else
				if (!i) spoil_out(" which ");
				else if (i < vn-1) spoil_out(", ");
				else spoil_out(" or ");
#endif

				spoil_out(vp[i]);
			}
#ifdef JP
			spoil_out("呪文を唱えることがあ");
#endif
		}

		if (breath || magic)
		{
#ifdef JP
			sprintf(buf, "る(確率:約1/%d)。",
				200 / (r_ptr->freq_inate + r_ptr->freq_spell));
			spoil_out(buf);
#else
			sprintf(buf, "; 1 time in %d.  ",
				200 / (r_ptr->freq_inate + r_ptr->freq_spell));
			spoil_out(buf);
#endif

		}

		/* Collect special abilities. */
		vn = 0;
#ifdef JP
if (flags7 & (RF7_HAS_LITE_1 | RF7_HAS_LITE_2))  vp[vn++] = "ダンジョンを照らす";
if (flags7 & (RF7_SELF_LITE_1 | RF7_SELF_LITE_2)) vp[vn++] = "光っている";
if (flags2 & (RF2_OPEN_DOOR)) vp[vn++] = "ドアを開ける";
if (flags2 & (RF2_BASH_DOOR)) vp[vn++] = "ドアを打ち破る";
if (flags2 & (RF2_PASS_WALL)) vp[vn++] = "壁をすり抜ける";
if (flags2 & (RF2_KILL_WALL)) vp[vn++] = "壁を掘り進む";
if (flags2 & (RF2_MOVE_BODY)) vp[vn++] = "弱いモンスターを押しのける";
if (flags2 & (RF2_KILL_BODY)) vp[vn++] = "弱いモンスターを倒す";
if (flags2 & (RF2_TAKE_ITEM)) vp[vn++] = "アイテムを拾う";
if (flags2 & (RF2_KILL_ITEM)) vp[vn++] = "アイテムを壊す";
#else
		if (flags7 & (RF7_HAS_LITE_1 | RF7_HAS_LITE_2))  vp[vn++] = "illuminate the dungeon";
		if (flags7 & (RF7_SELF_LITE_1 | RF7_SELF_LITE_2)) vp[vn++] = "illuminate the dungeon";
		if (flags2 & (RF2_OPEN_DOOR)) vp[vn++] = "open doors";
		if (flags2 & (RF2_BASH_DOOR)) vp[vn++] = "bash down doors";
		if (flags2 & (RF2_PASS_WALL)) vp[vn++] = "pass through walls";
		if (flags2 & (RF2_KILL_WALL)) vp[vn++] = "bore through walls";
		if (flags2 & (RF2_MOVE_BODY)) vp[vn++] = "push past weaker monsters";
		if (flags2 & (RF2_KILL_BODY)) vp[vn++] = "destroy weaker monsters";
		if (flags2 & (RF2_TAKE_ITEM)) vp[vn++] = "pick up objects";
		if (flags2 & (RF2_KILL_ITEM)) vp[vn++] = "destroy objects";
#endif


		if (vn)
		{
			spoil_out(wd_che[msex]);
			for (i = 0; i < vn; i++)
			{
#ifdef JP
                        if(i!=vn-1){
                          jverb1(vp[i],jverb_buf);
                          spoil_out(jverb_buf);
                          spoil_out("、");
                        }
                        else  spoil_out(vp[i]);
#else
				if (!i) spoil_out(" can ");
				else if (i < vn-1) spoil_out(", ");
				else spoil_out(" and ");
				spoil_out(vp[i]);
#endif

			}
			spoil_out(".  ");
		}

		if (flags2 & (RF2_INVISIBLE))
		{
			spoil_out(wd_che[msex]);
#ifdef JP
spoil_out("は透明で目に見えない。");
#else
			spoil_out(" is invisible.  ");
#endif

		}
		if (flags2 & (RF2_COLD_BLOOD))
		{
			spoil_out(wd_che[msex]);
#ifdef JP
spoil_out("は冷血動物である。");
#else
			spoil_out(" is cold blooded.  ");
#endif

		}
		if (flags2 & (RF2_EMPTY_MIND))
		{
			spoil_out(wd_che[msex]);
#ifdef JP
spoil_out("はテレパシーでは感知できない。");
#else
			spoil_out(" is not detected by telepathy.  ");
#endif

		}
		if (flags2 & (RF2_WEIRD_MIND))
		{
			spoil_out(wd_che[msex]);
#ifdef JP
spoil_out("はまれにテレパシーで感知できる。");
#else
			spoil_out(" is rarely detected by telepathy.  ");
#endif

		}
		if (flags2 & (RF2_MULTIPLY))
		{
			spoil_out(wd_che[msex]);
#ifdef JP
spoil_out("は爆発的に増殖する。");
#else
			spoil_out(" breeds explosively.  ");
#endif

		}
		if (flags2 & (RF2_REGENERATE))
		{
			spoil_out(wd_che[msex]);
#ifdef JP
spoil_out("は素早く体力を回復する。");
#else
			spoil_out(" regenerates quickly.  ");
#endif

		}

		/* Collect susceptibilities */
		vn = 0;
#ifdef JP
if (flags3 & (RF3_HURT_ROCK)) vp[vn++] = "岩を除去するもの";
if (flags3 & (RF3_HURT_LITE)) vp[vn++] = "明るい光";
if (flags3 & (RF3_HURT_FIRE)) vp[vn++] = "火";
if (flags3 & (RF3_HURT_COLD)) vp[vn++] = "冷気";
#else
		if (flags3 & (RF3_HURT_ROCK)) vp[vn++] = "rock remover";
		if (flags3 & (RF3_HURT_LITE)) vp[vn++] = "bright light";
		if (flags3 & (RF3_HURT_FIRE)) vp[vn++] = "fire";
		if (flags3 & (RF3_HURT_COLD)) vp[vn++] = "cold";
#endif


		if (vn)
		{
			spoil_out(wd_che[msex]);
			for (i = 0; i < vn; i++)
			{
				if (!i) spoil_out(" is hurt by ");
				else if (i < vn-1) spoil_out(", ");
				else spoil_out(" and ");
				spoil_out(vp[i]);
			}
			spoil_out(".  ");
		}

		/* Collect immunities */
		vn = 0;
#ifdef JP
if (flags3 & (RF3_IM_ACID)) vp[vn++] = "酸";
if (flags3 & (RF3_IM_ELEC)) vp[vn++] = "電撃";
if (flags3 & (RF3_IM_FIRE)) vp[vn++] = "火";
if (flags3 & (RF3_IM_COLD)) vp[vn++] = "冷気";
if (flags3 & (RF3_IM_POIS)) vp[vn++] = "毒";
#else
		if (flags3 & (RF3_IM_ACID)) vp[vn++] = "acid";
		if (flags3 & (RF3_IM_ELEC)) vp[vn++] = "lightning";
		if (flags3 & (RF3_IM_FIRE)) vp[vn++] = "fire";
		if (flags3 & (RF3_IM_COLD)) vp[vn++] = "cold";
		if (flags3 & (RF3_IM_POIS)) vp[vn++] = "poison";
#endif


		if (vn)
		{
			spoil_out(wd_che[msex]);
			for (i = 0; i < vn; i++)
			{
				if (!i) spoil_out(" resists ");
				else if (i < vn-1) spoil_out(", ");
				else spoil_out(" and ");
				spoil_out(vp[i]);
			}
			spoil_out(".  ");
		}

		/* Collect resistances */
		vn = 0;
#ifdef JP
if (flags3 & (RF3_RES_NETH)) vp[vn++] = "地獄";
if (flags3 & (RF3_RES_WATE)) vp[vn++] = "水";
if (flags3 & (RF3_RES_PLAS)) vp[vn++] = "プラズマ";
if (flags3 & (RF3_RES_NEXU)) vp[vn++] = "因果混乱";
if (flags3 & (RF3_RES_DISE)) vp[vn++] = "劣化";
if (flags3 & (RF3_RES_ALL )) vp[vn++] = "あらゆる効果";
#else
		if (flags3 & (RF3_RES_NETH)) vp[vn++] = "nether";
		if (flags3 & (RF3_RES_WATE)) vp[vn++] = "water";
		if (flags3 & (RF3_RES_PLAS)) vp[vn++] = "plasma";
		if (flags3 & (RF3_RES_NEXU)) vp[vn++] = "nexus";
		if (flags3 & (RF3_RES_DISE)) vp[vn++] = "disenchantment";
		if (flags3 & (RF3_RES_ALL )) vp[vn++] = "all";
#endif

		if (flags3 & (RF3_RES_TELE)) vp[vn++] = "teleportation";
#ifdef JP
if ((flags3 & RF3_RES_TELE) && !(r_ptr->flags1 & RF1_UNIQUE)) vp[vn++] = "テレポート";
#else
		if ((flags3 & RF3_RES_TELE) && !(r_ptr->flags1 & RF1_UNIQUE)) vp[vn++] = "teleportation";
#endif


		if (vn)
		{
			spoil_out(wd_che[msex]);
			for (i = 0; i < vn; i++)
			{
				if (!i) spoil_out(" resists ");
				else if (i < vn-1) spoil_out(", ");
				else spoil_out(" and ");
				spoil_out(vp[i]);
			}
			spoil_out(".  ");
		}

		/* Collect non-effects */
		vn = 0;
#ifdef JP
if (flags3 & (RF3_NO_STUN)) vp[vn++] = "朦朧としない";
if (flags3 & (RF3_NO_FEAR)) vp[vn++] = "恐怖を感じない";
if (flags3 & (RF3_NO_CONF)) vp[vn++] = "混乱しない";
if (flags3 & (RF3_NO_SLEEP)) vp[vn++] = "眠らされない";
if ((flags3 & RF3_RES_TELE) && (r_ptr->flags1 & RF1_UNIQUE)) vp[vn++] = "テレポートされない";
#else
		if (flags3 & (RF3_NO_STUN)) vp[vn++] = "stunned";
		if (flags3 & (RF3_NO_FEAR)) vp[vn++] = "frightened";
		if (flags3 & (RF3_NO_CONF)) vp[vn++] = "confused";
		if (flags3 & (RF3_NO_SLEEP)) vp[vn++] = "slept";
		if ((flags3 & RF3_RES_TELE) && (r_ptr->flags1 & RF1_UNIQUE)) vp[vn++] = "teleported";
#endif


		if (vn)
		{
			spoil_out(wd_che[msex]);
			for (i = 0; i < vn; i++)
			{
				if (!i) spoil_out(" cannot be ");
				else if (i < vn-1) spoil_out(", ");
				else spoil_out(" or ");
				spoil_out(vp[i]);
			}
			spoil_out(".  ");
		}

		spoil_out(wd_che[msex]);
#ifdef JP
spoil_out("は侵入者");
if (r_ptr->sleep > 200)     spoil_out("を無視しがちであるが");
else if (r_ptr->sleep > 95) spoil_out("に対してほとんど注意を払わないが");
else if (r_ptr->sleep > 75) spoil_out("に対してあまり注意を払わないが");
else if (r_ptr->sleep > 45) spoil_out("を見過ごしがちであるが");
else if (r_ptr->sleep > 25) spoil_out("をほんの少しは見ており");
else if (r_ptr->sleep > 10) spoil_out("をしばらくは見ており");
else if (r_ptr->sleep > 5)  spoil_out("を幾分注意深く見ており");
else if (r_ptr->sleep > 3)  spoil_out("を注意深く見ており");
else if (r_ptr->sleep > 1)  spoil_out("をかなり注意深く見ており");
else if (r_ptr->sleep > 0)  spoil_out("を警戒しており");
else spoil_out("をかなり警戒しており");
spoil_out("、");
sprintf(buf, " %d フィート先から侵入者に気付くことがある。",
	10 * r_ptr->aaf);
	spoil_out(buf);
#else
		if (r_ptr->sleep > 200)     spoil_out(" prefers to ignore");
		else if (r_ptr->sleep > 95) spoil_out(" pays very little attention to");
		else if (r_ptr->sleep > 75) spoil_out(" pays little attention to");
		else if (r_ptr->sleep > 45) spoil_out(" tends to overlook");
		else if (r_ptr->sleep > 25) spoil_out(" takes quite a while to see");
		else if (r_ptr->sleep > 10) spoil_out(" takes a while to see");
		else if (r_ptr->sleep > 5)  spoil_out(" is fairly observant of");
		else if (r_ptr->sleep > 3)  spoil_out(" is observant of");
		else if (r_ptr->sleep > 1)  spoil_out(" is very observant of");
		else if (r_ptr->sleep > 0)  spoil_out(" is vigilant for");
		else spoil_out(" is ever vigilant for");

		sprintf(buf, " intruders, which %s may notice from %d feet.  ",
			wd_lhe[msex], 10 * r_ptr->aaf);
		spoil_out(buf);
#endif


		i = 0;
		if (flags1 & (RF1_DROP_60)) i += 1;
		if (flags1 & (RF1_DROP_90)) i += 2;
		if (flags1 & (RF1_DROP_1D2)) i += 2;
		if (flags1 & (RF1_DROP_2D2)) i += 4;
		if (flags1 & (RF1_DROP_3D2)) i += 6;
		if (flags1 & (RF1_DROP_4D2)) i += 8;

		/* Drops gold and/or items */
		if (i)
		{
			sin = FALSE;
			spoil_out(wd_che[msex]);
			spoil_out(" will carry");

			if (i == 1)
			{
				spoil_out(" a"); sin = TRUE;
			}
			else if (i == 2)
			{
				spoil_out(" one or two");
				sin = TRUE;
			}
			else
			{
				sprintf(buf, " up to %u", i);
				spoil_out(buf);
			}

			if (flags1 & (RF1_DROP_GREAT))
			{
				if (sin) spoil_out("n");
				spoil_out(" exceptional object");
			}
			else if (flags1 & (RF1_DROP_GOOD))
			{
				spoil_out(" good object");
			}
			else if (flags1 & (RF1_ONLY_ITEM))
			{
				spoil_out(" object");
			}
			else if (flags1 & (RF1_ONLY_GOLD))
			{
				spoil_out(" treasure");
			}
			else
			{
				spoil_out(" object");
				if (i > 1) spoil_out("s");
				spoil_out(" or treasure");
			}
			if (i > 1) spoil_out("s");

			spoil_out(".  ");
		}

		/* Count the actual attacks */
		for (i = 0, j = 0; j < 4; j++)
		{
			if (r_ptr->blow[j].method) i++;
		}

		/* Examine the actual attacks */
		for (k = 0, j = 0; j < 4; j++)
		{
			if (!r_ptr->blow[j].method) continue;

			if (r_ptr->blow[j].method == RBM_SHOOT) continue;

			/* No method yet */
			p = "???";

			/* Acquire the method */
			switch (r_ptr->blow[j].method)
			{
#ifdef JP
case RBM_HIT:		p = "殴る"; break;
case RBM_TOUCH:		p = "触る"; break;
case RBM_PUNCH:		p = "パンチする"; break;
case RBM_KICK:		p = "蹴る"; break;
case RBM_CLAW:		p = "ひっかく"; break;
case RBM_BITE:		p = "噛む"; break;
case RBM_STING:		p = "刺す"; break;
case RBM_SLASH:		p = "斬る"; break;
#else
				case RBM_HIT:    p = "hit"; break;
				case RBM_TOUCH:  p = "touch"; break;
				case RBM_PUNCH:  p = "punch"; break;
				case RBM_KICK:   p = "kick"; break;
				case RBM_CLAW:   p = "claw"; break;
				case RBM_BITE:   p = "bite"; break;
				case RBM_STING:  p = "sting"; break;
				case RBM_SLASH:  p = "slash"; break;
#endif
#ifdef JP
case RBM_BUTT:		p = "角で突く"; break;
case RBM_CRUSH:		p = "体当たりする"; break;
case RBM_ENGULF:	p = "飲み込む"; break;
case RBM_CHARGE: 	p = "請求書をよこす"; break;
case RBM_CRAWL:		p = "体の上を這い回る"; break;
case RBM_DROOL:		p = "よだれをたらす"; break;
case RBM_SPIT:		p = "つばを吐く"; break;
case RBM_EXPLODE:	p = "爆発する"; break;
case RBM_GAZE:		p = "にらむ"; break;
case RBM_WAIL:		p = "泣き叫ぶ"; break;
case RBM_SPORE:		p = "胞子を飛ばす"; break;
#else
				case RBM_BUTT:   p = "butt"; break;
				case RBM_CRUSH:  p = "crush"; break;
				case RBM_ENGULF: p = "engulf"; break;
				case RBM_CHARGE: p = "charge";  break;
				case RBM_CRAWL:  p = "crawl on you"; break;
				case RBM_DROOL:  p = "drool on you"; break;
				case RBM_SPIT:   p = "spit"; break;
				case RBM_EXPLODE: p = "explode";  break;
				case RBM_GAZE:   p = "gaze"; break;
				case RBM_WAIL:   p = "wail"; break;
				case RBM_SPORE:  p = "release spores"; break;
#endif

				case RBM_XXX4:   break;
#ifdef JP
case RBM_BEG:		p = "金をせがむ"; break;
case RBM_INSULT:	p = "侮辱する"; break;
case RBM_MOAN:		p = "うめく"; break;
case RBM_SHOW:  	p = "歌う"; break;
#else
				case RBM_BEG:    p = "beg"; break;
				case RBM_INSULT: p = "insult"; break;
				case RBM_MOAN:   p = "moan"; break;
				case RBM_SHOW:   p = "sing"; break;
#endif

			}


			/* Default effect */
			q = "???";

			/* Acquire the effect */
			switch (r_ptr->blow[j].effect)
			{
#ifdef JP
case RBE_SUPERHURT:
case RBE_HURT:    	q = "攻撃する"; break;
case RBE_POISON:  	q = "毒をくらわす"; break;
case RBE_UN_BONUS:	q = "劣化させる"; break;
case RBE_UN_POWER:	q = "魔力を吸い取る"; break;
case RBE_EAT_GOLD:	q = "金を盗む"; break;
case RBE_EAT_ITEM:	q = "アイテムを盗む"; break;
case RBE_EAT_FOOD:	q = "あなたの食料を食べる"; break;
case RBE_EAT_LITE:	q = "明かりを吸収する"; break;
case RBE_ACID:    	q = "酸を飛ばす"; break;
case RBE_ELEC:    	q = "感電させる"; break;
#else
				case RBE_SUPERHURT:
				case RBE_HURT:          q = "attack"; break;
				case RBE_POISON:        q = "poison"; break;
				case RBE_UN_BONUS:      q = "disenchant"; break;
				case RBE_UN_POWER:      q = "drain charges"; break;
				case RBE_EAT_GOLD:      q = "steal gold"; break;
				case RBE_EAT_ITEM:      q = "steal items"; break;
				case RBE_EAT_FOOD:      q = "eat your food"; break;
				case RBE_EAT_LITE:      q = "absorb light"; break;
				case RBE_ACID:          q = "shoot acid"; break;
				case RBE_ELEC:          q = "electrocute"; break;
#endif

#ifdef JP
case RBE_FIRE:    	q = "燃やす"; break;
case RBE_COLD:    	q = "凍らせる"; break;
case RBE_BLIND:   	q = "盲目にする"; break;
case RBE_CONFUSE: 	q = "混乱させる"; break;
case RBE_TERRIFY: 	q = "恐怖させる"; break;
case RBE_PARALYZE:	q = "麻痺させる"; break;
case RBE_LOSE_STR:	q = "腕力を減少させる"; break;
case RBE_LOSE_INT:	q = "知能を減少させる"; break;
case RBE_LOSE_WIS:	q = "賢さを減少させる"; break;
case RBE_LOSE_DEX:	q = "器用さを減少させる"; break;
case RBE_LOSE_CON:	q = "耐久力を減少させる"; break;
case RBE_LOSE_CHR:	q = "魅力を減少させる"; break;
case RBE_LOSE_ALL:	q = "全ステータスを減少させる"; break;
case RBE_SHATTER:	q = "粉砕する"; break;
case RBE_EXP_10:	q = "経験値を減少(10d6+)させる"; break;
case RBE_EXP_20:	q = "経験値を減少(20d6+)させる"; break;
case RBE_EXP_40:	q = "経験値を減少(40d6+)させる"; break;
case RBE_EXP_80:	q = "経験値を減少(80d6+)させる"; break;
case RBE_DISEASE:	q = "病気にする"; break;
case RBE_TIME:		q = "時間逆転"; break;
case RBE_EXP_VAMP:	q = "生命力を吸収する"; break;
case RBE_DR_MANA:	q = "魔力を奪う"; break;
#else
				case RBE_FIRE:          q = "burn"; break;
				case RBE_COLD:          q = "freeze"; break;
				case RBE_BLIND:         q = "blind"; break;
				case RBE_CONFUSE:       q = "confuse"; break;
				case RBE_TERRIFY:       q = "terrify"; break;
				case RBE_PARALYZE:      q = "paralyze"; break;
				case RBE_LOSE_STR:      q = "reduce strength"; break;
				case RBE_LOSE_INT:      q = "reduce intelligence"; break;
				case RBE_LOSE_WIS:      q = "reduce wisdom"; break;
				case RBE_LOSE_DEX:      q = "reduce dexterity"; break;
				case RBE_LOSE_CON:      q = "reduce constitution"; break;
				case RBE_LOSE_CHR:      q = "reduce charisma"; break;
				case RBE_LOSE_ALL:      q = "reduce all stats"; break;
				case RBE_SHATTER:       q = "shatter"; break;
				case RBE_EXP_10:        q = "lower experience (by 10d6+)"; break;
				case RBE_EXP_20:        q = "lower experience (by 20d6+)"; break;
				case RBE_EXP_40:        q = "lower experience (by 40d6+)"; break;
				case RBE_EXP_80:        q = "lower experience (by 80d6+)"; break;
				case RBE_DISEASE:       q = "disease"; break;
				case RBE_TIME:          q = "time"; break;
				case RBE_EXP_VAMP:      q = "drain life force"; break;
				case RBE_DR_MANA:       q = "drain mana force"; break;
#endif
			}


			if (!k)
			{
				spoil_out(wd_che[msex]);
				spoil_out(" can ");
			}
			else if (k < i-1)
			{
				spoil_out(", ");
			}
			else
			{
				spoil_out(", and ");
			}

			/* Describe the method */
			spoil_out(p);

			/* Describe the effect, if any */
			if (r_ptr->blow[j].effect)
			{
				spoil_out(" to ");
				spoil_out(q);
				if (r_ptr->blow[j].d_dice && r_ptr->blow[j].d_side)
				{
					spoil_out(" with damage");
					if (r_ptr->blow[j].d_side == 1)
						sprintf(buf, " %d", r_ptr->blow[j].d_dice);
					else
						sprintf(buf, " %dd%d",
						r_ptr->blow[j].d_dice, r_ptr->blow[j].d_side);
					spoil_out(buf);
				}
			}

			k++;
		}

		if (k)
		{
			spoil_out(".  ");
		}
		else if (flags1 & (RF1_NEVER_BLOW))
		{
			sprintf(buf, "%s has no physical attacks.  ", wd_che[msex]);
			spoil_out(buf);
		}

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






/*
 * Forward declare
 */
extern void do_cmd_spoilers(void);

/*
 * Create Spoiler files -BEN-
 */
void do_cmd_spoilers(void)
{
	int i;


	/* Save the screen */
	screen_save();


	/* Drop priv's */
	safe_setuid_drop();


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

		/* Prompt */
#ifdef JP
prt("コマンド:", 18, 0);
#else
		prt("Command: ", 12, 0);
#endif


		/* Get a choice */
		i = inkey();

		/* Escape */
		if (i == ESCAPE)
		{
			break;
		}

		/* Option (1) */
		else if (i == '1')
		{
			spoil_obj_desc("obj-desc.spo");
		}

		/* Option (2) */
		else if (i == '2')
		{
			spoil_artifact("artifact.spo");
		}

		/* Option (3) */
		else if (i == '3')
		{
			spoil_mon_desc("mon-desc.spo");
		}

		/* Option (4) */
		else if (i == '4')
		{
			spoil_mon_info("mon-info.spo");
		}

		/* Oops */
		else
		{
			bell();
		}

		/* Flush messages */
		msg_print(NULL);
	}


	/* Grab priv's */
	safe_setuid_grab();


	/* Restore the screen */
	screen_load();
}

/*
 * Fill in an object description structure for a given object
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

/* Create a spoiler file entry for an artifact */

static void spoiler_print_randart(object_type *o_ptr, obj_desc_list *art_ptr)
{
	pval_info_type *pval_ptr = &art_ptr->pval_info;

	char buf[80];

	/* Don't indent the first line */
	fprintf(fff, "%s\n", art_ptr->description);
	
	/* unidentified */
	if (!(o_ptr->ident & (IDENT_MENTAL)))
	{
#ifdef JP
		fprintf(fff, "%s不明\n",INDENT1);
#else
		fprintf(fff, "%sUnknown\n",INDENT1);
#endif
	}
	else {
		/* An "empty" pval description indicates that the pval affects nothing */
		if (pval_ptr->pval_desc[0])
		{
			/* Mention the effects of pval */
#ifdef JP
			sprintf(buf, "%sの修正:", pval_ptr->pval_desc);
#else
			sprintf(buf, "%s to", pval_ptr->pval_desc);
#endif
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
#ifdef JP
			fprintf(fff, "%s発動: %s\n", INDENT1, art_ptr->activation);
#else
			fprintf(fff, "%sActivates for %s\n", INDENT1, art_ptr->activation);
#endif
		}
	}
	/* End with the miscellaneous facts */
	fprintf(fff, "%s%s\n\n", INDENT1, art_ptr->misc_desc);
}

/* Create a part of file for random artifacts */

static void spoil_random_artifact_aux(object_type *o_ptr, int i)
{
	obj_desc_list artifact;

	if (!object_known_p(o_ptr) || !o_ptr->art_name
		|| o_ptr->tval != group_artifact[i].tval)
		return;

	/* Analyze the artifact */
	random_artifact_analyze(o_ptr, &artifact);

	/* Write out the artifact description to the spoiler file */
	spoiler_print_randart(o_ptr, &artifact);
}

/*
 * Create a list file for random artifacts
 */
void spoil_random_artifact(cptr fname)
{
	int i,j;

	store_type  *st_ptr;
	object_type *q_ptr;

	char buf[1024];


	/* Drop priv's */
	safe_setuid_drop();

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_USER, fname);

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

	/* Grab priv's */
	safe_setuid_grab();

	/* Message */
	msg_print("Successfully created a list file.");
}

#else

#ifdef MACINTOSH
static int i = 0;
#endif /* MACINTOSH */

#endif
