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

/*
 * Macros for Keywords
 */
#define FLG_ALL             0
#define FLG_COLLECTING	    1
#define FLG_UNAWARE	    2 
#define FLG_UNIDENTIFIED    3 
#define FLG_IDENTIFIED	    4 
#define FLG_STAR_IDENTIFIED 5 
#define FLG_BOOSTED	    6 
#define FLG_MORE_THAN	    7 
#define FLG_DICE	    8 
#define FLG_MORE_BONUS	    9 
#define FLG_MORE_BONUS2	    10
#define FLG_WORTHLESS	    11
#define FLG_ARTIFACT	    12
#define FLG_EGO		    13
#define FLG_NAMELESS	    14
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
#define FLG_FAVORITE        48

#define FLG_NOUN_BEGIN      FLG_ITEMS
#define FLG_NOUN_END        FLG_FAVORITE

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
#define KEY_FAVORITE "得意武器"

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
#define KEY_FAVORITE "favorite weapons"

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
 * A function to create new entry
 */
static bool autopick_new_entry(autopick_type *entry, cptr str, bool allow_default)
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

	/* Skip empty line unless allow_default */
	if (!allow_default && *buf == 0) return FALSE;

	/* Skip comment line */
	if (*buf == 0 && insc) return FALSE;

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
	else if (MATCH_KEY2(KEY_FAVORITE)) ADD_FLG_NOUN(FLG_FAVORITE);

	/* Last 'keyword' must be at the correct location */
	if (*ptr == ':')
		ptr++;
#ifdef JP
	else if (ptr[0] == kanji_colon[0] && ptr[1] == kanji_colon[1])
		ptr += 2;
