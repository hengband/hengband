/*!
 *  @file flavor.c
 *  @brief オブジェクトの記述処理 / Mbject flavor code
 *  @date 2014/01/03
 *  @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 *\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 */

#include "angband.h"

/*!
 * @brief 最初から簡易な名称が明らかになるベースアイテムの判定。 /  Certain items, if aware, are known instantly 
 * @param i ベースアイテムID
 * @return 簡易名称を明らかにするならTRUEを返す。
 * @details
 * This function is used only by "flavor_init()"
 */
static bool object_easy_know(int i)
{
	object_kind *k_ptr = &k_info[i];

	/* Analyze the "tval" */
	switch (k_ptr->tval)
	{
		/* Spellbooks */
		case TV_LIFE_BOOK:
		case TV_SORCERY_BOOK:
		case TV_NATURE_BOOK:
		case TV_CHAOS_BOOK:
		case TV_DEATH_BOOK:
		case TV_TRUMP_BOOK:
		case TV_ARCANE_BOOK:
		case TV_CRAFT_BOOK:
		case TV_DAEMON_BOOK:
		case TV_CRUSADE_BOOK:
		case TV_MUSIC_BOOK:
		case TV_HISSATSU_BOOK:
		case TV_HEX_BOOK:
		{
			return (TRUE);
		}

		/* Simple items */
		case TV_FLASK:
		case TV_JUNK:
		case TV_BOTTLE:
		case TV_SKELETON:
		case TV_SPIKE:
		case TV_WHISTLE:
		{
			return (TRUE);
		}

		/* All Food, Potions, Scrolls, Rods */
		case TV_FOOD:
		case TV_POTION:
		case TV_SCROLL:
		case TV_ROD:
		{
			return (TRUE);
		}
	}

	/* Nope */
	return (FALSE);
}

/*!
 * @brief 各種語彙からランダムな名前を作成する / Create a name from random parts.
 * @param out_string 作成した名を保管する参照ポインタ
 * @return なし
 * @details 日本語の場合 aname_j.txt 英語の場合確率に応じて
 * syllables 配列と elvish.txt を組み合わせる。\n
 */
void get_table_name_aux(char *out_string)
{
#ifdef JP
	char Syllable[80];
	get_rnd_line("aname_j.txt", 1, Syllable);
	strcpy(out_string, Syllable);
	get_rnd_line("aname_j.txt", 2, Syllable);
	strcat(out_string, Syllable);
#else
#define MAX_SYLLABLES 164       /* Used with scrolls (see below) */

	static cptr syllables[MAX_SYLLABLES] = {
		"a", "ab", "ag", "aks", "ala", "an", "ankh", "app",
		"arg", "arze", "ash", "aus", "ban", "bar", "bat", "bek",
		"bie", "bin", "bit", "bjor", "blu", "bot", "bu",
		"byt", "comp", "con", "cos", "cre", "dalf", "dan",
		"den", "der", "doe", "dok", "eep", "el", "eng", "er", "ere", "erk",
		"esh", "evs", "fa", "fid", "flit", "for", "fri", "fu", "gan",
		"gar", "glen", "gop", "gre", "ha", "he", "hyd", "i",
		"ing", "ion", "ip", "ish", "it", "ite", "iv", "jo",
		"kho", "kli", "klis", "la", "lech", "man", "mar",
		"me", "mi", "mic", "mik", "mon", "mung", "mur", "nag", "nej",
		"nelg", "nep", "ner", "nes", "nis", "nih", "nin", "o",
		"od", "ood", "org", "orn", "ox", "oxy", "pay", "pet",
		"ple", "plu", "po", "pot", "prok", "re", "rea", "rhov",
		"ri", "ro", "rog", "rok", "rol", "sa", "san", "sat",
		"see", "sef", "seh", "shu", "ski", "sna", "sne", "snik",
		"sno", "so", "sol", "sri", "sta", "sun", "ta", "tab",
		"tem", "ther", "ti", "tox", "trol", "tue", "turs", "u",
		"ulk", "um", "un", "uni", "ur", "val", "viv", "vly",
		"vom", "wah", "wed", "werg", "wex", "whon", "wun", "x",
		"yerg", "yp", "zun", "tri", "blaa", "jah", "bul", "on",
		"foo", "ju", "xuxu"
	};

	int testcounter = randint1(3) + 1;

	strcpy(out_string, "");

	if (randint1(3) == 2)
	{
		while (testcounter--)
			strcat(out_string, syllables[randint0(MAX_SYLLABLES)]);
	}
	else
	{
		char Syllable[80];
		testcounter = randint1(2) + 1;
		while (testcounter--)
		{
			(void)get_rnd_line("elvish.txt", 0, Syllable);
			strcat(out_string, Syllable);
		}
	}

	out_string[0] = toupper(out_string[1]);

	out_string[16] = '\0';
#endif
}

/*!
 * @brief ランダムな名前をアーティファクト銘として整形する。 / Create a name from random parts with quotes.
 * @param out_string 作成した名を保管する参照ポインタ
 * @return なし
 * @details get_table_name_aux()ほぼ完全に実装を依存している。
 */
void get_table_name(char *out_string)
{
	char buff[80];
	get_table_name_aux(buff);
	sprintf(out_string, _("『%s』", "'%s'"), buff);
}

/*!
 * @brief ランダムなシンダリン銘を作成する / Make random Sindarin name
 * @param out_string 作成した名を保管する参照ポインタ
 * @return なし
 * @details sname.txtが語幹の辞書となっている。
 */
void get_table_sindarin_aux(char *out_string)
{
	char Syllable[80];
#ifdef JP
	char tmp[80];
#endif

	get_rnd_line("sname.txt", 1, Syllable);
#ifdef JP
	strcpy(tmp, Syllable);
#else
	strcpy(out_string, Syllable);
#endif

	get_rnd_line("sname.txt", 2, Syllable);
#ifdef JP
	strcat(tmp, Syllable);
	sindarin_to_kana(out_string, tmp);
#else
	strcat(out_string, Syllable);
#endif
}

/*!
 * @brief シンダリン銘をアーティファクト用に整形する。 / Make random Sindarin name with quotes
 * @param out_string 作成した名を保管する参照ポインタ
 * @return なし
 * @details get_table_sindarin_aux()ほぼ完全に実装を依存している。
 */
void get_table_sindarin(char *out_string)
{
	char buff[80];
	get_table_sindarin_aux(buff);
	sprintf(out_string, _("『%s』", "'%s'"), buff);
}


/*!
 * @brief ベースアイテムの未確定名を共通tval間でシャッフルする / Shuffle flavor indices of a group of objects with given tval
 * @param tval シャッフルしたいtval
 * @return なし
 * @details 巻物、各種魔道具などに利用される。
 */
static void shuffle_flavors(byte tval)
{
	s16b *k_idx_list;
	int k_idx_list_num = 0;
	int i;

	/* Allocate an array for a list of k_idx */
	C_MAKE(k_idx_list, max_k_idx, s16b);

	/* Search objects with given tval for shuffle */
	for (i = 0; i < max_k_idx; i++)
	{
		object_kind *k_ptr = &k_info[i];

		/* Skip non-Rings */
		if (k_ptr->tval != tval) continue;

		/* Paranoia -- Skip objects without flavor */
		if (!k_ptr->flavor) continue;

		/* Skip objects with a fixed flavor name */
		if (have_flag(k_ptr->flags, TR_FIXED_FLAVOR)) continue;

		/* Remember k_idx */
		k_idx_list[k_idx_list_num] = i;

		/* Increase number of remembered indices */
		k_idx_list_num++;
	}

	/* Shuffle flavors */
	for (i = 0; i < k_idx_list_num; i++)
	{
		object_kind *k1_ptr = &k_info[k_idx_list[i]];
		object_kind *k2_ptr = &k_info[k_idx_list[randint0(k_idx_list_num)]];

		/* Swap flavors of this pair */
		s16b tmp = k1_ptr->flavor;
		k1_ptr->flavor = k2_ptr->flavor;
		k2_ptr->flavor = tmp;
	}

	/* Free an array for a list of k_idx */
	C_KILL(k_idx_list, max_k_idx, s16b);
}

/*!
 * @brief ゲーム開始時に行われるベースアイテムの初期化ルーチン / Prepare the "variable" part of the "k_info" array.
 * @return なし
 * @details
 * Prepare the "variable" part of the "k_info" array.\n
 *\n
 * The "color"/"metal"/"type" of an item is its "flavor".\n
 * For the most part, flavors are assigned randomly each game.\n
 *\n
 * Initialize descriptions for the "colored" objects, including:\n
 * Rings, Amulets, Staffs, Wands, Rods, Food, Potions, Scrolls.\n
 *\n
 * The first 4 entries for potions are fixed (Water, Apple Juice,\n
 * Slime Mold Juice, Unused Potion).\n
 *\n
 * Scroll titles are always between 6 and 14 letters long.  This is\n
 * ensured because every title is composed of whole words, where every\n
 * word is from 1 to 8 letters long (one or two syllables of 1 to 4\n
 * letters each), and that no scroll is finished until it attempts to\n
 * grow beyond 15 letters.  The first time this can happen is when the\n
 * current title has 6 letters and the new word has 8 letters, which\n
 * would result in a 6 letter scroll title.\n
 *\n
 * Duplicate titles are avoided by requiring that no two scrolls share\n
 * the same first four letters (not the most efficient method, and not\n
 * the least efficient method, but it will always work).\n
 *\n
 * Hack -- make sure everything stays the same for each saved game\n
 * This is accomplished by the use of a saved "random seed", as in\n
 * "town_gen()".  Since no other functions are called while the special\n
 * seed is in effect, so this function is pretty "safe".\n
 *\n
 * Note that the "hacked seed" may provide an RNG with alternating parity!\n
 */
void flavor_init(void)
{
	int i;
	u32b state_backup[4];

	/* Hack -- Backup the RNG state */
	Rand_state_backup(state_backup);

	/* Hack -- Induce consistant flavors */
	Rand_state_set(seed_flavor);


	/* Initialize flavor index of each object by itself */
	for (i = 0; i < max_k_idx; i++)
	{
		object_kind *k_ptr = &k_info[i];

		/* Skip objects without flavor name */
		if (!k_ptr->flavor_name) continue;

		/*
		 * Initialize flavor index to itself
		 *  -> Shuffle it later
		 */
		k_ptr->flavor = i;
	}

	/* Shuffle Rings */
	shuffle_flavors(TV_RING);

	/* Shuffle Amulets */
	shuffle_flavors(TV_AMULET);

	/* Shuffle Staves */
	shuffle_flavors(TV_STAFF);

	/* Shuffle Wands */
	shuffle_flavors(TV_WAND);

	/* Shuffle Rods */
	shuffle_flavors(TV_ROD);

	/* Shuffle Mushrooms */
	shuffle_flavors(TV_FOOD);

	/* Shuffle Potions */
	shuffle_flavors(TV_POTION);

	/* Shuffle Scrolls */
	shuffle_flavors(TV_SCROLL);


	/* Hack -- Restore the RNG state */
	Rand_state_restore(state_backup);

	/* Analyze every object */
	for (i = 1; i < max_k_idx; i++)
	{
		object_kind *k_ptr = &k_info[i];

		/* Skip "empty" objects */
		if (!k_ptr->name) continue;

		/* No flavor yields aware */
		if (!k_ptr->flavor) k_ptr->aware = TRUE;

		/* Check for "easily known" */
		k_ptr->easy_know = object_easy_know(i);
	}
}


