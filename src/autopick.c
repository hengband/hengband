/* File: autopick.c */

/* Purpose: Object Auto-picker/Destroyer */

/*
 * Copyright (c) 2002  Mogami
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#include "angband.h"

#define MAX_LINELEN 1024

static object_type autopick_last_destroyed_object;

/*
 * Macros for Keywords
 */
#define FLG_ALL             0
#define FLG_COLLECTING	    1
#define FLG_UNIDENTIFIED    2
#define FLG_IDENTIFIED	    3
#define FLG_STAR_IDENTIFIED 4
#define FLG_BOOSTED	    5
#define FLG_MORE_THAN	    6
#define FLG_DICE	    7
#define FLG_MORE_BONUS	    8
#define FLG_MORE_BONUS2	    9
#define FLG_WORTHLESS	    10
#define FLG_ARTIFACT	    11
#define FLG_EGO		    12
#define FLG_NAMELESS	    13
#define FLG_UNAWARE	    14
#define FLG_WANTED	    15
#define FLG_UNIQUE	    16
#define FLG_HUMAN	    17
#define FLG_UNREADABLE	    18
#define FLG_REALM1	    19
#define FLG_REALM2	    20
#define FLG_FIRST	    21
#define FLG_SECOND	    22
#define FLG_THIRD	    23
#define FLG_FOURTH	    24

#define FLG_ITEMS	    30
#define FLG_WEAPONS	    31
#define FLG_ARMORS	    32
#define FLG_MISSILES	    33
#define FLG_DEVICES	    34
#define FLG_LIGHTS	    35
#define FLG_JUNKS	    36
#define FLG_SPELLBOOKS	    37
#define FLG_HAFTED	    38
#define FLG_SHIELDS	    39
#define FLG_BOWS	    40
#define FLG_RINGS	    41
#define FLG_AMULETS	    42
#define FLG_SUITS	    43
#define FLG_CLOAKS	    44
#define FLG_HELMS	    45
#define FLG_GLOVES	    46
#define FLG_BOOTS           47

#ifdef JP

#define KEY_ALL "すべての"

#ifdef MAC_MPW
/*
 * MEGA HACK -- MPWのバグ除け。
 * pre-process中に「収」の字の2バイト目が勝手に消えてしまう。
 */
#define KEY_COLLECTING "\x8e\xfb集中の"
#else
#define KEY_COLLECTING "収集中の"
#endif

#define KEY_UNIDENTIFIED "未鑑定の"
#define KEY_IDENTIFIED "鑑定済みの"
#define KEY_STAR_IDENTIFIED "*鑑定*済みの"
#define KEY_BOOSTED "ダイス目の違う"
#define KEY_MORE_THAN  "ダイス目"
#define KEY_DICE  "以上の"
#define KEY_MORE_BONUS  "修正値"
#define KEY_MORE_BONUS2  "以上の"
#define KEY_WORTHLESS "無価値の"
#define KEY_ARTIFACT "アーティファクト"
#define KEY_EGO "エゴ"
#define KEY_NAMELESS "無銘の"
#define KEY_UNAWARE "未判明の"
#define KEY_WANTED "賞金首の"
#define KEY_UNIQUE "ユニーク・モンスターの"
#define KEY_HUMAN "人間の"
#define KEY_UNREADABLE "読めない"
#define KEY_REALM1 "第一領域の"
#define KEY_REALM2 "第二領域の"
#define KEY_FIRST "1冊目の"
#define KEY_SECOND "2冊目の"
#define KEY_THIRD "3冊目の"
#define KEY_FOURTH "4冊目の"
#define KEY_ITEMS "アイテム"
#define KEY_WEAPONS "武器"
#define KEY_ARMORS "防具"
#define KEY_MISSILES "矢"
#define KEY_DEVICES "魔法アイテム"
#define KEY_LIGHTS "光源"
#define KEY_JUNKS "がらくた"
#define KEY_SPELLBOOKS "魔法書"
#define KEY_HAFTED "鈍器"
#define KEY_SHIELDS "盾"
#define KEY_BOWS "弓"
#define KEY_RINGS "指輪"
#define KEY_AMULETS "アミュレット"
#define KEY_SUITS "鎧"
#define KEY_CLOAKS "クローク"
#define KEY_HELMS "兜"
#define KEY_GLOVES "籠手"
#define KEY_BOOTS "靴"

#else 

#define KEY_ALL "all"
#define KEY_COLLECTING "collecting"
#define KEY_UNIDENTIFIED "unidentified"
#define KEY_IDENTIFIED "identified"
#define KEY_STAR_IDENTIFIED "*identified*"
#define KEY_BOOSTED "dice boosted"
#define KEY_MORE_THAN  "more than"
#define KEY_DICE  " dice"
#define KEY_MORE_BONUS  "more bonus than"
#define KEY_MORE_BONUS2  ""
#define KEY_WORTHLESS "worthless"
#define KEY_ARTIFACT "artifact"
#define KEY_EGO "ego"
#define KEY_NAMELESS "nameless"
#define KEY_UNAWARE "unaware"
#define KEY_WANTED "wanted"
#define KEY_UNIQUE "unique monster's"
#define KEY_HUMAN "human"
#define KEY_UNREADABLE "unreadable"
#define KEY_REALM1 "first realm's"
#define KEY_REALM2 "second realm's"
#define KEY_FIRST "first"
#define KEY_SECOND "second"
#define KEY_THIRD "third"
#define KEY_FOURTH "fourth"
#define KEY_ITEMS "items"
#define KEY_WEAPONS "weapons"
#define KEY_ARMORS "armors"
#define KEY_MISSILES "missiles"
#define KEY_DEVICES "magical devices"
#define KEY_LIGHTS "lights"
#define KEY_JUNKS "junks"
#define KEY_SPELLBOOKS "spellbooks"
#define KEY_HAFTED "hafted weapons"
#define KEY_SHIELDS "shields"
#define KEY_BOWS "bows"
#define KEY_RINGS "rings"
#define KEY_AMULETS "amulets"
#define KEY_SUITS "suits"
#define KEY_CLOAKS "cloaks"
#define KEY_HELMS "helms"
#define KEY_GLOVES "gloves"
#define KEY_BOOTS "boots"

#endif /* JP */

#define MATCH_KEY(KEY) (!strncmp(ptr, KEY, sizeof(KEY)-1)\
     ? (ptr += sizeof(KEY)-1, (' '==*ptr) ? ptr++ : 0, TRUE) : FALSE)
#define MATCH_KEY2(KEY) (!strncmp(ptr, KEY, sizeof(KEY)-1)\
     ? (prev_ptr = ptr, ptr += sizeof(KEY)-1, (' '==*ptr) ? ptr++ : 0, TRUE) : FALSE)

#ifdef JP
#define ADD_KEY(KEY) strcat(ptr, KEY)
#else
#define ADD_KEY(KEY) (strcat(ptr, KEY), strcat(ptr, " "))
#endif
#define ADD_KEY2(KEY) strcat(ptr, KEY)

#define ADD_FLG(FLG) (entry->flag[FLG / 32] |= (1L << (FLG % 32)))
#define REM_FLG(FLG) (entry->flag[FLG / 32] &= ~(1L << (FLG % 32)))
#define ADD_FLG_NOUN(FLG) (ADD_FLG(FLG), prev_flg = FLG)
#define IS_FLG(FLG) (entry->flag[FLG / 32] & (1L << (FLG % 32)))

#ifdef JP
	static char kanji_colon[] = "：";
#endif


/*
 * Reconstruct preference line from entry
 */
cptr autopick_line_from_entry(autopick_type *entry)
{
	char buf[MAX_LINELEN];
	char *ptr;
	bool sepa_flag = TRUE;

	*buf = '\0';
	if (!(entry->action & DO_DISPLAY)) strcat(buf, "(");
	if (entry->action & DO_QUERY_AUTOPICK) strcat(buf, ";");
	if (entry->action & DO_AUTODESTROY) strcat(buf, "!");
	if (entry->action & DONT_AUTOPICK) strcat(buf, "~");

	ptr = buf;

	if (IS_FLG(FLG_ALL)) ADD_KEY(KEY_ALL);
	if (IS_FLG(FLG_COLLECTING)) ADD_KEY(KEY_COLLECTING);
	if (IS_FLG(FLG_UNIDENTIFIED)) ADD_KEY(KEY_UNIDENTIFIED);
	if (IS_FLG(FLG_IDENTIFIED)) ADD_KEY(KEY_IDENTIFIED);
	if (IS_FLG(FLG_STAR_IDENTIFIED)) ADD_KEY(KEY_STAR_IDENTIFIED);
	if (IS_FLG(FLG_UNAWARE)) ADD_KEY(KEY_UNAWARE);
	if (IS_FLG(FLG_BOOSTED)) ADD_KEY(KEY_BOOSTED);

	if (IS_FLG(FLG_MORE_THAN))
	{
		ADD_KEY(KEY_MORE_THAN);
		strcat(ptr, format("%d", entry->dice));
		ADD_KEY(KEY_DICE);
	}

	if (IS_FLG(FLG_MORE_BONUS))
	{
		ADD_KEY(KEY_MORE_BONUS);
		strcat(ptr, format("%d", entry->bonus));
		ADD_KEY(KEY_MORE_BONUS2);
	}

	if (IS_FLG(FLG_UNREADABLE)) ADD_KEY(KEY_UNREADABLE);
	if (IS_FLG(FLG_REALM1)) ADD_KEY(KEY_REALM1);
	if (IS_FLG(FLG_REALM2)) ADD_KEY(KEY_REALM2);
	if (IS_FLG(FLG_FIRST)) ADD_KEY(KEY_FIRST);
	if (IS_FLG(FLG_SECOND)) ADD_KEY(KEY_SECOND);
	if (IS_FLG(FLG_THIRD)) ADD_KEY(KEY_THIRD);
	if (IS_FLG(FLG_FOURTH)) ADD_KEY(KEY_FOURTH);
	if (IS_FLG(FLG_WANTED)) ADD_KEY(KEY_WANTED);
	if (IS_FLG(FLG_UNIQUE)) ADD_KEY(KEY_UNIQUE);
	if (IS_FLG(FLG_HUMAN)) ADD_KEY(KEY_HUMAN);
	if (IS_FLG(FLG_WORTHLESS)) ADD_KEY(KEY_WORTHLESS);
	if (IS_FLG(FLG_NAMELESS)) ADD_KEY(KEY_NAMELESS);
	if (IS_FLG(FLG_EGO)) ADD_KEY(KEY_EGO);

	if (IS_FLG(FLG_ARTIFACT)) ADD_KEY(KEY_ARTIFACT);

	if (IS_FLG(FLG_ITEMS)) ADD_KEY2(KEY_ITEMS);
	else if (IS_FLG(FLG_WEAPONS)) ADD_KEY2(KEY_WEAPONS);
	else if (IS_FLG(FLG_ARMORS)) ADD_KEY2(KEY_ARMORS);
	else if (IS_FLG(FLG_MISSILES)) ADD_KEY2(KEY_MISSILES);
	else if (IS_FLG(FLG_DEVICES)) ADD_KEY2(KEY_DEVICES);
	else if (IS_FLG(FLG_LIGHTS)) ADD_KEY2(KEY_LIGHTS);
	else if (IS_FLG(FLG_JUNKS)) ADD_KEY2(KEY_JUNKS);
	else if (IS_FLG(FLG_SPELLBOOKS)) ADD_KEY2(KEY_SPELLBOOKS);
	else if (IS_FLG(FLG_HAFTED)) ADD_KEY2(KEY_HAFTED);
	else if (IS_FLG(FLG_SHIELDS)) ADD_KEY2(KEY_SHIELDS);
	else if (IS_FLG(FLG_BOWS)) ADD_KEY2(KEY_BOWS);
	else if (IS_FLG(FLG_RINGS)) ADD_KEY2(KEY_RINGS);
	else if (IS_FLG(FLG_AMULETS)) ADD_KEY2(KEY_AMULETS);
	else if (IS_FLG(FLG_SUITS)) ADD_KEY2(KEY_SUITS);
	else if (IS_FLG(FLG_CLOAKS)) ADD_KEY2(KEY_CLOAKS);
	else if (IS_FLG(FLG_HELMS)) ADD_KEY2(KEY_HELMS);
	else if (IS_FLG(FLG_GLOVES)) ADD_KEY2(KEY_GLOVES);
	else if (IS_FLG(FLG_BOOTS)) ADD_KEY2(KEY_BOOTS);

	/* You don't need sepalator after adjective */
	/* 'artifact' is not true adjective */
	else if (!IS_FLG(FLG_ARTIFACT))
		sepa_flag = FALSE;

	if (entry->name && entry->name[0])
	{
		int i, j = 0;

		if (sepa_flag) strcat(buf, ":");

		i = strlen(buf);
		while (entry->name[j] && i < MAX_LINELEN - 2 - 1)
		{
#ifdef JP
			if (iskanji(entry->name[j]))
				buf[i++] = entry->name[j++];
#endif
			buf[i++] = entry->name[j++];
		}
		buf[i] = '\0';
	}

	if (entry->insc)
	{
		int i, j = 0;
		strcat(buf, "#");
		i = strlen(buf);

		while (entry->insc[j] && i < MAX_LINELEN - 2)
		{
#ifdef JP
			if (iskanji(entry->insc[j]))
				buf[i++] = entry->insc[j++];
#endif
			buf[i++] = entry->insc[j++];
		}
		buf[i] = '\0';
	}

	return string_make(buf);
}


/*
 * Reconstruct preference line from entry and kill entry
 */
static cptr autopick_line_from_entry_kill(autopick_type *entry)
{
	cptr ptr = autopick_line_from_entry(entry);

	/* Free memory for original entry */
	autopick_free_entry(entry);

	return ptr;
}


/*
 * A function to create new entry
 */
