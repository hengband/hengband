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

/*
 * Macros for Keywords
 */
#define FLG_ALL             0
#define FLG_COLLECTING	    1
#define FLG_UNIDENTIFIED    2
#define FLG_IDENTIFIED	    3
#define FLG_STAR_IDENTIFIED 4
#define FLG_ARTIFACT	    5
#define FLG_EGO		    6
#define FLG_NAMELESS	    7
#define FLG_UNAWARE	    8
#define FLG_WORTHLESS	    9
#define FLG_BOOSTED	    10
#define FLG_MORE_THAN	    11
#define FLG_DICE	    12
#define FLG_WANTED	    13
#define FLG_UNIQUE	    14
#define FLG_HUMAN	    15
#define FLG_UNREADABLE	    16
#define FLG_REALM1	    17
#define FLG_REALM2	    18
#define FLG_FIRST	    19
#define FLG_SECOND	    20
#define FLG_THIRD	    21
#define FLG_FOURTH	    22

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
#define KEY_COLLECTING "収集中の"
#define KEY_UNIDENTIFIED "未鑑定の"
#define KEY_IDENTIFIED "鑑定済みの"
#define KEY_STAR_IDENTIFIED "*鑑定*済みの"
#define KEY_ARTIFACT "アーティファクト"
#define KEY_EGO "エゴ"
#define KEY_NAMELESS "無銘の"
#define KEY_UNAWARE "未判明の"
#define KEY_WORTHLESS "無価値の"
#define KEY_BOOSTED "ダイス目の違う"
#define KEY_MORE_THAN  "ダイス目"
#define KEY_DICE  "以上の"
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
#define KEY_ARTIFACT "artifact"
#define KEY_EGO "ego"
#define KEY_NAMELESS "nameless"
#define KEY_UNAWARE "unaware"
#define KEY_WORTHLESS "worthless"
#define KEY_BOOSTED "dice boosted"
#define KEY_MORE_THAN  "more than "
#define KEY_DICE  " dice "
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
     ? (prev_ptr = ptr, ptr += sizeof(KEY)-1, (' '==*ptr) ? ptr++ : 0, TRUE) : FALSE)

#ifdef JP
#define ADD_KEY(KEY) strcat(ptr, KEY)
#else
#define ADD_KEY(KEY) (strcat(ptr, KEY), strcat(ptr, " "))
#endif
#define ADD_KEY2(KEY) strcat(ptr, KEY)

#define ADD_FLG(FLG) (entry->flag[FLG / 32] |= (1L << (FLG % 32)))
#define REM_FLG(FLG) (entry->flag[FLG / 32] &= ~(1L << (FLG % 32)))
#define ADD_FLG2(FLG) (entry->flag[FLG / 32] |= (1L << (FLG % 32)), prev_flg = FLG)
#define IS_FLG(FLG) (entry->flag[FLG / 32] & (1L << (FLG % 32)))

#ifdef JP
	static char kanji_colon[] = "：";
#endif


/*
 * Reconstruct preference line from entry
 */