/*!
 * @brief 対象文字配列に一文字だけをコピーする。
 * @param t 保管先文字列ポインタ
 * @param c 保管したい1文字
 * @return なし
 * @details
 * Print a char "c" into a string "t", as if by sprintf(t, "%c", c),\n
 * and return a pointer to the terminator (t + 1).\n
 */
static char *object_desc_chr(char *t, char c)
{
	/* Copy the char */
	*t++ = c;

	/* Terminate */
	*t = '\0';

	/* Result */
	return (t);
}

/*!
 * @brief 対象文字配列に文字列をコピーする。
 * @param t 保管先文字列ポインタ
 * @param s コピーしたい文字列ポインタ
 * @return 保管先の末尾アドレス
 * @details
 * Print a string "s" into a string "t", as if by strcpy(t, s),
 * and return a pointer to the terminator.
 */
static char *object_desc_str(char *t, cptr s)
{
	/* Copy the string */
	while (*s) *t++ = *s++;

	/* Terminate */
	*t = '\0';

	/* Result */
	return (t);
}

/*!
 * @brief 対象文字配列に符号なし整数値をコピーする。
 * @param t 保管先文字列ポインタ
 * @param n コピーしたい数値
 * @return なし
 * @details
 * Print an unsigned number "n" into a string "t", as if by
 * sprintf(t, "%u", n), and return a pointer to the terminator.
 */
static char *object_desc_num(char *t, uint n)
{
	uint p;

	/* Find "size" of "n" */
	for (p = 1; n >= p * 10; p = p * 10) /* loop */;

	/* Dump each digit */
	while (p >= 1)
	{
		/* Dump the digit */
		*t++ = '0' + n / p;

		/* Remove the digit */
		n = n % p;

		/* Process next digit */
		p = p / 10;
	}

	/* Terminate */
	*t = '\0';

	/* Result */
	return (t);
}




#ifdef JP
/*!
 * @brief 日本語の個数表示ルーチン
 * @param t 保管先文字列ポインタ
 * @param o_ptr 記述したいオブジェクトの構造体参照ポインタ
 * @return なし
 * @details
 * cmd1.c で流用するために object_desc_japanese から移動した。
 */
char *object_desc_kosuu(char *t, object_type *o_ptr)
{
    t = object_desc_num(t, o_ptr->number);

    switch (o_ptr->tval)
    {
      case TV_BOLT:
      case TV_ARROW:
      case TV_POLEARM:
      case TV_STAFF:
      case TV_WAND:
      case TV_ROD:
      case TV_DIGGING:
      {
	  t = object_desc_str(t, "本");
	  break;
      }
      case TV_SCROLL:
      {
	  t = object_desc_str(t, "巻");
	  break;
      }
      case TV_POTION:
      {
	  t = object_desc_str(t, "服");
	  break;
      }
      case  TV_LIFE_BOOK:
      case  TV_SORCERY_BOOK:
      case  TV_NATURE_BOOK:
      case  TV_CHAOS_BOOK:
      case  TV_DEATH_BOOK:
      case  TV_TRUMP_BOOK:
      case  TV_ARCANE_BOOK:
      case  TV_CRAFT_BOOK:
      case  TV_DAEMON_BOOK:
      case  TV_CRUSADE_BOOK:
      case  TV_MUSIC_BOOK:
      case  TV_HISSATSU_BOOK:
	  case TV_HEX_BOOK:
      {
	  t = object_desc_str(t, "冊");
	  break;
      }
      case TV_SOFT_ARMOR:
      case TV_HARD_ARMOR:
      case TV_DRAG_ARMOR:
      case TV_CLOAK:
      {
	  t = object_desc_str(t, "着");
	  break;
      }
      case TV_SWORD:
      case TV_HAFTED:
      case TV_BOW:
      {
	  t = object_desc_str(t, "振");
	  break;
      }
      case TV_BOOTS:
      {
	  t = object_desc_str(t, "足");
	  break;
      }
      case TV_CARD:
      {
	  t = object_desc_str(t, "枚");
	  break;
      }
	    /* 食べもの by ita */
      case TV_FOOD:
      {
	  if(o_ptr->sval == SV_FOOD_JERKY)
	  {
	      t = object_desc_str(t, "切れ");
	      break;
	  }
      }
      default:
      {
	  if (o_ptr->number < 10)
	  {
	      t = object_desc_str(t, "つ");
	  }
	  else
	  {
	      t = object_desc_str(t, "個");
	  }
	  break;
      }
  }
  return (t);		       
}
#endif

/*!
 * @brief 対象文字配列に符号あり整数値をコピーする。
 * @param t 保管先文字列ポインタ
 * @param v コピーしたい数値
 * @return なし
 * @details
 * Print an signed number "v" into a string "t", as if by
 * sprintf(t, "%+d", n), and return a pointer to the terminator.
 * Note that we always print a sign, either "+" or "-".
 */
static char *object_desc_int(char *t, sint v)
{
	uint p, n;

	/* Negative */
	if (v < 0)
	{
		/* Take the absolute value */
		n = 0 - v;

		/* Use a "minus" sign */
		*t++ = '-';
	}

	/* Positive (or zero) */
	else
	{
		/* Use the actual number */
		n = v;

		/* Use a "plus" sign */
		*t++ = '+';
	}

	/* Find "size" of "n" */
	for (p = 1; n >= p * 10; p = p * 10) /* loop */;

	/* Dump each digit */
	while (p >= 1)
	{
		/* Dump the digit */
		*t++ = '0' + n / p;

		/* Remove the digit */
		n = n % p;

		/* Process next digit */
		p = p / 10;
	}

	/* Terminate */
	*t = '\0';

	/* Result */
	return (t);
}


/*!
 * オブジェクトの特性表示記号テーブルの構造体 / Structs and tables for Auto Inscription for flags
 */
typedef struct flag_insc_table
{
#ifdef JP
	cptr japanese;
#endif
	cptr english;
	int flag;
	int except_flag;
} flag_insc_table;

#ifdef JP
/*! オブジェクトの特性表示記号テーブルの定義(pval要素) */
static flag_insc_table flag_insc_plus[] =
{
	{ "攻", "At", TR_BLOWS, -1 },
	{ "速", "Sp", TR_SPEED, -1 },
	{ "腕", "St", TR_STR, -1 },
	{ "知", "In", TR_INT, -1 },
	{ "賢", "Wi", TR_WIS, -1 },
	{ "器", "Dx", TR_DEX, -1 },
	{ "耐", "Cn", TR_CON, -1 },
	{ "魅", "Ch", TR_CHR, -1 },
	{ "道", "Md", TR_MAGIC_MASTERY, -1 },
	{ "隠", "Sl", TR_STEALTH, -1 },
	{ "探", "Sr", TR_SEARCH, -1 },
	{ "赤", "If", TR_INFRA, -1 },
	{ "掘", "Dg", TR_TUNNEL, -1 },
	{ NULL, NULL, 0, -1 }
};

/*! オブジェクトの特性表示記号テーブルの定義(免疫) */
static flag_insc_table flag_insc_immune[] =
{
	{ "酸", "Ac", TR_IM_ACID, -1 },
	{ "電", "El", TR_IM_ELEC, -1 },
	{ "火", "Fi", TR_IM_FIRE, -1 },
	{ "冷", "Co", TR_IM_COLD, -1 },
	{ NULL, NULL, 0, -1 }
};

/*! オブジェクトの特性表示記号テーブルの定義(耐性) */
static flag_insc_table flag_insc_resistance[] =
{
	{ "酸", "Ac", TR_RES_ACID, TR_IM_ACID },
	{ "電", "El", TR_RES_ELEC, TR_IM_ELEC },
	{ "火", "Fi", TR_RES_FIRE, TR_IM_FIRE },
	{ "冷", "Co", TR_RES_COLD, TR_IM_COLD },
	{ "毒", "Po", TR_RES_POIS, -1 },
	{ "閃", "Li", TR_RES_LITE, -1 },
	{ "暗", "Dk", TR_RES_DARK, -1 },
	{ "破", "Sh", TR_RES_SHARDS, -1 },
	{ "盲", "Bl", TR_RES_BLIND, -1 },
	{ "乱", "Cf", TR_RES_CONF, -1 },
	{ "轟", "So", TR_RES_SOUND, -1 },
	{ "獄", "Nt", TR_RES_NETHER, -1 },
	{ "因", "Nx", TR_RES_NEXUS, -1 },
	{ "沌", "Ca", TR_RES_CHAOS, -1 },
	{ "劣", "Di", TR_RES_DISEN, -1 },
	{ "恐", "Fe", TR_RES_FEAR, -1 },
	{ NULL, NULL, 0, -1 }
};

/*! オブジェクトの特性表示記号テーブルの定義(その他特性) */
static flag_insc_table flag_insc_misc[] =
{
	{ "易", "Es", TR_EASY_SPELL, -1 },
	{ "減", "Dm", TR_DEC_MANA, -1 },
	{ "投", "Th", TR_THROW, -1 },
	{ "反", "Rf", TR_REFLECT, -1 },
	{ "麻", "Fa", TR_FREE_ACT, -1 },
	{ "視", "Si", TR_SEE_INVIS, -1 },
	{ "経", "Hl", TR_HOLD_EXP, -1 },
	{ "遅", "Sd", TR_SLOW_DIGEST, -1 },
	{ "活", "Rg", TR_REGEN, -1 },
	{ "浮", "Lv", TR_LEVITATION, -1 },
	{ "明", "Lu", TR_LITE_1, -1 },
	{ "明", "Lu", TR_LITE_2, -1 },
	{ "明", "Lu", TR_LITE_3, -1 },
	{ "闇", "Dl", TR_LITE_M1, -1 },
	{ "闇", "Dl", TR_LITE_M2, -1 },
	{ "闇", "Dl", TR_LITE_M3, -1 },
	{ "警", "Wr", TR_WARNING, -1 },
	{ "倍", "Xm", TR_XTRA_MIGHT, -1 },
	{ "射", "Xs", TR_XTRA_SHOTS, -1 },
	{ "瞬", "Te", TR_TELEPORT, -1 },
	{ "怒", "Ag", TR_AGGRAVATE, -1 },
	{ "祝", "Bs", TR_BLESSED, -1 },
	{ "忌", "Ty", TR_TY_CURSE, -1 },
	{ "呪", "C-", TR_ADD_L_CURSE, -1 },
	{ "詛", "C+", TR_ADD_H_CURSE, -1 },
	{ NULL, NULL, 0, -1 }
};

/*! オブジェクトの特性表示記号テーブルの定義(オーラ) */
static flag_insc_table flag_insc_aura[] =
{
	{ "炎", "F", TR_SH_FIRE, -1 },
	{ "電", "E", TR_SH_ELEC, -1 },
	{ "冷", "C", TR_SH_COLD, -1 },
	{ "魔", "M", TR_NO_MAGIC, -1 },
	{ "瞬", "T", TR_NO_TELE, -1 },
	{ NULL, NULL, 0, -1 }
};