bool autopick_new_entry(autopick_type *entry, cptr str)
{
	cptr insc;
	int i;
	byte act = 0;
	char buf[MAX_LINELEN];
	cptr prev_ptr, ptr, old_ptr;
	int prev_flg;

	if (str[1] == ':') switch (str[0])
	{
	case '?': case '%':
	case 'A': case 'P': case 'C':
		return FALSE;
	}

	entry->flag[0] = entry->flag[1] = 0L;
	entry->dice = 0;

	act = DO_AUTOPICK | DO_DISPLAY;
	while (1)
	{
		if ((act & DO_AUTOPICK) && *str == '!')
		{
			act &= ~DO_AUTOPICK;
			act |= DO_AUTODESTROY;
			str++;
		}
		else if ((act & DO_AUTOPICK) && *str == '~')
		{
			act &= ~DO_AUTOPICK;
			act |= DONT_AUTOPICK;
			str++;
		}
		else if ((act & DO_AUTOPICK) && *str == ';')
		{
			act &= ~DO_AUTOPICK;
			act |= DO_QUERY_AUTOPICK;
			str++;
		}
		else if ((act & DO_DISPLAY) && *str == '(')
		{
			act &= ~DO_DISPLAY;
			str++;
		}
		else
			break;
	}

	/* don't mind upper or lower case */
	insc = NULL;
	for (i = 0; *str; i++)
	{
		char c = *str++;
#ifdef JP
		if (iskanji(c))
		{
			buf[i++] = c;
			buf[i] = *str++;
			continue;
		}
#endif
		/* Auto-inscription? */
		if (c == '#')
		{
			buf[i] = '\0';
			insc = str;
			break;
		}

		if (isupper(c)) c = tolower(c);

		buf[i] = c;
	}
	buf[i] = '\0';
	
	/* Skip empty line */
	if (*buf == 0) return FALSE;

	ptr = prev_ptr = buf;
	old_ptr = NULL;

	while (old_ptr != ptr)
	{
		/* Save current location */
		old_ptr = ptr;

		if (MATCH_KEY(KEY_ALL)) ADD_FLG(FLG_ALL);
		if (MATCH_KEY(KEY_COLLECTING)) ADD_FLG(FLG_COLLECTING);
		if (MATCH_KEY(KEY_UNIDENTIFIED)) ADD_FLG(FLG_UNIDENTIFIED);
		if (MATCH_KEY(KEY_IDENTIFIED)) ADD_FLG(FLG_IDENTIFIED);
		if (MATCH_KEY(KEY_STAR_IDENTIFIED)) ADD_FLG(FLG_STAR_IDENTIFIED);
		if (MATCH_KEY(KEY_BOOSTED)) ADD_FLG(FLG_BOOSTED);

		/*** Weapons whose dd*ds is more than nn ***/
		if (MATCH_KEY2(KEY_MORE_THAN))
		{
			int k = 0;
			entry->dice = 0;

			/* Drop leading spaces */
			while (' ' == *ptr) ptr++;

			/* Read number */
			while ('0' <= *ptr && *ptr <= '9')
			{
				entry->dice = 10 * entry->dice + (*ptr - '0');
				ptr++;
				k++;
			}

			if (k > 0 && k <= 2)
			{
				(void)MATCH_KEY(KEY_DICE);
				ADD_FLG(FLG_MORE_THAN);
			}
			else
				ptr = prev_ptr;
		}

		/*** Items whose magical bonus is more than n ***/
		if (MATCH_KEY2(KEY_MORE_BONUS))
		{
			int k = 0;
			entry->bonus = 0;

			/* Drop leading spaces */
			while (' ' == *ptr) ptr++;

			/* Read number */
			while ('0' <= *ptr && *ptr <= '9')
			{
				entry->bonus = 10 * entry->bonus + (*ptr - '0');
				ptr++;
				k++;
			}

			if (k > 0 && k <= 2)
			{
				(void)MATCH_KEY(KEY_MORE_BONUS2);
				ADD_FLG(FLG_MORE_BONUS);
			}
			else
				ptr = prev_ptr;
		}

		if (MATCH_KEY(KEY_WORTHLESS)) ADD_FLG(FLG_WORTHLESS);
		if (MATCH_KEY(KEY_EGO)) ADD_FLG(FLG_EGO);
		if (MATCH_KEY(KEY_NAMELESS)) ADD_FLG(FLG_NAMELESS);
		if (MATCH_KEY(KEY_UNAWARE)) ADD_FLG(FLG_UNAWARE);
		if (MATCH_KEY(KEY_WANTED)) ADD_FLG(FLG_WANTED);
		if (MATCH_KEY(KEY_UNIQUE)) ADD_FLG(FLG_UNIQUE);
		if (MATCH_KEY(KEY_HUMAN)) ADD_FLG(FLG_HUMAN);
		if (MATCH_KEY(KEY_UNREADABLE)) ADD_FLG(FLG_UNREADABLE);
		if (MATCH_KEY(KEY_REALM1)) ADD_FLG(FLG_REALM1);
		if (MATCH_KEY(KEY_REALM2)) ADD_FLG(FLG_REALM2);
		if (MATCH_KEY(KEY_FIRST)) ADD_FLG(FLG_FIRST);
		if (MATCH_KEY(KEY_SECOND)) ADD_FLG(FLG_SECOND);
		if (MATCH_KEY(KEY_THIRD)) ADD_FLG(FLG_THIRD);
		if (MATCH_KEY(KEY_FOURTH)) ADD_FLG(FLG_FOURTH);
	}

	/* Not yet found any noun */
	prev_flg = -1;

	if (MATCH_KEY2(KEY_ARTIFACT)) ADD_FLG_NOUN(FLG_ARTIFACT);

	if (MATCH_KEY2(KEY_ITEMS)) ADD_FLG_NOUN(FLG_ITEMS);
	else if (MATCH_KEY2(KEY_WEAPONS)) ADD_FLG_NOUN(FLG_WEAPONS);
	else if (MATCH_KEY2(KEY_ARMORS)) ADD_FLG_NOUN(FLG_ARMORS);
	else if (MATCH_KEY2(KEY_MISSILES)) ADD_FLG_NOUN(FLG_MISSILES);
	else if (MATCH_KEY2(KEY_DEVICES)) ADD_FLG_NOUN(FLG_DEVICES);
	else if (MATCH_KEY2(KEY_LIGHTS)) ADD_FLG_NOUN(FLG_LIGHTS);
	else if (MATCH_KEY2(KEY_JUNKS)) ADD_FLG_NOUN(FLG_JUNKS);
	else if (MATCH_KEY2(KEY_SPELLBOOKS)) ADD_FLG_NOUN(FLG_SPELLBOOKS);
	else if (MATCH_KEY2(KEY_HAFTED)) ADD_FLG_NOUN(FLG_HAFTED);
	else if (MATCH_KEY2(KEY_SHIELDS)) ADD_FLG_NOUN(FLG_SHIELDS);
	else if (MATCH_KEY2(KEY_BOWS)) ADD_FLG_NOUN(FLG_BOWS);
	else if (MATCH_KEY2(KEY_RINGS)) ADD_FLG_NOUN(FLG_RINGS);
	else if (MATCH_KEY2(KEY_AMULETS)) ADD_FLG_NOUN(FLG_AMULETS);
	else if (MATCH_KEY2(KEY_SUITS)) ADD_FLG_NOUN(FLG_SUITS);
	else if (MATCH_KEY2(KEY_CLOAKS)) ADD_FLG_NOUN(FLG_CLOAKS);
	else if (MATCH_KEY2(KEY_HELMS)) ADD_FLG_NOUN(FLG_HELMS);
	else if (MATCH_KEY2(KEY_GLOVES)) ADD_FLG_NOUN(FLG_GLOVES);
	else if (MATCH_KEY2(KEY_BOOTS)) ADD_FLG_NOUN(FLG_BOOTS);

	/* Last 'keyword' must be at the correct location */
	if (*ptr == ':')
		ptr++;
#ifdef JP
	else if (ptr[0] == kanji_colon[0] && ptr[1] == kanji_colon[1])
		ptr += 2;
#endif
	else if (*ptr == '\0')
		; /* nothing to do */
	else
	{
		/* Noun type? */
		if (prev_flg != -1)
		{
			/* A noun type keyword didn't end correctly */
			entry->flag[prev_flg/32] &= ~(1L<< (prev_flg%32));
			ptr = prev_ptr;
		}
	}

	/* Save this auto-picker entry line */
	entry->name = string_make(ptr);
	entry->action = act;
	entry->insc = string_make(insc);

	return TRUE;
}

/*
 * A function to delete entry
 */
void autopick_free_entry(autopick_type *entry)
{
	string_free(entry->name);
	string_free(entry->insc);
}


/*
 * A function for Auto-picker/destroyer
 * Examine whether the object matches to the entry
 */
static bool is_autopick_aux(object_type *o_ptr, autopick_type *entry, cptr o_name)
{
	int j;
	cptr ptr = entry->name;

	/*** Unidentified ***/
	if (IS_FLG(FLG_UNIDENTIFIED)
	    && (object_known_p(o_ptr) || (o_ptr->ident & IDENT_SENSE)))
		return FALSE;

	/*** Identified ***/
	if (IS_FLG(FLG_IDENTIFIED) && !object_known_p(o_ptr))
		return FALSE;

	/*** *Identified* ***/
	if (IS_FLG(FLG_STAR_IDENTIFIED) &&
	    (!object_known_p(o_ptr) || !(o_ptr->ident & IDENT_MENTAL)))
		return FALSE;

	/*** Dice boosted (weapon of slaying) ***/
	if (IS_FLG(FLG_BOOSTED))
	{
		object_kind *k_ptr = &k_info[o_ptr->k_idx];
			
		switch( o_ptr->tval )
		{
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_SWORD:
		case TV_DIGGING:
			if ((o_ptr->dd != k_ptr->dd) || (o_ptr->ds != k_ptr->ds))
				break;
			else
				return FALSE;
		default:
			return FALSE;
		}
	}

	/*** Weapons which dd*ds is more than nn ***/
	if (IS_FLG(FLG_MORE_THAN))
	{
		if (o_ptr->dd * o_ptr->ds < entry->dice)
			return FALSE;
	}
				
	/*** Weapons whic dd*ds is more than nn ***/
	if (IS_FLG(FLG_MORE_BONUS))
	{
		if (!object_known_p(o_ptr)) return FALSE;

		if (o_ptr->pval)
		{
			if (o_ptr->pval < entry->bonus) return FALSE;
		}
		else
		{
			if (o_ptr->to_h < entry->bonus &&
			    o_ptr->to_d < entry->bonus &&
			    o_ptr->to_a < entry->bonus &&
			    o_ptr->pval < entry->bonus)
				return FALSE;
		}
	}
				
	/*** Worthless items ***/
	if (IS_FLG(FLG_WORTHLESS) && object_value(o_ptr) > 0)
		return FALSE;

	/*** Artifact object ***/
	if (IS_FLG(FLG_ARTIFACT))
	{
		if (!object_known_p(o_ptr) || (!o_ptr->name1 && !o_ptr->art_name))
			return FALSE;
	}

	/*** Ego object ***/
	if (IS_FLG(FLG_EGO))
	{
		if (!object_known_p(o_ptr) || !o_ptr->name2)
			return FALSE;
	}

	/*** Nameless ***/
	if (IS_FLG(FLG_NAMELESS))
	{
		switch (o_ptr->tval)
		{
		case TV_WHISTLE:
		case TV_SHOT: case TV_ARROW: case TV_BOLT: case TV_BOW:
		case TV_DIGGING: case TV_HAFTED: case TV_POLEARM: case TV_SWORD: 
		case TV_BOOTS: case TV_GLOVES: case TV_HELM: case TV_CROWN:
		case TV_SHIELD: case TV_CLOAK:
		case TV_SOFT_ARMOR: case TV_HARD_ARMOR: case TV_DRAG_ARMOR:
		case TV_LITE: case TV_AMULET: case TV_RING: case TV_CARD:
			if ((!object_known_p(o_ptr) || o_ptr->inscription
			     || o_ptr->name1 || o_ptr->name2 || o_ptr->art_name))
				return FALSE;
			break;
		default:
			/* don't match */
			return FALSE;
		}
	}

	/*** Unaware items ***/
	if (IS_FLG(FLG_UNAWARE) && object_aware_p(o_ptr))
		return FALSE;

	/*** Wanted monster's corpse/skeletons ***/
	if (IS_FLG(FLG_WANTED) &&
	    (o_ptr->tval != TV_CORPSE || !object_is_shoukinkubi(o_ptr)))
		return FALSE;

	/*** Unique monster's corpse/skeletons/statues ***/
	if (IS_FLG(FLG_UNIQUE) &&
	    ((o_ptr->tval != TV_CORPSE && o_ptr->tval != TV_STATUE) ||
	     !(r_info[o_ptr->pval].flags1 & RF1_UNIQUE)))
		return FALSE;

	/*** Human corpse/skeletons (for Daemon magic) ***/
	if (IS_FLG(FLG_HUMAN) &&
	    (o_ptr->tval != TV_CORPSE ||
	     !strchr("pht", r_info[o_ptr->pval].d_char)))
		return FALSE;

	/*** Unreadable spellbooks ***/
	if (IS_FLG(FLG_UNREADABLE) &&
	    (o_ptr->tval < TV_LIFE_BOOK ||
	     check_book_realm(o_ptr->tval, o_ptr->sval)))
		return FALSE;

	/*** First realm spellbooks ***/
	if (IS_FLG(FLG_REALM1) && 
	    (REALM1_BOOK != o_ptr->tval ||
	     p_ptr->pclass == CLASS_SORCERER ||
	     p_ptr->pclass == CLASS_RED_MAGE))
		return FALSE;

	/*** Second realm spellbooks ***/
	if (IS_FLG(FLG_REALM2) &&
	    (REALM2_BOOK != o_ptr->tval ||
	     p_ptr->pclass == CLASS_SORCERER ||
	     p_ptr->pclass == CLASS_RED_MAGE))
		return FALSE;

	/*** First rank spellbooks ***/
	if (IS_FLG(FLG_FIRST) &&
	    (o_ptr->tval < TV_LIFE_BOOK || 0 != o_ptr->sval))
		return FALSE;

	/*** Second rank spellbooks ***/
	if (IS_FLG(FLG_SECOND) &&
	    (o_ptr->tval < TV_LIFE_BOOK || 1 != o_ptr->sval))
		return FALSE;

	/*** Third rank spellbooks ***/
	if (IS_FLG(FLG_THIRD) && 
	    (o_ptr->tval < TV_LIFE_BOOK || 2 != o_ptr->sval))
		return FALSE;

	/*** Fourth rank spellbooks ***/
	if (IS_FLG(FLG_FOURTH) &&
	    (o_ptr->tval < TV_LIFE_BOOK || 3 != o_ptr->sval))
		return FALSE;

	/*** Items ***/
	if (IS_FLG(FLG_WEAPONS))
	{
		switch(o_ptr->tval)
		{
		case TV_BOW: case TV_HAFTED: case TV_POLEARM:
		case TV_SWORD: case TV_DIGGING:
			break;
		default: return FALSE;
		}
	}
	else if (IS_FLG(FLG_ARMORS))
	{
		switch(o_ptr->tval)
		{
		case TV_BOOTS: case TV_GLOVES: case TV_CLOAK: case TV_CROWN:
		case TV_HELM: case TV_SHIELD: case TV_SOFT_ARMOR:
		case TV_HARD_ARMOR: case TV_DRAG_ARMOR:
			break;
		default: return FALSE;
		}
	}
	else if (IS_FLG(FLG_MISSILES))
	{
		switch(o_ptr->tval)
		{
		case TV_SHOT: case TV_BOLT: case TV_ARROW:
			break;
		default: return FALSE;
		}
	}
	else if (IS_FLG(FLG_DEVICES))
	{
		switch(o_ptr->tval)
		{
		case TV_SCROLL: case TV_STAFF: case TV_WAND: case TV_ROD:
			break;
		default: return FALSE;
		}
	}
	else if (IS_FLG(FLG_LIGHTS))
	{
		if (!(o_ptr->tval == TV_LITE))
			return FALSE;
	}
	else if (IS_FLG(FLG_JUNKS))
	{
		switch(o_ptr->tval)
		{
		case TV_SKELETON: case TV_BOTTLE:
		case TV_JUNK: case TV_STATUE:
			break;
		default: return FALSE;
		}
	}
	else if (IS_FLG(FLG_SPELLBOOKS))
	{
		if (!(o_ptr->tval >= TV_LIFE_BOOK))
			return FALSE;
	}
	else if (IS_FLG(FLG_HAFTED))
	{
		if (!(o_ptr->tval == TV_HAFTED))
			return FALSE;
	}
	else if (IS_FLG(FLG_SHIELDS))
	{
		if (!(o_ptr->tval == TV_SHIELD))
			return FALSE;
	}
	else if (IS_FLG(FLG_BOWS))
	{
		if (!(o_ptr->tval == TV_BOW))
			return FALSE;
	}
	else if (IS_FLG(FLG_RINGS))
	{
		if (!(o_ptr->tval == TV_RING))
			return FALSE;
	}
	else if (IS_FLG(FLG_AMULETS))
	{
		if (!(o_ptr->tval == TV_AMULET))
			return FALSE;
	}
	else if (IS_FLG(FLG_SUITS))
	{
		if (!(o_ptr->tval == TV_DRAG_ARMOR ||
		      o_ptr->tval == TV_HARD_ARMOR ||
		      o_ptr->tval == TV_SOFT_ARMOR))
			return FALSE;
	}
	else if (IS_FLG(FLG_CLOAKS))
	{
		if (!(o_ptr->tval == TV_CLOAK))
			return FALSE;
	}
	else if (IS_FLG(FLG_HELMS))
	{
		if (!(o_ptr->tval == TV_CROWN || o_ptr->tval == TV_HELM))
			return FALSE;
	}
	else if (IS_FLG(FLG_GLOVES))
	{
		if (!(o_ptr->tval == TV_GLOVES))
			return FALSE;
	}
	else if (IS_FLG(FLG_BOOTS))
	{
		if (!(o_ptr->tval == TV_BOOTS))
			return FALSE;
	}

	/* Keyword don't match */
	if (*ptr == '^')
	{
		ptr++;
		if (strncmp(o_name, ptr, strlen(ptr))) return FALSE;
	}
	else
	{
#ifdef JP
		if (!strstr_j(o_name, ptr)) return FALSE;
#else
		if (!strstr(o_name, ptr)) return FALSE;
#endif
	}

	/* TRUE when it need not to be 'collecting' */
	if (!IS_FLG(FLG_COLLECTING)) return TRUE;

	/* Check if there is a same item */
	for (j = 0; j < INVEN_PACK; j++)
	{
		/*
		 * 'Collecting' means the item must be absorbed 
		 * into an inventory slot.
		 * But an item can not be absorbed into itself!
		 */
		if ((&inventory[j] != o_ptr) &&
		    object_similar(&inventory[j], o_ptr))
			return TRUE;
	}

	/* Not collecting */
	return FALSE;
}