cptr autopick_line_from_entry(autopick_type *entry)
{
	char buf[1024];
	char *ptr;
	bool sepa_flag = TRUE;

	*buf = '\0';
	if (!(entry->action & DO_DISPLAY)) strcat(buf, "(");
	if (entry->action & DO_AUTODESTROY) strcat(buf, "!");
	if (entry->action & DONT_AUTOPICK) strcat(buf, "~");

	ptr = buf;

	if (IS_FLG(FLG_ALL)) ADD_KEY(KEY_ALL);
	if (IS_FLG(FLG_COLLECTING)) ADD_KEY(KEY_COLLECTING);
	if (IS_FLG(FLG_UNIDENTIFIED)) ADD_KEY(KEY_UNIDENTIFIED);
	if (IS_FLG(FLG_IDENTIFIED)) ADD_KEY(KEY_IDENTIFIED);
	if (IS_FLG(FLG_STAR_IDENTIFIED)) ADD_KEY(KEY_STAR_IDENTIFIED);
	if (IS_FLG(FLG_ARTIFACT)) ADD_KEY(KEY_ARTIFACT);
	if (IS_FLG(FLG_EGO)) ADD_KEY(KEY_EGO);
	if (IS_FLG(FLG_NAMELESS)) ADD_KEY(KEY_NAMELESS);
	if (IS_FLG(FLG_UNAWARE)) ADD_KEY(KEY_UNAWARE);
	if (IS_FLG(FLG_WORTHLESS)) ADD_KEY(KEY_WORTHLESS);
	if (IS_FLG(FLG_BOOSTED)) ADD_KEY(KEY_BOOSTED);

	if (IS_FLG(FLG_MORE_THAN))
	{
		ADD_KEY(KEY_MORE_THAN);
		strcat(ptr, format("%2d", entry->dice));
		ADD_KEY(KEY_DICE);
	}

	if (IS_FLG(FLG_WANTED)) ADD_KEY(KEY_WANTED);
	if (IS_FLG(FLG_UNIQUE)) ADD_KEY(KEY_UNIQUE);
	if (IS_FLG(FLG_HUMAN)) ADD_KEY(KEY_HUMAN);
	if (IS_FLG(FLG_UNREADABLE)) ADD_KEY(KEY_UNREADABLE);
	if (IS_FLG(FLG_REALM1)) ADD_KEY(KEY_REALM1);
	if (IS_FLG(FLG_REALM2)) ADD_KEY(KEY_REALM2);
	if (IS_FLG(FLG_FIRST)) ADD_KEY(KEY_FIRST);
	if (IS_FLG(FLG_SECOND)) ADD_KEY(KEY_SECOND);
	if (IS_FLG(FLG_THIRD)) ADD_KEY(KEY_THIRD);
	if (IS_FLG(FLG_FOURTH)) ADD_KEY(KEY_FOURTH);

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
	else
		sepa_flag = FALSE;

	if (entry->name && *entry->name)
	{
		if (sepa_flag)
			strcat(buf, ":");
		strcat(buf, entry->name);
	}
	else
	{
		if (entry->flag[0] == 0L && entry->flag[0] == 0L)
			return NULL;
	}

	if (entry->insc)
	{
		strcat(buf, "#");
		strcat(buf, entry->insc);
	}

	return string_make(buf);
}

/*
 * A function to create new entry
 */