/*! オブジェクトの特性表示記号テーブルの定義(属性スレイ) */
static flag_insc_table flag_insc_brand[] =
{
	{ "酸", "A", TR_BRAND_ACID, -1 },
	{ "電", "E", TR_BRAND_ELEC, -1 },
	{ "焼", "F", TR_BRAND_FIRE, -1 },
	{ "凍", "Co", TR_BRAND_COLD, -1 },
	{ "毒", "P", TR_BRAND_POIS, -1 },
	{ "沌", "Ca", TR_CHAOTIC, -1 },
	{ "吸", "V", TR_VAMPIRIC, -1 },
	{ "震", "Q", TR_IMPACT, -1 },
	{ "切", "S", TR_VORPAL, -1 },
	{ "理", "M", TR_FORCE_WEAPON, -1 },
	{ NULL, NULL, 0, -1 }
};

/*! オブジェクトの特性表示記号テーブルの定義(種族スレイ) */
static flag_insc_table flag_insc_kill[] =
{
	{ "邪", "*", TR_KILL_EVIL, -1 },
	{ "人", "p", TR_KILL_HUMAN, -1 },
	{ "龍", "D", TR_KILL_DRAGON, -1 },
	{ "オ", "o", TR_KILL_ORC, -1 },
	{ "ト", "T", TR_KILL_TROLL, -1 },
	{ "巨", "P", TR_KILL_GIANT, -1 },
	{ "デ", "U", TR_KILL_DEMON, -1 },
	{ "死", "L", TR_KILL_UNDEAD, -1 },
	{ "動", "Z", TR_KILL_ANIMAL, -1 },
	{ NULL, NULL, 0, -1 }
};

/*! オブジェクトの特性表示記号テーブルの定義(種族*スレイ*) */
static flag_insc_table flag_insc_slay[] =
{
	{ "邪", "*", TR_SLAY_EVIL, TR_KILL_EVIL },
	{ "人", "p", TR_SLAY_HUMAN, TR_KILL_HUMAN },
	{ "竜", "D", TR_SLAY_DRAGON, TR_KILL_DRAGON },
	{ "オ", "o", TR_SLAY_ORC, TR_KILL_ORC },
	{ "ト", "T", TR_SLAY_TROLL, TR_KILL_TROLL },
	{ "巨", "P", TR_SLAY_GIANT, TR_KILL_GIANT },
	{ "デ", "U", TR_SLAY_DEMON, TR_KILL_DEMON },
	{ "死", "L", TR_SLAY_UNDEAD, TR_KILL_UNDEAD },
	{ "動", "Z", TR_SLAY_ANIMAL, TR_KILL_ANIMAL },
	{ NULL, NULL, 0, -1 }
};

/*! オブジェクトの特性表示記号テーブルの定義(ESP1) */
static flag_insc_table flag_insc_esp1[] =
{
	{ "感", "Tele", TR_TELEPATHY, -1 },
	{ "邪", "Evil", TR_ESP_EVIL, -1 },
	{ "善", "Good", TR_ESP_GOOD, -1 },
	{ "無", "Nolv", TR_ESP_NONLIVING, -1 },
	{ "個", "Uniq", TR_ESP_UNIQUE, -1 },
	{ NULL, NULL, 0, -1 }
};

/*! オブジェクトの特性表示記号テーブルの定義(ESP2) */
static flag_insc_table flag_insc_esp2[] =
{
	{ "人", "p", TR_ESP_HUMAN, -1 },
	{ "竜", "D", TR_ESP_DRAGON, -1 },
	{ "オ", "o", TR_ESP_ORC, -1 },
	{ "ト", "T", TR_ESP_TROLL, -1 },
	{ "巨", "P", TR_ESP_GIANT, -1 },
	{ "デ", "U", TR_ESP_DEMON, -1 },
	{ "死", "L", TR_ESP_UNDEAD, -1 },
	{ "動", "Z", TR_ESP_ANIMAL, -1 },
	{ NULL, NULL, 0, -1 }
};

/*! オブジェクトの特性表示記号テーブルの定義(能力維持) */
static flag_insc_table flag_insc_sust[] =
{
	{ "腕", "St", TR_SUST_STR, -1 },
	{ "知", "In", TR_SUST_INT, -1 },
	{ "賢", "Wi", TR_SUST_WIS, -1 },
	{ "器", "Dx", TR_SUST_DEX, -1 },
	{ "耐", "Cn", TR_SUST_CON, -1 },
	{ "魅", "Ch", TR_SUST_CHR, -1 },
	{ NULL, NULL, 0, -1 }
};

#else
static flag_insc_table flag_insc_plus[] =
{
	{ "At", TR_BLOWS, -1 },
	{ "Sp", TR_SPEED, -1 },
	{ "St", TR_STR, -1 },
	{ "In", TR_INT, -1 },
	{ "Wi", TR_WIS, -1 },
	{ "Dx", TR_DEX, -1 },
	{ "Cn", TR_CON, -1 },
	{ "Ch", TR_CHR, -1 },
	{ "Md", TR_MAGIC_MASTERY, -1 },
	{ "Sl", TR_STEALTH, -1 },
	{ "Sr", TR_SEARCH, -1 },
	{ "If", TR_INFRA, -1 },
	{ "Dg", TR_TUNNEL, -1 },
	{ NULL, 0, -1 }
};

static flag_insc_table flag_insc_immune[] =
{
	{ "Ac", TR_IM_ACID, -1 },
	{ "El", TR_IM_ELEC, -1 },
	{ "Fi", TR_IM_FIRE, -1 },
	{ "Co", TR_IM_COLD, -1 },
	{ NULL, 0, -1 }
};

static flag_insc_table flag_insc_resistance[] =
{
	{ "Ac", TR_RES_ACID, TR_IM_ACID },
	{ "El", TR_RES_ELEC, TR_IM_ELEC },
	{ "Fi", TR_RES_FIRE, TR_IM_FIRE },
	{ "Co", TR_RES_COLD, TR_IM_COLD },
	{ "Po", TR_RES_POIS, -1 },
	{ "Li", TR_RES_LITE, -1 },
	{ "Dk", TR_RES_DARK, -1 },
	{ "Sh", TR_RES_SHARDS, -1 },
	{ "Bl", TR_RES_BLIND, -1 },
	{ "Cf", TR_RES_CONF, -1 },
	{ "So", TR_RES_SOUND, -1 },
	{ "Nt", TR_RES_NETHER, -1 },
	{ "Nx", TR_RES_NEXUS, -1 },
	{ "Ca", TR_RES_CHAOS, -1 },
	{ "Di", TR_RES_DISEN, -1 },
	{ "Fe", TR_RES_FEAR, -1 },
	{ NULL, 0, -1 }
};

static flag_insc_table flag_insc_misc[] =
{
	{ "Es", TR_EASY_SPELL, -1 },
	{ "Dm", TR_DEC_MANA, -1 },
	{ "Th", TR_THROW, -1 },
	{ "Rf", TR_REFLECT, -1 },
	{ "Fa", TR_FREE_ACT, -1 },
	{ "Si", TR_SEE_INVIS, -1 },
	{ "Hl", TR_HOLD_EXP, -1 },
	{ "Sd", TR_SLOW_DIGEST, -1 },
	{ "Rg", TR_REGEN, -1 },
	{ "Lv", TR_LEVITATION, -1 },
	{ "Lu", TR_LITE_1, -1 },
	{ "Lu", TR_LITE_2, -1 },
	{ "Lu", TR_LITE_3, -1 },
	{ "Dl", TR_LITE_M1, -1 },
	{ "Dl", TR_LITE_M2, -1 },
	{ "Dl", TR_LITE_M3, -1 },
	{ "Wr", TR_WARNING, -1 },
	{ "Xm", TR_XTRA_MIGHT, -1 },
	{ "Xs", TR_XTRA_SHOTS, -1 },
	{ "Te", TR_TELEPORT, -1 },
	{ "Ag", TR_AGGRAVATE, -1 },
	{ "Bs", TR_BLESSED, -1 },
	{ "Ty", TR_TY_CURSE, -1 },
	{ "C-", TR_ADD_L_CURSE, -1 },
	{ "C+", TR_ADD_H_CURSE, -1 },
	{ NULL, 0, -1 }
};

static flag_insc_table flag_insc_aura[] =
{
	{ "F", TR_SH_FIRE, -1 },
	{ "E", TR_SH_ELEC, -1 },
	{ "C", TR_SH_COLD, -1 },
	{ "M", TR_NO_MAGIC, -1 },
	{ "T", TR_NO_TELE, -1 },
	{ NULL, 0, -1 }
};

static flag_insc_table flag_insc_brand[] =
{
	{ "A", TR_BRAND_ACID, -1 },
	{ "E", TR_BRAND_ELEC, -1 },
	{ "F", TR_BRAND_FIRE, -1 },
	{ "Co", TR_BRAND_COLD, -1 },
	{ "P", TR_BRAND_POIS, -1 },
	{ "Ca", TR_CHAOTIC, -1 },
	{ "V", TR_VAMPIRIC, -1 },
	{ "Q", TR_IMPACT, -1 },
	{ "S", TR_VORPAL, -1 },
	{ "M", TR_FORCE_WEAPON, -1 },
	{ NULL, 0, -1 }
};

static flag_insc_table flag_insc_kill[] =
{
	{ "*", TR_KILL_EVIL, -1 },
	{ "p", TR_KILL_HUMAN, -1 },
	{ "D", TR_KILL_DRAGON, -1 },
	{ "o", TR_KILL_ORC, -1 },
	{ "T", TR_KILL_TROLL, -1 },
	{ "P", TR_KILL_GIANT, -1 },
	{ "U", TR_KILL_DEMON, -1 },
	{ "L", TR_KILL_UNDEAD, -1 },
	{ "Z", TR_KILL_ANIMAL, -1 },
	{ NULL, 0, -1 }
};

static flag_insc_table flag_insc_slay[] =
{
	{ "*", TR_SLAY_EVIL, TR_KILL_EVIL },
	{ "p", TR_SLAY_HUMAN, TR_KILL_HUMAN },
	{ "D", TR_SLAY_DRAGON, TR_KILL_DRAGON },
	{ "o", TR_SLAY_ORC, TR_KILL_ORC },
	{ "T", TR_SLAY_TROLL, TR_KILL_TROLL },
	{ "P", TR_SLAY_GIANT, TR_KILL_GIANT },
	{ "U", TR_SLAY_DEMON, TR_KILL_DEMON },
	{ "L", TR_SLAY_UNDEAD, TR_KILL_UNDEAD },
	{ "Z", TR_SLAY_ANIMAL, TR_KILL_ANIMAL },
	{ NULL, 0, -1 }
};

static flag_insc_table flag_insc_esp1[] =
{
	{ "Tele", TR_TELEPATHY, -1 },
	{ "Evil", TR_ESP_EVIL, -1 },
	{ "Good", TR_ESP_GOOD, -1 },
	{ "Nolv", TR_ESP_NONLIVING, -1 },
	{ "Uniq", TR_ESP_UNIQUE, -1 },
	{ NULL, 0, -1 }
};