#endif
	else if (*ptr == '\0')
	{
		/* There was no noun */
		if (prev_flg == -1)

		/* Add extra word "items" */
		ADD_FLG_NOUN(FLG_ITEMS);
	}
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
static void autopick_free_entry(autopick_type *entry)
{
	string_free(entry->name);
	string_free(entry->insc);
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
	autopick_new_entry(&entry, easy_autopick_inscription, TRUE);
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
	
	if (!autopick_new_entry(&entry, buf, FALSE)) return 0;

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
	else if (IS_FLG(FLG_FAVORITE)) ADD_KEY2(KEY_FAVORITE);

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
 * Favorite weapons
 */
static bool is_favorite(object_type *o_ptr, bool others_ok)
{
	/* Only weapons match */
	switch(o_ptr->tval)
	{
	case TV_BOW: case TV_HAFTED: case TV_POLEARM:
	case TV_SWORD: case TV_DIGGING:
		break;
	default: return FALSE;
	}

	/* Favorite weapons are varied depend on the class */
	switch (p_ptr->pclass)
	{
	case CLASS_PRIEST:
	{
		u32b flgs[TR_FLAG_SIZE];
		object_flags_known(o_ptr, flgs);

		if (!have_flag(flgs, TR_BLESSED) && 
		    !(o_ptr->tval == TV_HAFTED))
			return FALSE;
		break;
	}

	case CLASS_MONK:
	case CLASS_FORCETRAINER:
		/* Icky to wield? */
		if (!(s_info[p_ptr->pclass].w_max[o_ptr->tval-TV_BOW][o_ptr->sval]))
			return FALSE;
		break;

	case CLASS_BEASTMASTER:
	case CLASS_CAVALRY:
	{
		u32b flgs[TR_FLAG_SIZE];
		object_flags_known(o_ptr, flgs);

		/* Is it known to be suitable to using while riding? */
		if (!(have_flag(flgs, TR_RIDING)))
			return FALSE;

		break;
	}

	case CLASS_NINJA:
		/* Icky to wield? */
		if (s_info[p_ptr->pclass].w_max[o_ptr->tval-TV_BOW][o_ptr->sval] <= WEAPON_EXP_BEGINNER)
			return FALSE;
		break;

	default:
		/* Non-special class */
		if (others_ok) return TRUE;
		else return FALSE;
	}

	return TRUE;
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
	else if (IS_FLG(FLG_FAVORITE))
	{
		if (!is_favorite(o_ptr, TRUE))
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
		else if (p_ptr->pclass == CLASS_BEASTMASTER ||
			 p_ptr->pclass == CLASS_CAVALRY)
		{
			if (o_ptr->tval == TV_WAND &&
			    o_ptr->sval == SV_WAND_HEAL_MONSTER)
				return FALSE;
		}
	}
	
	if (o_ptr->tval == TV_GOLD) return FALSE;
	
	return TRUE;
}


/*
 * Automatically destroy an item if it is to be destroyed
 */
static object_type autopick_last_destroyed_object;

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


/********  Auto-picker/destroyer editor  **********/

#define MAX_YANK MAX_LINELEN
#define MAX_LINES 3000

#define MARK_MARK     0x01
#define MARK_BY_SHIFT 0x02

#define LSTAT_BYPASS      0x01
#define LSTAT_EXPRESSION  0x02

/* 
 * Struct for yank buffer
 */
typedef struct chain_str {
	struct chain_str *next;
	char s[1];
} chain_str_type;


/*
 * Data struct for text editor
 */
typedef struct {
	int wid, hgt;
	int cx, cy;
	int upper, left;
	int old_wid, old_hgt;
	int old_cy;
	int old_upper, old_left;
	int mx, my;
	byte mark;

	object_type *search_o_ptr;
	cptr search_str;
	cptr last_destroyed;

	chain_str_type *yank;
	bool yank_eol;

	cptr *lines_list;
	byte states[MAX_LINES];

	byte dirty_flags;
	int dirty_line;
	int filename_mode;
	int old_com_id;
} text_body_type;


/*
 * Dirty flag for text editor
 */
#define DIRTY_ALL        0x01
#define DIRTY_MODE       0x04
#define DIRTY_SCREEN     0x08
#define DIRTY_NOT_FOUND  0x10
#define DIRTY_NO_SEARCH  0x20
#define DIRTY_EXPRESSION 0x40


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
	else if (IS_FLG(FLG_FAVORITE))
		body_str = "得意武器";

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
	else if (IS_FLG(FLG_FAVORITE))
		body_str = "favorite weapons";

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
static void toggle_keyword(text_body_type *tb, int flg)
{
	int by1, by2, bx1, bx2, y;
	bool add = TRUE;
	bool fixed = FALSE;

	/* Select this line */
	if (!tb->mark)
	{
		tb->my = tb->cy;
		tb->mx = 0;
	}

	if (tb->my < tb->cy)
	{
		by1 = tb->my;
		by2 = tb->cy;
		bx1 = tb->mx;
		bx2 = tb->cx;
	}
	else
	{
		by2 = tb->my;
		by1 = tb->cy;
		bx2 = tb->mx;
		bx1 = tb->cx;
	}

	/* String on these lines are not really selected */  
	if (tb->lines_list[by1][bx1] == '\0' && by1 < tb->cy) by1++;
	if (bx2 == 0 && by1 < by2) by2--;


	/* Set/Reset flag of each line */
	for (y = by1; y <= by2; y++)
	{
		autopick_type an_entry, *entry = &an_entry;

		if (!autopick_new_entry(entry, tb->lines_list[y], !fixed)) continue;

		string_free(tb->lines_list[y]);

		if (!fixed)
		{
			/* Add? or Remove? */
			if (!IS_FLG(flg)) add = TRUE;
			else add = FALSE;

			/* No more change */
			fixed = TRUE;
		}

		/* You can use only one noun flag */
		if (FLG_NOUN_BEGIN <= flg && flg <= FLG_NOUN_END)
		{
			int i;
			for (i = FLG_NOUN_BEGIN; i <= FLG_NOUN_END; i++)
				REM_FLG(i);
		}
		
		/* You can use only one identify state flag */
		else if (FLG_UNAWARE <= flg && flg <= FLG_STAR_IDENTIFIED)
		{
			int i;
			for (i = FLG_UNAWARE; i <= FLG_STAR_IDENTIFIED; i++)
				REM_FLG(i);
		}
		
		/* You can use only one flag in artifact/ego/nameless */
		else if (FLG_ARTIFACT <= flg && flg <= FLG_NAMELESS)
		{
			int i;
			for (i = FLG_ARTIFACT; i <= FLG_NAMELESS; i++)
				REM_FLG(i);
		}
		
		if (add) ADD_FLG(flg);
		else REM_FLG(flg);
		
		tb->lines_list[y] = autopick_line_from_entry_kill(entry);
		
		/* Now dirty */
		tb->dirty_flags |= DIRTY_ALL;
	}
}


/*
 * Change command letter
 */
static void toggle_command_letter(text_body_type *tb, byte flg)
{
	autopick_type an_entry, *entry = &an_entry;
	int by1, by2, bx1, bx2, y;
	bool add = TRUE;
	bool fixed = FALSE;

	/* Select this line */
	if (!tb->mark)
	{
		tb->my = tb->cy;
		tb->mx = 0;
	}

	if (tb->my < tb->cy)
	{
		by1 = tb->my;
		by2 = tb->cy;
		bx1 = tb->mx;
		bx2 = tb->cx;
	}
	else
	{
		by2 = tb->my;
		by1 = tb->cy;
		bx2 = tb->mx;
		bx1 = tb->cx;
	}

	/* String on these lines are not really selected */  
	if (tb->lines_list[by1][bx1] == '\0' && by1 < tb->cy) by1++;
	if (bx2 == 0 && by1 < by2) by2--;

	/* Set/Reset flag of each line */
	for (y = by1; y <= by2; y++)
	{
		int wid = 0;

		if (!autopick_new_entry(entry, tb->lines_list[y], FALSE)) continue;

		string_free(tb->lines_list[y]);

		if (!fixed)
		{
			/* Add? or Remove? */
			if (!(entry->action & flg)) add = TRUE;
			else add = FALSE;

			/* No more change */
			fixed = TRUE;
		}

		/* Count number of letter (by negative number) */
		if (entry->action & DONT_AUTOPICK) wid--;
		else if (entry->action & DO_AUTODESTROY) wid--;
		else if (entry->action & DO_QUERY_AUTOPICK) wid--;
		if (!(entry->action & DO_DISPLAY)) wid--;

		/* Set/Reset the flag */
		if (flg != DO_DISPLAY)
		{
			entry->action &= ~(DO_AUTOPICK | DONT_AUTOPICK | DO_AUTODESTROY | DO_QUERY_AUTOPICK);
			if (add) entry->action |= flg;
			else entry->action |= DO_AUTOPICK;
		}
		else
		{
			entry->action &= ~(DO_DISPLAY);
			if (add) entry->action |= flg;
		}

		/* Correct cursor location */
		if (tb->cy == y)
		{
			if (entry->action & DONT_AUTOPICK) wid++;
			else if (entry->action & DO_AUTODESTROY) wid++;
			else if (entry->action & DO_QUERY_AUTOPICK) wid++;
			if (!(entry->action & DO_DISPLAY)) wid++;

			if (wid > 0) tb->cx++;
			if (wid < 0 && tb->cx > 0) tb->cx--;
		}
			
		tb->lines_list[y] = autopick_line_from_entry_kill(entry);
			
		/* Now dirty */
		tb->dirty_flags |= DIRTY_ALL;
	}
}

/*
 * Delete or insert string
 */
static void add_keyword(text_body_type *tb, int flg)
{
	int by1, by2, bx1, bx2, y;

	/* Select this line */
	if (!tb->mark)
	{
		tb->my = tb->cy;
		tb->mx = 0;
	}

	if (tb->my < tb->cy)
	{
		by1 = tb->my;
		by2 = tb->cy;
		bx1 = tb->mx;
		bx2 = tb->cx;
	}
	else
	{
		by2 = tb->my;
		by1 = tb->cy;
		bx2 = tb->mx;
		bx1 = tb->cx;
	}

	/* String on these lines are not really selected */  
	if (tb->lines_list[by1][bx1] == '\0' && by1 < tb->cy) by1++;
	if (bx2 == 0 && by1 < by2) by2--;

	/* Set/Reset flag of each line */
	for (y = by1; y <= by2; y++)
	{
		autopick_type an_entry, *entry = &an_entry;

		if (!autopick_new_entry(entry, tb->lines_list[y], FALSE)) continue;

		/* There is the flag already */
		if (IS_FLG(flg))
		{
			/* Free memory for the entry */
			autopick_free_entry(entry);
			
			continue;
		}
		
		string_free(tb->lines_list[y]);
		
		/* Remove all noun flag */
		if (FLG_NOUN_BEGIN <= flg && flg <= FLG_NOUN_END)
		{
			int i;
			for (i = FLG_NOUN_BEGIN; i <= FLG_NOUN_END; i++)
				REM_FLG(i);
		}
		
		ADD_FLG(flg);
		
		tb->lines_list[y] = autopick_line_from_entry_kill(entry);

		/* Now dirty */
		tb->dirty_flags |= DIRTY_ALL;
	}
}


/*
 * Check if this line is expression or not.
 * And update it if it is.
 */
static void check_expression_line(text_body_type *tb, int y)
{
	cptr s = tb->lines_list[y];

	if ((s[0] == '?' && s[1] == ':') ||
	    (tb->states[y] & LSTAT_BYPASS))
	{
		/* Expressions need re-evaluation */
		tb->dirty_flags |= DIRTY_EXPRESSION;
	}
}


/*
 * Insert return code and split the line
 */
static bool insert_return_code(text_body_type *tb)
{
	char buf[MAX_LINELEN];
	int i, j, k;

	for (k = 0; tb->lines_list[k]; k++)
		/* count number of lines */ ;

	if (k >= MAX_LINES - 2) return FALSE;
	k--;

	/* Move down lines */
	for (; tb->cy < k; k--)
	{
		tb->lines_list[k+1] = tb->lines_list[k];
		tb->states[k+1] = tb->states[k];
	}

	/* Split current line */
	for (i = j = 0; tb->lines_list[tb->cy][i] && i < tb->cx; i++)
	{
#ifdef JP
		if (iskanji(tb->lines_list[tb->cy][i]))
			buf[j++] = tb->lines_list[tb->cy][i++];
#endif
		buf[j++] = tb->lines_list[tb->cy][i];
	}
	buf[j] = '\0';
	tb->lines_list[tb->cy+1] = string_make(&tb->lines_list[tb->cy][i]);
	string_free(tb->lines_list[tb->cy]);
	tb->lines_list[tb->cy] = string_make(buf);

	/* Expressions need re-evaluation */
	tb->dirty_flags |= DIRTY_EXPRESSION;

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
	else if (is_favorite(o_ptr, FALSE))
		ADD_FLG(FLG_FAVORITE);
	else if (o_ptr->tval == TV_POLEARM || o_ptr->tval == TV_SWORD
		 || o_ptr->tval == TV_DIGGING || o_ptr->tval == TV_HAFTED)
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
 * Choose an item for search
 */
static byte get_object_for_search(object_type **o_handle, cptr *search_strp)
{
	char buf[MAX_NLEN+20];
	object_type *o_ptr;
	cptr q, s;

	/* Get an item */
#ifdef JP
	q = "どのアイテムを検索しますか? ";
	s = "アイテムを持っていない。";
#else
	q = "Enter which item? ";
	s = "You have nothing to enter.";
#endif
	o_ptr = choose_object(q, s);
	if (!o_ptr) return 0;

	*o_handle = o_ptr;

	string_free(*search_strp);
	object_desc(buf, *o_handle, FALSE, 3);
	*search_strp = string_make(format("<%s>", buf));
	return 1;
}


/*
 * Prepare for search by destroyed object
 */
static byte get_destroyed_object_for_search(object_type **o_handle, cptr *search_strp)
{
	char buf[MAX_NLEN+20];

	if (!autopick_last_destroyed_object.k_idx) return 0;

	*o_handle = &autopick_last_destroyed_object;

	string_free(*search_strp);
	object_desc(buf, *o_handle, FALSE, 3);
	*search_strp = string_make(format("<%s>", buf));
	return 1;
}


/*
 * Choose an item or string for search
 */
static byte get_string_for_search(object_type **o_handle, cptr *search_strp)
{
	int pos = 0;
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
		bool back = FALSE;
		int key;
		size_t trig_len;

		/* Place cursor */
		Term_gotoxy(col + pos, 0);

		/* Get a key */
		key = inkey();

		/* Count length of macro trigger which induced this key */
		trig_len = strlen(inkey_macro_trigger_string);

		/* HACK -- ignore macro defined on ASCII keys */
		if (trig_len == 1)
		{
			/* Get original key */
			key = inkey_macro_trigger_string[0];

			/* Kill further macro expansion */
			flush();
		}

		/* Delete key */
		if (key == 0x7F) key = KTRL('d');


		/* Analyze the key */
		switch (key)
		{
		case ESCAPE:
			return 0;

		case KTRL('r'):
			back = TRUE;
			/* Fall through */

		case '\n':
		case '\r':
		case KTRL('s'):
			if (!pos && *o_handle) return (back ? -1 : 1);
			string_free(*search_strp);
			*search_strp = string_make(buf);
			*o_handle = NULL;
			return (back ? -1 : 1);

		case KTRL('i'):
			/* Get an item */
			return get_object_for_search(o_handle, search_strp);

		case KTRL('l'):
			/* Prepare string for destroyed object if there is one. */
			if (get_destroyed_object_for_search(o_handle, search_strp))
				return 1;
			break;

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
			if (iskanji(key))
			{
				int next;

				inkey_base = TRUE;
				next = inkey ();
				if (pos + 1 < MAX_NLEN)
				{
					buf[pos++] = key;
					buf[pos] = next;
					k_flag[pos++] = 1;
				}
				else bell();
			}
			else if (pos < MAX_NLEN && isprint(key))
			{
				buf[pos] = key;
				k_flag[pos++] = 0;
			}
			else bell();
#else
			if (pos < MAX_NLEN && isprint(key)) buf[pos++] = key;
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
static bool search_for_object(text_body_type *tb, object_type *o_ptr, bool forward)
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
	
	i = tb->cy;

	while (1)
	{
		if (forward)
		{
			if (!tb->lines_list[++i]) break;
		}
		else
		{
			if (--i < 0) break;
		}

		/* Ignore bypassed lines */
		if (tb->states[i] & LSTAT_BYPASS) continue;

		if (!autopick_new_entry(entry, tb->lines_list[i], FALSE)) continue;

		if (is_autopick_aux(o_ptr, entry, o_name))
		{
			tb->cx = 0;
			tb->cy = i;
			return TRUE;
		}
	}

	return FALSE;
}


/*
 * Search next line matches to the string
 */
static bool search_for_string(text_body_type *tb, cptr search_str, bool forward)
{
	int i = tb->cy;

	while (1)
	{
		cptr pos;

		if (forward)
		{
			if (!tb->lines_list[++i]) break;
		}
		else
		{
			if (--i < 0) break;
		}

		/* Ignore bypassed lines */
		if (tb->states[i] & LSTAT_BYPASS) continue;

#ifdef JP
		pos = strstr_j(tb->lines_list[i], search_str);
#else
		pos = strstr(tb->lines_list[i], search_str);
#endif
		if (pos)
		{
			tb->cx = (int)(pos - tb->lines_list[i]);
			tb->cy = i;
			return TRUE;
		}
	}

	return FALSE;
}




/*
 * Editor command id's
 */
#define EC_QUIT                1 
#define EC_REVERT	       2 
#define EC_HELP                3 
#define EC_RETURN	       4        
#define EC_LEFT		       5 
#define EC_DOWN		       6 
#define EC_UP		       7 
#define EC_RIGHT	       8 
#define EC_BOL		       9 
#define EC_EOL		       10
#define EC_PGUP		       11
#define EC_PGDOWN	       12
#define EC_TOP		       13
#define EC_BOTTOM	       14
#define EC_CUT		       15
#define EC_COPY		       16
#define EC_PASTE	       17
#define EC_BLOCK	       18
#define EC_KILL_LINE	       19
#define EC_DELETE_CHAR	       20
#define EC_BACKSPACE	       21
#define EC_SEARCH_STR	       22
#define EC_SEARCH_FORW         23
#define EC_SEARCH_BACK         24
#define EC_SEARCH_OBJ	       25
#define EC_SEARCH_DESTROYED    26
#define EC_INSERT_OBJECT       27
#define EC_INSERT_DESTROYED    28
#define EC_INSERT_BLOCK	       29
#define EC_INSERT_MACRO	       30
#define EC_INSERT_KEYMAP       31
#define EC_CL_AUTOPICK	       32
#define EC_CL_DESTROY	       33
#define EC_CL_LEAVE	       34
#define EC_CL_QUERY	       35
#define EC_CL_NO_DISP	       36
#define EC_OK_COLLECTING       37
#define EC_IK_UNAWARE	       38
#define EC_IK_UNIDENTIFIED     39
#define EC_IK_IDENTIFIED       40
#define EC_IK_STAR_IDENTIFIED  41
#define EC_OK_BOOSTED	       42
#define EC_OK_MORE_THAN	       43
#define EC_OK_MORE_BONUS       44
#define EC_OK_WORTHLESS	       45
#define EC_OK_ARTIFACT	       46
#define EC_OK_EGO	       47
#define EC_OK_NAMELESS	       48
#define EC_OK_WANTED	       49
#define EC_OK_UNIQUE	       50
#define EC_OK_HUMAN	       51
#define EC_OK_UNREADABLE       52
#define EC_OK_REALM1	       53
#define EC_OK_REALM2	       54
#define EC_OK_FIRST	       55
#define EC_OK_SECOND	       56
#define EC_OK_THIRD	       57
#define EC_OK_FOURTH	       58
#define EC_KK_WEAPONS	       59
#define EC_KK_FAVORITE	       60
#define EC_KK_ARMORS	       61
#define EC_KK_MISSILES	       62
#define EC_KK_DEVICES	       63
#define EC_KK_LIGHTS	       64
#define EC_KK_JUNKS	       65
#define EC_KK_SPELLBOOKS       66
#define EC_KK_SHIELDS	       67
#define EC_KK_BOWS	       68
#define EC_KK_RINGS	       69
#define EC_KK_AMULETS	       70
#define EC_KK_SUITS	       71
#define EC_KK_CLOAKS	       72
#define EC_KK_HELMS	       73
#define EC_KK_GLOVES	       74
#define EC_KK_BOOTS	       75


/* Manu names */
#ifdef JP

#define MN_QUIT "セーブして終了" 
#define MN_REVERT "全ての変更を破棄" 
#define MN_HELP "ヘルプ" 

#define MN_MOVE "カーソル移動" 
#define MN_LEFT "左" 
#define MN_DOWN "下" 
#define MN_UP "上" 
#define MN_RIGHT "右" 
#define MN_BOL "行の先頭" 
#define MN_EOL "行の終端" 
#define MN_PGUP "上のページ" 
#define MN_PGDOWN "下のページ" 
#define MN_TOP "1行目へ移動" 
#define MN_BOTTOM "最下行へ移動" 

#define MN_EDIT "編集" 
#define MN_CUT "カット" 
#define MN_COPY "コピー" 
#define MN_PASTE "ペースト" 
#define MN_BLOCK "選択範囲の指定" 
#define MN_KILL_LINE "行の残りを削除" 
#define MN_DELETE_CHAR "1文字削除" 
#define MN_BACKSPACE "バックスペース" 
#define MN_RETURN "改行" 
#define MN_RETURN "改行" 

#define MN_SEARCH "検索" 
#define MN_SEARCH_STR "文字列で検索" 
#define MN_SEARCH_FORW "前方へ再検索" 
#define MN_SEARCH_BACK "後方へ再検索" 
#define MN_SEARCH_OBJ "アイテムを選択して検索" 
#define MN_SEARCH_DESTROYED "自動破壊されたアイテムで検索" 

#define MN_INSERT "色々挿入" 
#define MN_INSERT_OBJECT "選択したアイテムの名前を挿入" 
#define MN_INSERT_DESTROYED "自動破壊されたアイテムの名前を挿入" 
#define MN_INSERT_BLOCK "条件分岐ブロックの例を挿入" 
#define MN_INSERT_MACRO "マクロ定義を挿入" 
#define MN_INSERT_KEYMAP "キーマップ定義を挿入" 

#define MN_COMMAND_LETTER "拾い/破壊/放置の選択" 
#define MN_CL_AUTOPICK "「 」 (自動拾い)" 
#define MN_CL_DESTROY "「!」 (自動破壊)" 
#define MN_CL_LEAVE "「~」 (放置)" 
#define MN_CL_QUERY "「;」 (確認して拾う)" 
#define MN_CL_NO_DISP "「(」 (マップコマンドで表示しない)" 

#define MN_ADJECTIVE_GEN "形容詞(一般)の選択" 

#define MN_ADJECTIVE_SPECIAL "形容詞(特殊)の選択" 
#define MN_BOOSTED "ダイス目の違う (武器)" 
#define MN_MORE_THAN "ダイス目 # 以上の (武器)" 
#define MN_MORE_BONUS "修正値 # 以上の (指輪等)" 
#define MN_ARTIFACT "アーティファクト (装備)" 
#define MN_EGO "エゴ (装備)" 
#define MN_NAMELESS "無銘の (装備)" 
#define MN_WANTED "賞金首の (死体)" 
#define MN_UNIQUE "ユニーク・モンスターの (死体)" 
#define MN_HUMAN "人間の (死体)" 
#define MN_UNREADABLE "読めない (魔法書)" 
#define MN_REALM1 "第一領域の (魔法書)" 
#define MN_REALM2 "第二領域の (魔法書)" 
#define MN_FIRST "1冊目の (魔法書)" 
#define MN_SECOND "2冊目の (魔法書)" 
#define MN_THIRD "3冊目の (魔法書)" 
#define MN_FOURTH "4冊目の (魔法書)" 

#define MN_NOUN "名詞の選択" 

#else

#define MN_QUIT "Save & Quit" 
#define MN_REVERT "Revert all changes" 
#define MN_HELP "Help" 

#define MN_MOVE "Move cursor" 
#define MN_LEFT "Left" 
#define MN_DOWN "Down" 
#define MN_UP "Up" 
#define MN_RIGHT "Right" 
#define MN_BOL "Beggining of line" 
#define MN_EOL "End of line" 
#define MN_PGUP "Page up" 
#define MN_PGDOWN "Page down" 
#define MN_TOP "Top" 
#define MN_BOTTOM "Bottom" 

#define MN_EDIT "Edit" 
#define MN_CUT "Cut" 
#define MN_COPY "Copy" 
#define MN_PASTE "Paste" 
#define MN_BLOCK "Select block" 
#define MN_KILL_LINE "Kill rest of line" 
#define MN_DELETE_CHAR "Delete character" 
#define MN_BACKSPACE "Backspace" 
#define MN_RETURN "Return" 
#define MN_RETURN "Return" 

#define MN_SEARCH "Search" 
#define MN_SEARCH_STR "Search by string" 
#define MN_SEARCH_FORW "Search forward" 
#define MN_SEARCH_BACK "Search backward" 
#define MN_SEARCH_OBJ "Search by inventory object" 
#define MN_SEARCH_DESTROYED "Search by destroyed object" 

#define MN_INSERT "Insert..." 
#define MN_INSERT_OBJECT "Insert name of choosen object" 
#define MN_INSERT_DESTROYED "Insert name of destroyed object" 
#define MN_INSERT_BLOCK "Insert conditional block" 
#define MN_INSERT_MACRO "Insert a macro definition" 
#define MN_INSERT_KEYMAP "Insert a keymap definition" 

#define MN_COMMAND_LETTER "Command letter" 
#define MN_CL_AUTOPICK "' ' (Auto pick)" 
#define MN_CL_DESTROY "'!' (Auto destroy)" 
#define MN_CL_LEAVE "'~' (Leave it on the floor)" 
#define MN_CL_QUERY "';' (Query to pick up)" 
#define MN_CL_NO_DISP "'(' (No display on the large map)" 

#define MN_ADJECTIVE_GEN "Adjective (general)" 

#define MN_ADJECTIVE_SPECIAL "Adjective (special)" 
#define MN_BOOSTED "dice boosted (weapons)" 
#define MN_MORE_THAN "more than # dice (weapons)" 
#define MN_MORE_BONUS "more bonus than # (rings etc.)" 
#define MN_ARTIFACT "artifact (equipments)" 
#define MN_EGO "ego (equipments)" 
#define MN_NAMELESS "nameless (equipments)" 
#define MN_WANTED "wanted (corpse)" 
#define MN_UNIQUE "unique (corpse)" 
#define MN_HUMAN "human (corpse)" 
#define MN_UNREADABLE "unreadable (spellbooks)" 
#define MN_REALM1 "realm1 (spellbooks)" 
#define MN_REALM2 "realm2 (spellbooks)" 
#define MN_FIRST "first (spellbooks)" 
#define MN_SECOND "second (spellbooks)" 
#define MN_THIRD "third (spellbooks)" 
#define MN_FOURTH "fourth (spellbooks)" 

#define MN_NOUN "Keywords (noun)" 

#endif


typedef struct {
	cptr name;
	int level;
	int key;
	int com_id;
} command_menu_type;


command_menu_type menu_data[] =
{
	{MN_QUIT, 0, KTRL('q'), EC_QUIT}, 
	{MN_REVERT, 0, KTRL('z'), EC_REVERT},
	{MN_HELP, 0, -1, EC_HELP},

	{MN_MOVE, 0, -1, -1},
	{MN_LEFT, 1, KTRL('b'), EC_LEFT},
	{MN_DOWN, 1, KTRL('n'), EC_DOWN},
	{MN_UP, 1, KTRL('p'), EC_UP},
	{MN_RIGHT, 1, KTRL('f'), EC_RIGHT},
	{MN_BOL, 1, KTRL('a'), EC_BOL},
	{MN_EOL, 1, KTRL('e'), EC_EOL},
	{MN_PGUP, 1, KTRL('o'), EC_PGUP},
	{MN_PGDOWN, 1, KTRL('l'), EC_PGDOWN},
	{MN_TOP, 1, KTRL('y'), EC_TOP},
	{MN_BOTTOM, 1, KTRL('u'), EC_BOTTOM},

	{MN_EDIT, 0, -1, -1},
	{MN_CUT, 1, KTRL('x'), EC_CUT},
	{MN_COPY, 1, KTRL('c'), EC_COPY},
	{MN_PASTE, 1, KTRL('v'), EC_PASTE},
	{MN_BLOCK, 1, KTRL('g'), EC_BLOCK},
	{MN_KILL_LINE, 1, KTRL('k'), EC_KILL_LINE},
	{MN_DELETE_CHAR, 1, KTRL('d'), EC_DELETE_CHAR},
	{MN_BACKSPACE, 1, KTRL('h'), EC_BACKSPACE},
	{MN_RETURN, 1, KTRL('j'), EC_RETURN},
	{MN_RETURN, 1, KTRL('m'), EC_RETURN},

	{MN_SEARCH, 0, -1, -1},
	{MN_SEARCH_STR, 1, KTRL('s'), EC_SEARCH_STR},
	{MN_SEARCH_FORW, 1, -1, EC_SEARCH_FORW},
	{MN_SEARCH_BACK, 1, KTRL('r'), EC_SEARCH_BACK},
	{MN_SEARCH_OBJ, 1, -1, EC_SEARCH_OBJ},
	{MN_SEARCH_DESTROYED, 1, -1, EC_SEARCH_DESTROYED},

	{MN_INSERT, 0, -1, -1},
	{MN_INSERT_OBJECT, 1, KTRL('i'), EC_INSERT_OBJECT},
	{MN_INSERT_DESTROYED, 1, -1, EC_INSERT_DESTROYED},
	{MN_INSERT_BLOCK, 1, -1, EC_INSERT_BLOCK},
	{MN_INSERT_MACRO, 1, -1, EC_INSERT_MACRO},
	{MN_INSERT_KEYMAP, 1, -1, EC_INSERT_KEYMAP},

 	{MN_COMMAND_LETTER, 0, -1, -1},
	{MN_CL_AUTOPICK, 1, -1, EC_CL_AUTOPICK},
	{MN_CL_DESTROY, 1, -1, EC_CL_DESTROY},
	{MN_CL_LEAVE, 1, -1, EC_CL_LEAVE},
	{MN_CL_QUERY, 1, -1, EC_CL_QUERY},
	{MN_CL_NO_DISP, 1, -1, EC_CL_NO_DISP},

 	{MN_ADJECTIVE_GEN, 0, -1, -1},
	{KEY_UNAWARE, 1, -1, EC_IK_UNAWARE},
	{KEY_UNIDENTIFIED, 1, -1, EC_IK_UNIDENTIFIED},
	{KEY_IDENTIFIED, 1, -1, EC_IK_IDENTIFIED},
	{KEY_STAR_IDENTIFIED, 1, -1, EC_IK_STAR_IDENTIFIED},
	{KEY_COLLECTING, 1, -1, EC_OK_COLLECTING},
	{KEY_WORTHLESS, 1, -1, EC_OK_WORTHLESS},

 	{MN_ADJECTIVE_SPECIAL, 0, -1, -1},
	{MN_BOOSTED, 1, -1, EC_OK_BOOSTED},
	{MN_MORE_THAN, 1, -1, EC_OK_MORE_THAN},
	{MN_MORE_BONUS, 1, -1, EC_OK_MORE_BONUS},
	{MN_ARTIFACT, 1, -1, EC_OK_ARTIFACT},
	{MN_EGO, 1, -1, EC_OK_EGO},
	{MN_NAMELESS, 1, -1, EC_OK_NAMELESS},
	{MN_WANTED, 1, -1, EC_OK_WANTED},
	{MN_UNIQUE, 1, -1, EC_OK_UNIQUE},
	{MN_HUMAN, 1, -1, EC_OK_HUMAN},
	{MN_UNREADABLE, 1, -1, EC_OK_UNREADABLE},
	{MN_REALM1, 1, -1, EC_OK_REALM1},
	{MN_REALM2, 1, -1, EC_OK_REALM2},
	{MN_FIRST, 1, -1, EC_OK_FIRST},
	{MN_SECOND, 1, -1, EC_OK_SECOND},
	{MN_THIRD, 1, -1, EC_OK_THIRD},
	{MN_FOURTH, 1, -1, EC_OK_FOURTH},

 	{MN_NOUN, 0, -1, -1},
	{KEY_WEAPONS, 1, -1, EC_KK_WEAPONS},
	{KEY_FAVORITE, 1, -1, EC_KK_FAVORITE},
	{KEY_ARMORS, 1, -1, EC_KK_ARMORS},
	{KEY_MISSILES, 1, -1, EC_KK_MISSILES},
	{KEY_DEVICES, 1, -1, EC_KK_DEVICES},
	{KEY_LIGHTS, 1, -1, EC_KK_LIGHTS},
	{KEY_JUNKS, 1, -1, EC_KK_JUNKS},
	{KEY_SPELLBOOKS, 1, -1, EC_KK_SPELLBOOKS},
	{KEY_SHIELDS, 1, -1, EC_KK_SHIELDS},
	{KEY_BOWS, 1, -1, EC_KK_BOWS},
	{KEY_RINGS, 1, -1, EC_KK_RINGS},
	{KEY_AMULETS, 1, -1, EC_KK_AMULETS},
	{KEY_SUITS, 1, -1, EC_KK_SUITS},
	{KEY_CLOAKS, 1, -1, EC_KK_CLOAKS},
	{KEY_HELMS, 1, -1, EC_KK_HELMS},
	{KEY_GLOVES, 1, -1, EC_KK_GLOVES},
	{KEY_BOOTS, 1, -1, EC_KK_BOOTS},

	{NULL, -1, -1, 0}
};


/*
 * Find a command by 'key'.
 */
static int get_com_id(char key)
{
	int i;

	for (i = 0; menu_data[i].name; i++)
	{
		if (menu_data[i].key == key)
		{
			return menu_data[i].com_id;
		}
	}

	return 0;
}


/*
 * Display the menu, and get a command 
 */
static int do_command_menu(int level, int start)
{
	int i;
	int max_len = 0;
	int max_menu_wid;
	int col0 = 5 + level*7;
	int row0 = 1 + level*3;
	byte menu_key = 0;
	int menu_id_list[26];
	bool redraw = TRUE;
	char linestr[MAX_LINELEN];

	/* Get max length */
	menu_key = 0;
	for (i = start; menu_data[i].level >= level; i++)
	{
		int len;

		/* Ignore lower level sub menus */
		if (menu_data[i].level > level) continue;

		len = strlen(menu_data[i].name);
		if (len > max_len) max_len = len;

		menu_id_list[menu_key] = i;
		menu_key++;
	}

	while (menu_key < 26)
	{
		menu_id_list[menu_key] = -1;
		menu_key++;
	}

	/* Extra space for displaying menu key and command key */
	max_menu_wid = max_len + 3 + 3;

	/* Prepare box line */
	linestr[0] = '\0';
	strcat(linestr, "+");
	for (i = 0; i < max_menu_wid + 2; i++)
	{
		strcat(linestr, "-");
	}
	strcat(linestr, "+");

	while (1)
	{
		int com_id;
		char key;
		int menu_id;

		if (redraw)
		{
			int row1 = row0 + 1;

			/* Draw top line */
			Term_putstr(col0, row0, -1, TERM_WHITE, linestr);

			/* Draw menu items */
			menu_key = 0;
			for (i = start; menu_data[i].level >= level; i++)
			{
				char com_key_str[3];
				cptr str;

				/* Ignore lower level sub menus */
				if (menu_data[i].level > level) continue;

				if (menu_data[i].com_id == -1)
				{
#ifdef JP
					strcpy(com_key_str, "▼");
#else
					strcpy(com_key_str, ">");
#endif
				}
				else if (menu_data[i].key != -1)
				{
					com_key_str[0] = '^';
					com_key_str[1] = menu_data[i].key + '@';
					com_key_str[2] = '\0';
				}
				else
				{
					com_key_str[0] = '\0';
				}

				str = format("| %c) %-*s %2s | ", menu_key + 'a', max_len, menu_data[i].name, com_key_str);

				Term_putstr(col0, row1++, -1, TERM_WHITE, str);

				menu_key++;
			}

			/* Draw bottom line */
			Term_putstr(col0, row1, -1, TERM_WHITE, linestr);

			/* The menu was shown */
			redraw = FALSE;
		}
#ifdef JP
		prt(format("(a-%c) コマンド:", menu_key + 'a' - 1), 0, 0);
#else
		prt(format("(a-%c) Command:", menu_key + 'a' - 1), 0, 0);
#endif
		key = inkey();

		if (key == ESCAPE) return 0;

		if ('a' <= key && key <= 'z')
		{
			menu_id = menu_id_list[key - 'a'];

			if (menu_id >= 0)
			{
				com_id = menu_data[menu_id].com_id;

				if (com_id == -1)
				{
					com_id = do_command_menu(level + 1, menu_id + 1);

					if (com_id) return com_id;
					else redraw = TRUE;
				}
				else if (com_id)
				{
					return com_id;
				}
			}
		}

		else
		{
			com_id = get_com_id(key);
			if (com_id) return com_id;
			else continue;
		}
	}
}


static chain_str_type *new_chain_str(cptr str)
{
	chain_str_type *chain;

	size_t len = strlen(str);

	chain = (chain_str_type *)ralloc(sizeof(chain_str_type) + len * sizeof(char));

	strcpy(chain->s, str);
	chain->next = NULL;

	return chain;
}


static void kill_yank_chain(text_body_type *tb)
{
	chain_str_type *chain = tb->yank;
	tb->yank = NULL;
	tb->yank_eol = TRUE;

	while (chain)
	{
		chain_str_type *next = chain->next;
		size_t len = strlen(chain->s);

		rnfree(chain, sizeof(chain_str_type) + len * sizeof(char));

		chain = next;
	}
}


static void add_str_to_yank(text_body_type *tb, cptr str)
{
	chain_str_type *chain;

	tb->yank_eol = FALSE;

	if (NULL == tb->yank)
	{
		tb->yank = new_chain_str(str);
		return;
	}

	chain = tb->yank;

	while (1)
	{
		if (!chain->next)
		{
			chain->next = new_chain_str(str);
			return;
		}

		/* Go to next */
		chain = chain->next;
	}
}


#define DESCRIPT_HGT 3

/*
 * Draw text
 */
static void draw_text_editor(text_body_type *tb)
{
	int i;
	int by1 = -1, bx1 = -1, by2 = -1, bx2 = -1;

	/* Get size */
	Term_get_size(&tb->wid, &tb->hgt);

	/*
	 * Top line (-1), description line (-3), separator (-1)
	 *  == -5
	 */
	tb->hgt -= 2 + DESCRIPT_HGT;

#ifdef JP
	/* Don't let cursor at second byte of kanji */
	for (i = 0; tb->lines_list[tb->cy][i]; i++)
		if (iskanji(tb->lines_list[tb->cy][i]))
		{
			i++;
			if (i == tb->cx)
			{
				tb->cx--;
				break;
			}
		}
#endif

	/* Scroll if necessary */
	if (tb->cy < tb->upper || tb->upper + tb->hgt <= tb->cy)
		tb->upper = tb->cy - (tb->hgt)/2;
	if (tb->upper < 0)
		tb->upper = 0;
	if ((tb->cx < tb->left + 10 && tb->left > 0) || tb->left + tb->wid - 5 <= tb->cx)
		tb->left = tb->cx - (tb->wid)*2/3;
	if (tb->left < 0)
		tb->left = 0;

	/* Redraw whole window after resize */
	if (tb->old_wid != tb->wid || tb->old_hgt != tb->hgt)
		tb->dirty_flags |= DIRTY_SCREEN;

	/* Redraw all text after scroll */
	else if (tb->old_upper != tb->upper || tb->old_left != tb->left)
		tb->dirty_flags |= DIRTY_ALL;


	if (tb->dirty_flags & DIRTY_SCREEN)
	{
		tb->dirty_flags |= (DIRTY_ALL | DIRTY_MODE);

		/* Clear screen */
		Term_clear();
	}

	/* Redraw mode line */
	if (tb->dirty_flags & DIRTY_MODE)
	{
		char buf[MAX_LINELEN];

		int sepa_length = tb->wid;

		/* Separator */
		for (i = 0; i < sepa_length; i++)
			buf[i] = '-';
		buf[i] = '\0';

		Term_putstr(0, tb->hgt + 1, sepa_length, TERM_WHITE, buf);
	}

	if (tb->dirty_flags & DIRTY_EXPRESSION)
	{
		int y;
		byte state = 0;

		for (y = 0; tb->lines_list[y]; y++)
		{
			char f;
			cptr v;
			cptr s = tb->lines_list[y];

			/* Update this line's state */
			tb->states[y] = state;

			if (*s++ != '?') continue;
			if (*s++ != ':') continue;

			/* Parse the expr */
			v = process_pref_file_expr(&s, &f);

			/* Set flag */
			state = (streq(v, "0") ? LSTAT_BYPASS : 0);

			/* Re-update this line's state */
			tb->states[y] = state | LSTAT_EXPRESSION;
		}

		tb->dirty_flags |= DIRTY_ALL;
	}

	if (tb->mark)
	{
		int tmp_cx = tb->cx;
		int len = strlen(tb->lines_list[tb->cy]);

		/* Correct cursor location */
		if (tb->cx > len) tmp_cx = len;

		tb->dirty_flags |= DIRTY_ALL;

		if (tb->my < tb->cy ||
		    (tb->my == tb->cy && tb->mx < tmp_cx))
		{
			by1 = tb->my;
			bx1 = tb->mx;
			by2 = tb->cy;
			bx2 = tmp_cx;
		}
		else
		{
			by2 = tb->my;
			bx2 = tb->mx;
			by1 = tb->cy;
			bx1 = tmp_cx;
		}
	}

	/* Dump up to tb->hgt lines of messages */
	for (i = 0; i < tb->hgt; i++)
	{
		int j;
		int leftcol = 0;
		cptr msg;
		byte color;
		int y = tb->upper+i;

		/* clean or dirty? */
		if (!(tb->dirty_flags & DIRTY_ALL) && (tb->dirty_line != y))
			continue;

		msg = tb->lines_list[y];
		if (!msg) break;

		/* Apply horizontal scroll */
		for (j = 0; *msg; msg++, j++)
		{
			if (j == tb->left) break;
#ifdef JP
			if (j > tb->left)
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
		Term_erase(0, i + 1, tb->wid);

		/* Bypassed line will be displayed by darker color */
		if (tb->states[y] & LSTAT_BYPASS) color = TERM_SLATE;
		else color = TERM_WHITE;

		if (!tb->mark)
		{
			/* Dump the messages, bottom to top */
			Term_putstr(leftcol, i + 1, tb->wid - 1, color, msg);
		}

		else
		{
			int x0 = leftcol + tb->left;

			int sx0 = 0;
			int sx1 = 0;

			if (by1 <= y && y < by2) sx1 = strlen(msg);
			if (y == by1) sx0 = bx1;
			if (y == by2) sx1 = bx2;

			Term_gotoxy(leftcol, i + 1);
			if (x0 < sx0) Term_addstr(sx0 - x0, color, msg);
			if (x0 < sx1) Term_addstr(sx1 - sx0, TERM_YELLOW, msg + (sx0 - x0));
			Term_addstr(-1, color, msg + (sx1 - x0));
		}
	}

	for (; i < tb->hgt; i++)
	{
		/* Erase line */
		Term_erase(0, i + 1, tb->wid);
	}

	/* Display information when updated */
	if (tb->old_cy != tb->cy || (tb->dirty_flags & (DIRTY_ALL | DIRTY_NOT_FOUND | DIRTY_NO_SEARCH)) || tb->dirty_line == tb->cy)
	{
		autopick_type an_entry, *entry = &an_entry;

		/* Clear information line */
		for (i = 0; i < DESCRIPT_HGT; i++)
		{
			/* Erase line */
			Term_erase(0, tb->hgt + 2 + i, tb->wid);
		}

		/* Display information */
		if (tb->dirty_flags & DIRTY_NOT_FOUND)
		{
#ifdef JP
			prt(format("パターンが見つかりません: %s", tb->search_str), tb->hgt + 1 + 1, 0);
#else
			prt(format("Pattern not found: %s", tb->search_str), tb->hgt + 1 + 1, 0);
#endif
		}
		else if (tb->dirty_flags & DIRTY_NO_SEARCH)
		{
#ifdef JP
			prt("検索中のパターンがありません('/'で検索)。", tb->hgt + 1 + 1, 0);
#else
			prt("No pattern to search. (Press '/' to search.)", tb->hgt +1 + 1, 0);
#endif
		}
		else if (tb->lines_list[tb->cy][0] == '#')
		{
#ifdef JP
			prt("この行はコメントです。", tb->hgt +1 + 1, 0);
#else
			prt("This line is a comment.", tb->hgt +1 + 1, 0);
#endif
		}
		else if (tb->lines_list[tb->cy][1] == ':')
		{
			cptr str = NULL;

			switch(tb->lines_list[tb->cy][0])
			{
			case '?':
#ifdef JP
				str = "この行は条件分岐式です。";
#else
				str = "This line is a Conditional Expression.";
#endif

				break;
			case 'A':
#ifdef JP
				str = "この行はマクロの実行内容を定義します。";
#else
				str = "This line defines a Macro action.";
#endif
				break;
			case 'P':
#ifdef JP
				str = "この行はマクロのトリガー・キーを定義します。";
#else
				str = "This line defines a Macro trigger key.";
#endif
				break;
			case 'C':
#ifdef JP
				str = "この行はキー配置を定義します。";
#else
				str = "This line defines a Keymap.";
#endif
				break;
			}

			/* Draw the first line */
			if (str) prt(str, tb->hgt +1 + 1, 0);

			str = NULL;

			switch(tb->lines_list[tb->cy][0])
			{
			case '?':
				if (tb->states[tb->cy] & LSTAT_BYPASS)
				{
#ifdef JP
					str = "現在の式の値は「偽(=0)」です。";
#else
					str = "  The expression is 'False'(=0) currently.";
#endif
				}
				else
				{
#ifdef JP
					str = "現在の式の値は「真(=1)」です。";
#else
					str = "  The expression is 'True'(=1) currently.";
#endif
				}
				break;

			case 'A':
			case 'P':
			case 'C':
				if (tb->states[tb->cy] & LSTAT_BYPASS)
				{
#ifdef JP
					str = "この行は現在は無効な状態です。";
#else
					str = "  This line is bypassed currently.";
#endif
				}
				break;
			}

			/* Draw the second line */
			if (str) prt(str, tb->hgt +1 + 2, 0);
		}

		/* Get description of an autopicker preference line */
		else if (autopick_new_entry(entry, tb->lines_list[tb->cy], FALSE))
		{
			char buf[MAX_LINELEN];
			char temp[MAX_LINELEN];
			cptr t;

			describe_autopick(buf, entry);

			if (tb->states[tb->cy] & LSTAT_BYPASS)
			{
#ifdef JP
				strcat(buf, "この行は現在は無効な状態です。");
#else
				strcat(buf, "  This line is bypassed currently.");
#endif
			}

			roff_to_buf(buf, 81, temp, sizeof(temp));
			t = temp;
			for (i = 0; i < 3; i++)
			{
				if(t[0] == 0)
					break; 
				else
				{
					prt(t, tb->hgt +1 + 1 + i, 0);
					t += strlen(t) + 1;
				}
			}
			autopick_free_entry(entry);
		}
	}
}


/*
 * Kill segment of a line
 */
static void kill_line_segment(text_body_type *tb, int y, int x0, int x1, bool whole)
{
	char buf[MAX_LINELEN];
	cptr s = tb->lines_list[y];
	char *d = buf;
	int x;

	/* No segment? */
	if (x0 == x1) return;

	/* Kill whole line? */
	if (whole && x0 == 0 && s[x1] == '\0' && tb->lines_list[y+1])
	{
		int i;

		string_free(tb->lines_list[y]);

		/* Shift lines up */
		for (i = y; tb->lines_list[i+1]; i++)
			tb->lines_list[i] = tb->lines_list[i+1];
		tb->lines_list[i] = NULL;

		/* Expressions need re-evaluation */
		tb->dirty_flags |= DIRTY_EXPRESSION;

		return;
	}

	/* Before the segment */
	for (x = 0; x < x0; x++)
		*(d++) = s[x];

	/* After the segment */
	for (x = x1; s[x]; x++)
		*(d++) = s[x];

	*d = '\0';

	/* Replace */
	string_free(tb->lines_list[y]);
	tb->lines_list[y] = string_make(buf);

	/* Expressions may need re-evaluation */
	check_expression_line(tb, y);
}


/*
 * Get a trigger key and insert ASCII string for the trigger
 */
static bool insert_macro_line(text_body_type *tb)
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

	tb->cx = 0;

	/* Insert preference string */
	insert_return_code(tb);
	string_free(tb->lines_list[tb->cy]);
	tb->lines_list[tb->cy] = string_make(format("P:%s", tmp));

	/* Acquire action */
	i = macro_find_exact(buf);

	if (i == -1)
	{
		/* Nothing defined */
		tmp[0] = '\0';
	}
	else
	{
		/* Analyze the current action */
		ascii_to_text(tmp, macro__act[i]);
	}

	/* Insert blank action preference line */
	insert_return_code(tb);
	string_free(tb->lines_list[tb->cy]);
	tb->lines_list[tb->cy] = string_make(format("A:%s", tmp));

	return TRUE;
}


/*
 * Get a command key and insert ASCII string for the key
 */
static bool insert_keymap_line(text_body_type *tb)
{
	char tmp[1024];
	char buf[2];
	int mode;
	cptr act;

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

	tb->cx = 0;

	/* Insert preference string */
	insert_return_code(tb);
	string_free(tb->lines_list[tb->cy]);
	tb->lines_list[tb->cy] = string_make(format("C:%d:%s", mode, tmp));

	/* Look up the keymap */
	act = keymap_act[mode][(byte)(buf[0])];

	/* Insert blank action preference line */
	insert_return_code(tb);
	string_free(tb->lines_list[tb->cy]);
	tb->lines_list[tb->cy] = string_make(format("A:%s", act));

	return TRUE;
}


/*
 * Execute a single editor command
 */
static bool do_editor_command(text_body_type *tb, int com_id)
{
	switch(com_id)
	{
	case EC_QUIT:
		return TRUE;

	case EC_REVERT:
		/* Revert to original */
#ifdef JP
		if (!get_check("全ての変更を破棄して元の状態に戻します。よろしいですか？ ")) break;
#else
		if (!get_check("Discard all changes and revert to original file. Are you sure? ")) break;
#endif

		free_text_lines(tb->lines_list);
		tb->lines_list = read_pickpref_text_lines(&tb->filename_mode);
		tb->dirty_flags |= DIRTY_ALL | DIRTY_MODE | DIRTY_EXPRESSION;
		tb->cx = tb->cy = 0;
		tb->mark = 0;
		break;

	case EC_HELP:
		/* Peruse the main help file */
#ifdef JP
		(void)show_file(TRUE, "jeditor.txt", NULL, 0, 0);
#else
		(void)show_file(TRUE, "editor.txt", NULL, 0, 0);
#endif
		/* Redraw all */
		tb->dirty_flags |= DIRTY_SCREEN;

		break;

	case EC_RETURN:
		/* Split a line or insert end of line */

		/* Ignore selection */
		if (tb->mark)
		{
			tb->mark = 0;

			/* Now dirty */
			tb->dirty_flags |= DIRTY_ALL;
		}

		insert_return_code(tb);
		tb->cy++;
		tb->cx = 0;

		/* Now dirty */
		tb->dirty_flags |= DIRTY_ALL;
		break;

	case EC_LEFT:
		/* Back */
		if (0 < tb->cx)
		{
			int len;

			tb->cx--;
			len = strlen(tb->lines_list[tb->cy]);
			if (len < tb->cx) tb->cx = len;
		}
		else if (tb->cy > 0)
		{
			tb->cy--;
			tb->cx = strlen(tb->lines_list[tb->cy]);
		}
		break;

	case EC_DOWN:
		/* Next line */
		if (tb->lines_list[tb->cy + 1]) tb->cy++;
		break;

	case EC_UP:
		/* Previous line */
		if (tb->cy > 0) tb->cy--;
		break;

	case EC_RIGHT:
	{
		/* Forward */

		int len;
#ifdef JP
		if (iskanji(tb->lines_list[tb->cy][tb->cx])) tb->cx++;
#endif
		tb->cx++;
		len = strlen(tb->lines_list[tb->cy]);
		if (len < tb->cx)
		{
			if (tb->lines_list[tb->cy + 1])
			{
				tb->cy++;
				tb->cx = 0;
			}
			else
				tb->cx = len;
		}
		break;
	}

	case EC_BOL:
		/* Beginning of line */
		tb->cx = 0;
		break;

	case EC_EOL:
		/* End of line */
		tb->cx = strlen(tb->lines_list[tb->cy]);
		break;

	case EC_PGUP:
		while (0 < tb->cy && tb->upper <= tb->cy)
			tb->cy--;
		while (0 < tb->upper && tb->cy + 1 < tb->upper + tb->hgt)
			tb->upper--;
		break;

	case EC_PGDOWN:
		/* Page down */
		while (tb->cy < tb->upper + tb->hgt && tb->lines_list[tb->cy + 1])
			tb->cy++;
		tb->upper = tb->cy;
		break;

	case EC_TOP:
		tb->cy = 0;
		break;

	case EC_BOTTOM:
		while (tb->lines_list[tb->cy + 1])
			tb->cy++;
		break;

	case EC_CUT:
	{	
		int by1, bx1, by2, bx2;
		int y;

		/* Copy the text first */
		do_editor_command(tb, EC_COPY);

		if (tb->my < tb->cy ||
		    (tb->my == tb->cy && tb->mx < tb->cx))
		{
			by1 = tb->my;
			bx1 = tb->mx;
			by2 = tb->cy;
			bx2 = tb->cx;
		}
		else
		{
			by2 = tb->my;
			bx2 = tb->mx;
			by1 = tb->cy;
			bx1 = tb->cx;
		}

		/* Kill lines in reverse order */
		for (y = by2; y >= by1; y--)
		{
			int x0 = 0;
			int x1 = strlen(tb->lines_list[y]);

			if (y == by1) x0 = bx1;
			if (y == by2) x1 = bx2;

			kill_line_segment(tb, y, x0, x1, TRUE);
		}

		/* Correct cursor position */
		tb->cy = by1;
		tb->cx = bx1;

		/* Disable selection */
		tb->mark = 0;

		/* Now dirty */
		tb->dirty_flags |= DIRTY_ALL;

		break;
	}

	case EC_COPY:
	{	
		int by1, bx1, by2, bx2;
		int y;
		int len = strlen(tb->lines_list[tb->cy]);

		/* Correct cursor location */
		if (tb->cx > len) tb->cx = len;

		/* Use single line? */
		if (!tb->mark)
		{
			tb->my = tb->cy;
			tb->mx = 0;
			if (tb->lines_list[tb->cy + 1])
			{
				/* Select a single line */
				tb->cx = 0;
				tb->cy++;
			}
			else
			{
				/* Select bottom line */
				tb->cx = len;
			}
		}

		if (tb->my < tb->cy ||
		    (tb->my == tb->cy && tb->mx < tb->cx))
		{
			by1 = tb->my;
			bx1 = tb->mx;
			by2 = tb->cy;
			bx2 = tb->cx;
		}
		else
		{
			by2 = tb->my;
			bx2 = tb->mx;
			by1 = tb->cy;
			bx1 = tb->cx;
		}

		/* Kill old yank buffer */
		kill_yank_chain(tb);

		/* Copy string to yank buffer */
		for (y = by1; y <= by2; y++)
		{
			int i;
			char buf[MAX_LINELEN];

			int x0 = 0;
			int x1 = strlen(tb->lines_list[y]);

			if (y == by1) x0 = bx1;
			if (y == by2) x1 = bx2;

			for (i = 0; i < x1 - x0; i++)
			{
				buf[i] = tb->lines_list[y][x0 + i];
			}
			buf[i] = '\0';

			add_str_to_yank(tb, buf);
		}

		/* Disable selection */
		tb->mark = 0;

		/* Now dirty */
		tb->dirty_flags |= DIRTY_ALL;
		break;
	}

	case EC_PASTE:
	{
		/* Paste killed text */

		chain_str_type *chain = tb->yank;
		int len = strlen(tb->lines_list[tb->cy]);

		/* Nothing to do? */
		if (!chain) break;

		/* Correct cursor location */
		if (tb->cx > len) tb->cx = len;

		/* Ignore selection */
		if (tb->mark)
		{
			tb->mark = 0;

			/* Now dirty */
			tb->dirty_flags |= DIRTY_ALL;
		}

		/* Paste text */
		while (chain)
		{
			cptr yank_str = chain->s;

			char buf[MAX_LINELEN];
			int i;
			char rest[MAX_LINELEN], *rest_ptr = rest;

			/* Save preceding string */
			for(i = 0; i < tb->cx; i++)
				buf[i] = tb->lines_list[tb->cy][i];

			strcpy(rest, &(tb->lines_list[tb->cy][i]));

			/* Paste yank buffer */
			while (*yank_str && i < MAX_LINELEN-1)
			{
				buf[i++] = *yank_str++;
			}

			/* Terminate */
			buf[i] = '\0';

			chain = chain->next;

			if (chain || tb->yank_eol)
			{
				/* There is an end of line between chain nodes */

				insert_return_code(tb);

				/* Replace this line with new one */
				string_free(tb->lines_list[tb->cy]);
				tb->lines_list[tb->cy] = string_make(buf);

				/* Move to next line */
				tb->cx = 0;
				tb->cy++;

				continue;
			}

			/* Final line doesn't have end of line */

			tb->cx = strlen(buf);

			/* Rest of original line */
			while (*rest_ptr && i < MAX_LINELEN-1)
			{
				buf[i++] = *rest_ptr++;
			}

			/* Terminate */
			buf[i] = '\0';

			/* Replace this line with new one */
			string_free(tb->lines_list[tb->cy]);
			tb->lines_list[tb->cy] = string_make(buf);

			/* Finish */
			break;
		}

		/* Now dirty */
		tb->dirty_flags |= DIRTY_ALL;

		/* Expressions need re-evaluation */
		tb->dirty_flags |= DIRTY_EXPRESSION;

		break;
	}

	case EC_BLOCK:
		if (tb->mark)
		{
			/* Disable the selection */
			tb->mark = 0;

			/* Redraw text */
			tb->dirty_flags |= DIRTY_ALL;
		}
		else
		{
			tb->mark = MARK_MARK;

			/* Repeating this command swaps cursor position */
			if (com_id == tb->old_com_id)
			{
				int tmp;

				tmp = tb->cy;
				tb->cy = tb->my;
				tb->my = tmp;
				tmp = tb->cx;
				tb->cx = tb->mx;
				tb->mx = tmp;

				/* Redraw text */
				tb->dirty_flags |= DIRTY_ALL;
			}
			else
			{
				int len = strlen(tb->lines_list[tb->cy]);

				/* Mark the point 1 */
				tb->my = tb->cy;
				tb->mx = tb->cx;

				/* Correct cursor location */
				if (tb->cx > len) tb->mx = len;
			}
		}
		break;

	case EC_KILL_LINE:
	{
		/* Kill rest of line */

		int len = strlen(tb->lines_list[tb->cy]);

		/* Correct cursor location */
		if (tb->cx > len) tb->cx = len;

		/* Ignore selection */
		if (tb->mark)
		{
			tb->mark = 0;

			/* Now dirty */
			tb->dirty_flags |= DIRTY_ALL;
		}

		/* Append only if this command is repeated. */
		if (tb->old_com_id != com_id)
		{
			kill_yank_chain(tb);
			tb->yank = NULL;
		}

		/* Really deleted some text */
		if (tb->cx < len)
		{
			/* Add deleted string to yank buffer */
			add_str_to_yank(tb, &(tb->lines_list[tb->cy][tb->cx]));

			kill_line_segment(tb, tb->cy, tb->cx, len, FALSE);

			/* Now dirty */
			tb->dirty_line = tb->cy;

			/* Leave end of line character */
			break;
		}

		/* Cut the end of line character only */
		if (tb->yank_eol) add_str_to_yank(tb, "");

		/* Cut end of line */
		tb->yank_eol = TRUE;

		do_editor_command(tb, EC_DELETE_CHAR);
		break;
	}

	case EC_DELETE_CHAR:
		/* DELETE == go forward + BACK SPACE */

		/* Ignore selection */
		if (tb->mark)
		{
			tb->mark = 0;

			/* Now dirty */
			tb->dirty_flags |= DIRTY_ALL;
		}

#ifdef JP
		if (iskanji(tb->lines_list[tb->cy][tb->cx])) tb->cx++;
#endif
		tb->cx++;

		do_editor_command(tb, EC_BACKSPACE);
		break;

	case EC_BACKSPACE:
	{
		/* BACK SPACE */

		int len, i, j, k;
		char buf[MAX_LINELEN];

		/* Ignore selection */
		if (tb->mark)
		{
			tb->mark = 0;

			/* Now dirty */
			tb->dirty_flags |= DIRTY_ALL;
		}

		len = strlen(tb->lines_list[tb->cy]);
		if (len < tb->cx)
		{
			if (tb->lines_list[tb->cy + 1])
			{
				tb->cy++;
				tb->cx = 0;
			}
			else
			{
				tb->cx = len;
				break;
			}
		}

		if (tb->cx == 0)
		{
			/* delete a return code and union two lines */
			if (tb->cy == 0) break;
			tb->cx = strlen(tb->lines_list[tb->cy-1]);
			strcpy(buf, tb->lines_list[tb->cy-1]);
			strcat(buf, tb->lines_list[tb->cy]);
			string_free(tb->lines_list[tb->cy-1]);
			string_free(tb->lines_list[tb->cy]);
			tb->lines_list[tb->cy-1] = string_make(buf);

			for (i = tb->cy; tb->lines_list[i+1]; i++)
				tb->lines_list[i] = tb->lines_list[i+1];

			tb->lines_list[i] = NULL;
			tb->cy--;

			/* Now dirty */
			tb->dirty_flags |= DIRTY_ALL;

			/* Expressions need re-evaluation */
			tb->dirty_flags |= DIRTY_EXPRESSION;

			break;
		}

		for (i = j = k = 0; tb->lines_list[tb->cy][i] && i < tb->cx; i++)
		{
			k = j;
#ifdef JP
			if (iskanji(tb->lines_list[tb->cy][i]))
				buf[j++] = tb->lines_list[tb->cy][i++];
#endif
			buf[j++] = tb->lines_list[tb->cy][i];
		}
		while (j > k)
		{
			tb->cx--;
			j--;
		}
		for (; tb->lines_list[tb->cy][i]; i++)
			buf[j++] = tb->lines_list[tb->cy][i];
		buf[j] = '\0';
		string_free(tb->lines_list[tb->cy]);
		tb->lines_list[tb->cy] = string_make(buf);

		/* Now dirty */
		tb->dirty_line = tb->cy;

		/* Expressions may need re-evaluation */
		check_expression_line(tb, tb->cy);

		break;
	}

	case EC_SEARCH_STR:
	{
		byte search_dir;

		/* Become dirty because of item/equip menu */
		tb->dirty_flags |= DIRTY_SCREEN;

		search_dir = get_string_for_search(&tb->search_o_ptr, &tb->search_str);

		if (!search_dir) break;

		if (search_dir == 1) do_editor_command(tb, EC_SEARCH_FORW);
		else do_editor_command(tb, EC_SEARCH_BACK);
		break;
	}

	case EC_SEARCH_FORW:
		if (tb->search_o_ptr)
		{
			if (!search_for_object(tb, tb->search_o_ptr, TRUE)) tb->dirty_flags |= DIRTY_NOT_FOUND;
		}
		else if (tb->search_str)
		{
			if (!search_for_string(tb, tb->search_str, TRUE)) tb->dirty_flags |= DIRTY_NOT_FOUND;
		}
		else
		{
			tb->dirty_flags |= DIRTY_NO_SEARCH;
		}
		break;

	case EC_SEARCH_BACK:
		if (tb->search_o_ptr)
		{
			if (!search_for_object(tb, tb->search_o_ptr, FALSE)) tb->dirty_flags |= DIRTY_NOT_FOUND;
		}
		else if (tb->search_str)
		{
			if (!search_for_string(tb, tb->search_str, FALSE)) tb->dirty_flags |= DIRTY_NOT_FOUND;
		}
		else
		{
			tb->dirty_flags |= DIRTY_NO_SEARCH;
		}
		break;

	case EC_SEARCH_OBJ:
		/* Become dirty because of item/equip menu */
		tb->dirty_flags |= DIRTY_SCREEN;

		if (!get_object_for_search(&tb->search_o_ptr, &tb->search_str)) break;

		do_editor_command(tb, EC_SEARCH_FORW);
		break;

	case EC_SEARCH_DESTROYED:
		if (!get_destroyed_object_for_search(&tb->search_o_ptr, &tb->search_str)) break;

		do_editor_command(tb, EC_SEARCH_FORW);
		break;

	case EC_INSERT_OBJECT:
	{
		/* Insert choosen item name */

		autopick_type an_entry, *entry = &an_entry;

		if (!entry_from_choosed_object(entry))
		{
			/* Now dirty because of item/equip menu */
			tb->dirty_flags |= DIRTY_SCREEN;
			break;
		}

		tb->cx = 0;
		insert_return_code(tb);
		string_free(tb->lines_list[tb->cy]);
		tb->lines_list[tb->cy] = autopick_line_from_entry_kill(entry);

		/* Now dirty because of item/equip menu */
		tb->dirty_flags |= DIRTY_SCREEN;

		break;
	}

	case EC_INSERT_DESTROYED:
		/* Insert a name of last destroyed item */
		if (tb->last_destroyed)
		{
			tb->cx = 0;
			insert_return_code(tb);
			string_free(tb->lines_list[tb->cy]);
			tb->lines_list[tb->cy] = string_make(tb->last_destroyed);

			/* Now dirty */
			tb->dirty_flags |= DIRTY_ALL;
		}
		break;

	case EC_INSERT_BLOCK:
	{
		/* Insert a conditinal expression line */
		char classrace[80];

		/* Conditional Expression for Class and Race */
		sprintf(classrace, "?:[AND [EQU $RACE %s] [EQU $CLASS %s]]", 
#ifdef JP
			rp_ptr->E_title, cp_ptr->E_title
#else
			rp_ptr->title, cp_ptr->title
#endif
			);

		tb->cx = 0;
		insert_return_code(tb);
		string_free(tb->lines_list[tb->cy]);
		tb->lines_list[tb->cy] = string_make(classrace);
		tb->cy++;
		insert_return_code(tb);
		string_free(tb->lines_list[tb->cy]);
		tb->lines_list[tb->cy] = string_make("?:1");

		/* Now dirty */
		tb->dirty_flags |= DIRTY_ALL;
		break;
	}

	case EC_INSERT_MACRO:
		/* Draw_everythig (delete menu) */
		draw_text_editor(tb);

		/* Erase line */
		Term_erase(0, tb->cy - tb->upper + 1, tb->wid);

		/* Prompt */
#ifdef JP
		Term_putstr(0, tb->cy - tb->upper + 1, tb->wid - 1, TERM_YELLOW, "P:<トリガーキー>: ");
#else
		Term_putstr(0, tb->cy - tb->upper + 1, tb->wid - 1, TERM_YELLOW, "P:<Trigger key>: ");
#endif
		if (insert_macro_line(tb))
		{
			/* Prepare to input action */
			tb->cx = 2;

			/* Now dirty */
			tb->dirty_flags |= DIRTY_ALL;
			tb->dirty_flags |= DIRTY_MODE;
		}

		break;

	case EC_INSERT_KEYMAP:
		/* Draw_everythig (delete menu) */
		draw_text_editor(tb);

		/* Erase line */
		Term_erase(0, tb->cy - tb->upper + 1, tb->wid);

		/* Prompt */
#ifdef JP
		Term_putstr(0, tb->cy - tb->upper + 1, tb->wid - 1, TERM_YELLOW, format("C:%d:<コマンドキー>: ", (rogue_like_commands ? KEYMAP_MODE_ROGUE : KEYMAP_MODE_ORIG)));
#else
		Term_putstr(0, tb->cy - tb->upper + 1, tb->wid - 1, TERM_YELLOW, format("C:%d:<Keypress>: ", (rogue_like_commands ? KEYMAP_MODE_ROGUE : KEYMAP_MODE_ORIG)));
#endif

		if (insert_keymap_line(tb))
		{
			/* Prepare to input action */
			tb->cx = 2;

			/* Now dirty */
			tb->dirty_flags |= DIRTY_ALL;
			tb->dirty_flags |= DIRTY_MODE;
		}				
		break;

	case EC_CL_AUTOPICK: toggle_command_letter(tb, DO_AUTOPICK); break;
	case EC_CL_DESTROY: toggle_command_letter(tb, DO_AUTODESTROY); break;
	case EC_CL_LEAVE: toggle_command_letter(tb, DONT_AUTOPICK); break;
	case EC_CL_QUERY: toggle_command_letter(tb, DO_QUERY_AUTOPICK); break;
	case EC_CL_NO_DISP: toggle_command_letter(tb, DO_DISPLAY); break;

	case EC_IK_UNAWARE: toggle_keyword(tb, FLG_UNAWARE); break;
	case EC_IK_UNIDENTIFIED: toggle_keyword(tb, FLG_UNIDENTIFIED); break;
	case EC_IK_IDENTIFIED: toggle_keyword(tb, FLG_IDENTIFIED); break;
	case EC_IK_STAR_IDENTIFIED: toggle_keyword(tb, FLG_STAR_IDENTIFIED); break;
	case EC_KK_WEAPONS: toggle_keyword(tb, FLG_WEAPONS); break;
	case EC_KK_FAVORITE: toggle_keyword(tb, FLG_FAVORITE); break;
	case EC_KK_ARMORS: toggle_keyword(tb, FLG_ARMORS); break;
	case EC_KK_MISSILES: toggle_keyword(tb, FLG_MISSILES); break;
	case EC_KK_DEVICES: toggle_keyword(tb, FLG_DEVICES); break;
	case EC_KK_LIGHTS: toggle_keyword(tb, FLG_LIGHTS); break;
	case EC_KK_JUNKS: toggle_keyword(tb, FLG_JUNKS); break;
	case EC_KK_SPELLBOOKS: toggle_keyword(tb, FLG_SPELLBOOKS); break;
	case EC_KK_SHIELDS: toggle_keyword(tb, FLG_SHIELDS); break;
	case EC_KK_BOWS: toggle_keyword(tb, FLG_BOWS); break;
	case EC_KK_RINGS: toggle_keyword(tb, FLG_RINGS); break;
	case EC_KK_AMULETS: toggle_keyword(tb, FLG_AMULETS); break;
	case EC_KK_SUITS: toggle_keyword(tb, FLG_SUITS); break;
	case EC_KK_CLOAKS: toggle_keyword(tb, FLG_CLOAKS); break;
	case EC_KK_HELMS: toggle_keyword(tb, FLG_HELMS); break;
	case EC_KK_GLOVES: toggle_keyword(tb, FLG_GLOVES); break;
	case EC_KK_BOOTS: toggle_keyword(tb, FLG_BOOTS); break;
	case EC_OK_COLLECTING: toggle_keyword(tb, FLG_COLLECTING); break;
	case EC_OK_BOOSTED: toggle_keyword(tb, FLG_BOOSTED); break;
	case EC_OK_MORE_THAN: toggle_keyword(tb, FLG_MORE_THAN); break;
	case EC_OK_MORE_BONUS: toggle_keyword(tb, FLG_MORE_BONUS); break;
	case EC_OK_WORTHLESS: toggle_keyword(tb, FLG_WORTHLESS); break;
	case EC_OK_ARTIFACT: toggle_keyword(tb, FLG_ARTIFACT); break;
	case EC_OK_EGO: toggle_keyword(tb, FLG_EGO); break;
	case EC_OK_NAMELESS: toggle_keyword(tb, FLG_NAMELESS); break;
	case EC_OK_WANTED: toggle_keyword(tb, FLG_WANTED); break;
	case EC_OK_UNIQUE: toggle_keyword(tb, FLG_UNIQUE); break;
	case EC_OK_HUMAN: toggle_keyword(tb, FLG_HUMAN); break;
	case EC_OK_UNREADABLE:
		toggle_keyword(tb, FLG_UNREADABLE);
		add_keyword(tb, FLG_SPELLBOOKS);
		break;
	case EC_OK_REALM1:
		toggle_keyword(tb, FLG_REALM1);
		add_keyword(tb, FLG_SPELLBOOKS);
		break;
	case EC_OK_REALM2:
		toggle_keyword(tb, FLG_REALM2);
		add_keyword(tb, FLG_SPELLBOOKS);
		break;
	case EC_OK_FIRST:
		toggle_keyword(tb, FLG_FIRST);
		add_keyword(tb, FLG_SPELLBOOKS);
		break;
	case EC_OK_SECOND:
		toggle_keyword(tb, FLG_SECOND);
		add_keyword(tb, FLG_SPELLBOOKS);
		break;
	case EC_OK_THIRD:
		toggle_keyword(tb, FLG_THIRD);
		add_keyword(tb, FLG_SPELLBOOKS);
		break;
	case EC_OK_FOURTH:
		toggle_keyword(tb, FLG_FOURTH);
		add_keyword(tb, FLG_SPELLBOOKS);
		break;
	}

	/* Save old command */
	tb->old_com_id = com_id;

	return FALSE;
}


/*
 * Insert single letter at cursor position.
 */
static void insert_single_letter(text_body_type *tb, int key)
{
	int i, j, len;
	char buf[MAX_LINELEN];

	/* Save preceding string */
	for (i = j = 0; tb->lines_list[tb->cy][i] && i < tb->cx; i++)
		buf[j++] = tb->lines_list[tb->cy][i];

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
			tb->cx += 2;
		}
		else
			bell();
	}
	else
#endif
	{
		if (j+1 < MAX_LINELEN)
			buf[j++] = key;
		tb->cx++;
	}

	/* Add following */
	for (; tb->lines_list[tb->cy][i] && j + 1 < MAX_LINELEN; i++)
		buf[j++] = tb->lines_list[tb->cy][i];
	buf[j] = '\0';

	/* Replace current line with new line */
	string_free(tb->lines_list[tb->cy]);
	tb->lines_list[tb->cy] = string_make(buf);

	/* Move to correct collumn */
	len = strlen(tb->lines_list[tb->cy]);
	if (len < tb->cx) tb->cx = len;

	/* Now dirty */
	tb->dirty_line = tb->cy;

	/* Expressions may need re-evaluation */
	check_expression_line(tb, tb->cy);
}

/*
 * In-game editor of Object Auto-picker/Destoryer
 */
void do_cmd_edit_autopick(void)
{
	text_body_type text_body, *tb = &text_body;

	autopick_type an_entry, *entry = &an_entry;
	char buf[MAX_LINELEN];

	int i;
	int key = -1;

	static s32b old_autosave_turn = 0L;
	bool quit = FALSE;

	tb->cx = tb->cy = tb->upper = tb->left = 0;
	tb->mark = 0;
	tb->mx = tb->my = 0;
	tb->old_cy = tb->old_upper = tb->old_left = -1;
	tb->old_wid = tb->old_hgt = -1;
	tb->old_com_id = 0;

	tb->yank = NULL;
	tb->search_o_ptr = NULL;
	tb->search_str = NULL;
	tb->last_destroyed = NULL;
	tb->dirty_flags = DIRTY_ALL | DIRTY_MODE | DIRTY_EXPRESSION;
	tb->dirty_line = -1;
	tb->filename_mode = PT_WITH_PNAME;

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
		tb->last_destroyed = autopick_line_from_entry_kill(entry);
	}

	/* Read or initialize whole text */
	tb->lines_list = read_pickpref_text_lines(&tb->filename_mode);

	/* Reset cursor position if needed */
	for (i = 0; i < tb->cy; i++)
	{
		if (!tb->lines_list[i])
		{
			tb->cy = tb->cx = 0;
			break;
		}
	}

	/* Save the screen */
	screen_save();

	/* Process requests until done */
	while (!quit)
	{
		int com_id = 0;
		size_t trig_len;

		/* Draw_everythig */
		draw_text_editor(tb);

		/* Display header line */
#ifdef JP
		prt("(^Q:終了, ESC:メニュー, その他:入力)", 0, 0);
#else	
		prt("(^Q:quit, ESC:menu, Other:input text)", 0, 0);
#endif
		if (!tb->mark)
		{
			/* Display current position */
			prt (format("(%d,%d)", tb->cx, tb->cy), 0, 60);
		}
		else
		{
			/* Display current position and mark position */
			prt (format("(%d,%d)-(%d,%d)", tb->mx, tb->my, tb->cx, tb->cy), 0, 60);
		}

		/* Place cursor */
		Term_gotoxy(tb->cx - tb->left, tb->cy - tb->upper + 1);

		/* Now clean */
		tb->dirty_flags = 0;
		tb->dirty_line = -1;

		/* Save old key and location */
		tb->old_cy = tb->cy;
		tb->old_upper = tb->upper;
		tb->old_left = tb->left;
		tb->old_wid = tb->wid;
		tb->old_hgt = tb->hgt;

		/* Get a command */
		key = inkey();

		/* Count length of macro trigger which induced this key */
		trig_len = strlen(inkey_macro_trigger_string);

		/* HACK -- ignore macro defined on ASCII keys */
		if (trig_len == 1)
		{
			/* Get original key */
			key = inkey_macro_trigger_string[0];

			/* Kill further macro expansion */
			flush();
		}

		/* Delete key */
		if (key == 0x7F) key = KTRL('d');


		/* Cursor key macroes to direction command */
		if (trig_len > 1)
		{
			switch (key)
			{
			case '2':
				com_id = EC_DOWN;
				break;
			case '4':
				com_id = EC_LEFT;
				break;
			case '6':
				com_id = EC_RIGHT;
				break;
			case '8':
				com_id = EC_UP;
				break;
			}

			if (com_id)
			{
				/*
				 * Un-shifted cursor keys cancells
				 * selection created by shift+cursor.
				 */
				if (tb->mark & MARK_BY_SHIFT)
				{
					tb->mark = 0;

					/* Now dirty */
					tb->dirty_flags |= DIRTY_ALL;
				}
			}

			/* Mega Hack!!! Start selection with shift + cursor keys */
			else
			{
				char buf[1024];

				/* Get ascii form */
				ascii_to_text(buf, inkey_macro_trigger_string);

				if (strstr(buf, "shift-Down"))
					com_id = EC_DOWN;
				else if (strstr(buf, "shift-Left"))
					com_id = EC_LEFT;
				else if (strstr(buf, "shift-Right"))
					com_id = EC_RIGHT;
				else if (strstr(buf, "shift-Up"))
					com_id = EC_UP;

				if (com_id)
				{
					/* Kill further macro expansion */
					flush();

					/* Start selection */
					if (!tb->mark)
					{
						int len = strlen(tb->lines_list[tb->cy]);

						tb->mark = MARK_MARK | MARK_BY_SHIFT;
						tb->my = tb->cy;
						tb->mx = tb->cx;

						/* Correct cursor location */
						if (tb->cx > len) tb->mx = len;
						
						/* Need to redraw text */
						if (com_id == EC_UP || com_id == EC_DOWN)
						{
							/* Redraw all text */
							tb->dirty_flags |= DIRTY_ALL;
						}
						else
						{
							tb->dirty_line = tb->cy;
						}
					}
				}
			}
		}

		if (com_id)
		{
			/* Already done */
		}

		/* Open the menu */
		else if (key == ESCAPE)
		{
			com_id = do_command_menu(0, 0);

			/* Redraw all text later */
			tb->dirty_flags |= DIRTY_SCREEN;
		}

		/* Insert a character */
		else if (!iscntrl(key & 0xff))
		{
			/* Ignore selection */
			if (tb->mark)
			{
				tb->mark = 0;

				/* Now dirty */
				tb->dirty_flags |= DIRTY_ALL;
			}

			insert_single_letter(tb, key);

			/* Next loop */
			continue;
		}

		/* Other commands */
		else
		{
			com_id = get_com_id(key);
		}

		if (com_id) quit = do_editor_command(tb, com_id);
	} /* while (1) */

	/* Restore the screen */
	screen_load();

	switch (tb->filename_mode)
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

	write_text_lines(buf, tb->lines_list);
	free_text_lines(tb->lines_list);

	string_free(tb->last_destroyed);

	/* Destroy string chain */
	kill_yank_chain(tb);

	/* Reload autopick pref */
	process_pickpref_file(buf);

	/* HACK -- reset start_time so that playtime is not increase while edit */
	start_time = time(NULL);
}