/*
 * A function for Auto-picker/destroyer
 * Examine whether the object matches to the list of keywords or not.
 */
int is_autopick(object_type *o_ptr)
{
	int i;
	char o_name[MAX_NLEN];

	if (o_ptr->tval == TV_GOLD) return -1;
	
	object_desc(o_name, o_ptr, FALSE, 3);

	/* Force to be lower case string */
	for (i = 0; o_name[i]; i++)
	{
#ifdef JP
		if (iskanji(o_name[i]))
			i++;
		else
#endif
		if (isupper(o_name[i]))
			o_name[i] = tolower(o_name[i]);
	}
	
	for (i=0; i < max_autopick; i++)
	{
		autopick_type *entry = &autopick_list[i];

		if (is_autopick_aux(o_ptr, entry, o_name)) return i;
	}

	/* No matching entry */
	return -1;
}


/*
 * Automatically destroy items in this grid.
 */
static bool is_opt_confirm_destroy(object_type *o_ptr)
{
	if (!destroy_items) return FALSE;

	/* Known to be worthless? */
	if (leave_worth)
		if (object_value(o_ptr) > 0) return FALSE;
	
	if (leave_equip)
		if ((o_ptr->tval >= TV_SHOT) && (o_ptr->tval <= TV_DRAG_ARMOR)) return FALSE;
	
	if (leave_chest)
		if ((o_ptr->tval == TV_CHEST) && o_ptr->pval) return FALSE;
	
	if (leave_wanted)
	{
		if (o_ptr->tval == TV_CORPSE
		    && object_is_shoukinkubi(o_ptr)) return FALSE;
	}
	
	if (leave_corpse)
		if (o_ptr->tval == TV_CORPSE) return FALSE;
	
	if (leave_junk)
		if ((o_ptr->tval == TV_SKELETON) || (o_ptr->tval == TV_BOTTLE) || (o_ptr->tval == TV_JUNK) || (o_ptr->tval == TV_STATUE)) return FALSE;

	if (leave_special)
	{
#if 0
		if (p_ptr->prace == RACE_SKELETON)
		{
			if (o_ptr->tval == TV_SKELETON ||
			    (o_ptr->tval == TV_CORPSE && o_ptr->sval == SV_SKELETON))
				return FALSE;
		}
		else 
#endif
		if (p_ptr->prace == RACE_DEMON)
		{
			if (o_ptr->tval == TV_CORPSE &&
			    o_ptr->sval == SV_CORPSE &&
			    strchr("pht", r_info[o_ptr->pval].d_char))
				return FALSE;
		}

		if (p_ptr->pclass == CLASS_ARCHER)
		{
			if (o_ptr->tval == TV_SKELETON ||
			    (o_ptr->tval == TV_CORPSE && o_ptr->sval == SV_SKELETON))
				return FALSE;
		}
		else if (p_ptr->pclass == CLASS_NINJA)
		{
			if (o_ptr->tval == TV_LITE &&
			    o_ptr->name2 == EGO_LITE_DARKNESS)
				return FALSE;
		}
	}
	
	if (o_ptr->tval == TV_GOLD) return FALSE;
	
	return TRUE;
}


/*
 *  Auto inscription
 */
void auto_inscribe_item(int item, int idx)
{
	object_type *o_ptr;

	/* Get the item (in the pack) */
	if (item >= 0) o_ptr = &inventory[item];

	/* Get the item (on the floor) */
	else o_ptr = &o_list[0 - item];

	/* Auto-inscription or Re-inscribe for resistances {%} */
	if ((idx < 0 || !autopick_list[idx].insc) && !o_ptr->inscription)
		return;

	if (!o_ptr->inscription)
		o_ptr->inscription = quark_add(autopick_list[idx].insc);

	if (item > INVEN_PACK)
	{
		/* Redraw inscription */
		p_ptr->window |= (PW_EQUIP);

		/* {.} and {$} effect p_ptr->warning and TRC_TELEPORT_SELF */
		p_ptr->update |= (PU_BONUS);
	}
	else if (item >= 0)
	{
		/* Redraw inscription */
		p_ptr->window |= (PW_INVEN);
	}
}


/*
 * Automatically destroy an item if it is to be destroyed
 */
bool auto_destroy_item(int item, int autopick_idx)
{
	bool destroy = FALSE;
	object_type *o_ptr;

	/* Don't destroy equipped items */
	if (item > INVEN_PACK) return FALSE;

	/* Get the item (in the pack) */
	if (item >= 0) o_ptr = &inventory[item];

	/* Get the item (on the floor) */
	else o_ptr = &o_list[0 - item];

	/* Easy-Auto-Destroyer */
	if (is_opt_confirm_destroy(o_ptr)) destroy = TRUE;

	/* Protected by auto-picker */
	if (autopick_idx >= 0 &&
	    !(autopick_list[autopick_idx].action & DO_AUTODESTROY))
		destroy = FALSE;

	if (!always_pickup)
	{
		/* Auto-picker/destroyer */
		if (autopick_idx >= 0 &&
		    (autopick_list[autopick_idx].action & DO_AUTODESTROY))
			destroy = TRUE;
	}

	/* Not to be destroyed */
	if (!destroy) return FALSE;

	/* Now decided to destroy */

	disturb(0,0);

	/* Artifact? */
	if (!can_player_destroy_object(o_ptr))
	{
		char o_name[MAX_NLEN];

		/* Describe the object (with {terrible/special}) */
		object_desc(o_name, o_ptr, TRUE, 3);

		/* Message */
#ifdef JP
		msg_format("%sは破壊不能だ。", o_name);
#else
		msg_format("You cannot auto-destroy %s.", o_name);
#endif

		/* Done */
		return TRUE;
	}

	/* Record name of destroyed item */
	COPY(&autopick_last_destroyed_object, o_ptr, object_type);

	/* Destroy Later */
	o_ptr->marked |= OM_AUTODESTROY;
	p_ptr->notice |= PN_AUTODESTROY;

	return TRUE;
}


/*
 *  Auto-destroy marked item
 */
static void delayed_auto_destroy_aux(int item)
{
	object_type *o_ptr;

	/* Get the item (in the pack) */
	if (item >= 0) o_ptr = &inventory[item];

	/* Get the item (on the floor) */
	else o_ptr = &o_list[0 - item];

	if (o_ptr->k_idx && o_ptr->marked & OM_AUTODESTROY)
	{
		char o_name[MAX_NLEN];

		/* Describe the object (with {terrible/special}) */
		object_desc(o_name, o_ptr, TRUE, 3);

		/* Eliminate the item (from the pack) */
		if (item >= 0)
		{
			inven_item_increase(item, -(o_ptr->number));
			inven_item_optimize(item);
		}

		/* Eliminate the item (from the floor) */
		else
		{
			delete_object_idx(0 - item);
		}

		/* Print a message */
#ifdef JP
		msg_format("%sを自動破壊します。", o_name);
#else
		msg_format("Auto-destroying %s.", o_name);
#endif
	}
}


/*
 *  Auto-destroy marked item in inventry and on floor
 */
void delayed_auto_destroy(void)
{
	int item;

	/* 
	 * Scan inventry in reverse order to prevent
	 * skipping after inven_item_optimize()
	 */
	for (item = INVEN_TOTAL - 1; item >= 0 ; item--)
		delayed_auto_destroy_aux(item);

	/* Scan the pile of objects */
	item = cave[py][px].o_idx;
	while (item)
	{
		int next = o_list[item].next_o_idx;
		delayed_auto_destroy_aux(-item);
		item = next;
	}
}


/*
 * Automatically pickup/destroy items in this grid.
 */
void auto_pickup_items(cave_type *c_ptr)
{
	s16b this_o_idx, next_o_idx = 0;
	
	/* Scan the pile of objects */
	for (this_o_idx = c_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		int idx;
	
		/* Acquire object */
		object_type *o_ptr = &o_list[this_o_idx];
		
		/* Acquire next object */
		next_o_idx = o_ptr->next_o_idx;

		idx = is_autopick(o_ptr);

		/* Item index for floor -1,-2,-3,...  */
		auto_inscribe_item((-this_o_idx), idx);

		if (idx >= 0 &&
			(autopick_list[idx].action & (DO_AUTOPICK | DO_QUERY_AUTOPICK)))
		{
			disturb(0,0);

			if (!inven_carry_okay(o_ptr))
			{
				char o_name[MAX_NLEN];

				/* Describe the object */
				object_desc(o_name, o_ptr, TRUE, 3);

				/* Message */
#ifdef JP
				msg_format("ザックには%sを入れる隙間がない。", o_name);
#else
				msg_format("You have no room for %s.", o_name);
#endif
				/* Hack - remember that the item has given a message here. */
				o_ptr->marked |= OM_NOMSG;

				continue;
			}
			else if (autopick_list[idx].action & DO_QUERY_AUTOPICK)
			{
				char out_val[MAX_NLEN+20];
				char o_name[MAX_NLEN];

				if (o_ptr->marked & OM_NO_QUERY)
				{
					/* Already answered as 'No' */
					continue;
				}

				/* Describe the object */
				object_desc(o_name, o_ptr, TRUE, 3);

#ifdef JP
				sprintf(out_val, "%sを拾いますか? ", o_name);
#else
				sprintf(out_val, "Pick up %s? ", o_name);
#endif

				if (!get_check(out_val))
				{
					/* Hack - remember that the item has given a message here. */
					o_ptr->marked |= (OM_NOMSG | OM_NO_QUERY);
					continue;
				}

			}
			py_pickup_aux(this_o_idx);

			continue;
		}
		
		/*
		 * Do auto-destroy;
		 * When always_pickup is 'yes', we disable
		 * auto-destroyer from autopick function, and do only
		 * easy-auto-destroyer.
		 */
		else
		{
			if (auto_destroy_item((-this_o_idx), idx))
				continue;
		}
	}
}


/*
 * Describe which kind of object is Auto-picked/destroyed
 */