static flag_insc_table flag_insc_esp2[] =
{
	{ "p", TR_ESP_HUMAN, -1 },
	{ "D", TR_ESP_DRAGON, -1 },
	{ "o", TR_ESP_ORC, -1 },
	{ "T", TR_ESP_TROLL, -1 },
	{ "P", TR_ESP_GIANT, -1 },
	{ "U", TR_ESP_DEMON, -1 },
	{ "L", TR_ESP_UNDEAD, -1 },
	{ "Z", TR_ESP_ANIMAL, -1 },
	{ NULL, 0, -1 }
};

static flag_insc_table flag_insc_sust[] =
{
	{ "St", TR_SUST_STR, -1 },
	{ "In", TR_SUST_INT, -1 },
	{ "Wi", TR_SUST_WIS, -1 },
	{ "Dx", TR_SUST_DEX, -1 },
	{ "Cn", TR_SUST_CON, -1 },
	{ "Ch", TR_SUST_CHR, -1 },
	{ NULL, 0, -1 }
};
#endif

/* オブジェクトフラグを追加するための簡易なマクロ / Simple macro for get_inscription() */
#define ADD_INSC(STR) (void)(ptr = object_desc_str(ptr, (STR)))

/*!
 * @brief get_inscriptionのサブセットとしてオブジェクトの特性フラグを返す / Helper function for get_inscription()
 * @param fi_ptr 参照する特性表示記号テーブル
 * @param flgs 対応するオブジェクトのフラグ文字列
 * @param kanji TRUEならば漢字記述/FALSEならば英語記述
 * @param ptr フラグ群を保管する文字列参照ポインタ
 * @return フラグ群を保管する文字列参照ポインタ(ptrと同じ)
 * @details
 * Print an signed number "v" into a string "t", as if by
 * sprintf(t, "%+d", n), and return a pointer to the terminator.
 * Note that we always print a sign, either "+" or "-".
 */
static char *inscribe_flags_aux(flag_insc_table *fi_ptr, u32b flgs[TR_FLAG_SIZE], bool kanji, char *ptr)
{
#ifndef JP
	(void)kanji;
#endif

	while (fi_ptr->english)
	{
		if (have_flag(flgs, fi_ptr->flag) &&
		    (fi_ptr->except_flag == -1 || !have_flag(flgs, fi_ptr->except_flag)))
#ifdef JP
			ADD_INSC(kanji ? fi_ptr->japanese : fi_ptr->english);
#else
			ADD_INSC(fi_ptr->english);
#endif
		fi_ptr++;
	}

	return ptr;
}


/*!
 * @brief オブジェクトの特性表示記号テーブル1つに従いオブジェクトの特性フラグ配列に1つでも該当の特性があるかを返す / Special variation of have_flag for auto-inscription
 * @param fi_ptr 参照する特性表示記号テーブル
 * @param flgs 対応するオブジェクトのフラグ文字列
 * @return 1つでも該当の特性があったらTRUEを返す。
 */
static bool have_flag_of(flag_insc_table *fi_ptr, u32b flgs[TR_FLAG_SIZE])
{
	while (fi_ptr->english)
	{
		if (have_flag(flgs, fi_ptr->flag) &&
		   (fi_ptr->except_flag == -1 || !have_flag(flgs, fi_ptr->except_flag)))
			return (TRUE);
		fi_ptr++;
	}

	return (FALSE);
}

/*!
 * @brief オブジェクト名の特性短縮表記をまとめて提示する。
 * @param ptr 特性短縮表記を格納する文字列ポインタ
 * @param o_ptr 特性短縮表記を得たいオブジェクト構造体の参照ポインタ
 * @param kanji TRUEならば漢字表記 / FALSEなら英語表記
 * @param all TRUEならばベースアイテム上で明らかなフラグは省略する
 * @return ptrと同じアドレス
 */
static char *get_ability_abbreviation(char *ptr, object_type *o_ptr, bool kanji, bool all)
{
	char *prev_ptr = ptr;
	u32b flgs[TR_FLAG_SIZE];

	/* Extract the flags */
	object_flags(o_ptr, flgs);

	/* Remove obvious flags */
	if (!all)
	{
		object_kind *k_ptr = &k_info[o_ptr->k_idx];
		int j;
				
		/* Base object */
		for (j = 0; j < TR_FLAG_SIZE; j++)
			flgs[j] &= ~k_ptr->flags[j];

		if (object_is_fixed_artifact(o_ptr))
		{
			artifact_type *a_ptr = &a_info[o_ptr->name1];
					
			for (j = 0; j < TR_FLAG_SIZE; j++)
				flgs[j] &= ~a_ptr->flags[j];
		}

		if (object_is_ego(o_ptr))
		{
			ego_item_type *e_ptr = &e_info[o_ptr->name2];
					
			for (j = 0; j < TR_FLAG_SIZE; j++)
				flgs[j] &= ~e_ptr->flags[j];
		}
	}

	/* Remove lite flags when this is a dark lite object */
	if (have_dark_flag(flgs))
	{
		if (have_flag(flgs, TR_LITE_1)) remove_flag(flgs, TR_LITE_1);
		if (have_flag(flgs, TR_LITE_2)) remove_flag(flgs, TR_LITE_2);
		if (have_flag(flgs, TR_LITE_3)) remove_flag(flgs, TR_LITE_3);
	}
	else if (have_lite_flag(flgs))
	{
		add_flag(flgs, TR_LITE_1);
		if (have_flag(flgs, TR_LITE_2)) remove_flag(flgs, TR_LITE_2);
		if (have_flag(flgs, TR_LITE_3)) remove_flag(flgs, TR_LITE_3);
	}

	/* Plusses */
	if (have_flag_of(flag_insc_plus, flgs))
	{
		if (kanji)
			ADD_INSC("+");
	}
	ptr = inscribe_flags_aux(flag_insc_plus, flgs, kanji, ptr);

	/* Immunity */
	if (have_flag_of(flag_insc_immune, flgs))
	{
		if (!kanji && ptr != prev_ptr)
		{
			ADD_INSC(";");
			prev_ptr = ptr;
		}
		ADD_INSC("*");
	}
	ptr = inscribe_flags_aux(flag_insc_immune, flgs, kanji, ptr);

	/* Resistance */
	if (have_flag_of(flag_insc_resistance, flgs))
	{
		if (kanji)
			ADD_INSC("r");
		else if (ptr != prev_ptr)
		{
			ADD_INSC(";");
			prev_ptr = ptr;
		}
	}
	ptr = inscribe_flags_aux(flag_insc_resistance, flgs, kanji, ptr);

	/* Misc Ability */
	if (have_flag_of(flag_insc_misc, flgs))
	{
		if (ptr != prev_ptr)
		{
			ADD_INSC(";");
			prev_ptr = ptr;
		}
	}
	ptr = inscribe_flags_aux(flag_insc_misc, flgs, kanji, ptr);

	/* Aura */
	if (have_flag_of(flag_insc_aura, flgs))
	{
		ADD_INSC("[");
	}
	ptr = inscribe_flags_aux(flag_insc_aura, flgs, kanji, ptr);

	/* Brand Weapon */
	if (have_flag_of(flag_insc_brand, flgs))
		ADD_INSC("|");
	ptr = inscribe_flags_aux(flag_insc_brand, flgs, kanji, ptr);

	/* Kill Weapon */
	if (have_flag_of(flag_insc_kill, flgs))
		ADD_INSC("/X");
	ptr = inscribe_flags_aux(flag_insc_kill, flgs, kanji, ptr);

	/* Slay Weapon */
	if (have_flag_of(flag_insc_slay, flgs))
		ADD_INSC("/");
	ptr = inscribe_flags_aux(flag_insc_slay, flgs, kanji, ptr);

	/* Esp */
	if (kanji)
	{
		if (have_flag_of(flag_insc_esp1, flgs) ||
		    have_flag_of(flag_insc_esp2, flgs))
			ADD_INSC("~");
		ptr = inscribe_flags_aux(flag_insc_esp1, flgs, kanji, ptr);
		ptr = inscribe_flags_aux(flag_insc_esp2, flgs, kanji, ptr);
	}
	else
	{
		if (have_flag_of(flag_insc_esp1, flgs))
			ADD_INSC("~");
		ptr = inscribe_flags_aux(flag_insc_esp1, flgs, kanji, ptr);
		if (have_flag_of(flag_insc_esp2, flgs))
			ADD_INSC("~");
		ptr = inscribe_flags_aux(flag_insc_esp2, flgs, kanji, ptr);
	}

	/* sustain */
	if (have_flag_of(flag_insc_sust, flgs))
	{
		ADD_INSC("(");
	}
	ptr = inscribe_flags_aux(flag_insc_sust, flgs, kanji, ptr);

	*ptr = '\0';

	return ptr;
}


/*!
 * @brief オブジェクト名の特性短縮表記＋刻み内容を提示する。 / Get object inscription with auto inscription of object flags.
 * @param buff 特性短縮表記を格納する文字列ポインタ
 * @param o_ptr 特性短縮表記を得たいオブジェクト構造体の参照ポインタ
 * @return なし
 */
static void get_inscription(char *buff, object_type *o_ptr)
{
	cptr insc = quark_str(o_ptr->inscription);
	char *ptr = buff;

	/* Not fully identified */
	if (!(o_ptr->ident & IDENT_MENTAL))
	{
		/* Copy until end of line or '#' */
		while (*insc)
		{
			if (*insc == '#') break;
#ifdef JP
			if (iskanji(*insc)) *buff++ = *insc++;
#endif
			*buff++ = *insc++;
		}

		*buff = '\0';
		return;
	}

	*buff = '\0';
	for (; *insc; insc++)
	{
		/* Ignore fake artifact inscription */
		if (*insc == '#') break;

		/* {%} will be automatically converted */
		else if ('%' == *insc)
		{
			bool kanji = FALSE;
			bool all;
			cptr start = ptr;

			/* check for too long inscription */
			if (ptr >= buff + MAX_NLEN) continue;

#ifdef JP
			if ('%' == insc[1])
			{
				insc++;
				kanji = FALSE;
			}
			else
			{
				kanji = TRUE;
			}
#endif
			if ('a' == insc[1] && 'l' == insc[2] && 'l' == insc[3])
			{
				all = TRUE;
				insc += 3;
			}
			else
			{
				all = FALSE;
			}

			ptr = get_ability_abbreviation(ptr, o_ptr, kanji, all);

			if (ptr == start)
				ADD_INSC(" ");
		}
		else
		{
			*ptr++ = *insc;
		}
	}
	*ptr = '\0';
}

/*!
 * @brief オブジェクトがクエストの達成目的か否かを返す。
 * @param o_ptr 特性短縮表記を得たいオブジェクト構造体の参照ポインタ
 * @return 現在クエスト達成目的のアイテムならばTRUEを返す。
 */