bool autopick_new_entry(autopick_type *entry, cptr str)
{
	cptr insc;
	int i;
	byte act = 0;
	char buf[1024];
	cptr prev_ptr, ptr;
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

	/* Found flags */
	prev_ptr = ptr = buf;
	prev_flg = -1;
	if (MATCH_KEY(KEY_ALL)) ADD_FLG(FLG_ALL);
	if (MATCH_KEY(KEY_COLLECTING)) ADD_FLG(FLG_COLLECTING);
	if (MATCH_KEY(KEY_UNIDENTIFIED)) ADD_FLG(FLG_UNIDENTIFIED);
	if (MATCH_KEY(KEY_IDENTIFIED)) ADD_FLG(FLG_IDENTIFIED);
	if (MATCH_KEY(KEY_STAR_IDENTIFIED)) ADD_FLG(FLG_STAR_IDENTIFIED);
	if (MATCH_KEY(KEY_ARTIFACT)) ADD_FLG(FLG_ARTIFACT);
	if (MATCH_KEY(KEY_EGO)) ADD_FLG(FLG_EGO);
	if (MATCH_KEY(KEY_NAMELESS)) ADD_FLG(FLG_NAMELESS);
	if (MATCH_KEY(KEY_UNAWARE)) ADD_FLG(FLG_UNAWARE);
	if (MATCH_KEY(KEY_WORTHLESS)) ADD_FLG(FLG_WORTHLESS);
	if (MATCH_KEY(KEY_BOOSTED)) ADD_FLG(FLG_BOOSTED);

	/*** Weapons whic dd*ds is more than nn ***/
	if (MATCH_KEY(KEY_MORE_THAN))
	{
		if (isdigit(ptr[0]) && isdigit(ptr[1]))
		{
			entry->dice = (ptr[0] - '0') * 10 + (ptr[1] - '0');
			ptr += 2;
			(void)MATCH_KEY(KEY_DICE);
			ADD_FLG(FLG_MORE_THAN);
		}
		else
			ptr = prev_ptr;
	}

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

	/* Reset previous word location */
	prev_ptr = ptr;

	if (MATCH_KEY(KEY_ITEMS)) ADD_FLG2(FLG_ITEMS);
	else if (MATCH_KEY(KEY_WEAPONS)) ADD_FLG2(FLG_WEAPONS);
	else if (MATCH_KEY(KEY_ARMORS)) ADD_FLG2(FLG_ARMORS);
	else if (MATCH_KEY(KEY_MISSILES)) ADD_FLG2(FLG_MISSILES);
	else if (MATCH_KEY(KEY_DEVICES)) ADD_FLG2(FLG_DEVICES);
	else if (MATCH_KEY(KEY_LIGHTS)) ADD_FLG2(FLG_LIGHTS);
	else if (MATCH_KEY(KEY_JUNKS)) ADD_FLG2(FLG_JUNKS);
	else if (MATCH_KEY(KEY_SPELLBOOKS)) ADD_FLG2(FLG_SPELLBOOKS);
	else if (MATCH_KEY(KEY_HAFTED)) ADD_FLG2(FLG_HAFTED);
	else if (MATCH_KEY(KEY_SHIELDS)) ADD_FLG2(FLG_SHIELDS);
	else if (MATCH_KEY(KEY_BOWS)) ADD_FLG2(FLG_BOWS);
	else if (MATCH_KEY(KEY_RINGS)) ADD_FLG2(FLG_RINGS);
	else if (MATCH_KEY(KEY_AMULETS)) ADD_FLG2(FLG_AMULETS);
	else if (MATCH_KEY(KEY_SUITS)) ADD_FLG2(FLG_SUITS);
	else if (MATCH_KEY(KEY_CLOAKS)) ADD_FLG2(FLG_CLOAKS);
	else if (MATCH_KEY(KEY_HELMS)) ADD_FLG2(FLG_HELMS);
	else if (MATCH_KEY(KEY_GLOVES)) ADD_FLG2(FLG_GLOVES);
	else if (MATCH_KEY(KEY_BOOTS)) ADD_FLG2(FLG_BOOTS);

	/* Last 'keyword' must be at the correct location */
	if (*ptr == ':')
		ptr++;
	else if (*ptr == '\0')
		; /* nothing to do */
#ifdef JP
	else if (ptr[0] == kanji_colon[0] && ptr[1] == kanji_colon[1])
		ptr += 2;
#endif
	else
	{
		if (prev_flg != -1)
			entry->flag[prev_flg/32] &= ~(1L<< (prev_flg%32));
		ptr = prev_ptr;
	}
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
		bool flag = FALSE;
		cptr ptr = autopick_list[i].name;

		/*** Unidentified ***/
		if (IS_FLG(FLG_UNIDENTIFIED)
		    && (object_known_p(o_ptr) || (o_ptr->ident & IDENT_SENSE)))
			continue;

		/*** Identified ***/
		if (IS_FLG(FLG_IDENTIFIED) && !object_known_p(o_ptr))
			continue;

		/*** *Identified* ***/
		if (IS_FLG(FLG_STAR_IDENTIFIED) &&
		    (!object_known_p(o_ptr) || !(o_ptr->ident & IDENT_MENTAL)))
			continue;

		/*** Artifact object ***/
		if (IS_FLG(FLG_ARTIFACT))
		{
			if (!object_known_p(o_ptr) || (!o_ptr->name1 && !o_ptr->art_name))
				continue;
		}

		/*** Ego object ***/
		if (IS_FLG(FLG_EGO))
		{
			if (!object_known_p(o_ptr) || !o_ptr->name2)
				continue;
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
					continue;
				break;
			default:
				/* don't match */
				continue;
			}
		}

		/*** Unaware items ***/
		if (IS_FLG(FLG_UNAWARE) && object_aware_p(o_ptr))
			continue;

		/*** Worthless items ***/
		if (IS_FLG(FLG_WORTHLESS) && object_value(o_ptr) > 0)
			continue;

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
					continue;
			default:
				continue;
			}
		}

		/*** Weapons whic dd*ds is more than nn ***/
		if (IS_FLG(FLG_MORE_THAN))
		{
			if (o_ptr->dd * o_ptr->ds < entry->dice)
				continue;
		}
				
		/*** Wanted monster's corpse/skeletons ***/
		if (IS_FLG(FLG_WANTED) &&
		    (o_ptr->tval != TV_CORPSE || !object_is_shoukinkubi(o_ptr)))
			continue;

		/*** Unique monster's corpse/skeletons/statues ***/
		if (IS_FLG(FLG_UNIQUE) &&
		    ((o_ptr->tval != TV_CORPSE && o_ptr->tval != TV_STATUE) ||
		    !(r_info[o_ptr->pval].flags1 & RF1_UNIQUE)))
			continue;

		/*** Human corpse/skeletons (for Daemon magic) ***/
		if (IS_FLG(FLG_HUMAN) &&
		    (o_ptr->tval != TV_CORPSE ||
		    !strchr("pht", r_info[o_ptr->pval].d_char)))
			continue;

		/*** Unreadable spellbooks ***/
		if (IS_FLG(FLG_UNREADABLE) &&
		    (o_ptr->tval < TV_LIFE_BOOK ||
		    check_book_realm(o_ptr->tval, o_ptr->sval)))
			continue;

		/*** First realm spellbooks ***/
		if (IS_FLG(FLG_REALM1) && 
		    (REALM1_BOOK != o_ptr->tval ||
		    p_ptr->pclass == CLASS_SORCERER ||
		    p_ptr->pclass == CLASS_RED_MAGE))
			continue;

		/*** Second realm spellbooks ***/
		if (IS_FLG(FLG_REALM2) &&
		    (REALM2_BOOK != o_ptr->tval ||
		    p_ptr->pclass == CLASS_SORCERER ||
		    p_ptr->pclass == CLASS_RED_MAGE))
			continue;

		/*** First rank spellbooks ***/
		if (IS_FLG(FLG_FIRST) &&
		    (o_ptr->tval < TV_LIFE_BOOK || 0 != o_ptr->sval))
			continue;

		/*** Second rank spellbooks ***/
		if (IS_FLG(FLG_SECOND) &&
		    (o_ptr->tval < TV_LIFE_BOOK || 1 != o_ptr->sval))
			continue;

		/*** Third rank spellbooks ***/
		if (IS_FLG(FLG_THIRD) && 
		    (o_ptr->tval < TV_LIFE_BOOK || 2 != o_ptr->sval))
			continue;

		/*** Fourth rank spellbooks ***/
		if (IS_FLG(FLG_FOURTH) &&
		    (o_ptr->tval < TV_LIFE_BOOK || 3 != o_ptr->sval))
			continue;

		/*** Items ***/
		if (IS_FLG(FLG_WEAPONS))
		{
			switch(o_ptr->tval)
			{
			case TV_BOW: case TV_HAFTED: case TV_POLEARM:
			case TV_SWORD: case TV_DIGGING:
				break;
			default: continue;
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
			default: continue;
			}
		}
		else if (IS_FLG(FLG_MISSILES))
		{
			switch(o_ptr->tval)
			{
			case TV_SHOT: case TV_BOLT: case TV_ARROW:
				break;
			default: continue;
			}
		}
		else if (IS_FLG(FLG_DEVICES))
		{
			switch(o_ptr->tval)
			{
			case TV_SCROLL: case TV_STAFF: case TV_WAND: case TV_ROD:
				break;
			default: continue;
			}
		}
		else if (IS_FLG(FLG_LIGHTS))
		{
			if (!(o_ptr->tval == TV_LITE))
				continue;
		}
		else if (IS_FLG(FLG_JUNKS))
		{
			switch(o_ptr->tval)
			{
			case TV_SKELETON: case TV_BOTTLE:
			case TV_JUNK: case TV_STATUE:
				break;
			default: continue;
			}
		}
		else if (IS_FLG(FLG_SPELLBOOKS))
		{
			if (!(o_ptr->tval >= TV_LIFE_BOOK))
				continue;
		}
		else if (IS_FLG(FLG_HAFTED))
		{
			if (!(o_ptr->tval == TV_HAFTED))
				continue;
		}
		else if (IS_FLG(FLG_SHIELDS))
		{
			if (!(o_ptr->tval == TV_SHIELD))
				continue;
		}
		else if (IS_FLG(FLG_BOWS))
		{
			if (!(o_ptr->tval == TV_BOW))
				continue;
		}
		else if (IS_FLG(FLG_RINGS))
		{
			if (!(o_ptr->tval == TV_RING))
				continue;
		}
		else if (IS_FLG(FLG_AMULETS))
		{
			if (!(o_ptr->tval == TV_AMULET))
				continue;
		}
		else if (IS_FLG(FLG_SUITS))
		{
			if (!(o_ptr->tval == TV_DRAG_ARMOR ||
			      o_ptr->tval == TV_HARD_ARMOR ||
			      o_ptr->tval == TV_SOFT_ARMOR))
				continue;
		}
		else if (IS_FLG(FLG_CLOAKS))
		{
			if (!(o_ptr->tval == TV_CLOAK))
				continue;
		}
		else if (IS_FLG(FLG_HELMS))
		{
			if (!(o_ptr->tval == TV_CROWN || o_ptr->tval == TV_HELM))
				continue;
		}
		else if (IS_FLG(FLG_GLOVES))
		{
			if (!(o_ptr->tval == TV_GLOVES))
				continue;
		}
		else if (IS_FLG(FLG_BOOTS))
		{
			if (!(o_ptr->tval == TV_BOOTS))
				continue;
		}


		if (*ptr == '^')
		{
			ptr++;
			if (!strncmp(o_name, ptr, strlen(ptr)))
				flag = TRUE;
		}
		else