static void describe_autopick(char *buff, autopick_type *entry)
{
	cptr str = entry->name;
	byte act = entry->action;
	cptr insc = entry->insc;
	int i;

	bool top = FALSE;

#ifdef JP
	cptr before_str[100], body_str;
	int before_n = 0;

	body_str = "アイテム";

	/*** Collecting items ***/
	/*** Which can be absorbed into a slot as a bundle ***/
	if (IS_FLG(FLG_COLLECTING))
		before_str[before_n++] = "収集中で既に持っているスロットにまとめられる";
	
	/*** Unidentified ***/
	if (IS_FLG(FLG_UNIDENTIFIED))
		before_str[before_n++] = "未鑑定の";

	/*** Identified ***/
	if (IS_FLG(FLG_IDENTIFIED))
		before_str[before_n++] = "鑑定済みの";

	/*** *Identified* ***/
	if (IS_FLG(FLG_STAR_IDENTIFIED))
		before_str[before_n++] = "完全に鑑定済みの";

	/*** Dice boosted (weapon of slaying) ***/
	if (IS_FLG(FLG_BOOSTED))
	{
		before_str[before_n++] = "ダメージダイスが通常より大きい";
		body_str = "武器";
	}

	/*** Weapons whose dd*ds is more than nn ***/
	if (IS_FLG(FLG_MORE_THAN))
	{
		static char more_than_desc_str[] = "___";
		before_str[before_n++] = "ダメージダイスの最大値が";
		body_str = "武器";
			
		sprintf(more_than_desc_str,"%d", entry->dice);
		before_str[before_n++] = more_than_desc_str;
		before_str[before_n++] = "以上の";
	}

	/*** Items whose magical bonus is more than nn ***/
	if (IS_FLG(FLG_MORE_BONUS))
	{
		static char more_bonus_desc_str[] = "___";
		before_str[before_n++] = "修正値が(+";
			
		sprintf(more_bonus_desc_str,"%d", entry->bonus);
		before_str[before_n++] = more_bonus_desc_str;
		before_str[before_n++] = ")以上の";
	}

	/*** Worthless items ***/
	if (IS_FLG(FLG_WORTHLESS))
		before_str[before_n++] = "店で無価値と判定される";

	/*** Artifact ***/
	if (IS_FLG(FLG_ARTIFACT))
	{
		before_str[before_n++] = "アーティファクトの";
		body_str = "装備";
	}

	/*** Ego ***/
	if (IS_FLG(FLG_EGO))
	{
		before_str[before_n++] = "エゴアイテムの";
		body_str = "装備";
	}

	/*** Nameless ***/
	if (IS_FLG(FLG_NAMELESS))
	{
		before_str[before_n++] = "エゴでもアーティファクトでもない";
		body_str = "装備";
	}

	/*** Unaware items ***/
	if (IS_FLG(FLG_UNAWARE))
		before_str[before_n++] = "未鑑定でその効果も判明していない";

	/*** Wanted monster's corpse/skeletons ***/
	if (IS_FLG(FLG_WANTED))
	{
		before_str[before_n++] = "ハンター事務所で賞金首とされている";
		body_str = "死体や骨";
	}

	/*** Human corpse/skeletons (for Daemon magic) ***/
	if (IS_FLG(FLG_HUMAN))
	{
		before_str[before_n++] = "悪魔魔法で使うための人間やヒューマノイドの";
		body_str = "死体や骨";
	}

	/*** Unique monster's corpse/skeletons/statues ***/
	if (IS_FLG(FLG_UNIQUE))
	{
		before_str[before_n++] = "ユニークモンスターの";
		body_str = "死体や骨";
	}

	/*** Unreadable spellbooks ***/
	if (IS_FLG(FLG_UNREADABLE))
	{
		before_str[before_n++] = "あなたが読めない領域の";
		body_str = "魔法書";
	}

	/*** First realm spellbooks ***/
	if (IS_FLG(FLG_REALM1))
	{
		before_str[before_n++] = "第一領域の";
		body_str = "魔法書";
	}

	/*** Second realm spellbooks ***/
	if (IS_FLG(FLG_REALM2))
	{
		before_str[before_n++] = "第二領域の";
		body_str = "魔法書";
	}

	/*** First rank spellbooks ***/
	if (IS_FLG(FLG_FIRST))
	{
		before_str[before_n++] = "全4冊の内の1冊目の";
		body_str = "魔法書";
	}

	/*** Second rank spellbooks ***/
	if (IS_FLG(FLG_SECOND))
	{
		before_str[before_n++] = "全4冊の内の2冊目の";
		body_str = "魔法書";
	}

	/*** Third rank spellbooks ***/
	if (IS_FLG(FLG_THIRD))
	{
		before_str[before_n++] = "全4冊の内の3冊目の";
		body_str = "魔法書";
	}

	/*** Fourth rank spellbooks ***/
	if (IS_FLG(FLG_FOURTH))
	{
		before_str[before_n++] = "全4冊の内の4冊目の";
		body_str = "魔法書";
	}

	/*** Items ***/
	if (IS_FLG(FLG_ITEMS))
		; /* Nothing to do */
	else if (IS_FLG(FLG_WEAPONS))
		body_str = "武器";
	else if (IS_FLG(FLG_ARMORS))
		body_str = "防具";
	else if (IS_FLG(FLG_MISSILES))
		body_str = "弾や矢やクロスボウの矢";
	else if (IS_FLG(FLG_DEVICES))
		body_str = "巻物や魔法棒や杖やロッド";
	else if (IS_FLG(FLG_LIGHTS))
		body_str = "光源用のアイテム";
	else if (IS_FLG(FLG_JUNKS))
		body_str = "折れた棒等のガラクタ";
	else if (IS_FLG(FLG_SPELLBOOKS))
		body_str = "魔法書";
	else if (IS_FLG(FLG_HAFTED))
		body_str = "鈍器";
	else if (IS_FLG(FLG_SHIELDS))
		body_str = "盾";
	else if (IS_FLG(FLG_BOWS))
		body_str = "スリングや弓やクロスボウ";
	else if (IS_FLG(FLG_RINGS))
		body_str = "指輪";
	else if (IS_FLG(FLG_AMULETS))
		body_str = "アミュレット";
	else if (IS_FLG(FLG_SUITS))
		body_str = "鎧";
	else if (IS_FLG(FLG_CLOAKS))
		body_str = "クローク";
	else if (IS_FLG(FLG_HELMS))
		body_str = "ヘルメットや冠";
	else if (IS_FLG(FLG_GLOVES))
		body_str = "籠手";
	else if (IS_FLG(FLG_BOOTS))
		body_str = "ブーツ";

	*buff = '\0';
	if (!before_n) 
		strcat(buff, "全ての");
	else for (i = 0; i < before_n && before_str[i]; i++)
		strcat(buff, before_str[i]);

	strcat(buff, body_str);

	if (*str)
	{
		if (*str == '^')
		{
			str++;
			top = TRUE;
		}

		strcat(buff, "で、名前が「");
		strncat(buff, str, 80);
		if (top)
			strcat(buff, "」で始まるもの");
		else
			strcat(buff, "」を含むもの");
	}

	if (insc)
	{
		strncat(buff, format("に「%s」", insc), 80);

		if (strstr(insc, "%%all"))
			strcat(buff, "(%%allは全能力を表す英字の記号で置換)");
		else if (strstr(insc, "%all"))
			strcat(buff, "(%allは全能力を表す記号で置換)");
		else if (strstr(insc, "%%"))
			strcat(buff, "(%%は追加能力を表す英字の記号で置換)");
		else if (strstr(insc, "%"))
			strcat(buff, "(%は追加能力を表す記号で置換)");

		strcat(buff, "と刻んで");
	}
	else
		strcat(buff, "を");

	if (act & DONT_AUTOPICK)
		strcat(buff, "放置する。");
	else if (act & DO_AUTODESTROY)
		strcat(buff, "破壊する。");
	else if (act & DO_QUERY_AUTOPICK)
		strcat(buff, "確認の後に拾う。");
	else
		strcat(buff, "拾う。");

	if (act & DO_DISPLAY)
	{
		if (act & DONT_AUTOPICK)
			strcat(buff, "全体マップ('M')で'N'を押したときに表示する。");
		else if (act & DO_AUTODESTROY)
			strcat(buff, "全体マップ('M')で'K'を押したときに表示する。");
		else
			strcat(buff, "全体マップ('M')で'M'を押したときに表示する。");
	}
	else
		strcat(buff, "全体マップには表示しない");

#else /* JP */

	cptr before_str[20], after_str[20], which_str[20], whose_str[20], body_str;
	int before_n = 0, after_n = 0, which_n = 0, whose_n = 0;

	body_str = "items";

	/*** Collecting items ***/
	/*** Which can be absorbed into a slot as a bundle ***/
	if (IS_FLG(FLG_COLLECTING))
		which_str[which_n++] = "can be absorbed into an existing inventory slot";
	
	/*** Unidentified ***/
	if (IS_FLG(FLG_UNIDENTIFIED))
		before_str[before_n++] = "unidentified";

	/*** Identified ***/
	if (IS_FLG(FLG_IDENTIFIED))
		before_str[before_n++] = "identified";

	/*** *Identified* ***/
	if (IS_FLG(FLG_STAR_IDENTIFIED))
		before_str[before_n++] = "fully identified";

	/*** Worthless items ***/
	if (IS_FLG(FLG_WORTHLESS))
	{
		before_str[before_n++] = "worthless";
		which_str[which_n++] = "can not be sold at stores";
	}

	/*** Artifacto ***/
	if (IS_FLG(FLG_ARTIFACT))
	{
		before_str[before_n++] = "artifact";
	}

	/*** Ego ***/
	if (IS_FLG(FLG_EGO))
	{
		before_str[before_n++] = "ego";
	}

	/*** Nameless ***/
	if (IS_FLG(FLG_NAMELESS))
	{
		body_str = "equipment";
		which_str[which_n++] = "is neither ego-item nor artifact";
	}

	/*** Unaware items ***/
	if (IS_FLG(FLG_UNAWARE))
	{
		before_str[before_n++] = "unidentified";
		whose_str[whose_n++] = "basic abilities are not known";
	}

	/*** Dice boosted (weapon of slaying) ***/
	if (IS_FLG(FLG_BOOSTED))
	{
		body_str = "weapons";
		whose_str[whose_n++] = "damage dice is bigger than normal";
	}

	/*** Weapons whose dd*ds is more than nn ***/
	if (IS_FLG(FLG_MORE_THAN))
	{
		static char more_than_desc_str[] =
			"maximum damage from dice is bigger than __";
		body_str = "weapons";
			
		sprintf(more_than_desc_str + sizeof(more_than_desc_str) - 3,
			"%d", entry->dice);
		whose_str[whose_n++] = more_than_desc_str;
	}

	/*** Items whose magical bonus is more than nn ***/
	if (IS_FLG(FLG_MORE_BONUS))
	{
		static char more_bonus_desc_str[] =
			"magical bonus is bigger than (+__)";
			
		sprintf(more_bonus_desc_str + sizeof(more_bonus_desc_str) - 4,
			"%d)", entry->bonus);
		whose_str[whose_n++] = more_bonus_desc_str;
	}

	/*** Wanted monster's corpse/skeletons ***/
	if (IS_FLG(FLG_WANTED))
	{
		body_str = "corpse or skeletons";
		which_str[which_n++] = "is wanted at the Hunter's Office";
	}

	/*** Human corpse/skeletons (for Daemon magic) ***/
	if (IS_FLG(FLG_HUMAN))
	{
		before_str[before_n++] = "humanoid";
		body_str = "corpse or skeletons";
		which_str[which_n++] = "can be used for Daemon magic";
	}

	/*** Unique monster's corpse/skeletons/statues ***/
	if (IS_FLG(FLG_UNIQUE))
	{
		before_str[before_n++] = "unique monster's";
		body_str = "corpse or skeletons";
	}

	/*** Unreadable spellbooks ***/
	if (IS_FLG(FLG_UNREADABLE))
	{
		body_str = "spellbooks";
		after_str[after_n++] = "of different realms from yours";
	}

	/*** First realm spellbooks ***/
	if (IS_FLG(FLG_REALM1))
	{
		body_str = "spellbooks";
		after_str[after_n++] = "of your first realm";
	}

	/*** Second realm spellbooks ***/
	if (IS_FLG(FLG_REALM2))
	{
		body_str = "spellbooks";
		after_str[after_n++] = "of your second realm";
	}

	/*** First rank spellbooks ***/
	if (IS_FLG(FLG_FIRST))
	{
		before_str[before_n++] = "first one of four";
		body_str = "spellbooks";
	}

	/*** Second rank spellbooks ***/
	if (IS_FLG(FLG_SECOND))
	{
		before_str[before_n++] = "second one of four";
		body_str = "spellbooks";
	}

	/*** Third rank spellbooks ***/
	if (IS_FLG(FLG_THIRD))
	{
		before_str[before_n++] = "third one of four";
		body_str = "spellbooks";
	}

	/*** Fourth rank spellbooks ***/
	if (IS_FLG(FLG_FOURTH))
	{
		before_str[before_n++] = "fourth one of four";
		body_str = "spellbooks";
	}

	/*** Items ***/
	if (IS_FLG(FLG_ITEMS))
		; /* Nothing to do */
	else if (IS_FLG(FLG_WEAPONS))
		body_str = "weapons";
	else if (IS_FLG(FLG_ARMORS))
		body_str = "armors";
	else if (IS_FLG(FLG_MISSILES))
		body_str = "shots, arrows or crossbow bolts";
	else if (IS_FLG(FLG_DEVICES))
		body_str = "scrolls, wands, staves or rods";
	else if (IS_FLG(FLG_LIGHTS))
		body_str = "light sources";
	else if (IS_FLG(FLG_JUNKS))
		body_str = "junk such as broken sticks";
	else if (IS_FLG(FLG_SPELLBOOKS))
		body_str = "spellbooks";
	else if (IS_FLG(FLG_HAFTED))
		body_str = "hafted weapons";
	else if (IS_FLG(FLG_SHIELDS))
		body_str = "shields";
	else if (IS_FLG(FLG_BOWS))
		body_str = "slings, bows or crossbows";
	else if (IS_FLG(FLG_RINGS))
		body_str = "rings";
	else if (IS_FLG(FLG_AMULETS))
		body_str = "amulets";
	else if (IS_FLG(FLG_SUITS))
		body_str = "body armors";
	else if (IS_FLG(FLG_CLOAKS))
		body_str = "cloaks";
	else if (IS_FLG(FLG_HELMS))
		body_str = "helms or crowns";
	else if (IS_FLG(FLG_GLOVES))
		body_str = "gloves";
	else if (IS_FLG(FLG_BOOTS))
		body_str = "boots";

	/* Prepare a string for item name */
	if (*str)
	{
		if (*str == '^')
		{
			str++;
			top = TRUE;
			whose_str[whose_n++] = "name is beginning with \"";
		}
		else
			which_str[which_n++] = "have \"";
	}


	/* Describe action flag */
	if (act & DONT_AUTOPICK)
		strcpy(buff, "Leave on floor ");
	else if (act & DO_AUTODESTROY)
		strcpy(buff, "Destroy ");
	else if (act & DO_QUERY_AUTOPICK)
		strcpy(buff, "Ask to pick up ");
	else
		strcpy(buff, "Pickup ");

	/* Auto-insctiption */
	if (insc)
	{
		strncat(buff, format("and inscribe \"%s\"", insc), 80);

		if (strstr(insc, "%all"))
			strcat(buff, ", replacing %all with code string representing all abilities,");
		else if (strstr(insc, "%"))
			strcat(buff, ", replacing % with code string representing extra random abilities,");

		strcat(buff, " on ");
	}

	/* Adjective */
	if (!before_n) 
		strcat(buff, "all ");
	else for (i = 0; i < before_n && before_str[i]; i++)
	{
		strcat(buff, before_str[i]);
		strcat(buff, " ");
	}

	/* Item class */
	strcat(buff, body_str);

	/* of ... */
	for (i = 0; i < after_n && after_str[i]; i++)
	{
		strcat(buff, " ");
		strcat(buff, after_str[i]);
	}

	/* which ... */
	for (i = 0; i < whose_n && whose_str[i]; i++)
	{
		if (i == 0)
			strcat(buff, " whose ");
		else
			strcat(buff, ", and ");

		strcat(buff, whose_str[i]);
	}

	/* Item name ; whose name is beginning with "str" */
	if (*str && top)
	{
		strcat(buff, str);
		strcat(buff, "\"");
	}

	/* whose ..., and which .... */
	if (whose_n && which_n)
		strcat(buff, ", and ");

	/* which ... */
	for (i = 0; i < which_n && which_str[i]; i++)
	{
		if (i == 0)
			strcat(buff, " which ");
		else
			strcat(buff, ", and ");

		strcat(buff, which_str[i]);
	}

	/* Item name ; which have "str" as part of its name */
	if (*str && !top)
	{
		strncat(buff, str, 80);
		strcat(buff, "\" as part of its name");
	}
	strcat(buff, ".");

	/* Describe whether it will be displayed on the full map or not */
	if (act & DO_DISPLAY)
	{
		if (act & DONT_AUTOPICK)
			strcat(buff, "  Display these items when you press the N key in the full 'M'ap.");
		else if (act & DO_AUTODESTROY)
			strcat(buff, "  Display these items when you press the K key in the full 'M'ap.");
		else
			strcat(buff, "  Display these items when you press the M key in the full 'M'ap.");
	}
	else
		strcat(buff, " Not displayed in the full map.");
#endif /* JP */

}