bool object_is_quest_target(object_type *o_ptr)
{
	if (p_ptr->inside_quest)
	{
		int a_idx = quest[p_ptr->inside_quest].k_idx;
		if (a_idx)
		{
			artifact_type *a_ptr = &a_info[a_idx];
			if (!(a_ptr->gen_flags & TRG_INSTA_ART))
			{
				if((o_ptr->tval == a_ptr->tval) && (o_ptr->sval == a_ptr->sval))
				{
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}


/*!
 * @brief オブジェクトの各表記を返すメイン関数 / Creates a description of the item "o_ptr", and stores it in "out_val".
 * @param buf 表記を返すための文字列参照ポインタ
 * @param o_ptr 特性短縮表記を得たいオブジェクト構造体の参照ポインタ
 * @param mode 表記に関するオプション指定
 * @return 現在クエスト達成目的のアイテムならばTRUEを返す。
 * @details
 * One can choose the "verbosity" of the description, including whether\n
 * or not the "number" of items should be described, and how much detail\n
 * should be used when describing the item.\n
 *\n
 * The given "buf" must be MAX_NLEN chars long to hold the longest possible\n
 * description, which can get pretty long, including incriptions, such as:\n
 * "no more Maces of Disruption (Defender) (+10,+10) [+5] (+3 to stealth)".\n
 * Note that the inscription will be clipped to keep the total description\n
 * under MAX_NLEN-1 chars (plus a terminator).\n
 *\n
 * Note the use of "object_desc_num()" and "object_desc_int()" as hyper-efficient,\n
 * portable, versions of some common "sprintf()" commands.\n
 *\n
 * Note that all ego-items (when known) append an "Ego-Item Name", unless\n
 * the item is also an artifact, which should NEVER happen.\n
 *\n
 * Note that all artifacts (when known) append an "Artifact Name", so we\n
 * have special processing for "Specials" (artifact Lites, Rings, Amulets).\n
 * The "Specials" never use "modifiers" if they are "known", since they\n
 * have special "descriptions", such as "The Necklace of the Dwarves".\n
 *\n
 * Special Lite's use the "k_info" base-name (Phial, Star, or Arkenstone),\n
 * plus the artifact name, just like any other artifact, if known.\n
 *\n
 * Special Ring's and Amulet's, if not "aware", use the same code as normal\n
 * rings and amulets, and if "aware", use the "k_info" base-name (Ring or\n
 * Amulet or Necklace).  They will NEVER "append" the "k_info" name.  But,\n
 * they will append the artifact name, just like any artifact, if known.\n
 *\n
 * Hack -- Display "The One Ring" as "a Plain Gold Ring" until aware.\n
 *\n
 * Mode:\n
 *   OD_NAME_ONLY        : The Cloak of Death\n
 *   OD_NAME_AND_ENCHANT : The Cloak of Death [1,+3]\n
 *   OD_OMIT_INSCRIPTION : The Cloak of Death [1,+3] (+2 to Stealth)\n
 *   0                   : The Cloak of Death [1,+3] (+2 to Stealth) {nifty}\n
 *\n
 *   OD_OMIT_PREFIX      : Forbidden numeric prefix\n
 *   OD_NO_PLURAL        : Forbidden use of plural \n
 *   OD_STORE            : Assume to be aware and known\n
 *   OD_NO_FLAVOR        : Allow to hidden flavor\n
 *   OD_FORCE_FLAVOR     : Get un-shuffled flavor name\n
 */
void object_desc(char *buf, object_type *o_ptr, u32b mode)
{
	/* Extract object kind name */
	cptr            kindname = k_name + k_info[o_ptr->k_idx].name;

	/* Extract default "base" string */
	cptr            basenm = kindname;

	/* Assume no "modifier" string */
	cptr            modstr = "";

	int             power;
	int				fire_rate;

	bool            aware = FALSE;
	bool            known = FALSE;
	bool            flavor = TRUE;

	bool            show_weapon = FALSE;
	bool            show_armour = FALSE;

	cptr            s, s0;
	char            *t;

	char            p1 = '(', p2 = ')';
	char            b1 = '[', b2 = ']';
	char            c1 = '{', c2 = '}';

	char            tmp_val[MAX_NLEN+160];
	char            tmp_val2[MAX_NLEN+10];
	char            fake_insc_buf[30];

	u32b flgs[TR_FLAG_SIZE];

	object_type *bow_ptr;

	object_kind *k_ptr = &k_info[o_ptr->k_idx];
	object_kind *flavor_k_ptr = &k_info[k_ptr->flavor];

	/* Extract some flags */
	object_flags(o_ptr, flgs);

	/* See if the object is "aware" */
	if (object_is_aware(o_ptr)) aware = TRUE;

	/* See if the object is "known" */
	if (object_is_known(o_ptr)) known = TRUE;

	/* Allow flavors to be hidden when aware */
	if (aware && ((mode & OD_NO_FLAVOR) || plain_descriptions)) flavor = FALSE;

	/* Object is in the inventory of a store or spoiler */
	if ((mode & OD_STORE) || (o_ptr->ident & IDENT_STORE))
	{
		/* Don't show flavors */
		flavor = FALSE;

		/* Pretend known and aware */
		aware = TRUE;
		known = TRUE;
	}

	/* Force to be flavor name only */
	if (mode & OD_FORCE_FLAVOR)
	{
		aware = FALSE;
		flavor = TRUE;
		known = FALSE;

		/* Cancel shuffling */
		flavor_k_ptr = k_ptr;
	}

	/* Analyze the object */
	switch (o_ptr->tval)
	{
		/* Some objects are easy to describe */
		case TV_SKELETON:
		case TV_BOTTLE:
		case TV_JUNK:
		case TV_SPIKE:
		case TV_FLASK:
		case TV_CHEST:
		case TV_WHISTLE:
		{
			break;
		}

		case TV_CAPTURE:
		{
			monster_race *r_ptr = &r_info[o_ptr->pval];

			if (known)
			{
				if (!o_ptr->pval)
				{
					modstr = _(" (空)", " (empty)");
				}
				else
				{
#ifdef JP
					sprintf(tmp_val2, " (%s)",r_name + r_ptr->name);
					modstr = tmp_val2;
#else
					cptr t = r_name + r_ptr->name;

					if (!(r_ptr->flags1 & RF1_UNIQUE))
					{
						sprintf(tmp_val2, " (%s%s)", (is_a_vowel(*t) ? "an " : "a "), t);

						modstr = tmp_val2;
					}
					else
					{
						sprintf(tmp_val2, "(%s)", t);

						modstr = t;
					}
#endif
				}
			}
			break;
		}

		/* Figurines/Statues */
		case TV_FIGURINE:
		case TV_STATUE:
		{
			monster_race *r_ptr = &r_info[o_ptr->pval];

#ifdef JP
			modstr = r_name + r_ptr->name;
#else
			cptr t = r_name + r_ptr->name;

			if (!(r_ptr->flags1 & RF1_UNIQUE))
			{
				sprintf(tmp_val2, "%s%s", (is_a_vowel(*t) ? "an " : "a "), t);

				modstr = tmp_val2;
			}
			else
			{
				modstr = t;
			}
#endif


			break;
		}

		/* Corpses */
		case TV_CORPSE:
		{
			monster_race *r_ptr = &r_info[o_ptr->pval];

			modstr = r_name + r_ptr->name;

#ifdef JP
			basenm = "#%";
#else
			if (r_ptr->flags1 & RF1_UNIQUE)
				basenm = "& % of #";
			else
				basenm = "& # %";
#endif

			break;
		}

		/* Missiles/ Bows/ Weapons */
		case TV_SHOT:
		case TV_BOLT:
		case TV_ARROW:
		case TV_BOW:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_SWORD:
		case TV_DIGGING:
		{
			show_weapon = TRUE;
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
			show_armour = TRUE;
			break;
		}

		/* Lites (including a few "Specials") */
		case TV_LITE:
		{
			break;
		}

		/* Amulets (including a few "Specials") */
		case TV_AMULET:
		{
			/* Known artifacts */
			if (aware)
			{
				if (object_is_fixed_artifact(o_ptr)) break;
				if (k_ptr->gen_flags & TRG_INSTA_ART) break;
			}

			/* Color the object */
			modstr = k_name + flavor_k_ptr->flavor_name;

#ifdef JP
			if (!flavor)    basenm = "%のアミュレット";
			else if (aware) basenm = "%の#アミュレット";
			else            basenm = "#アミュレット";
#else
			if (!flavor)    basenm = "& Amulet~ of %";
			else if (aware) basenm = "& # Amulet~ of %";
			else            basenm = "& # Amulet~";
#endif

			break;
		}

		/* Rings (including a few "Specials") */
		case TV_RING:
		{
			/* Known artifacts */
			if (aware)
			{
				if (object_is_fixed_artifact(o_ptr)) break;
				if (k_ptr->gen_flags & TRG_INSTA_ART) break;
			}

			/* Color the object */
			modstr = k_name + flavor_k_ptr->flavor_name;

#ifdef JP
			if (!flavor)    basenm = "%の指輪";
			else if (aware) basenm = "%の#指輪";
			else            basenm = "#指輪";
#else
			if (!flavor)    basenm = "& Ring~ of %";
			else if (aware) basenm = "& # Ring~ of %";
			else            basenm = "& # Ring~";
#endif

			if (!k_ptr->to_h && !k_ptr->to_d && (o_ptr->to_h || o_ptr->to_d)) show_weapon = TRUE;

			break;
		}

		case TV_CARD:
		{
			break;
		}

		case TV_STAFF:
		{
			/* Color the object */
			modstr = k_name + flavor_k_ptr->flavor_name;

#ifdef JP
			if (!flavor)    basenm = "%の杖";
			else if (aware) basenm = "%の#杖";
			else            basenm = "#杖";
#else
			if (!flavor)    basenm = "& Staff~ of %";
			else if (aware) basenm = "& # Staff~ of %";
			else            basenm = "& # Staff~";
#endif

			break;
		}

		case TV_WAND:
		{
			/* Color the object */
			modstr = k_name + flavor_k_ptr->flavor_name;

#ifdef JP
			if (!flavor)    basenm = "%の魔法棒";
			else if (aware) basenm = "%の#魔法棒";
			else            basenm = "#魔法棒";
#else
			if (!flavor)    basenm = "& Wand~ of %";
			else if (aware) basenm = "& # Wand~ of %";
			else            basenm = "& # Wand~";
#endif

			break;
		}

		case TV_ROD:
		{
			/* Color the object */
			modstr = k_name + flavor_k_ptr->flavor_name;

#ifdef JP
			if (!flavor)    basenm = "%のロッド";
			else if (aware) basenm = "%の#ロッド";
			else            basenm = "#ロッド";
#else
			if (!flavor)    basenm = "& Rod~ of %";
			else if (aware) basenm = "& # Rod~ of %";
			else            basenm = "& # Rod~";
#endif

			break;
		}

		case TV_SCROLL:
		{
			/* Color the object */
			modstr = k_name + flavor_k_ptr->flavor_name;

#ifdef JP
			if (!flavor)    basenm = "%の巻物";
			else if (aware) basenm = "「#」と書かれた%の巻物";
			else            basenm = "「#」と書かれた巻物";
#else
			if (!flavor)    basenm = "& Scroll~ of %";
			else if (aware) basenm = "& Scroll~ titled \"#\" of %";
			else            basenm = "& Scroll~ titled \"#\"";
#endif

			break;
		}

		case TV_POTION:
		{
			/* Color the object */
			modstr = k_name + flavor_k_ptr->flavor_name;

#ifdef JP
			if (!flavor)    basenm = "%の薬";
			else if (aware) basenm = "%の#薬";
			else            basenm = "#薬";
#else
			if (!flavor)    basenm = "& Potion~ of %";
			else if (aware) basenm = "& # Potion~ of %";
			else            basenm = "& # Potion~";
#endif

			break;
		}

		case TV_FOOD:
		{
			/* Ordinary food is "boring" */
			if (!k_ptr->flavor_name) break;

			/* Color the object */
			modstr = k_name + flavor_k_ptr->flavor_name;

#ifdef JP
			if (!flavor)    basenm = "%のキノコ";
			else if (aware) basenm = "%の#キノコ";
			else            basenm = "#キノコ";
#else
			if (!flavor)    basenm = "& Mushroom~ of %";
			else if (aware) basenm = "& # Mushroom~ of %";
			else            basenm = "& # Mushroom~";
#endif

			break;
		}

		case TV_PARCHMENT:
		{
			basenm = _("羊皮紙 - %", "& Parchment~ - %");
			break;
		}

		/* Magic Books */
		case TV_LIFE_BOOK:
		{
#ifdef JP
			basenm = "生命の魔法書%";
#else
			if (mp_ptr->spell_book == TV_LIFE_BOOK)
				basenm = "& Book~ of Life Magic %";
			else
				basenm = "& Life Spellbook~ %";
#endif

			break;
		}

		case TV_SORCERY_BOOK:
		{
#ifdef JP
			basenm = "仙術の魔法書%";
#else
			if (mp_ptr->spell_book == TV_LIFE_BOOK)
				basenm = "& Book~ of Sorcery %";
			else
				basenm = "& Sorcery Spellbook~ %";
#endif

			break;
		}

		case TV_NATURE_BOOK:
		{
#ifdef JP
			basenm = "自然の魔法書%";
#else
			if (mp_ptr->spell_book == TV_LIFE_BOOK)
				basenm = "& Book~ of Nature Magic %";
			else
				basenm = "& Nature Spellbook~ %";
#endif

			break;
		}

		case TV_CHAOS_BOOK:
		{
#ifdef JP
			basenm = "カオスの魔法書%";
#else
			if (mp_ptr->spell_book == TV_LIFE_BOOK)
				basenm = "& Book~ of Chaos Magic %";
			else
				basenm = "& Chaos Spellbook~ %";
#endif

			break;
		}

		case TV_DEATH_BOOK:
		{
#ifdef JP
			basenm = "暗黒の魔法書%";
#else
			if (mp_ptr->spell_book == TV_LIFE_BOOK)
				basenm = "& Book~ of Death Magic %";
			else
				basenm = "& Death Spellbook~ %";
#endif

			break;
		}

		case TV_TRUMP_BOOK:
		{
#ifdef JP
			basenm = "トランプの魔法書%";
#else
			if (mp_ptr->spell_book == TV_LIFE_BOOK)
				basenm = "& Book~ of Trump Magic %";
			else
				basenm = "& Trump Spellbook~ %";
#endif

			break;
		}

		case TV_ARCANE_BOOK:
		{
#ifdef JP
			basenm = "秘術の魔法書%";
#else
			if (mp_ptr->spell_book == TV_LIFE_BOOK)
				basenm = "& Book~ of Arcane Magic %";
			else
				basenm = "& Arcane Spellbook~ %";
#endif

			break;
		}

		case TV_CRAFT_BOOK:
		{
#ifdef JP
			basenm = "匠の魔法書%";
#else
			if (mp_ptr->spell_book == TV_LIFE_BOOK)
				basenm = "& Book~ of Craft Magic %";
			else
				basenm = "& Craft Spellbook~ %";
#endif

			break;
		}

		case TV_DAEMON_BOOK:
		{
#ifdef JP
			basenm = "悪魔の魔法書%";
#else
			if (mp_ptr->spell_book == TV_LIFE_BOOK)
				basenm = "& Book~ of Daemon Magic %";
			else
				basenm = "& Daemon Spellbook~ %";
#endif

			break;
		}

		case TV_CRUSADE_BOOK:
		{
#ifdef JP
			basenm = "破邪の魔法書%";
#else
			if (mp_ptr->spell_book == TV_LIFE_BOOK)
				basenm = "& Book~ of Crusade Magic %";
			else
				basenm = "& Crusade Spellbook~ %";
#endif

			break;
		}

		case TV_MUSIC_BOOK:
		{
			basenm = _("歌集%", "& Song Book~ %");
			break;
		}

		case TV_HISSATSU_BOOK:
		{
			basenm = _("& 武芸の書%", "Book~ of Kendo %");
			break;
		}

		case TV_HEX_BOOK:
		{
#ifdef JP
			basenm = "呪術の魔法書%";
#else
			if (mp_ptr->spell_book == TV_LIFE_BOOK)
				basenm = "& Book~ of Hex Magic %";
			else
				basenm = "& Hex Spellbook~ %";
#endif

			break;
		}

		/* Hack -- Gold/Gems */
		case TV_GOLD:
		{
			strcpy(buf, basenm);
			return;
		}

		/* Used in the "inventory" routine */
		default:
		{
			strcpy(buf, _("(なし)", "(nothing)"));
			return;
		}
	}

	/* Use full name from k_info or a_info */
	if (aware && have_flag(flgs, TR_FULL_NAME))
	{
		if (known && o_ptr->name1) basenm = a_name + a_info[o_ptr->name1].name;
		else basenm = kindname;
	}

	/* Start dumping the result */
	t = tmp_val;

#ifdef JP
	if (basenm[0] == '&')
		s = basenm + 2;
	else
		s = basenm;

	/* No prefix */
	if (mode & OD_OMIT_PREFIX)
	{
		/* Nothing */
	}
	else if (o_ptr->number > 1)
	{
		t = object_desc_kosuu(t, o_ptr);
		t = object_desc_str(t, "の ");
	}

	/* 英語の場合アーティファクトは The が付くので分かるが
	 * 日本語では分からないのでマークをつける 
	 */
	if (known)
	{
		if (object_is_fixed_artifact(o_ptr)) t = object_desc_str(t, "★");
		else if (o_ptr->art_name) t = object_desc_str(t, "☆");
	}

#else

	/* The object "expects" a "number" */
	if (basenm[0] == '&')
	{
		/* Skip the ampersand (and space) */
		s = basenm + 2;

		/* No prefix */
		if (mode & OD_OMIT_PREFIX)
		{
			/* Nothing */
		}

		/* Hack -- None left */
		else if (o_ptr->number <= 0)
		{
			t = object_desc_str(t, "no more ");
		}

		/* Extract the number */
		else if (o_ptr->number > 1)
		{
			t = object_desc_num(t, o_ptr->number);
			t = object_desc_chr(t, ' ');
		}

		/* Hack -- The only one of its kind */
		else if ((known && object_is_artifact(o_ptr)) ||
		         ((o_ptr->tval == TV_CORPSE) &&
		          (r_info[o_ptr->pval].flags1 & RF1_UNIQUE)))
		{
			t = object_desc_str(t, "The ");
		}

		/* A single one */
		else
		{
			bool vowel;

			switch (*s)
			{
			case '#': vowel = is_a_vowel(modstr[0]); break;
			case '%': vowel = is_a_vowel(*kindname); break;
			default:  vowel = is_a_vowel(*s); break;
			}

			if (vowel)
			{
				/* A single one, with a vowel */
				t = object_desc_str(t, "an ");
			}
			else
			{
				/* A single one, without a vowel */
				t = object_desc_str(t, "a ");
			}
		}
	}

	/* Hack -- objects that "never" take an article */
	else
	{
		/* No ampersand */
		s = basenm;

		/* No pref */
		if (mode & OD_OMIT_PREFIX)
		{
			/* Nothing */
		}

		/* Hack -- all gone */
		else if (o_ptr->number <= 0)
		{
			t = object_desc_str(t, "no more ");
		}

		/* Prefix a number if required */
		else if (o_ptr->number > 1)
		{
			t = object_desc_num(t, o_ptr->number);
			t = object_desc_chr(t, ' ');
		}

		/* Hack -- The only one of its kind */
		else if (known && object_is_artifact(o_ptr))
		{
			t = object_desc_str(t, "The ");
		}

		/* Hack -- single items get no prefix */
		else
		{
			/* Nothing */
		}
	}
#endif

	/* Paranoia -- skip illegal tildes */
	/* while (*s == '~') s++; */

#ifdef JP
	if (object_is_smith(o_ptr))
	{
		t = object_desc_str(t, format("鍛冶師%sの", player_name));
	}

	/* 伝説のアイテム、名のあるアイテムの名前を付加する */
	if (known)
	{
		/* ランダム・アーティファクト */
		if (o_ptr->art_name)
		{
			cptr temp = quark_str(o_ptr->art_name);

			/* '『' から始まらない伝説のアイテムの名前は最初に付加する */
			/* 英語版のセーブファイルから来た 'of XXX' は,「XXXの」と表示する */
			if (strncmp(temp, "of ", 3) == 0)
			{
				t = object_desc_str(t, &temp[3]);
				t = object_desc_str(t, "の");
			}
			else if ((strncmp(temp, "『", 2) != 0) &&
				 (strncmp(temp, "《", 2) != 0) &&
				 (temp[0] != '\''))
				t = object_desc_str(t, temp);
		}
		/* 伝説のアイテム */
		else if (o_ptr->name1 && !have_flag(flgs, TR_FULL_NAME))
		{
			artifact_type *a_ptr = &a_info[o_ptr->name1];
			/* '『' から始まらない伝説のアイテムの名前は最初に付加する */
			if (strncmp(a_name + a_ptr->name, "『", 2) != 0)
			{
				t = object_desc_str(t, a_name + a_ptr->name);
			}
		}
		/* 名のあるアイテム */
		else if (object_is_ego(o_ptr))
		{
			ego_item_type *e_ptr = &e_info[o_ptr->name2];
			t = object_desc_str(t, e_name + e_ptr->name);
		}
	}
#endif

	/* Copy the string */
	for (s0 = NULL; *s || s0; )
	{
		/* The end of the flavour/kind string. */
		if (!*s)
		{
			s = s0 + 1;
			s0 = NULL;
		}

		/* Begin to append the modifier (flavor) */
		else if ((*s == '#') && !s0)
		{
			s0 = s;
			s = modstr;

			/* Paranoia -- Never append multiple modstrs */
			modstr = "";
		}

		/* Begin to append the kind name */
		else if ((*s == '%') && !s0)
		{
			s0 = s;
			s = kindname;

			/* Paranoia -- Never append multiple kindnames */
			kindname = "";
		}

#ifndef JP
		/* Pluralizer */
		else if (*s == '~')
		{
			/* Add a plural if needed */
			if (!(mode & OD_NO_PLURAL) && (o_ptr->number != 1))
			{
				char k = t[-1];

				/* XXX XXX XXX Mega-Hack */

				/* Hack -- "Cutlass-es" and "Torch-es" */
				if ((k == 's') || (k == 'h')) *t++ = 'e';

				/* Add an 's' */
				*t++ = 's';
			}
			s++;
		}
#endif

		/* Normal */
		else
		{
			/* Copy */
			*t++ = *s++;
		}
	}

	/* Terminate */
	*t = '\0';


#ifdef JP
	/* '『'から始まる伝説のアイテムの名前は最後に付加する */
	if (known)
	{
		/* ランダムアーティファクトの名前はセーブファイルに記録
		   されるので、英語版の名前もそれらしく変換する */
		if (o_ptr->art_name)
		{
			char temp[256];
			int itemp;
			strcpy(temp, quark_str(o_ptr->art_name));
			/* MEGA HACK by ita */
			if (strncmp(temp, "『", 2) == 0 ||
			    strncmp(temp, "《", 2) == 0)
				t = object_desc_str(t, temp);
			else if (temp[0] == '\'')
			{
				itemp = strlen(temp);
				temp[itemp - 1] = 0;
				t = object_desc_str(t, "『");
				t = object_desc_str(t, &temp[1]);
				t = object_desc_str(t, "』");
			}
		}
		else if (object_is_fixed_artifact(o_ptr))
		{
			artifact_type *a_ptr = &a_info[o_ptr->name1];
			if (strncmp(a_name + a_ptr->name, "『", 2) == 0)
			{
				t = object_desc_str(t, a_name + a_ptr->name);
			}
		}
		else if (o_ptr->inscription)
		{
			cptr str = quark_str(o_ptr->inscription);

			while(*str)
			{
				if (iskanji(*str))
				{
					str += 2;
					continue;
				}
				if (*str == '#') break;
				str++;
			}
			if (*str)
			{
				/* Find the '#' */
				cptr str = my_strchr(quark_str(o_ptr->inscription), '#');

				/* Add the false name */
				t = object_desc_str(t,"『");
				t = object_desc_str(t, &str[1]);
				t = object_desc_str(t,"』");
			}
		}
	}
#else
	if (object_is_smith(o_ptr))
	{
		t = object_desc_str(t,format(" of %s the Smith",player_name));
	}

	/* Hack -- Append "Artifact" or "Special" names */
	if (known && !have_flag(flgs, TR_FULL_NAME))
	{
		/* Is it a new random artifact ? */
		if (o_ptr->art_name)
		{
			t = object_desc_chr(t, ' ');
			t = object_desc_str(t, quark_str(o_ptr->art_name));
		}

		/* Grab any artifact name */
		else if (object_is_fixed_artifact(o_ptr))
		{
			artifact_type *a_ptr = &a_info[o_ptr->name1];

			t = object_desc_chr(t, ' ');
			t = object_desc_str(t, a_name + a_ptr->name);
		}

		/* Grab any ego-item name */
		else
		{
			if (object_is_ego(o_ptr))
			{
				ego_item_type *e_ptr = &e_info[o_ptr->name2];

				t = object_desc_chr(t, ' ');
				t = object_desc_str(t, e_name + e_ptr->name);
			}

			if (o_ptr->inscription && my_strchr(quark_str(o_ptr->inscription), '#'))
			{
				/* Find the '#' */
				cptr str = my_strchr(quark_str(o_ptr->inscription), '#');

				/* Add the false name */
				t = object_desc_chr(t, ' ');
				t = object_desc_str(t, &str[1]);
			}
		}
	}
#endif


	/* No more details wanted */
	if (mode & OD_NAME_ONLY) goto object_desc_done;

	/* Hack -- Chests must be described in detail */
	if (o_ptr->tval == TV_CHEST)
	{
		/* Not searched yet */
		if (!known)
		{
			/* Nothing */
		}

		/* May be "empty" */
		else if (!o_ptr->pval)
		{
			t = object_desc_str(t, _("(空)", " (empty)"));
		}

		/* May be "disarmed" */
		else if (o_ptr->pval < 0)
		{
			if (chest_traps[0 - o_ptr->pval])
			{
				t = object_desc_str(t, _("(解除済)", " (disarmed)"));
			}
			else
			{
				t = object_desc_str(t, _("(非施錠)", " (unlocked)"));
			}
		}

		/* Describe the traps, if any */
		else
		{
			/* Describe the traps */
			switch (chest_traps[o_ptr->pval])
			{
				case 0:
				{
					t = object_desc_str(t, _("(施錠)", " (Locked)"));
					break;
				}
				case CHEST_LOSE_STR:
				{
					t = object_desc_str(t, _("(毒針)", " (Poison Needle)"));
					break;
				}
				case CHEST_LOSE_CON:
				{
					t = object_desc_str(t, _("(毒針)", " (Poison Needle)"));
					break;
				}
				case CHEST_POISON:
				{
					t = object_desc_str(t, _("(ガス・トラップ)", " (Gas Trap)"));
					break;
				}
				case CHEST_PARALYZE:
				{
					t = object_desc_str(t, _("(ガス・トラップ)", " (Gas Trap)"));
					break;
				}
				case CHEST_EXPLODE:
				{
					t = object_desc_str(t, _("(爆発装置)", " (Explosion Device)"));
					break;
				}
				case CHEST_SUMMON:
				case CHEST_BIRD_STORM:
				case CHEST_E_SUMMON:
				case CHEST_H_SUMMON:
				{
					t = object_desc_str(t, _("(召喚のルーン)", " (Summoning Runes)"));
					break;
				}
				case CHEST_RUNES_OF_EVIL:
				{
					t = object_desc_str(t, _("(邪悪なルーン)", " (Gleaming Black Runes)"));
					break;
				}
				case CHEST_ALARM:
				{
					t = object_desc_str(t, _("(警報装置)", " (Alarm)"));
					break;
				}
				default:
				{
					t = object_desc_str(t, _("(マルチ・トラップ)", " (Multiple Traps)"));
					break;
				}
			}
		}
	}


	/* Display the item like a weapon */
	if (have_flag(flgs, TR_SHOW_MODS)) show_weapon = TRUE;

	/* Display the item like a weapon */
	if (object_is_smith(o_ptr) && (o_ptr->xtra3 == 1 + ESSENCE_SLAY_GLOVE))
		show_weapon = TRUE;

	/* Display the item like a weapon */
	if (o_ptr->to_h && o_ptr->to_d) show_weapon = TRUE;

	/* Display the item like armour */
	if (o_ptr->ac) show_armour = TRUE;


	/* Dump base weapon info */
	switch (o_ptr->tval)
	{
		/* Missiles and Weapons */
		case TV_SHOT:
		case TV_BOLT:
		case TV_ARROW:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_SWORD:
	    case TV_DIGGING:
		
		/* In Vault Quest, hide the dice of target weapon. */
		if(object_is_quest_target(o_ptr) && !known)
		{
			break;
		}

		/* Append a "damage" string */
		t = object_desc_chr(t, ' ');
		t = object_desc_chr(t, p1);
		t = object_desc_num(t, o_ptr->dd);
		t = object_desc_chr(t, 'd');
		t = object_desc_num(t, o_ptr->ds);
		t = object_desc_chr(t, p2);

		/* All done */
		break;


		/* Bows get a special "damage string" */
		case TV_BOW:

		/* Mega-Hack -- Extract the "base power" */
		power = bow_tmul(o_ptr->sval);

		/* Apply the "Extra Might" flag */
		if (have_flag(flgs, TR_XTRA_MIGHT)) power++;

		/* Append a special "damage" string */
		t = object_desc_chr(t, ' ');
		t = object_desc_chr(t, p1);
		t = object_desc_chr(t, 'x');
		t = object_desc_num(t, power);
		t = object_desc_chr(t, p2);
		
		fire_rate = calc_num_fire(o_ptr);
		/* Show Fire rate */
		if (fire_rate != 0 && power > 0 && known)
		{	
			fire_rate = bow_energy(o_ptr->sval) / fire_rate;
			
			t = object_desc_chr(t, ' ');
			t = object_desc_chr(t, p1);			
			t = object_desc_num(t,  fire_rate/100);
			t = object_desc_chr(t,  '.');
			t = object_desc_num(t,  fire_rate%100);
			t = object_desc_str(t, "turn");
			t = object_desc_chr(t, p2);
		}
		
		/* All done */
		break;
	}


	/* Add the weapon bonuses */
	if (known)
	{
		/* Show the tohit/todam on request */
		if (show_weapon)
		{
			t = object_desc_chr(t, ' ');
			t = object_desc_chr(t, p1);
			t = object_desc_int(t, o_ptr->to_h);
			t = object_desc_chr(t, ',');
			t = object_desc_int(t, o_ptr->to_d);
			t = object_desc_chr(t, p2);
		}

		/* Show the tohit if needed */
		else if (o_ptr->to_h)
		{
			t = object_desc_chr(t, ' ');
			t = object_desc_chr(t, p1);
			t = object_desc_int(t, o_ptr->to_h);
			t = object_desc_chr(t, p2);
		}

		/* Show the todam if needed */
		else if (o_ptr->to_d)
		{
			t = object_desc_chr(t, ' ');
			t = object_desc_chr(t, p1);
			t = object_desc_int(t, o_ptr->to_d);
			t = object_desc_chr(t, p2);
		}
	}

	bow_ptr = &inventory[INVEN_BOW];

	/* If have a firing weapon + ammo matches bow */
	if (bow_ptr->k_idx && (o_ptr->tval == p_ptr->tval_ammo))
	{
		int avgdam = o_ptr->dd * (o_ptr->ds + 1) * 10 / 2;
		int tmul = bow_tmul(bow_ptr->sval);
		s16b energy_fire = bow_energy(bow_ptr->sval);

		/* See if the bow is "known" - then set damage bonus */
		if (object_is_known(bow_ptr)) avgdam += (bow_ptr->to_d * 10);

		/* Effect of ammo */
		if (known) avgdam += (o_ptr->to_d * 10);

		/* Get extra "power" from "extra might" */
		if (p_ptr->xtra_might) tmul++;

		tmul = tmul * (100 + (int)(adj_str_td[p_ptr->stat_ind[A_STR]]) - 128);

		/* Launcher multiplier */
		avgdam *= tmul;
		avgdam /= (100 * 10);

		/* Get extra damage from concentration */
		if (p_ptr->concent) avgdam = boost_concentration_damage(avgdam);
		
		if (avgdam < 0) avgdam = 0;

		/* Display (shot damage/ shot damage with critical/ avg damage with critical) */
		t = object_desc_chr(t, ' ');
		t = object_desc_chr(t, p1);
		
		if(show_ammo_no_crit)
		{
			/* Damage with no-crit */
			t = object_desc_num(t, avgdam);
			t = object_desc_str(t, show_ammo_detail ? "/shot " : "/");
		}
		
		/* Apply Expect damage of Critical */
		avgdam = calc_expect_crit_shot(o_ptr->weight, o_ptr->to_h, bow_ptr->to_h, avgdam);
		t = object_desc_num(t, avgdam);
		
		t = show_ammo_no_crit ? object_desc_str(t, show_ammo_detail ? "/crit " : "/")
							  : object_desc_str(t, show_ammo_detail ? "/shot " : "/");
	
		if (p_ptr->num_fire == 0)
		{
			t = object_desc_chr(t, '0');
		}
		else
		{
			/* Calc effects of energy */
			avgdam *= (p_ptr->num_fire * 100);
			avgdam /= energy_fire;
			t = object_desc_num(t, avgdam);
			t = object_desc_str(t, show_ammo_detail ? "/turn" : "");
			
			if(show_ammo_crit_ratio)
			{
				int percent = calc_crit_ratio_shot(o_ptr->weight, 
						known ? o_ptr->to_h : 0, 
						object_is_known(bow_ptr) ? bow_ptr->to_h : 0,  avgdam);
				
				t = object_desc_chr(t, '/');
				t = object_desc_num(t, percent / 100);
				t = object_desc_chr(t, '.');
				t = object_desc_num(t, percent % 100);
				t = object_desc_str(t, show_ammo_detail ? "% crit" : "%");
			}
		}

		t = object_desc_chr(t, p2);
	}
	else if ((p_ptr->pclass == CLASS_NINJA) && (o_ptr->tval == TV_SPIKE))
	{
		int avgdam = p_ptr->mighty_throw ? (1 + 3) : 1;
		s16b energy_fire = 100 - p_ptr->lev;

		avgdam += ((p_ptr->lev + 30) * (p_ptr->lev + 30) - 900) / 55;

		/* Display (shot damage/ avg damage) */
		t = object_desc_chr(t, ' ');
		t = object_desc_chr(t, p1);
		t = object_desc_num(t, avgdam);
		t = object_desc_chr(t, '/');

		/* Calc effects of energy */
		avgdam = 100 * avgdam / energy_fire;

		t = object_desc_num(t, avgdam);
		t = object_desc_chr(t, p2);
	}

	/* Add the armor bonuses */
	if (known)
	{
		/* Show the armor class info */
		if (show_armour)
		{
			t = object_desc_chr(t, ' ');
			t = object_desc_chr(t, b1);
			t = object_desc_num(t, o_ptr->ac);
			t = object_desc_chr(t, ',');
			t = object_desc_int(t, o_ptr->to_a);
			t = object_desc_chr(t, b2);
		}

		/* No base armor, but does increase armor */
		else if (o_ptr->to_a)
		{
			t = object_desc_chr(t, ' ');
			t = object_desc_chr(t, b1);
			t = object_desc_int(t, o_ptr->to_a);
			t = object_desc_chr(t, b2);
		}
	}

	/* Hack -- always show base armor */
	else if (show_armour)
	{
		t = object_desc_chr(t, ' ');
		t = object_desc_chr(t, b1);
		t = object_desc_num(t, o_ptr->ac);
		t = object_desc_chr(t, b2);
	}


	/* No more details wanted */
	if (mode & OD_NAME_AND_ENCHANT) goto object_desc_done;


	if (known) /* Known item only */
	{
		/*
		 * Hack -- Wands and Staffs have charges.  Make certain how many charges
		 * a stack of staffs really has is clear. -LM-
		 */
		if (((o_ptr->tval == TV_STAFF) || (o_ptr->tval == TV_WAND)))
		{
			/* Dump " (N charges)" */
			t = object_desc_chr(t, ' ');
			t = object_desc_chr(t, p1);

			/* Clear explaination for staffs. */
			if ((o_ptr->tval == TV_STAFF) && (o_ptr->number > 1))
			{
				t = object_desc_num(t, o_ptr->number);
				t = object_desc_str(t, "x ");
			}
			t = object_desc_num(t, o_ptr->pval);
#ifdef JP
			t = object_desc_str(t, "回分");
#else
			t = object_desc_str(t, " charge");
			if (o_ptr->pval != 1) t = object_desc_chr(t, 's');
#endif

			t = object_desc_chr(t, p2);
		}
		/* Hack -- Rods have a "charging" indicator.  Now that stacks of rods may
		 * be in any state of charge or discharge, this now includes a number. -LM-
		 */
		else if (o_ptr->tval == TV_ROD)
		{
			/* Hack -- Dump " (# charging)" if relevant */
			if (o_ptr->timeout)
			{
				/* Stacks of rods display an exact count of charging rods. */
				if (o_ptr->number > 1)
				{
					/* Paranoia. */
					if (k_ptr->pval == 0) k_ptr->pval = 1;

					/* Find out how many rods are charging, by dividing
					 * current timeout by each rod's maximum timeout.
					 * Ensure that any remainder is rounded up.  Display
					 * very discharged stacks as merely fully discharged.
					 */
					power = (o_ptr->timeout + (k_ptr->pval - 1)) / k_ptr->pval;
					if (power > o_ptr->number) power = o_ptr->number;

					/* Display prettily. */
					t = object_desc_str(t, " (");
					t = object_desc_num(t, power);
					t = object_desc_str(t, _("本 充填中)", " charging)"));
				}

				/* "one Rod of Perception (1 charging)" would look tacky. */
				else
				{
					t = object_desc_str(t, _("(充填中)", " (charging)"));
				}
			}
		}

		/* Dump "pval" flags for wearable items */
		if (have_pval_flags(flgs))
		{
			/* Start the display */
			t = object_desc_chr(t, ' ');
			t = object_desc_chr(t, p1);

			/* Dump the "pval" itself */
			t = object_desc_int(t, o_ptr->pval);

			/* Do not display the "pval" flags */
			if (have_flag(flgs, TR_HIDE_TYPE))
			{
				/* Nothing */
			}

			/* Speed */
			else if (have_flag(flgs, TR_SPEED))
			{
				/* Dump " to speed" */
				t = object_desc_str(t, _("加速", " to speed"));
			}

			/* Attack speed */
			else if (have_flag(flgs, TR_BLOWS))
			{
				/* Add " attack" */
#ifdef JP
				t = object_desc_str(t, "攻撃");
#else
				t = object_desc_str(t, " attack");

				/* Add "attacks" */
				if (ABS(o_ptr->pval) != 1) t = object_desc_chr(t, 's');
#endif
			}

			/* Stealth */
			else if (have_flag(flgs, TR_STEALTH))
			{
				/* Dump " to stealth" */
				t = object_desc_str(t, _("隠密", " to stealth"));
			}

			/* Search */
			else if (have_flag(flgs, TR_SEARCH))
			{
				/* Dump " to searching" */
				t = object_desc_str(t, _("探索", " to searching"));
			}

			/* Infravision */
			else if (have_flag(flgs, TR_INFRA))
			{
				/* Dump " to infravision" */
				t = object_desc_str(t, _("赤外線視力", " to infravision"));
			}

			/* Finish the display */
			t = object_desc_chr(t, p2);
		}

		/* Hack -- Process Lanterns/Torches */
		if ((o_ptr->tval == TV_LITE) && (!(object_is_fixed_artifact(o_ptr) || (o_ptr->sval == SV_LITE_FEANOR))))
		{
			/* Hack -- Turns of light for normal lites */
#ifdef JP
			t = object_desc_chr(t, '(');
#else
			t = object_desc_str(t, " (with ");
#endif

			if (o_ptr->name2 == EGO_LITE_LONG) t = object_desc_num(t, o_ptr->xtra4 * 2);
			else t = object_desc_num(t, o_ptr->xtra4);
			t = object_desc_str(t, _("ターンの寿命)", " turns of light)"));
		}

		/* Indicate charging objects, but not rods. */
		if (o_ptr->timeout && (o_ptr->tval != TV_ROD))
		{
			/* Hack -- Dump " (charging)" if relevant */
			t = object_desc_str(t, _("(充填中)", " (charging)"));
		}
	}


	/* No more details wanted */
	if (mode & OD_OMIT_INSCRIPTION) goto object_desc_done;


	/* Prepare real inscriptions in a buffer */
	tmp_val2[0] = '\0';

	/* Auto abbreviation inscribe */
	if ((abbrev_extra || abbrev_all) && (o_ptr->ident & IDENT_MENTAL))
	{
		if (!o_ptr->inscription || !my_strchr(quark_str(o_ptr->inscription), '%'))
		{
			bool kanji, all;

#ifdef JP
			kanji = TRUE;
#else
			kanji = FALSE;
#endif
			all = abbrev_all;

			get_ability_abbreviation(tmp_val2, o_ptr, kanji, all);
		}
	}

	/* Use the standard inscription if available */
	if (o_ptr->inscription)
	{
		char buff[1024];

		if (tmp_val2[0]) strcat(tmp_val2, ", ");

		/* Get inscription and convert {%} */
		get_inscription(buff, o_ptr);

		/* strcat with correct treating of kanji */
		my_strcat(tmp_val2, buff, sizeof(tmp_val2));
	}


	/* No fake inscription yet */
	fake_insc_buf[0] = '\0';

	/* Use the game-generated "feeling" otherwise, if available */
	if (o_ptr->feeling)
	{
		strcpy(fake_insc_buf, game_inscriptions[o_ptr->feeling]);
	}

	/* Note "cursed" if the item is known to be cursed */
	else if (object_is_cursed(o_ptr) && (known || (o_ptr->ident & IDENT_SENSE)))
	{
		strcpy(fake_insc_buf, _("呪われている", "cursed"));
	}

	/* Note "unidentified" if the item is unidentified */
	else if (((o_ptr->tval == TV_RING) || (o_ptr->tval == TV_AMULET)
		   || (o_ptr->tval == TV_LITE) || (o_ptr->tval == TV_FIGURINE))
		 && aware && !known
		 && !(o_ptr->ident & IDENT_SENSE))
	{
		strcpy(fake_insc_buf, _("未鑑定", "unidentified"));
	}

	/* Mega-Hack -- note empty wands/staffs */
	else if (!known && (o_ptr->ident & IDENT_EMPTY))
	{
		strcpy(fake_insc_buf, _("空", "empty"));
	}

	/* Note "tried" if the object has been tested unsuccessfully */
	else if (!aware && object_is_tried(o_ptr))
	{
		strcpy(fake_insc_buf, _("未判明", "tried"));
	}

	/* Note the discount, if any */
	if (o_ptr->discount)
	{
		/* Hidden by real inscription unless in a store */
		if (!tmp_val2[0] || (o_ptr->ident & IDENT_STORE))
		{
			char discount_num_buf[4];

			/* Append to other fake inscriptions if any */
			if (fake_insc_buf[0]) strcat(fake_insc_buf, ", ");

			(void)object_desc_num(discount_num_buf, o_ptr->discount);
			strcat(fake_insc_buf, discount_num_buf);
			strcat(fake_insc_buf, _("%引き", "% off"));
		}
	}


	/* Append the inscription, if any */
	if (fake_insc_buf[0] || tmp_val2[0])
	{
		/* Append the inscription */
		t = object_desc_chr(t, ' ');
		t = object_desc_chr(t, c1);

		/* Append fake inscriptions */
		if (fake_insc_buf[0])
		{
			t = object_desc_str(t, fake_insc_buf);
		}

		/* Append a separater */
		if (fake_insc_buf[0] && tmp_val2[0])
		{
			t = object_desc_chr(t, ',');
			t = object_desc_chr(t, ' ');
		}

		/* Append real inscriptions */
		if (tmp_val2[0])
		{
			t = object_desc_str(t, tmp_val2);
		}

		t = object_desc_chr(t, c2);
	}

object_desc_done:
	my_strcpy(buf, tmp_val, MAX_NLEN);
}