#ifdef JP
			if (strstr_j(o_name, ptr))
#else
			if (strstr(o_name, ptr))
#endif
		{
			flag = TRUE;
		}

		if (flag)
		{
			int j;
			if (!IS_FLG(FLG_COLLECTING))
				return i;
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
					return i;
			}
		}
	}/* for */

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

	if (idx >= 0 && autopick_list[idx].insc && !o_ptr->inscription)
	{
		o_ptr->inscription = inscribe_flags(o_ptr, autopick_list[idx].insc);

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
}


/*
 * Automatically destroy an item if it is to be destroyed
 */
bool auto_destroy_item(int item, int autopick_idx)
{
	char o_name[MAX_NLEN];
	object_type *o_ptr;

	/* Don't destroy equipped items */
	if (item > INVEN_PACK) return FALSE;

	/* Get the item (in the pack) */
	if (item >= 0) o_ptr = &inventory[item];

	/* Get the item (on the floor) */
	else o_ptr = &o_list[0 - item];

	if ((autopick_idx == -1 && is_opt_confirm_destroy(o_ptr)) ||
	    (autopick_idx >= 0 && (autopick_list[autopick_idx].action & DO_AUTODESTROY)))
	{
		disturb(0,0);

		/* Describe the object (with {terrible/special}) */
		object_desc(o_name, o_ptr, TRUE, 3);

		/* Artifact? */
		if (!can_player_destroy_object(o_ptr))
		{
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
		autopick_free_entry(&autopick_entry_last_destroyed);
		autopick_entry_from_object(&autopick_entry_last_destroyed, o_ptr);

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
			
		return TRUE;
	}

	return FALSE;
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

		if (idx >= 0 && (autopick_list[idx].action & DO_AUTOPICK))
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
				continue;
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
			if (auto_destroy_item((-this_o_idx), (!always_pickup ? idx : -2)))
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
	cptr before_str[20], body_str;
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

	/*** Worthless items ***/
	if (IS_FLG(FLG_WORTHLESS))
		before_str[before_n++] = "店で無価値と判定される";

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
			
		sprintf(more_than_desc_str,"%2d", entry->dice);
		before_str[before_n++] = more_than_desc_str;
		before_str[before_n++] = "以上の";
	}

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
		body_str = "小手";
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
		strncat(buff, format("に「%s」と刻んで", insc), 80);
	else
		strcat(buff, "を");

	if (act & DONT_AUTOPICK)
		strcat(buff, "放置する。");
	else if (act & DO_AUTODESTROY)
		strcat(buff, "破壊する。");
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

	/*** Worthless items ***/
	if (IS_FLG(FLG_WORTHLESS))
	{
		before_str[before_n++] = "worthless";
		which_str[which_n++] = "can not be sold at stores";
	}

	/*** Dice boosted (weapon of slaying) ***/
	if (IS_FLG(FLG_BOOSTED))
	{
		body_str = "weapons";
		whose_str[whose_n++] = "damage dice is bigger than normal";
	}

	/*** Weapons whic dd*ds is more than nn ***/
	if (IS_FLG(FLG_MORE_THAN))
	{
		static char more_than_desc_str[] =
			"maximum damage from dice is bigger than __";
		body_str = "weapons";
			
		sprintf(more_than_desc_str + sizeof(more_than_desc_str) - 3,
			"%2d", entry->dice);
		whose_str[whose_n++] = more_than_desc_str;
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
	else
		strcpy(buff, "Pickup ");

	/* Auto-insctiption */
	if (insc)
		strncat(buff, format("and inscribe \"%s\" on ", insc), 80);

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
			strcat(buff, "  Display these items when you press 'N' in the full map('M').");
		else if (act & DO_AUTODESTROY)
			strcat(buff, "  Display these items when you press 'K' in the full map('M').");
		else
			strcat(buff, "  Display these items when you press 'M' in the full map('M').");
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
		/* Hack -- drop permissions */
		safe_setuid_drop();
		path_build(buf, 1024, ANGBAND_DIR_USER, filename);
	}
	else
	{
		path_build(buf, 1024, ANGBAND_DIR_PREF, filename);
	}
	
	/* Open the file */
	fff = my_fopen(buf, "r");

	if (fff)
	{
		/* Allocate list of pointers */
		C_MAKE(lines_list, MAX_LINES, cptr);

		/* Parse it */
		while (0 == my_fgets(fff, buf, 1024))
		{
			lines_list[lines++] = string_make(buf);
			if (lines >= MAX_LINES - 1) break;
		}
		if (lines == 0)
			lines_list[0] = string_make("");

		my_fclose(fff);
	}

	/* Grab priv's */
	safe_setuid_grab();

	if (!fff) return NULL;
	return lines_list;
}