#define MAX_LINES 3000

/*
 * Read whole lines of a file to memory
 */
static cptr *read_text_lines(cptr filename, bool user)
{
	cptr *lines_list = NULL;
	FILE *fff;

	int lines = 0;
	char buf[1024];

	if (user)
	{
		path_build(buf, sizeof(buf), ANGBAND_DIR_USER, filename);
	}
	else
	{
		path_build(buf, sizeof(buf), ANGBAND_DIR_PREF, filename);
	}
	
	/* Open the file */
	fff = my_fopen(buf, "r");

	if (fff)
	{
		/* Allocate list of pointers */
		C_MAKE(lines_list, MAX_LINES, cptr);

		/* Parse it */
		while (0 == my_fgets(fff, buf, sizeof(buf)))
		{
			lines_list[lines++] = string_make(buf);
			if (lines >= MAX_LINES - 1) break;
		}
		if (lines == 0)
			lines_list[0] = string_make("");

		my_fclose(fff);
	}

	if (!fff) return NULL;
	return lines_list;
}


#define PT_DEFAULT 0
#define PT_WITH_PNAME 1

static cptr *read_pickpref_text_lines(int *filename_mode_p)
{
	char buf[1024];
	cptr *lines_list;

#ifdef JP
	sprintf(buf, "picktype-%s.prf", player_name);
#else
	sprintf(buf, "pickpref-%s.prf", player_name);
#endif
	lines_list = read_text_lines(buf, TRUE);

	if (!lines_list)
	{
#ifdef JP
		lines_list = read_text_lines("picktype.prf", TRUE);
#else
		lines_list = read_text_lines("pickpref.prf", TRUE);
#endif
		*filename_mode_p = PT_DEFAULT;
	}

	if (!lines_list)
	{
#ifdef JP
		lines_list = read_text_lines("picktype.prf", FALSE);
#else
		lines_list = read_text_lines("pickpref.prf", FALSE);
#endif
		*filename_mode_p = PT_WITH_PNAME;
	}

	if (!lines_list)
	{
		/* Allocate list of pointers */
		C_MAKE(lines_list, MAX_LINES, cptr);
		lines_list[0] = string_make("");
		*filename_mode_p = PT_WITH_PNAME;
	}
	return lines_list;
}

/*
 * Write whole lines of memory to a file.
 */
static bool write_text_lines(cptr filename, cptr *lines_list)
{
	FILE *fff;

	int lines = 0;
	char buf[1024];

	/* Build the filename */
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, filename);
	
	/* Open the file */
	fff = my_fopen(buf, "w");
	if (fff)
	{
		for (lines = 0; lines_list[lines]; lines++)
			my_fputs(fff, lines_list[lines], 1024);

		my_fclose(fff);
	}

	if (!fff) return FALSE;
	return TRUE;
}


/*
 * Free memory of lines_list.
 */
static void free_text_lines(cptr *lines_list)
{
	int lines;

	for (lines = 0; lines_list[lines]; lines++)
		string_free(lines_list[lines]);

	/* free list of pointers */
	C_FREE((char **)lines_list, MAX_LINES, char *);
}


/*
 * Delete or insert string
 */
static void toggle_string(cptr *lines_list, int flg, int y)
{
	autopick_type an_entry, *entry = &an_entry;

	if (!autopick_new_entry(entry, lines_list[y]))
		return;

	string_free(lines_list[y]);
	if (IS_FLG(flg)) 
		REM_FLG(flg);
	else
		ADD_FLG(flg);

	lines_list[y] = autopick_line_from_entry_kill(entry);
}

/*
 * Insert return code and split the line
 */
static bool insert_return_code(cptr *lines_list, int cx, int cy)
{
	char buf[MAX_LINELEN];
	int i, j, k;

	for (k = 0; lines_list[k]; k++)
		/* count number of lines */ ;

	if (k >= MAX_LINES - 2) return FALSE;
	k--;

	/* Move down lines */
	for (; cy < k; k--)
		lines_list[k+1] = lines_list[k];

	/* Split current line */
	for (i = j = 0; lines_list[cy][i] && i < cx; i++)
	{
#ifdef JP
		if (iskanji(lines_list[cy][i]))
			buf[j++] = lines_list[cy][i++];
#endif
		buf[j++] = lines_list[cy][i];
	}
	buf[j] = '\0';
	lines_list[cy+1] = string_make(&lines_list[cy][i]);
	string_free(lines_list[cy]);
	lines_list[cy] = string_make(buf);
	return TRUE;
}


/*
 * Get auto-picker entry from o_ptr.
 */
void autopick_entry_from_object(autopick_type *entry, object_type *o_ptr)
{
	char o_name[MAX_NLEN];
	object_desc(o_name, o_ptr, FALSE, 0);

	entry->name = string_make(o_name);
	entry->insc = string_make(quark_str(o_ptr->inscription));
	entry->action = DO_AUTOPICK | DO_DISPLAY;
	entry->flag[0] = entry->flag[1] = 0L;
	entry->dice = 0;

	if (!object_aware_p(o_ptr))
		ADD_FLG(FLG_UNAWARE);
	if (object_value(o_ptr) <= 0)
		ADD_FLG(FLG_WORTHLESS);

	if (object_known_p(o_ptr))
	{
		if (o_ptr->name2)
			ADD_FLG(FLG_EGO);
		else if (o_ptr->name1 || o_ptr->art_name)
			ADD_FLG(FLG_ARTIFACT);
	}

	switch(o_ptr->tval)
	{
		object_kind *k_ptr; 
	case TV_HAFTED: case TV_POLEARM: case TV_SWORD: case TV_DIGGING:
		k_ptr = &k_info[o_ptr->k_idx];
		if ((o_ptr->dd != k_ptr->dd) || (o_ptr->ds != k_ptr->ds))
			ADD_FLG(FLG_BOOSTED);
	}

	if (o_ptr->tval == TV_CORPSE && object_is_shoukinkubi(o_ptr))
	{
		REM_FLG(FLG_WORTHLESS);
		ADD_FLG(FLG_WANTED);
	}

	if ((o_ptr->tval == TV_CORPSE || o_ptr->tval == TV_STATUE)
	    && (r_info[o_ptr->pval].flags1 & RF1_UNIQUE))
	{
		REM_FLG(FLG_WORTHLESS);
		ADD_FLG(FLG_UNIQUE);
	}

	if (o_ptr->tval == TV_CORPSE && strchr("pht", r_info[o_ptr->pval].d_char))
	{
		REM_FLG(FLG_WORTHLESS);
		ADD_FLG(FLG_HUMAN);
	}

	if (o_ptr->tval >= TV_LIFE_BOOK &&
	    !check_book_realm(o_ptr->tval, o_ptr->sval))
		ADD_FLG(FLG_UNREADABLE);

	if (REALM1_BOOK == o_ptr->tval &&
	    p_ptr->pclass != CLASS_SORCERER &&
	    p_ptr->pclass != CLASS_RED_MAGE)
		ADD_FLG(FLG_REALM1);

	if (REALM2_BOOK == o_ptr->tval &&
	    p_ptr->pclass != CLASS_SORCERER &&
	    p_ptr->pclass != CLASS_RED_MAGE)
		ADD_FLG(FLG_REALM2);

	if (o_ptr->tval >= TV_LIFE_BOOK && 0 == o_ptr->sval)
		ADD_FLG(FLG_FIRST);
	if (o_ptr->tval >= TV_LIFE_BOOK && 1 == o_ptr->sval)
		ADD_FLG(FLG_SECOND);
	if (o_ptr->tval >= TV_LIFE_BOOK && 2 == o_ptr->sval)
		ADD_FLG(FLG_THIRD);
	if (o_ptr->tval >= TV_LIFE_BOOK && 3 == o_ptr->sval)
		ADD_FLG(FLG_FOURTH);

	if (o_ptr->tval == TV_SHOT || o_ptr->tval == TV_BOLT
		 || o_ptr->tval == TV_ARROW)
		ADD_FLG(FLG_MISSILES);
	else if (o_ptr->tval == TV_SCROLL || o_ptr->tval == TV_STAFF
		 || o_ptr->tval == TV_WAND || o_ptr->tval == TV_ROD)
		ADD_FLG(FLG_DEVICES);
	else if (o_ptr->tval == TV_LITE)
		ADD_FLG(FLG_LIGHTS);
	else if (o_ptr->tval == TV_SKELETON || o_ptr->tval == TV_BOTTLE
		 || o_ptr->tval == TV_JUNK || o_ptr->tval == TV_STATUE)
		ADD_FLG(FLG_JUNKS);
	else if (o_ptr->tval >= TV_LIFE_BOOK)
		ADD_FLG(FLG_SPELLBOOKS);
	else if (o_ptr->tval == TV_HAFTED)
		ADD_FLG(FLG_HAFTED);
	else if (o_ptr->tval == TV_POLEARM || o_ptr->tval == TV_SWORD
		 || o_ptr->tval == TV_DIGGING)
		ADD_FLG(FLG_WEAPONS);
	else if (o_ptr->tval == TV_SHIELD)
		ADD_FLG(FLG_SHIELDS);
	else if (o_ptr->tval == TV_BOW)
		ADD_FLG(FLG_BOWS);
	else if (o_ptr->tval == TV_RING)
		ADD_FLG(FLG_RINGS);
	else if (o_ptr->tval == TV_AMULET)
		ADD_FLG(FLG_AMULETS);
	else if (o_ptr->tval == TV_DRAG_ARMOR || o_ptr->tval == TV_HARD_ARMOR ||
		 o_ptr->tval == TV_SOFT_ARMOR)
		ADD_FLG(FLG_SUITS);
	else if (o_ptr->tval == TV_CLOAK)
		ADD_FLG(FLG_CLOAKS);
	else if (o_ptr->tval == TV_HELM)
		ADD_FLG(FLG_HELMS);
	else if (o_ptr->tval == TV_GLOVES)
		ADD_FLG(FLG_GLOVES);
	else if (o_ptr->tval == TV_BOOTS)
		ADD_FLG(FLG_BOOTS);

	return;
}


/*
 * Choose an item and get auto-picker entry from it.
 */
static object_type *choose_object(cptr q, cptr s)
{
	int item;

	if (!get_item(&item, q, s, (USE_INVEN | USE_FLOOR | USE_EQUIP))) return NULL;

	/* Get the item (in the pack) */
	if (item >= 0) return &inventory[item];

	/* Get the item (on the floor) */
	else return &o_list[0 - item];
}


/*
 * Choose an item and get auto-picker entry from it.
 */
static bool entry_from_choosed_object(autopick_type *entry)
{
	object_type *o_ptr;
	cptr q, s;

	/* Get an item */
#ifdef JP
	q = "どのアイテムを登録しますか? ";
	s = "アイテムを持っていない。";
#else
	q = "Enter which item? ";
	s = "You have nothing to enter.";
#endif
	o_ptr = choose_object(q, s);
	if (!o_ptr) return FALSE;

	autopick_entry_from_object(entry, o_ptr);
	return TRUE;
}


/*
 * Choose an item or string for search
 */
static bool get_string_for_search(object_type **o_handle, cptr *search_strp)
{
	int pos = 0;
	cptr q, s;
	char buf[MAX_NLEN+20];

#ifdef JP
	int k_flag[MAX_NLEN+20];
	char prompt[] = "検索(^I:持ち物 ^L:破壊された物): ";
#else
	char prompt[] = "Search key(^I:inven ^L:destroyed): ";
#endif
	int col = sizeof(prompt) - 1;

	if (*search_strp) strcpy(buf, *search_strp);
	else buf[0] = '\0';

	/* Display prompt */
	prt(prompt, 0, 0);

	/* Display the default answer */
	Term_erase(col, 0, 255);
	Term_putstr(col, 0, -1, TERM_YELLOW, buf);

	/* Process input */
	while (1)
	{
		object_type *o_ptr;
		int i;

		/* Place cursor */
		Term_gotoxy(col + pos, 0);

		/* Do not process macros except special keys */
		inkey_special = TRUE;

		/* Get a key */
		i = inkey();

		/* Analyze the key */
		switch (i)
		{
		case ESCAPE:
			pos = 0;
			return FALSE;

		case '\n':
		case '\r':
			if (!pos && *o_handle) return TRUE;
			string_free(*search_strp);
			*search_strp = string_make(buf);
			*o_handle = NULL;
			return TRUE;

		case KTRL('i'):
			/* Get an item */
#ifdef JP
			q = "どのアイテムを検索しますか? ";
			s = "アイテムを持っていない。";
#else
			q = "Enter which item? ";
			s = "You have nothing to enter.";
#endif
			o_ptr = choose_object(q, s);
			if (!o_ptr) return FALSE;

			*o_handle = o_ptr;

			string_free(*search_strp);
			object_desc(buf, *o_handle, FALSE, 3);
			*search_strp = string_make(format("<%s>", buf));
			return TRUE;

		case KTRL('l'):
			if (!autopick_last_destroyed_object.k_idx) break;
			*o_handle = &autopick_last_destroyed_object;

			string_free(*search_strp);
			object_desc(buf, *o_handle, FALSE, 3);
			*search_strp = string_make(format("<%s>", buf));
			return TRUE;

		case 0x7F:
		case '\010':
#ifdef JP
			if (pos > 0)
			{
				pos--;
				if (k_flag[pos]) pos--;
			}
#else
			if (pos > 0) pos--;
#endif
			break;

		default:
#ifdef JP
			if (iskanji(i))
			{
				int next;

				inkey_base = TRUE;
				next = inkey ();
				if (pos + 1 < MAX_NLEN)
				{
					buf[pos++] = i;
					buf[pos] = next;
					k_flag[pos++] = 1;
				}
				else bell();
			}
			else if (pos < MAX_NLEN && isprint(i))
			{
				buf[pos] = i;
				k_flag[pos++] = 0;
			}
			else bell();
#else
			if (pos < MAX_NLEN && isprint(i)) buf[pos++] = i;
			else bell();
#endif
			break;
		}

		/* Terminate */
		buf[pos] = '\0';

		/* Update the entry */
		Term_erase(col, 0, 255);
		Term_putstr(col, 0, -1, TERM_WHITE, buf);
	}

	/* Never reached */
}


/*
 * Search next line matches for o_ptr
 */
static bool search_for_object(cptr *lines_list, object_type *o_ptr, int *cxp, int *cyp, bool forward)
{
	int i;
	autopick_type an_entry, *entry = &an_entry;
	char o_name[MAX_NLEN];

	object_desc(o_name, o_ptr, FALSE, 3);

	/* Force to be lower case string */
	for (i = 0; o_name[i]; i++)
	{
#ifdef JP
		if (iskanji(o_name[i]))
			i++;
		else
#endif
		if (isupper(o_name[i]))
			o_name[i] = tolower(o_name[i]);
	}
	
	i = *cyp;

	while (1)
	{
		if (forward)
		{
			if (!lines_list[++i]) break;
		}
		else
		{
			if (--i < 0) break;
		}

		if (!autopick_new_entry(entry, lines_list[i])) continue;

		if (is_autopick_aux(o_ptr, entry, o_name))
		{
			*cxp = 0;
			*cyp = i;
			return TRUE;
		}
	}

	return FALSE;
}


/*
 * Search next line matches to the string
 */
static bool search_for_string(cptr *lines_list, cptr search_str, int *cxp, int *cyp, bool forward)
{
	int i = *cyp;

	while (1)
	{
		cptr pos;

		if (forward)
		{
			if (!lines_list[++i]) break;
		}
		else
		{
			if (--i < 0) break;
		}
#ifdef JP
		pos = strstr_j(lines_list[i], search_str);
#else
		pos = strstr(lines_list[i], search_str);
#endif
		if (pos)
		{
			*cxp = (int)(pos - lines_list[i]);
			*cyp = i;
			return TRUE;
		}
	}

	return FALSE;
}


/*
 * Initialize auto-picker preference
 */
void init_autopicker(void)
{
	static const char easy_autopick_inscription[] = "(:=g";
	autopick_type entry;
	int i;

	/* Clear old entries */
	for( i = 0; i < max_autopick; i++)
		autopick_free_entry(&autopick_list[i]);

	max_autopick = 0;

	/* There is always one entry "=g" */
	autopick_new_entry(&entry, easy_autopick_inscription);
	autopick_list[max_autopick++] = entry;
}



/*
 *  Process line for auto picker/destroyer.
 */
errr process_pickpref_file_line(char *buf)
{
	autopick_type entry;
	int i;

	if (max_autopick == MAX_AUTOPICK)
		return 1;
	
	/* Nuke illegal char */
	for(i = 0; buf[i]; i++)
	{
#ifdef JP
		if (iskanji(buf[i]))
		{
			i++;
			continue;
		}
#endif
		if (isspace(buf[i]) && buf[i] != ' ')
			break;
	}
	buf[i] = 0;
	
	if (!autopick_new_entry(&entry, buf)) return 0;

	/* Already has the same entry? */ 
	for(i = 0; i < max_autopick; i++)
		if(!strcmp(entry.name, autopick_list[i].name)
		   && entry.flag[0] == autopick_list[i].flag[0]
		   && entry.flag[1] == autopick_list[i].flag[1]
		   && entry.dice == autopick_list[i].dice
		   && entry.bonus == autopick_list[i].bonus) return 0;

	autopick_list[max_autopick++] = entry;
	return 0;
}


/*
 * Get a trigger key and insert ASCII string for the trigger
 */
static bool insert_macro_line(cptr *lines_list, int cy)
{
	char tmp[1024];
	char buf[1024];
	int i, n = 0;

	/* Flush */
	flush();

	/* Do not process macros */
	inkey_base = TRUE;

	/* First key */
	i = inkey();

	/* Read the pattern */
	while (i)
	{
		/* Save the key */
		buf[n++] = i;

		/* Do not process macros */
		inkey_base = TRUE;

		/* Do not wait for keys */
		inkey_scan = TRUE;

		/* Attempt to read a key */
		i = inkey();
	}

	/* Terminate */
	buf[n] = '\0';

	/* Flush */
	flush();

	/* Convert the trigger */
	ascii_to_text(tmp, buf);

	/* Null */
	if(!tmp[0]) return FALSE;

	/* Insert preference string */
	insert_return_code(lines_list, 0, cy);
	string_free(lines_list[cy]);
	lines_list[cy] = string_make(format("P:%s", tmp));

	/* Insert blank action preference line */
	insert_return_code(lines_list, 0, cy);
	string_free(lines_list[cy]);
	lines_list[cy] = string_make("A:");

	return TRUE;
}


/*
 * Get a command key and insert ASCII string for the key
 */
static bool insert_keymap_line(cptr *lines_list, int cy)
{
	char tmp[1024];
	char buf[2];
	int mode;

	/* Roguelike */
	if (rogue_like_commands)
	{
		mode = KEYMAP_MODE_ROGUE;
	}

	/* Original */
	else
	{
		mode = KEYMAP_MODE_ORIG;
	}

	/* Flush */
	flush();

	/* Get a key */
	buf[0] = inkey();
	buf[1] = '\0';

	/* Flush */
	flush();

	/* Convert the trigger */
	ascii_to_text(tmp, buf);

	/* Null */
	if(!tmp[0]) return FALSE;

	/* Insert preference string */
	insert_return_code(lines_list, 0, cy);
	string_free(lines_list[cy]);
	lines_list[cy] = string_make(format("C:%d:%s", mode, tmp));

	/* Insert blank action preference line */
	insert_return_code(lines_list, 0, cy);
	string_free(lines_list[cy]);
	lines_list[cy] = string_make("A:");

	return TRUE;
}


/*
 * Description of control commands
 */

#define WID_DESC 31

static cptr ctrl_command_desc[] =
{
#ifdef JP
#define LAST_DESTROYED 6
	"^P ^N ^B ^F 上下左右に移動",
	"^A ^E 行の先頭、終端",
	"^Q 入力/コマンドモード切り替え",
	"^R 変更を全て取り消して元に戻す",
	"------------------------------------",
	"^I 持ち物/装備から選択",
	"^L",
	"^K カーソルから終端まで削除",
	"^Y 削除(^K)した行を挿入",
	"^C 種族、職業の条件式を挿入",
	"------------------------------------",
	"^S 変更 (!破壊/~放置/拾う)",
	"^G \"(\" 全体マップで表示しない",
	"^O \"#\" 自動刻み",
	"------------------------------------",
	"^U 未鑑定/未判明/鑑定/*鑑定*",
	"^W \"無価値の\"",
	"^X 無銘/エゴ/アーティファクト",
	"^Z \"収集中の\"",
	NULL
#else
#define LAST_DESTROYED 6
	"^P ^N ^B ^F Move Cursor",
	"^A ^E Beginning and End of Line",
	"^Q Toggle Insert/Command mode",
	"^R Revert to Original File",
	"------------------------------------",
	"^I Object in Inventry/Equipment",
	"^L",
	"^K Kill Rest of Line",
	"^Y Insert killed(^K) text",
	"^C Insert conditional expression",
	"------------------------------------",
	"^S Toggle(!Destroy/~Leave/Pick)",
	"^G \"(\" No display in the 'M'ap",
	"^O \"#\" Auto-Inscribe",
	"------------------------------------",
	"^U Toggle 'identified' state",
	"^W \"worthless\"",
	"^X Toggle nameless/ego/artifact",
	"^Z \"collecting\"",
	NULL
#endif
};


#define MAX_YANK MAX_LINELEN
#define DIRTY_ALL 0x01
#define DIRTY_COMMAND 0x02
#define DIRTY_MODE 0x04
#define DIRTY_SCREEN 0x08
#define DIRTY_NOT_FOUND 0x10
#define DIRTY_NO_SEARCH 0x20

/*
 * In-game editor of Object Auto-picker/Destoryer
 */