static cptr *read_pickpref_text_lines(void)
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
	}

	if (!lines_list)
	{
#ifdef JP
		lines_list = read_text_lines("picktype.prf", FALSE);
#else
		lines_list = read_text_lines("pickpref.prf", FALSE);
#endif
	}

	if (!lines_list)
	{
		/* Allocate list of pointers */
		C_MAKE(lines_list, MAX_LINES, cptr);
		lines_list[0] = string_make("");
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

	/* Hack -- drop permissions */
	safe_setuid_drop();

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_USER, filename);
	
	/* Open the file */
	fff = my_fopen(buf, "w");
	if (fff)
	{
		for (lines = 0; lines_list[lines]; lines++)
			my_fputs(fff, lines_list[lines], 1024);

		my_fclose(fff);
	}

	/* Grab priv's */
	safe_setuid_grab();

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
 * Insert string
 */
static void insert_string(cptr *lines_list, cptr str, int x, int y)
{
	char buf[1024];
	int i, j;

	for (i = j = 0; lines_list[y][i] && i < x; i++)
		buf[j++] = lines_list[y][i];

	while (*str) buf[j++] = *str++;

	for (; lines_list[y][i]; i++)
		buf[j++] = lines_list[y][i];
	buf[j] = '\0';
	string_free(lines_list[y]);
	lines_list[y] = string_make(buf);
}