void do_cmd_edit_autopick(void)
{
	static int cx = 0, cy = 0;
	static int upper = 0, left = 0;

	object_type *search_o_ptr = NULL;
	cptr search_str = NULL;
	cptr last_destroyed = NULL;
	char last_destroyed_command[WID_DESC+3];
	char yank_buf[MAX_YANK];
	char classrace[80];
	autopick_type an_entry, *entry = &an_entry;
	char buf[MAX_LINELEN];
	cptr *lines_list;
	int filename_mode = PT_WITH_PNAME;

	int i, j, k, len;

	int old_upper = -1, old_left = -1;
	int old_cy = -1;
	int key = -1, old_key;
	bool repeated_clearing = FALSE;
	bool edit_mode = FALSE;

	byte dirty_flags = DIRTY_ALL | DIRTY_COMMAND | DIRTY_MODE;
	int dirty_line = -1;

	int wid, hgt, old_wid = -1, old_hgt = -1;

	static s32b old_autosave_turn = 0L;

	if (turn < old_autosave_turn)
	{
		while (old_autosave_turn > turn) old_autosave_turn -= TURNS_PER_TICK * TOWN_DAWN;
	}

	/* Autosave */
	if (turn > old_autosave_turn + 100L)
	{
		do_cmd_save_game(TRUE);
		old_autosave_turn = turn;
	}

	/* HACK -- Reset start_time to stop counting playtime while edit */
	update_playtime();

	/* Free old entries */
	init_autopicker();

	/* Command Description of the 'Last Destroyed Item' */
	if (autopick_last_destroyed_object.k_idx)
	{
		autopick_entry_from_object(entry, &autopick_last_destroyed_object);
		last_destroyed = autopick_line_from_entry_kill(entry);

		my_strcpy(last_destroyed_command, format("^L \"%s\"", last_destroyed), sizeof(last_destroyed_command));
	}
	else
	{
#ifdef JP
		strcpy(last_destroyed_command, "^L 最後に自動破壊したアイテム名");
#else
		strcpy(last_destroyed_command, "^L Last destroyed object");
#endif
	}
	ctrl_command_desc[LAST_DESTROYED] = last_destroyed_command;

	/* Conditional Expression for Class and Race */
	sprintf(classrace, "?:[AND [EQU $RACE %s] [EQU $CLASS %s]]", 
#ifdef JP
		rp_ptr->E_title, cp_ptr->E_title
#else
		rp_ptr->title, cp_ptr->title
#endif
		);

	/* Clear yank buffer */
	yank_buf[0] = '\0';

	/* Read or initialize whole text */
	lines_list = read_pickpref_text_lines(&filename_mode);

	/* Reset cursor position if needed */
	for (i = 0; i < cy; i++)
	{
		if (!lines_list[i])
		{
			cy = cx = 0;
			break;
		}
	}

	/* Save the screen */
	screen_save();

	/* Process requests until done */
	while (1)
	{
		/* Get size */
		Term_get_size(&wid, &hgt);

#ifdef JP
		/* Don't let cursor at second byte of kanji */
		for (i = 0; lines_list[cy][i]; i++)
			if (iskanji(lines_list[cy][i]))
			{
				i++;
				if (i == cx)
				{
					cx--;
					break;
				}
			}
#endif

		/* Scroll if necessary */
		if (cy < upper || upper + hgt - 4 <= cy)
			upper = cy - (hgt-4)/2;
		if (upper < 0)
			upper = 0;
		if ((cx < left + 10 && left > 0) || left + wid - WID_DESC - 5 <= cx)
			left = cx - (wid - WID_DESC)*2/3;
		if (left < 0)
			left = 0;

		/* Redraw whole window after resize */
		if (old_wid != wid || old_hgt != hgt)
			dirty_flags |= DIRTY_SCREEN;

		/* Redraw all text after scroll */
		else if (old_upper != upper || old_left != left)
			dirty_flags |= DIRTY_ALL;


		if (dirty_flags & DIRTY_SCREEN)
		{
			dirty_flags |= (DIRTY_ALL | DIRTY_COMMAND | DIRTY_MODE);

			/* Clear screen */
			Term_clear();
		}

		if (dirty_flags & DIRTY_COMMAND)
		{
			/* Display control command */
			for (i = 0; ctrl_command_desc[i]; i++)
				Term_putstr(wid - WID_DESC, i + 1, WID_DESC, TERM_WHITE, ctrl_command_desc[i]);
		}

		/* Redraw mode line */
		if (dirty_flags & DIRTY_MODE)
		{
			int sepa_length = wid - WID_DESC;

			/* Separator */
			for (i = 0; i < sepa_length; i++)
				buf[i] = '-';
			buf[i] = '\0';

			/* Mode line */
			if (edit_mode)
				strncpy(buf + sepa_length - 21, " (INSERT MODE)  ", 16);
			else
				strncpy(buf + sepa_length - 21, " (COMMAND MODE) ", 16);

			Term_putstr(0, hgt - 3, sepa_length, (byte) (edit_mode ? TERM_YELLOW : TERM_WHITE), buf);
		}
		
		/* Dump up to 20, or hgt-4, lines of messages */
		for (i = 0; i < hgt - 4; i++)
		{
			int leftcol = 0;
			cptr msg;

			/* clean or dirty? */
			if (!(dirty_flags & DIRTY_ALL) && (dirty_line != upper+i))
				continue;

			msg = lines_list[upper+i];
			if (!msg) break;

			/* Apply horizontal scroll */
			for (j = 0; *msg; msg++, j++)
			{
				if (j == left) break;
#ifdef JP
				if (j > left)
				{
					leftcol = 1;
					break;
				}
				if (iskanji(*msg))
				{
					msg++;
					j++;
				}
#endif
			}

			/* Erase line */
			Term_erase(0, i + 1, wid - WID_DESC);

			/* Dump the messages, bottom to top */
			Term_putstr(leftcol, i + 1, wid - WID_DESC - 1, TERM_WHITE, msg);
		}

		for (; i < hgt - 4; i++)
		{
			/* Erase line */
			Term_erase(0, i + 1, wid - WID_DESC);
		}

		/* Display header line */
#ifdef JP
		if (edit_mode)
			prt("^Q ESC でコマンドモードへ移行、通常の文字はそのまま入力", 0, 0);
		else
			prt("q _ で終了、hjkl2468 で移動、^Q a i で入力モード、/ n N で検索", 0, 0);
#else	
		if (edit_mode)
			prt("Press ^Q ESC to command mode, any letters to insert", 0, 0);
		else
			prt(format("Press q _ to quit, %s to move, ^Q a i to insert mode, /nN to find", rogue_like_commands ? "hjkl" : "2468"), 0, 0);
#endif
		/* Display current position */
		prt (format("(%d,%d)", cx, cy), 0, 70);

		/* Display information when updated */
		if (old_cy != cy || (dirty_flags & (DIRTY_ALL | DIRTY_NOT_FOUND | DIRTY_NO_SEARCH)) || dirty_line == cy)
		{
			/* Clear information line */
			Term_erase(0, hgt - 3 + 1, wid);
			Term_erase(0, hgt - 3 + 2, wid);

			/* Display information */
			if (dirty_flags & DIRTY_NOT_FOUND)
			{
#ifdef JP
				prt(format("パターンが見つかりません: %s", search_str), hgt - 3 + 1, 0);
#else
				prt(format("Pattern not found: %s", search_str), hgt - 3 + 1, 0);
#endif
			}
			else if (dirty_flags & DIRTY_NO_SEARCH)
			{
#ifdef JP
				prt("検索中のパターンがありません('/'で検索)。", hgt - 3 + 1, 0);
#else
				prt("No pattern to search. (Press '/' to search.)", hgt - 3 + 1, 0);
#endif
			}
			else if (lines_list[cy][0] == '#')
			{
#ifdef JP
				prt("この行はコメントです。", hgt - 3 + 1, 0);
#else
				prt("This line is a comment.", hgt - 3 + 1, 0);
#endif
			}
			else if (lines_list[cy][1] == ':')
			{
				switch(lines_list[cy][0])
				{
				case '?':
#ifdef JP
					prt("この行は条件分岐式です。", hgt - 3 + 1, 0);
#else
					prt("This line is a Conditional Expression.", hgt - 3 + 1, 0);
#endif
					break;
				case 'A':
#ifdef JP
					prt("この行はマクロの実行内容を定義します。", hgt - 3 + 1, 0);
#else
					prt("This line defines a Macro action.", hgt - 3 + 1, 0);
#endif
					break;
				case 'P':
#ifdef JP
					prt("この行はマクロのトリガー・キーを定義します。", hgt - 3 + 1, 0);
#else
					prt("This line defines a Macro trigger key.", hgt - 3 + 1, 0);
#endif
					break;
				case 'C':
#ifdef JP
					prt("この行はキー配置を定義します。", hgt - 3 + 1, 0);
#else
					prt("This line defines a Keymap.", hgt - 3 + 1, 0);
#endif
					break;
				}
			}

			/* Get description of an autopicker preference line */
			else if (autopick_new_entry(entry, lines_list[cy]))
			{
				char temp[1024];
				cptr t;

				describe_autopick(buf, entry);

				roff_to_buf(buf, 81, temp, sizeof(temp));
				t = temp;
				for (i = 0; i< 2; i++)
				{
					if(t[0] == 0)
						break; 
					else
					{
						prt(t, hgt - 3 + 1 + i, 0);
						t += strlen(t) + 1;
					}
				}
				autopick_free_entry(entry);
			}
		}

		/* Place cursor */
		Term_gotoxy(cx - left, cy - upper + 1);

		/* Now clean */
		dirty_flags = 0;
		dirty_line = -1;

		/* Save old key and location */
		old_cy = cy;
		old_key = key;
		old_upper = upper;
		old_left = left;
		old_wid = wid;
		old_hgt = hgt;

		/* Do not process macros except special keys */
		inkey_special = TRUE;

		/* Get a command */
		key = inkey();

		if (edit_mode)
		{
			if (key == ESCAPE)
			{
				edit_mode = FALSE;

				/* Mode line is now dirty */
				dirty_flags |= DIRTY_MODE;
			}

			/* Insert a character */
			else if (!iscntrl(key&0xff))
			{
				/* Save preceding string */
				for (i = j = 0; lines_list[cy][i] && i < cx; i++)
					buf[j++] = lines_list[cy][i];

				/* Add a character */
#ifdef JP
				if (iskanji(key))
				{
					int next;

					inkey_base = TRUE;
					next = inkey();
					if (j+2 < MAX_LINELEN)
					{
						buf[j++] = key;
						buf[j++] = next;
						cx += 2;
					}
					else
						bell();
				}
				else
#endif
				{
					if (j+1 < MAX_LINELEN)
						buf[j++] = key;
					cx++;
				}

				/* Add following */
				for (; lines_list[cy][i] && j + 1 < MAX_LINELEN; i++)
					buf[j++] = lines_list[cy][i];
				buf[j] = '\0';

				/* Replace current line with new line */
				string_free(lines_list[cy]);
				lines_list[cy] = string_make(buf);

				/* Move to correct collumn */
				len = strlen(lines_list[cy]);
				if (len < cx) cx = len;

				/* Now dirty */
				dirty_line = cy;
			}
		}
		else
		{
			/* Exit on 'q' */
			if (key == 'q' || key == '_') break;

			switch(key)
			{
			case 'a': case 'i':
				edit_mode = TRUE;

				/* Mode line is now dirty */
				dirty_flags |= DIRTY_MODE;
				break;
			case '~':
				if (!autopick_new_entry(entry, lines_list[cy]))
				{
					if (old_key != key) repeated_clearing = FALSE;

					/* Next line */
					if (lines_list[cy + 1]) cy++;
					cx = 0;
					break;
				}
				string_free(lines_list[cy]);

				if (old_key != key)
				{
					if (entry->action & DONT_AUTOPICK)
						repeated_clearing = TRUE;
					else
						repeated_clearing = FALSE;
				}

				entry->action &= ~DO_AUTODESTROY;
				entry->action &= ~DO_QUERY_AUTOPICK;
				if (!repeated_clearing)
				{
					entry->action &= ~DO_AUTOPICK;
					entry->action |= DONT_AUTOPICK;
				}
				else 
				{
					entry->action &= ~DONT_AUTOPICK;
					entry->action |= DO_AUTOPICK;
				}

				lines_list[cy] = autopick_line_from_entry_kill(entry);

				/* Now dirty */
				dirty_line = cy;

				/* Next line */
				if (lines_list[cy + 1]) cy++;
				cx = 0;
				break;
			case '!':
				if (!autopick_new_entry(entry, lines_list[cy]))
				{
					if (old_key != key) repeated_clearing = FALSE;

					/* Next line */
					if (lines_list[cy + 1]) cy++;
					cx = 0;
					break;
				}
				string_free(lines_list[cy]);

				if (old_key != key)
				{
					if (entry->action & DO_AUTODESTROY)
						repeated_clearing = TRUE;
					else
						repeated_clearing = FALSE;
				}

				entry->action &= ~DONT_AUTOPICK;
				entry->action &= ~DO_QUERY_AUTOPICK;
				if (!repeated_clearing)
				{
					entry->action &= ~DO_AUTOPICK;
					entry->action |= DO_AUTODESTROY;
				}
				else 
				{
					entry->action &= ~DO_AUTODESTROY;
					entry->action |= DO_AUTOPICK;
				}

				lines_list[cy] = autopick_line_from_entry_kill(entry);

				/* Now dirty */
				dirty_line = cy;

				/* Next line */
				if (lines_list[cy + 1]) cy++;
				cx = 0;
				break;
			case ';':
				if (!autopick_new_entry(entry, lines_list[cy]))
				{
					if (old_key != key) repeated_clearing = FALSE;

					/* Next line */
					if (lines_list[cy + 1]) cy++;
					cx = 0;
					break;
				}
				string_free(lines_list[cy]);

				if (old_key != key)
				{
					if (entry->action & DO_QUERY_AUTOPICK)
						repeated_clearing = TRUE;
					else
						repeated_clearing = FALSE;
				}

				entry->action &= ~DO_AUTODESTROY;
				entry->action &= ~DONT_AUTOPICK;
				if (!repeated_clearing)
				{
					entry->action &= ~DO_AUTOPICK;
					entry->action |= DO_QUERY_AUTOPICK;
				}
				else 
				{
					entry->action &= ~DO_QUERY_AUTOPICK;
					entry->action |= DO_AUTOPICK;
				}

				lines_list[cy] = autopick_line_from_entry_kill(entry);

				/* Now dirty */
				dirty_line = cy;

				/* Next line */
				if (lines_list[cy + 1]) cy++;
				cx = 0;
				break;
			case '(':
				/* Toggle display on the 'M'ap */
				if (!autopick_new_entry(entry, lines_list[cy]))
				{
					if (old_key != key) repeated_clearing = FALSE;

					/* Next line */
					if (lines_list[cy + 1]) cy++;
					cx = 0;
					break;
				}
				string_free(lines_list[cy]);

				if (old_key != key)
				{
					if (entry->action & DO_DISPLAY)
						repeated_clearing = TRUE;
					else
						repeated_clearing = FALSE;
				}

				if (!repeated_clearing)
					entry->action |= DO_DISPLAY;
				else
					entry->action &= ~DO_DISPLAY;

				lines_list[cy] = autopick_line_from_entry_kill(entry);

				/* Now dirty */
				dirty_line = cy;

				/* Next line */
				if (lines_list[cy + 1]) cy++;
				cx = 0;
				break;
			case '#':
			case '{':
				key = KTRL('o');
				break;
			case 'h': case '4':
				key = KTRL('b');
				break;
			case 'l': case '6':
				key = KTRL('f');
				break;
			case 'j': case '2':
				key = KTRL('n');
				break;
			case 'k': case '8':
				key = KTRL('p');
				break;
			case ' ':
				while (cy < upper + hgt-4 && lines_list[cy + 1])
					cy++;
				upper = cy;
				break;
			case '-': case 'b':
				while (0 < cy && upper <= cy)
					cy--;
				while (0 < upper && cy + 1 < upper + hgt - 4)
					upper--;
				break;

			case 'g':
				cy = 0;
				break;

			case 'G':
				while (lines_list[cy + 1])
					cy++;
				break;

			case 'm':
				/* Erase line */
				Term_erase(0, cy - upper + 1, wid - WID_DESC);

				/* Prompt */
#ifdef JP
				Term_putstr(0, cy - upper + 1, wid - WID_DESC - 1, TERM_YELLOW, "P:<トリガーキー>: ");
#else
				Term_putstr(0, cy - upper + 1, wid - WID_DESC - 1, TERM_YELLOW, "P:<Trigger key>: ");
#endif
				if (insert_macro_line(lines_list, cy))
				{
					/* Prepare to input action */
					cx = 2;
					edit_mode = TRUE;

					/* Now dirty */
					dirty_flags |= DIRTY_ALL;
					dirty_flags |= DIRTY_MODE;
				}

				break;

			case 'c':
				/* Erase line */
				Term_erase(0, cy - upper + 1, wid - WID_DESC);

				/* Prompt */
#ifdef JP
				Term_putstr(0, cy - upper + 1, wid - WID_DESC - 1, TERM_YELLOW, format("C:%d:<コマンドキー>: ", (rogue_like_commands ? KEYMAP_MODE_ROGUE : KEYMAP_MODE_ORIG)));
#else
				Term_putstr(0, cy - upper + 1, wid - WID_DESC - 1, TERM_YELLOW, format("C:%d:<Keypress>: ", (rogue_like_commands ? KEYMAP_MODE_ROGUE : KEYMAP_MODE_ORIG)));
#endif

				if (insert_keymap_line(lines_list, cy))
				{
					/* Prepare to input action */
					cx = 2;
					edit_mode = TRUE;

					/* Now dirty */
					dirty_flags |= DIRTY_ALL;
					dirty_flags |= DIRTY_MODE;
				}				
				break;
			case '/':
				/* Become dirty because of item/equip menu */
				dirty_flags |= DIRTY_SCREEN;

				if (!get_string_for_search(&search_o_ptr, &search_str))
					break;

				/* fall through */
			case 'n':
				if (search_o_ptr)
				{
					if (!search_for_object(lines_list, search_o_ptr, &cx, &cy, TRUE)) dirty_flags |= DIRTY_NOT_FOUND;
				}
				else if (search_str)
				{
					if (!search_for_string(lines_list, search_str, &cx, &cy, TRUE)) dirty_flags |= DIRTY_NOT_FOUND;
				}
				else
				{
					dirty_flags |= DIRTY_NO_SEARCH;
				}
				break;
			case 'N':
				if (search_o_ptr)
				{
					if (!search_for_object(lines_list, search_o_ptr, &cx, &cy, FALSE)) dirty_flags |= DIRTY_NOT_FOUND;
				}
				else if (search_str)
				{
					if (!search_for_string(lines_list, search_str, &cx, &cy, FALSE)) dirty_flags |= DIRTY_NOT_FOUND;
				}
				else
				{
					dirty_flags |= DIRTY_NO_SEARCH;
				}
				break;
			}
		}

		switch(key)
		{
		case KTRL('a'):
			/* Beginning of line */
			cx = 0;
			break;
		case KTRL('b'):
			/* Back */
			if (0 < cx)
			{
				cx--;
				len = strlen(lines_list[cy]);
				if (len < cx) cx = len;
			}
			else if (cy > 0)
			{
				cy--;
				cx = strlen(lines_list[cy]);
			}
			break;
		case KTRL('c'):
			/* Insert a conditinal expression line */
			insert_return_code(lines_list, 0, cy);
			string_free(lines_list[cy]);
			lines_list[cy] = string_make(classrace);
			cy++;
			insert_return_code(lines_list, 0, cy);
			string_free(lines_list[cy]);
			lines_list[cy] = string_make("?:1");
			cx = 0;

			/* Now dirty */
			dirty_flags |= DIRTY_ALL;
			break;
		case KTRL('e'):
			/* End of line */
			cx = strlen(lines_list[cy]);
			break;
		case KTRL('f'):
			/* Forward */
#ifdef JP
			if (iskanji(lines_list[cy][cx])) cx++;
#endif
			cx++;
			len = strlen(lines_list[cy]);
			if (len < cx)
			{
				if (lines_list[cy + 1])
				{
					cy++;
					cx = 0;
				}
				else
					cx = len;
			}
			break;
		case KTRL('g'):
			/* Toggle display on the 'M'ap */
			if (!autopick_new_entry(entry, lines_list[cy]))
				break;
			string_free(lines_list[cy]);

			if (entry->action & DO_DISPLAY)
			{
				entry->action &= ~DO_DISPLAY;
				cx++;
			}
			else
			{
				entry->action |= DO_DISPLAY;
				if (cx > 0) cx--;
			}

			lines_list[cy] = autopick_line_from_entry_kill(entry);

			/* Now dirty */
			dirty_line = cy;
			break;
		case KTRL('i'):
			/* Insert choosen item name */
			if (!entry_from_choosed_object(entry))
			{
				/* Now dirty because of item/equip menu */
				dirty_flags |= DIRTY_SCREEN;
				break;
			}

			insert_return_code(lines_list, 0, cy);
			string_free(lines_list[cy]);
			lines_list[cy] = autopick_line_from_entry_kill(entry);
			cx = 0;

			/* Now dirty because of item/equip menu */
			dirty_flags |= DIRTY_SCREEN;

			break;
		case KTRL('l'):
			/* Insert a name of last destroyed item */
			if (last_destroyed)
			{
				insert_return_code(lines_list, 0, cy);
				string_free(lines_list[cy]);
				lines_list[cy] = string_make(last_destroyed);
				cx = 0;

				/* Now dirty */
				dirty_flags |= DIRTY_ALL;
			}
			break;
		case '\n': case '\r':
			/* Split a line or insert end of line */
			insert_return_code(lines_list, cx, cy);
			cy++;
			cx = 0;

			/* Now dirty */
			dirty_flags |= DIRTY_ALL;
			break;
		case KTRL('n'):
			/* Next line */
			if (lines_list[cy + 1]) cy++;
			break;
		case KTRL('o'):
			/* Prepare to write auto-inscription text */
			if (!autopick_new_entry(entry, lines_list[cy]))
				break;
			string_free(lines_list[cy]);

			if (!entry->insc) entry->insc = string_make("");

			lines_list[cy] = autopick_line_from_entry_kill(entry);

			/* Move to collumn for auto inscription */
			for (cx = 0; lines_list[cy][cx]; cx++)
				if (lines_list[cy][cx] == '#') break;
			cx++;
			edit_mode = TRUE;

			/* Now dirty */
			dirty_line = cy;
			dirty_flags |= DIRTY_MODE;
			break;
		case KTRL('p'):
			/* Previous line */
			if (cy > 0) cy--;
			break;
		case KTRL('q'):
			/* Change mode */
			edit_mode = !edit_mode;
			
			/* Mode line is now dirty */
			dirty_flags |= DIRTY_MODE;
			break;
		case KTRL('r'):
			/* Revert to original */
#ifdef JP
			if (!get_check("全ての変更を破棄して元の状態に戻します。よろしいですか？ "))
#else
			if (!get_check("Discard all changes and revert to original file. Are you sure? "))
#endif
				break;

			free_text_lines(lines_list);
			lines_list = read_pickpref_text_lines(&filename_mode);
			dirty_flags |= DIRTY_ALL | DIRTY_MODE;
			cx = cy = 0;
			edit_mode = FALSE;
			break;
		case KTRL('s'):
			/* Rotate action; pickup/destroy/leave */
			if (!autopick_new_entry(entry, lines_list[cy]))
				break;
			string_free(lines_list[cy]);

			if (entry->action & DO_AUTOPICK)
			{
				entry->action &= ~DO_AUTOPICK;
				entry->action |= DO_AUTODESTROY;
			}
			else if (entry->action & DO_AUTODESTROY)
			{
				entry->action &= ~DO_AUTODESTROY;
				entry->action |= DONT_AUTOPICK;
			}
			else if (entry->action & DONT_AUTOPICK)
			{
				entry->action &= ~DONT_AUTOPICK;
				entry->action |= DO_AUTOPICK;
			}

			lines_list[cy] = autopick_line_from_entry_kill(entry);

			/* Now dirty */
			dirty_line = cy;

			break;
		case KTRL('t'):
			/* Nothing */
			break;
		case KTRL('u'):
			/* Rotate identify-state; identified/unidentified/... */
			if (!autopick_new_entry(entry, lines_list[cy]))
				break;
			string_free(lines_list[cy]);

			if (IS_FLG(FLG_UNIDENTIFIED)) 
			{
				REM_FLG(FLG_UNIDENTIFIED);
				ADD_FLG(FLG_UNAWARE);
				REM_FLG(FLG_IDENTIFIED);
				REM_FLG(FLG_STAR_IDENTIFIED);
			}
			else if (IS_FLG(FLG_UNAWARE)) 
			{
				REM_FLG(FLG_UNIDENTIFIED);
				REM_FLG(FLG_UNAWARE);
				ADD_FLG(FLG_IDENTIFIED);
				REM_FLG(FLG_STAR_IDENTIFIED);
			}
			else if (IS_FLG(FLG_STAR_IDENTIFIED)) 
			{
				REM_FLG(FLG_UNIDENTIFIED);
				REM_FLG(FLG_UNAWARE);
				REM_FLG(FLG_IDENTIFIED);
				REM_FLG(FLG_STAR_IDENTIFIED);
			}
			else if (IS_FLG(FLG_IDENTIFIED)) 
			{
				REM_FLG(FLG_UNIDENTIFIED);
				REM_FLG(FLG_UNAWARE);
				REM_FLG(FLG_IDENTIFIED);
				ADD_FLG(FLG_STAR_IDENTIFIED);
			}
			else
			{
				ADD_FLG(FLG_UNIDENTIFIED);
				REM_FLG(FLG_UNAWARE);
				REM_FLG(FLG_IDENTIFIED);
				REM_FLG(FLG_STAR_IDENTIFIED);
			}

			lines_list[cy] = autopick_line_from_entry_kill(entry);

			/* Now dirty */
			dirty_line = cy;
			break;
		case KTRL('v'):
			/* Scroll up */
			while (cy < upper + hgt-4 && lines_list[cy + 1])
				cy++;
			upper = cy;
			break;
		case KTRL('w'):
			/* Toggle 'worthless' */
			toggle_string(lines_list, FLG_WORTHLESS, cy);
			/* Now dirty */
			dirty_line = cy;
			break;
		case KTRL('x'):
			/* Rotate within nameless, ego, artifact */
			if (!autopick_new_entry(entry, lines_list[cy]))
				break;
			string_free(lines_list[cy]);

			if (IS_FLG(FLG_NAMELESS)) 
			{
				REM_FLG(FLG_NAMELESS);
				ADD_FLG(FLG_EGO);
				REM_FLG(FLG_ARTIFACT);
			}
			else if (IS_FLG(FLG_EGO)) 
			{
				REM_FLG(FLG_NAMELESS);
				REM_FLG(FLG_EGO);
				ADD_FLG(FLG_ARTIFACT);
			}
			else if (IS_FLG(FLG_ARTIFACT)) 
			{
				REM_FLG(FLG_NAMELESS);
				REM_FLG(FLG_EGO);
				REM_FLG(FLG_ARTIFACT);
			}
			else
			{
				ADD_FLG(FLG_NAMELESS);
				REM_FLG(FLG_EGO);
				REM_FLG(FLG_ARTIFACT);
			}

			lines_list[cy] = autopick_line_from_entry_kill(entry);

			/* Now dirty */
			dirty_line = cy;
			break;

		case KTRL('y'):
			/* Paste killed text */
			if (strlen(yank_buf))
			{
				bool ret = FALSE;

				for (j = 0; yank_buf[j]; j += strlen(yank_buf + j) + 1)
				{
					if (ret && '\n' == yank_buf[j])
					{
						ret = FALSE;
						continue;
					}

					/* Split current line */
					insert_return_code(lines_list, cx, cy);

					/* Save preceding string */
					for(i = 0; lines_list[cy][i]; i++)
						buf[i] = lines_list[cy][i];

					/* Paste yank buffer */
					if ('\n' != yank_buf[j])
					{
						int k = j;
						while (yank_buf[k] && i < MAX_LINELEN-1)
							buf[i++] = yank_buf[k++];
						ret = TRUE;
					}

					buf[i] = '\0';

					string_free(lines_list[cy]);
					lines_list[cy] = string_make(buf);

					/* Move to the beggining of next line */
					cx = 0;
					cy++;
				}

				/* Now dirty */
				dirty_flags |= DIRTY_ALL;
			}
			break;
		case KTRL('z'):
			/* Toggle 'collecting' */
			toggle_string(lines_list, FLG_COLLECTING, cy);
			/* Now dirty */
			dirty_line = cy;
			break;

		case KTRL('k'):
			/* Kill rest of line */
			if ((uint)cx > strlen(lines_list[cy]))
				cx = (int)strlen(lines_list[cy]);

			/* Save preceding string */
			for (i = 0; lines_list[cy][i] && i < cx; i++)
			{
#ifdef JP
				if (iskanji(lines_list[cy][i]))
				{
					buf[i] = lines_list[cy][i];
					i++;
				}
#endif
				buf[i] = lines_list[cy][i];
			}
			buf[i] = '\0';

			j = 0;
			if (old_key == key)
				while (yank_buf[j])
					j += strlen(yank_buf + j) + 1;

			/* Copy following to yank buffer */
			if (lines_list[cy][i])
			{
				while (lines_list[cy][i] && j < MAX_YANK - 2)
					yank_buf[j++] = lines_list[cy][i++];
				i = TRUE;
			}
			else
			{
				if (j < MAX_YANK - 2)
					yank_buf[j++] = '\n';
				i = FALSE;
			}
			yank_buf[j++] = '\0';
			yank_buf[j] = '\0';

			/* Replace current line with 'preceding string' */
			string_free(lines_list[cy]);
			lines_list[cy] = string_make(buf);

			if (i)
			{
				/* Now dirty */
				dirty_line = cy;
				break;
			}

			/* fall through */
		case KTRL('d'):
		case 0x7F:
			/* DELETE == go forward + BACK SPACE */
#ifdef JP
			if (iskanji(lines_list[cy][cx])) cx++;
#endif
			cx++;

			/* fall through */

		case '\010':
			/* BACK SPACE */
			len = strlen(lines_list[cy]);
			if (len < cx)
			{
				if (lines_list[cy + 1])
				{
					cy++;
					cx = 0;
				}
				else
				{
					cx = len;
					break;
				}
			}

			if (cx == 0)
			{
				/* delete a return code and union two lines */
				if (cy == 0) break;
				cx = strlen(lines_list[cy-1]);
				strcpy(buf, lines_list[cy-1]);
				strcat(buf, lines_list[cy]);
				string_free(lines_list[cy-1]);
				string_free(lines_list[cy]);
				lines_list[cy-1] = string_make(buf);
				for (i = cy; lines_list[i+1]; i++)
					lines_list[i] = lines_list[i+1];
				lines_list[i] = NULL;
				cy--;

				/* Now dirty */
				dirty_flags |= DIRTY_ALL;
				break;
			}

			for (i = j = k = 0; lines_list[cy][i] && i < cx; i++)
			{
				k = j;
#ifdef JP
				if (iskanji(lines_list[cy][i]))
					buf[j++] = lines_list[cy][i++];
#endif
				buf[j++] = lines_list[cy][i];
			}
			while (j > k)
			{
				cx--;
				j--;
			}
			for (; lines_list[cy][i]; i++)
				buf[j++] = lines_list[cy][i];
			buf[j] = '\0';
			string_free(lines_list[cy]);
			lines_list[cy] = string_make(buf);

			/* Now dirty */
			dirty_line = cy;
			break;
		}

	} /* while (1) */

	/* Restore the screen */
	screen_load();

	switch (filename_mode)
	{
	case PT_DEFAULT:
#ifdef JP
		strcpy(buf, "picktype.prf");
#else
		strcpy(buf, "pickpref.prf");
#endif
		break;

	case PT_WITH_PNAME:
#ifdef JP
		sprintf(buf, "picktype-%s.prf", player_name);
#else
		sprintf(buf, "pickpref-%s.prf", player_name);
#endif
		break;
	}

	write_text_lines(buf, lines_list);
	free_text_lines(lines_list);

	string_free(last_destroyed);

	/* Reload autopick pref */
	process_pickpref_file(buf);

	/* HACK -- reset start_time so that playtime is not increase while edit */
	start_time = time(NULL);
}