/*
 * Delete n letters
 */
static void delete_string(cptr *lines_list, int n, int x, int y)
{
	int i, j;
	char buf[1024];

	for (i = j = 0; lines_list[y][i] && i < x; i++)
	{
#ifdef JP
		if (iskanji(lines_list[y][i]))
			buf[j++] = lines_list[y][i++];
#endif
		buf[j++] = lines_list[y][i];
	}
	i += n;

	for (; lines_list[y][i]; i++)
		buf[j++] = lines_list[y][i];
	buf[j] = '\0';
	string_free(lines_list[y]);
	lines_list[y] = string_make(buf);
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

	lines_list[y] = autopick_line_from_entry(entry);
}

/*
 * Insert return code and split the line
 */
static bool insert_return_code(cptr *lines_list, int cx, int cy)
{
	char buf[1024];
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
static bool entry_from_choosed_object(autopick_type *entry)
{
	int item;
	object_type *o_ptr;
	cptr q, s;

	/* Get an item */
#ifdef JP
	q = "どのアイテムを登録しますか? ";
	s = "アイテムを持っていない。";
#else
	q = "Entry which item? ";
	s = "You have nothing to entry.";
#endif
	if (!get_item(&item, q, s, (USE_INVEN | USE_FLOOR | USE_EQUIP))) return FALSE;

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}

	autopick_entry_from_object(entry, o_ptr);
	return TRUE;
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


#define MAX_YANK 1024
#define DIRTY_ALL 0x01
#define DIRTY_COMMAND 0x02
#define DIRTY_MODE 0x04
#define DIRTY_SCREEN 0x04

/*
 * In-game editor of Object Auto-picker/Destoryer
 */
void do_cmd_edit_autopick(void)
{
	static int cx = 0, cy = 0;
	static int upper = 0, left = 0;

	cptr last_destroyed;
	char last_destroyed_command[WID_DESC+3];
	char yank_buf[MAX_YANK];
	char classrace[80];
	autopick_type an_entry, *entry = &an_entry;
	char buf[1024];
	cptr *lines_list;

	int i, j, k, len;
	cptr tmp;

	int old_upper = -1, old_left = -1;
	int old_cy = -1;
	int key = -1, old_key;

	bool edit_mode = FALSE;

	byte dirty_flags = DIRTY_ALL | DIRTY_COMMAND | DIRTY_MODE;
	int dirty_line = -1;

	int wid, hgt, old_wid = -1, old_hgt = -1;

	/* Free old entries */
	init_autopicker();

	/* Name of the Last Destroyed Item */
	last_destroyed = autopick_line_from_entry(&autopick_entry_last_destroyed);

	/* Command Description of the Last Destroyed Item */
	if (last_destroyed)
	{
		strcpy(last_destroyed_command, "^L \"");
		strncpy(last_destroyed_command + 4, last_destroyed, WID_DESC-4);
		last_destroyed_command[WID_DESC+2] = '\0';
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
	lines_list = read_pickpref_text_lines();

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
			dirty_flags = DIRTY_ALL | DIRTY_COMMAND | DIRTY_MODE;

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
			prt("q _ で終了、hjkl2468 で移動、^Q a i で入力モード", 0, 0);
#else	
		if (edit_mode)
			prt("Press ^Q ESC to command mode, any letters to insert", 0, 0);
		else
			prt("Press q _ to quit, hjkl2468 to move, ^Q a i to insert mode", 0, 0);
#endif
		/* Display current position */
		prt (format("(%d,%d)", cx, cy), 0, 70);

		/* Display information when updated */
		if (old_cy != cy || (dirty_flags & DIRTY_ALL) || dirty_line == cy)
		{
			/* Clear information line */
			Term_erase(0, hgt - 3 + 1, wid);
			Term_erase(0, hgt - 3 + 2, wid);

			/* Display information */
			if (lines_list[cy][0] == '#')
			{
#ifdef JP
				prt("この行はコメントです。", hgt - 3 + 1, 0);
#else
				prt("This line is comment.", hgt - 3 + 1, 0);
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
					prt("This line is Conditional Expression.", hgt - 3 + 1, 0);
#endif
					break;
				case 'A':
#ifdef JP
					prt("この行はマクロの実行内容を定義します。", hgt - 3 + 1, 0);
#else
					prt("This line defines Macro action.", hgt - 3 + 1, 0);
#endif
					break;
				case 'P':
#ifdef JP
					prt("この行はマクロのトリガー・キーを定義します。", hgt - 3 + 1, 0);
#else
					prt("This line defines Macro trigger key.", hgt - 3 + 1, 0);
#endif
					break;
				case 'C':
#ifdef JP
					prt("この行はキー配置を定義します。", hgt - 3 + 1, 0);
#else
					prt("This line defines Keymap.", hgt - 3 + 1, 0);
#endif
					break;
				}
			}

			/* Get description of an autopicker preference line */
			else if (autopick_new_entry(entry, lines_list[cy]))
			{
				char temp[80*4];
				cptr t;

				describe_autopick(buf, entry);

				roff_to_buf(buf, 79, temp);
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
			else if (!iscntrl(key&0xff))
			{
				int next;

				for (i = j = 0; lines_list[cy][i] && i < cx; i++)
					buf[j++] = lines_list[cy][i];

#ifdef JP
                                if (iskanji(key))
				{
                                        inkey_base = TRUE;
                                        next = inkey();
                                        if (j+2 < 1024)
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
                                        if (j+1 < 1024)
						buf[j++] = key;
					cx++;
				}
				for (; lines_list[cy][i] && j + 1 < 1024; i++)
					buf[j++] = lines_list[cy][i];
				buf[j] = '\0';
				string_free(lines_list[cy]);
				lines_list[cy] = string_make(buf);
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
					break;
				string_free(lines_list[cy]);

				entry->action &= ~DO_AUTODESTROY;
				if (entry->action & DO_AUTOPICK)
				{
					entry->action &= ~DO_AUTOPICK;
					entry->action |= DONT_AUTOPICK;
				}
				else 
				{
					entry->action &= ~DONT_AUTOPICK;
					entry->action |= DO_AUTOPICK;
				}

				lines_list[cy] = autopick_line_from_entry(entry);

				/* Now dirty */
				dirty_line = cy;
				break;
			case '!':
				if (!autopick_new_entry(entry, lines_list[cy]))
					break;
				string_free(lines_list[cy]);

				entry->action &= ~DONT_AUTOPICK;
				if (entry->action & DO_AUTOPICK)
				{
					entry->action &= ~DO_AUTOPICK;
					entry->action |= DO_AUTODESTROY;
				}
				else 
				{
					entry->action &= ~DO_AUTODESTROY;
					entry->action |= DO_AUTOPICK;
				}

				lines_list[cy] = autopick_line_from_entry(entry);

				/* Now dirty */
				dirty_line = cy;
				break;
			case '(':
				key = KTRL('g');
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
			lines_list[cy] = string_make(classrace);
			cy++;
			insert_return_code(lines_list, 0, cy);
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
			/* Toggle display in the 'M'ap */
			if (lines_list[cy][0] != '(' && lines_list[cy][1] != '(')
				insert_string(lines_list, "(", 0, cy);
			else if (lines_list[cy][0] == '(')
				delete_string(lines_list, 1, 0, cy);
			else if (lines_list[cy][1] == '(')
				delete_string(lines_list, 1, 1, cy);

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
			tmp = autopick_line_from_entry(entry);
			autopick_free_entry(entry);
			if (tmp)
			{
				insert_return_code(lines_list, 0, cy);
				lines_list[cy] = tmp;
				cx = 0;

				/* Now dirty because of item/equip menu */
				dirty_flags |= DIRTY_SCREEN;
			}
			break;
		case KTRL('l'):
			/* Insert a name of last destroyed item */
			if (last_destroyed)
			{
				insert_return_code(lines_list, 0, cy);
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
			for (i = 0; lines_list[cy][i]; i++)
				if (lines_list[cy][i] == '#') break;
			if (!lines_list[cy][i]) insert_string(lines_list, "#", i, cy);
			cx = i + 1;
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
			if (!get_check("Discard all change and revert to original file. Are you sure? "))
#endif
				break;

			free_text_lines(lines_list);
			lines_list = read_pickpref_text_lines();
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

			lines_list[cy] = autopick_line_from_entry(entry);
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

			lines_list[cy] = autopick_line_from_entry(entry);

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

			lines_list[cy] = autopick_line_from_entry(entry);

			/* Now dirty */
			dirty_line = cy;
			break;

		case KTRL('y'):
			/* Paste killed text */
			if (strlen(yank_buf))
			{
				cx = 0;
				for (j = 0; yank_buf[j]; j += strlen(yank_buf + j) + 1)
				{
					insert_return_code(lines_list, 0, cy);
					lines_list[cy] = string_make(yank_buf + j);
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
			if (lines_list[cy][0] != '\0' && (unsigned int) cx < strlen(lines_list[cy]))
			{
				for (i = j = 0; lines_list[cy][i] && i < cx; i++)
				{
#ifdef JP
					if (iskanji(lines_list[cy][i]))
						buf[j++] = lines_list[cy][i++];
#endif
					buf[j++] = lines_list[cy][i];
				}
				buf[j] = '\0';

				j = 0;
				if (old_key == KTRL('k'))
					while (yank_buf[j])
						j += strlen(yank_buf + j) + 1;

				if (j < MAX_YANK - 2)
				{
					strncpy(yank_buf + j, lines_list[cy] + i, MAX_YANK-j-2);
					yank_buf[MAX_YANK-2] = '\0';
					yank_buf[j + strlen(lines_list[cy] + i) + 1] = '\0';
				}
				string_free(lines_list[cy]);
				lines_list[cy] = string_make(buf);

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

			/* fall through */

		case '\010':
			/* BACK SPACE */
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

			for (i = j = 0; lines_list[cy][i] && i < cx; i++)
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

#ifdef JP
	sprintf(buf, "picktype-%s.prf", player_name);
#else
	sprintf(buf, "pickpref-%s.prf", player_name);
#endif
	write_text_lines(buf, lines_list);
	free_text_lines(lines_list);

	string_free(last_destroyed);

	/* Reload autopick pref */
	process_pickpref_file(buf);
}
