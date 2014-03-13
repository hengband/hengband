/* File: load.c */

/*
 * Copyright (c) 1997 Ben Harrison, and others
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */

/* Purpose: support for loading savefiles -BEN- */

#include "angband.h"


/*
 * This file loads savefiles from Angband 2.7.X and 2.8.X
 *
 * Ancient savefiles (pre-2.7.0) are loaded by another file.
 *
 * Note that Angband 2.7.0 through 2.7.3 are now officially obsolete,
 * and savefiles from those versions may not be successfully converted.
 *
 * We attempt to prevent corrupt savefiles from inducing memory errors.
 *
 * Note that this file should not use the random number generator, the
 * object flavors, the visual attr/char mappings, or anything else which
 * is initialized *after* or *during* the "load character" function.
 *
 * This file assumes that the monster/object records are initialized
 * to zero, and the race/kind tables have been loaded correctly.  The
 * order of object stacks is currently not saved in the savefiles, but
 * the "next" pointers are saved, so all necessary knowledge is present.
 *
 * We should implement simple "savefile extenders" using some form of
 * "sized" chunks of bytes, with a {size,type,data} format, so everyone
 * can know the size, interested people can know the type, and the actual
 * data is available to the parsing routines that acknowledge the type.
 *
 * Consider changing the "globe of invulnerability" code so that it
 * takes some form of "maximum damage to protect from" in addition to
 * the existing "number of turns to protect for", and where each hit
 * by a monster will reduce the shield by that amount.
 *
 * XXX XXX XXX
 */



/*
 * Maximum number of tries for selection of a proper quest monster
 */
#define MAX_TRIES 100


/*
 * Local "savefile" pointer
 */
static FILE	*fff;

/*
 * Hack -- old "encryption" byte
 */
static byte	xor_byte;

/*
 * Hack -- simple "checksum" on the actual values
 */
static u32b	v_check = 0L;

/*
 * Hack -- simple "checksum" on the encoded bytes
 */
static u32b	x_check = 0L;

/*
 * Hack -- Japanese Kanji code
 * 0: Unknown
 * 1: ASCII
 * 2: EUC
 * 3: SJIS
 */
static byte kanji_code = 0;

/*
 * This function determines if the version of the savefile
 * currently being read is older than version "major.minor.patch.extra".
 */
static bool h_older_than(byte major, byte minor, byte patch, byte extra)
{
	/* Much older, or much more recent */
	if (h_ver_major < major) return (TRUE);
	if (h_ver_major > major) return (FALSE);

	/* Distinctly older, or distinctly more recent */
	if (h_ver_minor < minor) return (TRUE);
	if (h_ver_minor > minor) return (FALSE);

	/* Barely older, or barely more recent */
	if (h_ver_patch < patch) return (TRUE);
	if (h_ver_patch > patch) return (FALSE);

	/* Barely older, or barely more recent */
	if (h_ver_extra < extra) return (TRUE);
	if (h_ver_extra > extra) return (FALSE);

	/* Identical versions */
	return (FALSE);
}


/*
 * The above function, adapted for Zangband
 */
static bool z_older_than(byte x, byte y, byte z)
{
	/* Much older, or much more recent */
	if (z_major < x) return (TRUE);
	if (z_major > x) return (FALSE);

	/* Distinctly older, or distinctly more recent */
	if (z_minor < y) return (TRUE);
	if (z_minor > y) return (FALSE);

	/* Barely older, or barely more recent */
	if (z_patch < z) return (TRUE);
	if (z_patch > z) return (FALSE);

	/* Identical versions */
	return (FALSE);
}


/*
 * Hack -- Show information on the screen, one line at a time.
 *
 * Avoid the top two lines, to avoid interference with "msg_print()".
 */
static void note(cptr msg)
{
	static int y = 2;

	/* Draw the message */
	prt(msg, y, 0);

	/* Advance one line (wrap if needed) */
	if (++y >= 24) y = 2;

	/* Flush it */
	Term_fresh();
}


/*
 * The following functions are used to load the basic building blocks
 * of savefiles.  They also maintain the "checksum" info for 2.7.0+
 */

static byte sf_get(void)
{
	byte c, v;

	/* Get a character, decode the value */
	c = getc(fff) & 0xFF;
	v = c ^ xor_byte;
	xor_byte = c;

	/* Maintain the checksum info */
	v_check += v;
	x_check += xor_byte;

	/* Return the value */
	return (v);
}

static void rd_byte(byte *ip)
{
	*ip = sf_get();
}

static void rd_u16b(u16b *ip)
{
	(*ip) = sf_get();
	(*ip) |= ((u16b)(sf_get()) << 8);
}

static void rd_s16b(s16b *ip)
{
	rd_u16b((u16b*)ip);
}

static void rd_u32b(u32b *ip)
{
	(*ip) = sf_get();
	(*ip) |= ((u32b)(sf_get()) << 8);
	(*ip) |= ((u32b)(sf_get()) << 16);
	(*ip) |= ((u32b)(sf_get()) << 24);
}

static void rd_s32b(s32b *ip)
{
	rd_u32b((u32b*)ip);
}


/*
 * Hack -- read a string
 */
static void rd_string(char *str, int max)
{
	int i;

	/* Read the string */
	for (i = 0; TRUE; i++)
	{
		byte tmp8u;

		/* Read a byte */
		rd_byte(&tmp8u);

		/* Collect string while legal */
		if (i < max) str[i] = tmp8u;

		/* End of string */
		if (!tmp8u) break;
	}

	/* Terminate */
	str[max-1] = '\0';


#ifdef JP
	/* Convert Kanji code */
	switch (kanji_code)
	{
#ifdef SJIS
	case 2:
		/* EUC to SJIS */
		euc2sjis(str);
		break;
#endif

#ifdef EUC
	case 3:
		/* SJIS to EUC */
		sjis2euc(str);
		break;
#endif

	case 0:
	{
		/* 不明の漢字コードからシステムの漢字コードに変換 */
		byte code = codeconv(str);

		/* 漢字コードが判明したら、それを記録 */
		if (code) kanji_code = code;

		break;
	}
	default:
		/* No conversion needed */
		break;
	}
#endif
}


/*
 * Hack -- strip some bytes
 */
static void strip_bytes(int n)
{
	byte tmp8u;

	/* Strip the bytes */
	while (n--) rd_byte(&tmp8u);
}

#define OLD_MAX_MANE 22

/*
 * Read an object (Old method)
 *
 * This function attempts to "repair" old savefiles, and to extract
 * the most up to date values for various object fields.
 *
 * Note that Angband 2.7.9 introduced a new method for object "flags"
 * in which the "flags" on an object are actually extracted when they
 * are needed from the object kind, artifact index, ego-item index,
 * and two special "xtra" fields which are used to encode any "extra"
 * power of certain ego-items.  This had the side effect that items
 * imported from pre-2.7.9 savefiles will lose any "extra" powers they
 * may have had, and also, all "uncursed" items will become "cursed"
 * again, including Calris, even if it is being worn at the time.  As
 * a complete hack, items which are inscribed with "uncursed" will be
 * "uncursed" when imported from pre-2.7.9 savefiles.
 */
static void rd_item_old(object_type *o_ptr)
{
	char buf[128];


	/* Kind */
	rd_s16b(&o_ptr->k_idx);

	/* Location */
	rd_byte(&o_ptr->iy);
	rd_byte(&o_ptr->ix);

	/* Type/Subtype */
	rd_byte(&o_ptr->tval);
	rd_byte(&o_ptr->sval);

	if (z_older_than(10, 4, 4))
	{
		if (o_ptr->tval == 100) o_ptr->tval = TV_GOLD;
		if (o_ptr->tval == 98) o_ptr->tval = TV_MUSIC_BOOK;
		if (o_ptr->tval == 110) o_ptr->tval = TV_HISSATSU_BOOK;
	}

	/* Special pval */
	rd_s16b(&o_ptr->pval);

	rd_byte(&o_ptr->discount);
	rd_byte(&o_ptr->number);
	rd_s16b(&o_ptr->weight);

	rd_byte(&o_ptr->name1);
	rd_byte(&o_ptr->name2);
	rd_s16b(&o_ptr->timeout);

	rd_s16b(&o_ptr->to_h);
	rd_s16b(&o_ptr->to_d);
	rd_s16b(&o_ptr->to_a);

	rd_s16b(&o_ptr->ac);

	rd_byte(&o_ptr->dd);
	rd_byte(&o_ptr->ds);

	rd_byte(&o_ptr->ident);

	rd_byte(&o_ptr->marked);

	/* Object flags */
	rd_u32b(&o_ptr->art_flags[0]);
	rd_u32b(&o_ptr->art_flags[1]);
	rd_u32b(&o_ptr->art_flags[2]);
	if (h_older_than(1, 3, 0, 0)) o_ptr->art_flags[3] = 0L;
	else rd_u32b(&o_ptr->art_flags[3]);

	if (h_older_than(1, 3, 0, 0))
	{
		if (o_ptr->name2 == EGO_TELEPATHY)
			add_flag(o_ptr->art_flags, TR_TELEPATHY);
	}

	if (z_older_than(11, 0, 11))
	{
		o_ptr->curse_flags = 0L;
		if (o_ptr->ident & 0x40)
		{
			o_ptr->curse_flags |= TRC_CURSED;
			if (o_ptr->art_flags[2] & 0x40000000L) o_ptr->curse_flags |= TRC_HEAVY_CURSE;
			if (o_ptr->art_flags[2] & 0x80000000L) o_ptr->curse_flags |= TRC_PERMA_CURSE;
			if (object_is_fixed_artifact(o_ptr))
			{
				artifact_type *a_ptr = &a_info[o_ptr->name1];
				if (a_ptr->gen_flags & (TRG_HEAVY_CURSE)) o_ptr->curse_flags |= TRC_HEAVY_CURSE;
				if (a_ptr->gen_flags & (TRG_PERMA_CURSE)) o_ptr->curse_flags |= TRC_PERMA_CURSE;
			}
			else if (object_is_ego(o_ptr))
			{
				ego_item_type *e_ptr = &e_info[o_ptr->name2];
				if (e_ptr->gen_flags & (TRG_HEAVY_CURSE)) o_ptr->curse_flags |= TRC_HEAVY_CURSE;
				if (e_ptr->gen_flags & (TRG_PERMA_CURSE)) o_ptr->curse_flags |= TRC_PERMA_CURSE;
			}
		}
		o_ptr->art_flags[2] &= (0x1FFFFFFFL);
	}
	else
	{
		rd_u32b(&o_ptr->curse_flags);
	}

	/* Monster holding object */
	rd_s16b(&o_ptr->held_m_idx);

	/* Special powers */
	rd_byte(&o_ptr->xtra1);
	rd_byte(&o_ptr->xtra2);

	if (z_older_than(11, 0, 10))
	{
		if (o_ptr->xtra1 == EGO_XTRA_SUSTAIN)
		{
			switch (o_ptr->xtra2 % 6)
			{
			case 0: add_flag(o_ptr->art_flags, TR_SUST_STR); break;
			case 1: add_flag(o_ptr->art_flags, TR_SUST_INT); break;
			case 2: add_flag(o_ptr->art_flags, TR_SUST_WIS); break;
			case 3: add_flag(o_ptr->art_flags, TR_SUST_DEX); break;
			case 4: add_flag(o_ptr->art_flags, TR_SUST_CON); break;
			case 5: add_flag(o_ptr->art_flags, TR_SUST_CHR); break;
			}
			o_ptr->xtra2 = 0;
		}
		else if (o_ptr->xtra1 == EGO_XTRA_POWER)
		{
			switch (o_ptr->xtra2 % 11)
			{
			case  0: add_flag(o_ptr->art_flags, TR_RES_BLIND);  break;
			case  1: add_flag(o_ptr->art_flags, TR_RES_CONF);   break;
			case  2: add_flag(o_ptr->art_flags, TR_RES_SOUND);  break;
			case  3: add_flag(o_ptr->art_flags, TR_RES_SHARDS); break;
			case  4: add_flag(o_ptr->art_flags, TR_RES_NETHER); break;
			case  5: add_flag(o_ptr->art_flags, TR_RES_NEXUS);  break;
			case  6: add_flag(o_ptr->art_flags, TR_RES_CHAOS);  break;
			case  7: add_flag(o_ptr->art_flags, TR_RES_DISEN);  break;
			case  8: add_flag(o_ptr->art_flags, TR_RES_POIS);   break;
			case  9: add_flag(o_ptr->art_flags, TR_RES_DARK);   break;
			case 10: add_flag(o_ptr->art_flags, TR_RES_LITE);   break;
			}
			o_ptr->xtra2 = 0;
		}		
		else if (o_ptr->xtra1 == EGO_XTRA_ABILITY)
		{
			switch (o_ptr->xtra2 % 8)
			{
			case 0: add_flag(o_ptr->art_flags, TR_LEVITATION);     break;
			case 1: add_flag(o_ptr->art_flags, TR_LITE_1);        break;
			case 2: add_flag(o_ptr->art_flags, TR_SEE_INVIS);   break;
			case 3: add_flag(o_ptr->art_flags, TR_WARNING);     break;
			case 4: add_flag(o_ptr->art_flags, TR_SLOW_DIGEST); break;
			case 5: add_flag(o_ptr->art_flags, TR_REGEN);       break;
			case 6: add_flag(o_ptr->art_flags, TR_FREE_ACT);    break;
			case 7: add_flag(o_ptr->art_flags, TR_HOLD_EXP);   break;
			}
			o_ptr->xtra2 = 0;
		}
		o_ptr->xtra1 = 0;
	}

	if (z_older_than(10, 2, 3))
	{
		o_ptr->xtra3 = 0;
		o_ptr->xtra4 = 0;
		o_ptr->xtra5 = 0;
		if ((o_ptr->tval == TV_CHEST) || (o_ptr->tval == TV_CAPTURE))
		{
			o_ptr->xtra3 = o_ptr->xtra1;
			o_ptr->xtra1 = 0;
		}
		if (o_ptr->tval == TV_CAPTURE)
		{
			if (r_info[o_ptr->pval].flags1 & RF1_FORCE_MAXHP)
				o_ptr->xtra5 = maxroll(r_info[o_ptr->pval].hdice, r_info[o_ptr->pval].hside);
			else
				o_ptr->xtra5 = damroll(r_info[o_ptr->pval].hdice, r_info[o_ptr->pval].hside);
			if (ironman_nightmare)
			{
				o_ptr->xtra5 = (s16b)MIN(30000, o_ptr->xtra5*2L);
			}
			o_ptr->xtra4 = o_ptr->xtra5;
		}
	}
	else
	{
		rd_byte(&o_ptr->xtra3);
		if (h_older_than(1, 3, 0, 1))
		{
			if (object_is_smith(o_ptr) && o_ptr->xtra3 >= 1+96)
				o_ptr->xtra3 += -96 + MIN_SPECIAL_ESSENCE;
		}

		rd_s16b(&o_ptr->xtra4);
		rd_s16b(&o_ptr->xtra5);
	}

	if (z_older_than(11, 0, 5) && (((o_ptr->tval == TV_LITE) && ((o_ptr->sval == SV_LITE_TORCH) || (o_ptr->sval == SV_LITE_LANTERN))) || (o_ptr->tval == TV_FLASK)))
	{
		o_ptr->xtra4 = o_ptr->pval;
		o_ptr->pval = 0;
	}

	rd_byte(&o_ptr->feeling);

	/* Inscription */
	rd_string(buf, sizeof(buf));

	/* Save the inscription */
	if (buf[0]) o_ptr->inscription = quark_add(buf);

	rd_string(buf, sizeof(buf));
	if (buf[0]) o_ptr->art_name = quark_add(buf);

	/* The Python object */
	{
		s32b tmp32s;

		rd_s32b(&tmp32s);
		strip_bytes(tmp32s);
	}

	/* Mega-Hack -- handle "dungeon objects" later */
	if ((o_ptr->k_idx >= 445) && (o_ptr->k_idx <= 479)) return;

	if (z_older_than(10, 4, 10) && (o_ptr->name2 == EGO_YOIYAMI)) o_ptr->k_idx = lookup_kind(TV_SOFT_ARMOR, SV_YOIYAMI_ROBE);

	if (z_older_than(10, 4, 9))
	{
		if (have_flag(o_ptr->art_flags, TR_MAGIC_MASTERY))
		{
			remove_flag(o_ptr->art_flags, TR_MAGIC_MASTERY);
			add_flag(o_ptr->art_flags, TR_DEC_MANA);
		}
	}

	/* Paranoia */
	if (object_is_fixed_artifact(o_ptr))
	{
		artifact_type *a_ptr;

		/* Obtain the artifact info */
		a_ptr = &a_info[o_ptr->name1];

		/* Verify that artifact */
		if (!a_ptr->name) o_ptr->name1 = 0;
	}

	/* Paranoia */
	if (object_is_ego(o_ptr))
	{
		ego_item_type *e_ptr;

		/* Obtain the ego-item info */
		e_ptr = &e_info[o_ptr->name2];

		/* Verify that ego-item */
		if (!e_ptr->name) o_ptr->name2 = 0;

	}
}


/*
 * Read an object (New method)
 */
static void rd_item(object_type *o_ptr)
{
	object_kind *k_ptr;
	u32b flags;
	char buf[128];

	if (h_older_than(1, 5, 0, 0))
	{
		rd_item_old(o_ptr);
		return;
	}

	/*** Item save flags ***/
	rd_u32b(&flags);

	/*** Read un-obvious elements ***/
	/* Kind */
	rd_s16b(&o_ptr->k_idx);

	/* Location */
	rd_byte(&o_ptr->iy);
	rd_byte(&o_ptr->ix);

	/* Type/Subtype */
	k_ptr = &k_info[o_ptr->k_idx];
	o_ptr->tval = k_ptr->tval;
	o_ptr->sval = k_ptr->sval;

	/* Special pval */
	if (flags & SAVE_ITEM_PVAL) rd_s16b(&o_ptr->pval);
	else o_ptr->pval = 0;

	if (flags & SAVE_ITEM_DISCOUNT) rd_byte(&o_ptr->discount);
	else o_ptr->discount = 0;
	if (flags & SAVE_ITEM_NUMBER) rd_byte(&o_ptr->number);
	else o_ptr->number = 1;

	rd_s16b(&o_ptr->weight);

	if (flags & SAVE_ITEM_NAME1) rd_byte(&o_ptr->name1);
	else o_ptr->name1 = 0;
	if (flags & SAVE_ITEM_NAME2) rd_byte(&o_ptr->name2);
	else o_ptr->name2 = 0;
	if (flags & SAVE_ITEM_TIMEOUT) rd_s16b(&o_ptr->timeout);
	else o_ptr->timeout = 0;

	if (flags & SAVE_ITEM_TO_H) rd_s16b(&o_ptr->to_h);
	else o_ptr->to_h = 0;
	if (flags & SAVE_ITEM_TO_D) rd_s16b(&o_ptr->to_d);
	else o_ptr->to_d = 0;
	if (flags & SAVE_ITEM_TO_A) rd_s16b(&o_ptr->to_a);
	else o_ptr->to_a = 0;

	if (flags & SAVE_ITEM_AC) rd_s16b(&o_ptr->ac);
	else o_ptr->ac = 0;

	if (flags & SAVE_ITEM_DD) rd_byte(&o_ptr->dd);
	else o_ptr->dd = 0;
	if (flags & SAVE_ITEM_DS) rd_byte(&o_ptr->ds);
	else o_ptr->ds = 0;

	if (flags & SAVE_ITEM_IDENT) rd_byte(&o_ptr->ident);
	else o_ptr->ident = 0;

	if (flags & SAVE_ITEM_MARKED) rd_byte(&o_ptr->marked);
	else o_ptr->marked = 0;

	/* Object flags */
	if (flags & SAVE_ITEM_ART_FLAGS0) rd_u32b(&o_ptr->art_flags[0]);
	else o_ptr->art_flags[0] = 0;
	if (flags & SAVE_ITEM_ART_FLAGS1) rd_u32b(&o_ptr->art_flags[1]);
	else o_ptr->art_flags[1] = 0;
	if (flags & SAVE_ITEM_ART_FLAGS2) rd_u32b(&o_ptr->art_flags[2]);
	else o_ptr->art_flags[2] = 0;
	if (flags & SAVE_ITEM_ART_FLAGS3) rd_u32b(&o_ptr->art_flags[3]);
	else o_ptr->art_flags[3] = 0;
	if (flags & SAVE_ITEM_ART_FLAGS4) rd_u32b(&o_ptr->art_flags[4]);
	else o_ptr->art_flags[4] = 0;

	if (flags & SAVE_ITEM_CURSE_FLAGS) rd_u32b(&o_ptr->curse_flags);
	else o_ptr->curse_flags = 0;

	/* Monster holding object */
	if (flags & SAVE_ITEM_HELD_M_IDX) rd_s16b(&o_ptr->held_m_idx);
	else o_ptr->held_m_idx = 0;

	/* Special powers */
	if (flags & SAVE_ITEM_XTRA1) rd_byte(&o_ptr->xtra1);
	else o_ptr->xtra1 = 0;
	if (flags & SAVE_ITEM_XTRA2) rd_byte(&o_ptr->xtra2);
	else o_ptr->xtra2 = 0;

	if (flags & SAVE_ITEM_XTRA3) rd_byte(&o_ptr->xtra3);
	else o_ptr->xtra3 = 0;

	if (flags & SAVE_ITEM_XTRA4) rd_s16b(&o_ptr->xtra4);
	else o_ptr->xtra4 = 0;
	if (flags & SAVE_ITEM_XTRA5) rd_s16b(&o_ptr->xtra5);
	else o_ptr->xtra5 = 0;

	if (flags & SAVE_ITEM_FEELING) rd_byte(&o_ptr->feeling);
	else o_ptr->feeling = 0;

	if (flags & SAVE_ITEM_INSCRIPTION)
	{
		rd_string(buf, sizeof(buf));
		o_ptr->inscription = quark_add(buf);
	}
	else o_ptr->inscription = 0;

	if (flags & SAVE_ITEM_ART_NAME)
	{
		rd_string(buf, sizeof(buf));
		o_ptr->art_name = quark_add(buf);
	}
	else o_ptr->art_name = 0;
	
	if(h_older_than(2,1,2,4))
	{
		u32b flgs[TR_FLAG_SIZE];
		object_flags(o_ptr, flgs);
		
		if ((o_ptr->name2 == EGO_DARK) || (o_ptr->name2 == EGO_ANCIENT_CURSE) || (o_ptr->name1 == ART_NIGHT))
		{
			add_flag(o_ptr->art_flags, TR_LITE_M1);
			remove_flag(o_ptr->art_flags, TR_LITE_1);
			remove_flag(o_ptr->art_flags, TR_LITE_2);
			remove_flag(o_ptr->art_flags, TR_LITE_3);
		}
		else if (o_ptr->name2 == EGO_LITE_DARKNESS)
		{
			if (o_ptr->tval == TV_LITE)
			{
				if (o_ptr->sval == SV_LITE_TORCH)
				{
					add_flag(o_ptr->art_flags, TR_LITE_M1);
				}
				else if (o_ptr->sval == SV_LITE_LANTERN)
				{
					add_flag(o_ptr->art_flags, TR_LITE_M2);
				}
				else if (o_ptr->sval == SV_LITE_FEANOR)
				{
					add_flag(o_ptr->art_flags, TR_LITE_M3);
				}
			}
			else
			{
				/* Paranoia */
				add_flag(o_ptr->art_flags, TR_LITE_M1);
			}
		}
		else if (o_ptr->tval == TV_LITE)
		{
			if (object_is_fixed_artifact(o_ptr))
			{
				add_flag(o_ptr->art_flags, TR_LITE_3);
			}
			else if (o_ptr->sval == SV_LITE_TORCH)
			{
				add_flag(o_ptr->art_flags, TR_LITE_1);
				add_flag(o_ptr->art_flags, TR_LITE_FUEL);
			}
			else if (o_ptr->sval == SV_LITE_LANTERN)
			{
				add_flag(o_ptr->art_flags, TR_LITE_2);
				add_flag(o_ptr->art_flags, TR_LITE_FUEL);	
			}
			else if (o_ptr->sval == SV_LITE_FEANOR)
			{
				add_flag(o_ptr->art_flags, TR_LITE_2);
			}
		}
	}
}


/*
 * Read a monster (Old method)
 */
static void rd_monster_old(monster_type *m_ptr)
{
	byte tmp8u;
	char buf[128];

	/* Read the monster race */
	rd_s16b(&m_ptr->r_idx);

	if (z_older_than(11, 0, 12))
		m_ptr->ap_r_idx = m_ptr->r_idx;
	else
		rd_s16b(&m_ptr->ap_r_idx);

	if (z_older_than(11, 0, 14))
	{
		monster_race *r_ptr = &r_info[m_ptr->r_idx];

		m_ptr->sub_align = SUB_ALIGN_NEUTRAL;
		if (r_ptr->flags3 & RF3_EVIL) m_ptr->sub_align |= SUB_ALIGN_EVIL;
		if (r_ptr->flags3 & RF3_GOOD) m_ptr->sub_align |= SUB_ALIGN_GOOD;
	}
	else
		rd_byte(&m_ptr->sub_align);

	/* Read the other information */
	rd_byte(&m_ptr->fy);
	rd_byte(&m_ptr->fx);
	rd_s16b(&m_ptr->hp);
	rd_s16b(&m_ptr->maxhp);
	if (z_older_than(11, 0, 5))
	{
		m_ptr->max_maxhp = m_ptr->maxhp;
	}
	else
	{
		rd_s16b(&m_ptr->max_maxhp);
	}
	if(h_older_than(2, 1, 2, 1))
	{
		m_ptr->dealt_damage = 0;
	}
	else
	{
		rd_u32b(&m_ptr->dealt_damage); 
	}
	
	rd_s16b(&m_ptr->mtimed[MTIMED_CSLEEP]);
	rd_byte(&m_ptr->mspeed);
	if (z_older_than(10, 4, 2))
	{
		rd_byte(&tmp8u);
		m_ptr->energy_need = (s16b)tmp8u;
	}
	else rd_s16b(&m_ptr->energy_need);

	if (z_older_than(11, 0, 13))
		m_ptr->energy_need = 100 - m_ptr->energy_need;

	if (z_older_than(10,0,7))
	{
		m_ptr->mtimed[MTIMED_FAST] = 0;
		m_ptr->mtimed[MTIMED_SLOW] = 0;
	}
	else
	{
		rd_byte(&tmp8u);
		m_ptr->mtimed[MTIMED_FAST] = (s16b)tmp8u;
		rd_byte(&tmp8u);
		m_ptr->mtimed[MTIMED_SLOW] = (s16b)tmp8u;
	}
	rd_byte(&tmp8u);
	m_ptr->mtimed[MTIMED_STUNNED] = (s16b)tmp8u;
	rd_byte(&tmp8u);
	m_ptr->mtimed[MTIMED_CONFUSED] = (s16b)tmp8u;
	rd_byte(&tmp8u);
	m_ptr->mtimed[MTIMED_MONFEAR] = (s16b)tmp8u;

	if (z_older_than(10,0,10))
	{
		reset_target(m_ptr);
	}
	else if (z_older_than(10,0,11))
	{
		s16b tmp16s;
		rd_s16b(&tmp16s);
		reset_target(m_ptr);
	}
	else
	{
		rd_s16b(&m_ptr->target_y);
		rd_s16b(&m_ptr->target_x);
	}

	rd_byte(&tmp8u);
	m_ptr->mtimed[MTIMED_INVULNER] = (s16b)tmp8u;

	if (!(z_major == 2 && z_minor == 0 && z_patch == 6))
		rd_u32b(&m_ptr->smart);
	else
		m_ptr->smart = 0;

	if (z_older_than(10, 4, 5))
		m_ptr->exp = 0;
	else
		rd_u32b(&m_ptr->exp);

	if (z_older_than(10, 2, 2))
	{
		if (m_ptr->r_idx < 0)
		{
			m_ptr->r_idx = (0-m_ptr->r_idx);
			m_ptr->mflag2 |= MFLAG2_KAGE;
		}
	}
	else
	{
		rd_byte(&m_ptr->mflag2);
	}

	if (z_older_than(11, 0, 12))
	{
		if (m_ptr->mflag2 & MFLAG2_KAGE)
			m_ptr->ap_r_idx = MON_KAGE;
	}

	if (z_older_than(10, 1, 3))
	{
		m_ptr->nickname = 0;
	}
	else
	{
		rd_string(buf, sizeof(buf));
		if (buf[0]) m_ptr->nickname = quark_add(buf);
	}

	rd_byte(&tmp8u);
}


/*
 * Read a monster (New method)
 */
static void rd_monster(monster_type *m_ptr)
{
	u32b flags;
	char buf[128];
	byte tmp8u;

	if (h_older_than(1, 5, 0, 0))
	{
		rd_monster_old(m_ptr);
		return;
	}

	/*** Monster save flags ***/
	rd_u32b(&flags);

	/*** Read un-obvious elements ***/

	/* Read the monster race */
	rd_s16b(&m_ptr->r_idx);

	/* Read the other information */
	rd_byte(&m_ptr->fy);
	rd_byte(&m_ptr->fx);
	rd_s16b(&m_ptr->hp);
	rd_s16b(&m_ptr->maxhp);
	rd_s16b(&m_ptr->max_maxhp);
	if(h_older_than(2, 1, 2, 1))
	{
		m_ptr->dealt_damage = 0;
	}
	else
	{
		rd_u32b(&m_ptr->dealt_damage); 
	}

	/* Monster race index of its appearance */
	if (flags & SAVE_MON_AP_R_IDX) rd_s16b(&m_ptr->ap_r_idx);
	else m_ptr->ap_r_idx = m_ptr->r_idx;

	if (flags & SAVE_MON_SUB_ALIGN) rd_byte(&m_ptr->sub_align);
	else m_ptr->sub_align = 0;

	if (flags & SAVE_MON_CSLEEP) rd_s16b(&m_ptr->mtimed[MTIMED_CSLEEP]);
	else m_ptr->mtimed[MTIMED_CSLEEP] = 0;

	rd_byte(&m_ptr->mspeed);

	rd_s16b(&m_ptr->energy_need);

	if (flags & SAVE_MON_FAST)
	{
		rd_byte(&tmp8u);
		m_ptr->mtimed[MTIMED_FAST] = (s16b)tmp8u;
	}
	else m_ptr->mtimed[MTIMED_FAST] = 0;
	if (flags & SAVE_MON_SLOW)
	{
		rd_byte(&tmp8u);
		m_ptr->mtimed[MTIMED_SLOW] = (s16b)tmp8u;
	}
	else m_ptr->mtimed[MTIMED_SLOW] = 0;
	if (flags & SAVE_MON_STUNNED)
	{
		rd_byte(&tmp8u);
		m_ptr->mtimed[MTIMED_STUNNED] = (s16b)tmp8u;
	}
	else m_ptr->mtimed[MTIMED_STUNNED] = 0;
	if (flags & SAVE_MON_CONFUSED)
	{
		rd_byte(&tmp8u);
		m_ptr->mtimed[MTIMED_CONFUSED] = (s16b)tmp8u;
	}
	else m_ptr->mtimed[MTIMED_CONFUSED] = 0;
	if (flags & SAVE_MON_MONFEAR)
	{
		rd_byte(&tmp8u);
		m_ptr->mtimed[MTIMED_MONFEAR] = (s16b)tmp8u;
	}
	else m_ptr->mtimed[MTIMED_MONFEAR] = 0;

	if (flags & SAVE_MON_TARGET_Y) rd_s16b(&m_ptr->target_y);
	else m_ptr->target_y = 0;
	if (flags & SAVE_MON_TARGET_X) rd_s16b(&m_ptr->target_x);
	else m_ptr->target_x = 0;

	if (flags & SAVE_MON_INVULNER)
	{
		rd_byte(&tmp8u);
		m_ptr->mtimed[MTIMED_INVULNER] = (s16b)tmp8u;
	}
	else m_ptr->mtimed[MTIMED_INVULNER] = 0;

	if (flags & SAVE_MON_SMART) rd_u32b(&m_ptr->smart);
	else m_ptr->smart = 0;

	if (flags & SAVE_MON_EXP) rd_u32b(&m_ptr->exp);
	else m_ptr->exp = 0;

	m_ptr->mflag = 0; /* Not saved */

	if (flags & SAVE_MON_MFLAG2) rd_byte(&m_ptr->mflag2);
	else m_ptr->mflag2 = 0;

	if (flags & SAVE_MON_NICKNAME) 
	{
		rd_string(buf, sizeof(buf));
		m_ptr->nickname = quark_add(buf);
	}
	else m_ptr->nickname = 0;

	if (flags & SAVE_MON_PARENT) rd_s16b(&m_ptr->parent_m_idx);
	else m_ptr->parent_m_idx = 0;
}


/*
 * Old monster bit flags of racial resistances
 */
#define RF3_IM_ACID         0x00010000  /* Resist acid a lot */
#define RF3_IM_ELEC         0x00020000  /* Resist elec a lot */
#define RF3_IM_FIRE         0x00040000  /* Resist fire a lot */
#define RF3_IM_COLD         0x00080000  /* Resist cold a lot */
#define RF3_IM_POIS         0x00100000  /* Resist poison a lot */
#define RF3_RES_TELE        0x00200000  /* Resist teleportation */
#define RF3_RES_NETH        0x00400000  /* Resist nether a lot */
#define RF3_RES_WATE        0x00800000  /* Resist water */
#define RF3_RES_PLAS        0x01000000  /* Resist plasma */
#define RF3_RES_NEXU        0x02000000  /* Resist nexus */
#define RF3_RES_DISE        0x04000000  /* Resist disenchantment */
#define RF3_RES_ALL         0x08000000  /* Resist all */

#define MOVE_RF3_TO_RFR(R_PTR,RF3,RFR) \
{\
	if ((R_PTR)->r_flags3 & (RF3)) \
	{ \
		(R_PTR)->r_flags3 &= ~(RF3); \
		(R_PTR)->r_flagsr |= (RFR); \
	} \
}

#define RF4_BR_TO_RFR(R_PTR,RF4_BR,RFR) \
{\
	if ((R_PTR)->r_flags4 & (RF4_BR)) \
	{ \
		(R_PTR)->r_flagsr |= (RFR); \
	} \
}

#define RF4_BR_LITE         0x00004000  /* Breathe Lite */
#define RF4_BR_DARK         0x00008000  /* Breathe Dark */
#define RF4_BR_CONF         0x00010000  /* Breathe Confusion */
#define RF4_BR_SOUN         0x00020000  /* Breathe Sound */
#define RF4_BR_CHAO         0x00040000  /* Breathe Chaos */
#define RF4_BR_TIME         0x00200000  /* Breathe Time */
#define RF4_BR_INER         0x00400000  /* Breathe Inertia */
#define RF4_BR_GRAV         0x00800000  /* Breathe Gravity */
#define RF4_BR_SHAR         0x01000000  /* Breathe Shards */
#define RF4_BR_WALL         0x04000000  /* Breathe Force */
/*
 * Read the monster lore
 */
static void rd_lore(int r_idx)
{
	byte tmp8u;

	monster_race *r_ptr = &r_info[r_idx];

	/* Count sights/deaths/kills */
	rd_s16b(&r_ptr->r_sights);
	rd_s16b(&r_ptr->r_deaths);
	rd_s16b(&r_ptr->r_pkills);
	if (h_older_than(1, 7, 0, 5))
	{
		r_ptr->r_akills = r_ptr->r_pkills;
	}
	else
	{
		rd_s16b(&r_ptr->r_akills);
	}
	rd_s16b(&r_ptr->r_tkills);

	/* Count wakes and ignores */
	rd_byte(&r_ptr->r_wake);
	rd_byte(&r_ptr->r_ignore);

	/* Extra stuff */
	rd_byte(&r_ptr->r_xtra1);
	rd_byte(&r_ptr->r_xtra2);

	/* Count drops */
	rd_byte(&r_ptr->r_drop_gold);
	rd_byte(&r_ptr->r_drop_item);

	/* Count spells */
	rd_byte(&tmp8u);
	rd_byte(&r_ptr->r_cast_spell);

	/* Count blows of each type */
	rd_byte(&r_ptr->r_blows[0]);
	rd_byte(&r_ptr->r_blows[1]);
	rd_byte(&r_ptr->r_blows[2]);
	rd_byte(&r_ptr->r_blows[3]);

	/* Memorize flags */
	rd_u32b(&r_ptr->r_flags1);
	rd_u32b(&r_ptr->r_flags2);
	rd_u32b(&r_ptr->r_flags3);
	rd_u32b(&r_ptr->r_flags4);
	rd_u32b(&r_ptr->r_flags5);
	rd_u32b(&r_ptr->r_flags6);
	if (h_older_than(1, 5, 0, 3))
	{
		r_ptr->r_flagsr = 0L;

		/* Move RF3 resistance flags to RFR */
		MOVE_RF3_TO_RFR(r_ptr, RF3_IM_ACID,  RFR_IM_ACID);
		MOVE_RF3_TO_RFR(r_ptr, RF3_IM_ELEC,  RFR_IM_ELEC);
		MOVE_RF3_TO_RFR(r_ptr, RF3_IM_FIRE,  RFR_IM_FIRE);
		MOVE_RF3_TO_RFR(r_ptr, RF3_IM_COLD,  RFR_IM_COLD);
		MOVE_RF3_TO_RFR(r_ptr, RF3_IM_POIS,  RFR_IM_POIS);
		MOVE_RF3_TO_RFR(r_ptr, RF3_RES_TELE, RFR_RES_TELE);
		MOVE_RF3_TO_RFR(r_ptr, RF3_RES_NETH, RFR_RES_NETH);
		MOVE_RF3_TO_RFR(r_ptr, RF3_RES_WATE, RFR_RES_WATE);
		MOVE_RF3_TO_RFR(r_ptr, RF3_RES_PLAS, RFR_RES_PLAS);
		MOVE_RF3_TO_RFR(r_ptr, RF3_RES_NEXU, RFR_RES_NEXU);
		MOVE_RF3_TO_RFR(r_ptr, RF3_RES_DISE, RFR_RES_DISE);
		MOVE_RF3_TO_RFR(r_ptr, RF3_RES_ALL,  RFR_RES_ALL);

		/* Separate breathers resistance from RF4 to RFR */
		RF4_BR_TO_RFR(r_ptr, RF4_BR_LITE, RFR_RES_LITE);
		RF4_BR_TO_RFR(r_ptr, RF4_BR_DARK, RFR_RES_DARK);
		RF4_BR_TO_RFR(r_ptr, RF4_BR_SOUN, RFR_RES_SOUN);
		RF4_BR_TO_RFR(r_ptr, RF4_BR_CHAO, RFR_RES_CHAO);
		RF4_BR_TO_RFR(r_ptr, RF4_BR_TIME, RFR_RES_TIME);
		RF4_BR_TO_RFR(r_ptr, RF4_BR_INER, RFR_RES_INER);
		RF4_BR_TO_RFR(r_ptr, RF4_BR_GRAV, RFR_RES_GRAV);
		RF4_BR_TO_RFR(r_ptr, RF4_BR_SHAR, RFR_RES_SHAR);
		RF4_BR_TO_RFR(r_ptr, RF4_BR_WALL, RFR_RES_WALL);

		/* Resist confusion is merged to RF3_NO_CONF */
		if (r_ptr->r_flags4 & RF4_BR_CONF) r_ptr->r_flags3 |= RF3_NO_CONF;

		/* Misc resistance hack to RFR */
		if (r_idx == MON_STORMBRINGER) r_ptr->r_flagsr |= RFR_RES_CHAO;
		if (r_ptr->r_flags3 & RF3_ORC) r_ptr->r_flagsr |= RFR_RES_DARK;
	}
	else
	{
		rd_u32b(&r_ptr->r_flagsr);
	}

	/* Read the "Racial" monster limit per level */
	rd_byte(&r_ptr->max_num);

	/* Location in saved floor */
	rd_s16b(&r_ptr->floor_id);

	/* Later (?) */
	rd_byte(&tmp8u);

	/* Repair the lore flags */
	r_ptr->r_flags1 &= r_ptr->flags1;
	r_ptr->r_flags2 &= r_ptr->flags2;
	r_ptr->r_flags3 &= r_ptr->flags3;
	r_ptr->r_flags4 &= r_ptr->flags4;
	r_ptr->r_flags5 &= r_ptr->flags5;
	r_ptr->r_flags6 &= r_ptr->flags6;
	r_ptr->r_flagsr &= r_ptr->flagsr;
}




/*
 * Add the item "o_ptr" to the inventory of the "Home"
 *
 * In all cases, return the slot (or -1) where the object was placed
 *
 * Note that this is a hacked up version of "inven_carry()".
 *
 * Also note that it may not correctly "adapt" to "knowledge" bacoming
 * known, the player may have to pick stuff up and drop it again.
 */
static void home_carry(store_type *st_ptr, object_type *o_ptr)
{
	int 				slot;
	s32b			   value;
	int 	i;
	object_type *j_ptr;


	/* Check each existing item (try to combine) */
	for (slot = 0; slot < st_ptr->stock_num; slot++)
	{
		/* Get the existing item */
		j_ptr = &st_ptr->stock[slot];

		/* The home acts just like the player */
		if (object_similar(j_ptr, o_ptr))
		{
			/* Save the new number of items */
			object_absorb(j_ptr, o_ptr);

			/* All done */
			return;
		}
	}

	/* No space? */
	if (st_ptr->stock_num >= STORE_INVEN_MAX * 10) {
		return;
	}

	/* Determine the "value" of the item */
	value = object_value(o_ptr);

	/* Check existing slots to see if we must "slide" */
	for (slot = 0; slot < st_ptr->stock_num; slot++)
	{
		if (object_sort_comp(o_ptr, value, &st_ptr->stock[slot])) break;
	}

	/* Slide the others up */
	for (i = st_ptr->stock_num; i > slot; i--)
	{
		st_ptr->stock[i] = st_ptr->stock[i-1];
	}

	/* More stuff now */
	st_ptr->stock_num++;

	/* Insert the new item */
	st_ptr->stock[slot] = *o_ptr;

	chg_virtue(V_SACRIFICE, -1);

	/* Return the location */
	return;
}


/*
 * Read a store
 */
static errr rd_store(int town_number, int store_number)
{
	store_type *st_ptr;

	int j;

	byte own;
	byte tmp8u;
	s16b num;

	bool sort = FALSE;

	if (z_older_than(10, 3, 3) && (store_number == STORE_HOME))
	{
		st_ptr = &town[1].store[store_number];
		if (st_ptr->stock_num) sort = TRUE;
	}
	else
	{
		st_ptr = &town[town_number].store[store_number];
	}

	/* Read the basic info */
	rd_s32b(&st_ptr->store_open);
	rd_s16b(&st_ptr->insult_cur);
	rd_byte(&own);
	if (z_older_than(11, 0, 4))
	{
		rd_byte(&tmp8u);
		num = tmp8u;
	}
	else
	{
		rd_s16b(&num);
	}
	rd_s16b(&st_ptr->good_buy);
	rd_s16b(&st_ptr->bad_buy);

	/* Read last visit */
	rd_s32b(&st_ptr->last_visit);

	/* Extract the owner (see above) */
	st_ptr->owner = own;

	/* Read the items */
	for (j = 0; j < num; j++)
	{
		object_type forge;
		object_type *q_ptr;

		/* Get local object */
		q_ptr = &forge;

		/* Wipe the object */
		object_wipe(q_ptr);

		/* Read the item */
		rd_item(q_ptr);

		/* Acquire valid items */
		if (st_ptr->stock_num < (store_number == STORE_HOME ? (STORE_INVEN_MAX) * 10 : (store_number == STORE_MUSEUM ? (STORE_INVEN_MAX) * 50 : STORE_INVEN_MAX)))
		{
			int k;
			if (sort)
			{
				home_carry(st_ptr, q_ptr);
			}
			else
			{
				k = st_ptr->stock_num++;

				/* Acquire the item */
				object_copy(&st_ptr->stock[k], q_ptr);
			}
		}
	}

	/* Success */
	return (0);
}



/*
 * Read RNG state (added in 2.8.0)
 */
static void rd_randomizer(void)
{
	int i;

	u16b tmp16u;

	/* Tmp */
	rd_u16b(&tmp16u);

	/* Place */
	rd_u16b(&Rand_place);

	/* State */
	for (i = 0; i < RAND_DEG; i++)
	{
		rd_u32b(&Rand_state[i]);
	}
}



/*
 * Read options (ignore most pre-2.8.0 options)
 *
 * Note that the normal options are now stored as a set of 256 bit flags,
 * plus a set of 256 bit masks to indicate which bit flags were defined
 * at the time the savefile was created.  This will allow new options
 * to be added, and old options to be removed, at any time, without
 * hurting old savefiles.
 *
 * The window options are stored in the same way, but note that each
 * window gets 32 options, and their order is fixed by certain defines.
 */
static void rd_options(void)
{
	int i, n;

	byte b;

	u16b c;

	u32b flag[8];
	u32b mask[8];


	/*** Oops ***/

	/* Ignore old options */
	strip_bytes(16);


	/*** Special info */

	/* Read "delay_factor" */
	rd_byte(&b);
	delay_factor = b;

	/* Read "hitpoint_warn" */
	rd_byte(&b);
	hitpoint_warn = b;

	/* Read "mana_warn" */
	if(h_older_than(1, 7, 0, 0))
	{
		mana_warn=2;
	}
	else 
	{
		rd_byte(&b);
		mana_warn = b;
	}


	/*** Cheating options ***/

	rd_u16b(&c);

	if (c & 0x0002) p_ptr->wizard = TRUE;

	cheat_peek = (c & 0x0100) ? TRUE : FALSE;
	cheat_hear = (c & 0x0200) ? TRUE : FALSE;
	cheat_room = (c & 0x0400) ? TRUE : FALSE;
	cheat_xtra = (c & 0x0800) ? TRUE : FALSE;
	cheat_know = (c & 0x1000) ? TRUE : FALSE;
	cheat_live = (c & 0x2000) ? TRUE : FALSE;
	cheat_save = (c & 0x4000) ? TRUE : FALSE;

	rd_byte((byte *)&autosave_l);
	rd_byte((byte *)&autosave_t);
	rd_s16b(&autosave_freq);


	/*** Normal Options ***/

	/* Read the option flags */
	for (n = 0; n < 8; n++) rd_u32b(&flag[n]);

	/* Read the option masks */
	for (n = 0; n < 8; n++) rd_u32b(&mask[n]);

	/* Analyze the options */
	for (n = 0; n < 8; n++)
	{
		/* Analyze the options */
		for (i = 0; i < 32; i++)
		{
			/* Process valid flags */
			if (mask[n] & (1L << i))
			{
				/* Process valid flags */
				if (option_mask[n] & (1L << i))
				{
					/* Set */
					if (flag[n] & (1L << i))
					{
						/* Set */
						option_flag[n] |= (1L << i);
					}

					/* Clear */
					else
					{
						/* Clear */
						option_flag[n] &= ~(1L << i);
					}
				}
			}
		}
	}

	if (z_older_than(10, 4, 5))
	{
		if (option_flag[5] & (0x00000001 << 4)) option_flag[5] &= ~(0x00000001 << 4);
		else option_flag[5] |= (0x00000001 << 4);
		if (option_flag[2] & (0x00000001 << 5)) option_flag[2] &= ~(0x00000001 << 5);
		else option_flag[2] |= (0x00000001 << 5);
		if (option_flag[4] & (0x00000001 << 5)) option_flag[4] &= ~(0x00000001 << 5);
		else option_flag[4] |= (0x00000001 << 5);
		if (option_flag[5] & (0x00000001 << 0)) option_flag[5] &= ~(0x00000001 << 0);
		else option_flag[5] |= (0x00000001 << 0);
		if (option_flag[5] & (0x00000001 << 12)) option_flag[5] &= ~(0x00000001 << 12);
		else option_flag[5] |= (0x00000001 << 12);
		if (option_flag[1] & (0x00000001 << 0)) option_flag[1] &= ~(0x00000001 << 0);
		else option_flag[1] |= (0x00000001 << 0);
		if (option_flag[1] & (0x00000001 << 18)) option_flag[1] &= ~(0x00000001 << 18);
		else option_flag[1] |= (0x00000001 << 18);
		if (option_flag[1] & (0x00000001 << 19)) option_flag[1] &= ~(0x00000001 << 19);
		else option_flag[1] |= (0x00000001 << 19);
		if (option_flag[5] & (0x00000001 << 3)) option_flag[1] &= ~(0x00000001 << 3);
		else option_flag[5] |= (0x00000001 << 3);
	}

	/* Extract the options */
	extract_option_vars();


	/*** Window Options ***/

	/* Read the window flags */
	for (n = 0; n < 8; n++) rd_u32b(&flag[n]);

	/* Read the window masks */
	for (n = 0; n < 8; n++) rd_u32b(&mask[n]);

	/* Analyze the options */
	for (n = 0; n < 8; n++)
	{
		/* Analyze the options */
		for (i = 0; i < 32; i++)
		{
			/* Process valid flags */
			if (mask[n] & (1L << i))
			{
				/* Process valid flags */
				if (window_mask[n] & (1L << i))
				{
					/* Set */
					if (flag[n] & (1L << i))
					{
						/* Set */
						window_flag[n] |= (1L << i);
					}

					/* Clear */
					else
					{
						/* Clear */
						window_flag[n] &= ~(1L << i);
					}
				}
			}
		}
	}
}





/*
 * Hack -- strip the "ghost" info
 *
 * XXX XXX XXX This is such a nasty hack it hurts.
 */
static void rd_ghost(void)
{
	char buf[64];

	/* Strip name */
	rd_string(buf, sizeof(buf));

	/* Strip old data */
	strip_bytes(60);
}


/*
 * Save quick start data
 */
static void load_quick_start(void)
{
	byte tmp8u;
	int i;

	if (z_older_than(11, 0, 13))
	{
		previous_char.quick_ok = FALSE;
		return;
	}

	rd_byte(&previous_char.psex);
	rd_byte(&previous_char.prace);
	rd_byte(&previous_char.pclass);
	rd_byte(&previous_char.pseikaku);
	rd_byte(&previous_char.realm1);
	rd_byte(&previous_char.realm2);

	rd_s16b(&previous_char.age);
	rd_s16b(&previous_char.ht);
	rd_s16b(&previous_char.wt);
	rd_s16b(&previous_char.sc);
	rd_s32b(&previous_char.au);

	for (i = 0; i < 6; i++) rd_s16b(&previous_char.stat_max[i]);
	for (i = 0; i < 6; i++) rd_s16b(&previous_char.stat_max_max[i]);

	for (i = 0; i < PY_MAX_LEVEL; i++) rd_s16b(&previous_char.player_hp[i]);

	rd_s16b(&previous_char.chaos_patron);

	for (i = 0; i < 8; i++) rd_s16b(&previous_char.vir_types[i]);

	for (i = 0; i < 4; i++) rd_string(previous_char.history[i], sizeof(previous_char.history[i]));

	/* UNUSED : Was number of random quests */
	rd_byte(&tmp8u);

	rd_byte(&tmp8u);
	previous_char.quick_ok = (bool)tmp8u;
}

/*
 * Read the "extra" information
 */
static void rd_extra(void)
{
	int i,j;

	byte tmp8u;
	s16b tmp16s;
	u16b tmp16u;

	rd_string(player_name, sizeof(player_name));

	rd_string(p_ptr->died_from, sizeof(p_ptr->died_from));

	if (!h_older_than(1, 7, 0, 1))
	{
		char buf[1024];

		/* Read the message */
		rd_string(buf, sizeof buf);
		if (buf[0]) p_ptr->last_message = string_make(buf);
	}

	load_quick_start();

	for (i = 0; i < 4; i++)
	{
		rd_string(p_ptr->history[i], sizeof(p_ptr->history[i]));
	}

	/* Class/Race/Seikaku/Gender/Spells */
	rd_byte(&p_ptr->prace);
	rd_byte(&p_ptr->pclass);
	rd_byte(&p_ptr->pseikaku);
	rd_byte(&p_ptr->psex);
	rd_byte(&p_ptr->realm1);
	rd_byte(&p_ptr->realm2);
	rd_byte(&tmp8u); /* oops */

	if (z_older_than(10, 4, 4))
	{
		if (p_ptr->realm1 == 9) p_ptr->realm1 = REALM_MUSIC;
		if (p_ptr->realm2 == 9) p_ptr->realm2 = REALM_MUSIC;
		if (p_ptr->realm1 == 10) p_ptr->realm1 = REALM_HISSATSU;
		if (p_ptr->realm2 == 10) p_ptr->realm2 = REALM_HISSATSU;
	}

	/* Special Race/Class info */
	rd_byte(&p_ptr->hitdie);
	rd_u16b(&p_ptr->expfact);

	/* Age/Height/Weight */
	rd_s16b(&p_ptr->age);
	rd_s16b(&p_ptr->ht);
	rd_s16b(&p_ptr->wt);

	/* Read the stat info */
	for (i = 0; i < 6; i++) rd_s16b(&p_ptr->stat_max[i]);
	for (i = 0; i < 6; i++) rd_s16b(&p_ptr->stat_max_max[i]);
	for (i = 0; i < 6; i++) rd_s16b(&p_ptr->stat_cur[i]);

	strip_bytes(24); /* oops */

	rd_s32b(&p_ptr->au);

	rd_s32b(&p_ptr->max_exp);
	if (h_older_than(1, 5, 4, 1)) p_ptr->max_max_exp = p_ptr->max_exp;
	else rd_s32b(&p_ptr->max_max_exp);
	rd_s32b(&p_ptr->exp);

	if (h_older_than(1, 7, 0, 3))
	{
		rd_u16b(&tmp16u);
		p_ptr->exp_frac = (u32b)tmp16u;
	}
	else
	{
		rd_u32b(&p_ptr->exp_frac);
	}

	rd_s16b(&p_ptr->lev);

	for (i = 0; i < 64; i++) rd_s16b(&p_ptr->spell_exp[i]);
	if ((p_ptr->pclass == CLASS_SORCERER) && z_older_than(10, 4, 2))
	{
		for (i = 0; i < 64; i++) p_ptr->spell_exp[i] = SPELL_EXP_MASTER;
	}
	if (z_older_than(10, 3, 6))
		for (i = 0; i < 5; i++) for (j = 0; j < 60; j++) rd_s16b(&p_ptr->weapon_exp[i][j]);
	else
		for (i = 0; i < 5; i++) for (j = 0; j < 64; j++) rd_s16b(&p_ptr->weapon_exp[i][j]);
	for (i = 0; i < 10; i++) rd_s16b(&p_ptr->skill_exp[i]);
	if (z_older_than(10, 4, 1))
	{
		if (p_ptr->pclass != CLASS_BEASTMASTER) p_ptr->skill_exp[GINOU_RIDING] /= 2;
		p_ptr->skill_exp[GINOU_RIDING] = MIN(p_ptr->skill_exp[GINOU_RIDING], s_info[p_ptr->pclass].s_max[GINOU_RIDING]);
	}
	if (z_older_than(10, 3, 14))
	{
		for (i = 0; i < 108; i++) p_ptr->magic_num1[i] = 0;
		for (i = 0; i < 108; i++) p_ptr->magic_num2[i] = 0;
	}
	else
	{
		for (i = 0; i < 108; i++) rd_s32b(&p_ptr->magic_num1[i]);
		for (i = 0; i < 108; i++) rd_byte(&p_ptr->magic_num2[i]);
		if (h_older_than(1, 3, 0, 1))
		{
			if (p_ptr->pclass == CLASS_SMITH)
			{
				p_ptr->magic_num1[TR_ES_ATTACK] = p_ptr->magic_num1[96];
				p_ptr->magic_num1[96] = 0;
				p_ptr->magic_num1[TR_ES_AC] = p_ptr->magic_num1[97];
				p_ptr->magic_num1[97] = 0;
			}
		}
	}
	if (music_singing_any()) p_ptr->action = ACTION_SING;

	if (z_older_than(11, 0, 7))
	{
		p_ptr->start_race = p_ptr->prace;
		p_ptr->old_race1 = 0L;
		p_ptr->old_race2 = 0L;
		p_ptr->old_realm = 0;
	}
	else
	{
		rd_byte(&p_ptr->start_race);
		rd_s32b(&p_ptr->old_race1);
		rd_s32b(&p_ptr->old_race2);
		rd_s16b(&p_ptr->old_realm);
	}

	if (z_older_than(10, 0, 1))
	{
		for (i = 0; i < OLD_MAX_MANE; i++)
		{
			p_ptr->mane_spell[i] = -1;
			p_ptr->mane_dam[i] = 0;
		}
		p_ptr->mane_num = 0;
	}
	else if (z_older_than(10, 2, 3))
	{
		for (i = 0; i < OLD_MAX_MANE; i++)
		{
			rd_s16b(&tmp16s);
			rd_s16b(&tmp16s);
		}
		for (i = 0; i < MAX_MANE; i++)
		{
			p_ptr->mane_spell[i] = -1;
			p_ptr->mane_dam[i] = 0;
		}
		rd_s16b(&tmp16s);
		p_ptr->mane_num = 0;
	}
	else
	{
		for (i = 0; i < MAX_MANE; i++)
		{
			rd_s16b(&p_ptr->mane_spell[i]);
			rd_s16b(&p_ptr->mane_dam[i]);
		}
		rd_s16b(&p_ptr->mane_num);
	}

	if (z_older_than(10, 0, 3))
	{
		determine_bounty_uniques();

		for (i = 0; i < MAX_KUBI; i++)
		{
			/* Is this bounty unique already dead? */
			if (!r_info[kubi_r_idx[i]].max_num) kubi_r_idx[i] += 10000;
		}
	}
	else
	{
		for (i = 0; i < MAX_KUBI; i++)
		{
			rd_s16b(&kubi_r_idx[i]);
		}
	}

	if (z_older_than(10, 0, 3))
	{
		battle_monsters();
	}
	else
	{
		for (i = 0; i < 4; i++)
		{
			rd_s16b(&battle_mon[i]);
			if (z_older_than(10, 3, 4))
			{
				rd_s16b(&tmp16s);
				mon_odds[i] = tmp16s;
			}
			else rd_u32b(&mon_odds[i]);
		}
	}

	rd_s16b(&p_ptr->town_num);

	/* Read arena and rewards information */
	rd_s16b(&p_ptr->arena_number);
	if (h_older_than(1, 5, 0, 1))
	{
		/* Arena loser of previous version was marked number 99 */
		if (p_ptr->arena_number >= 99) p_ptr->arena_number = ARENA_DEFEATED_OLD_VER;
	}
	rd_s16b(&tmp16s);
	p_ptr->inside_arena = (bool)tmp16s;
	rd_s16b(&p_ptr->inside_quest);
	if (z_older_than(10, 3, 5)) p_ptr->inside_battle = FALSE;
	else
	{
		rd_s16b(&tmp16s);
		p_ptr->inside_battle = (bool)tmp16s;
	}
	rd_byte(&p_ptr->exit_bldg);
	rd_byte(&tmp8u);

	rd_s16b(&p_ptr->oldpx);
	rd_s16b(&p_ptr->oldpy);
	if (z_older_than(10, 3, 13) && !dun_level && !p_ptr->inside_arena) {p_ptr->oldpy = 33;p_ptr->oldpx = 131;}

	/* Was p_ptr->rewards[MAX_BACT] */
	rd_s16b(&tmp16s);
	for (i = 0; i < tmp16s; i++)
	{
		s16b tmp16s2;
		rd_s16b(&tmp16s2);
	}

	if (h_older_than(1, 7, 0, 3))
	{
		rd_s16b(&tmp16s);
		p_ptr->mhp = tmp16s;

		rd_s16b(&tmp16s);
		p_ptr->chp = tmp16s;

		rd_u16b(&tmp16u);
		p_ptr->chp_frac = (u32b)tmp16u;
	}
	else
	{
		rd_s32b(&p_ptr->mhp);
		rd_s32b(&p_ptr->chp);
		rd_u32b(&p_ptr->chp_frac);
	}

	if (h_older_than(1, 7, 0, 3))
	{
		rd_s16b(&tmp16s);
		p_ptr->msp = tmp16s;

		rd_s16b(&tmp16s);
		p_ptr->csp = tmp16s;

		rd_u16b(&tmp16u);
		p_ptr->csp_frac = (u32b)tmp16u;
	}
	else
	{
		rd_s32b(&p_ptr->msp);
		rd_s32b(&p_ptr->csp);
		rd_u32b(&p_ptr->csp_frac);
	}

	rd_s16b(&p_ptr->max_plv);
	if (z_older_than(10, 3, 8))
	{
		rd_s16b(&max_dlv[DUNGEON_ANGBAND]);
	}
	else
	{
		byte max = (byte)max_d_idx;

		rd_byte(&max);

		for(i = 0; i < max; i++)
		{
			rd_s16b(&max_dlv[i]);
			if (max_dlv[i] > d_info[i].maxdepth) max_dlv[i] = d_info[i].maxdepth;
		}
	}

	/* Repair maximum player level XXX XXX XXX */
	if (p_ptr->max_plv < p_ptr->lev) p_ptr->max_plv = p_ptr->lev;

	/* More info */
	strip_bytes(8);
	rd_s16b(&p_ptr->sc);
	rd_s16b(&p_ptr->concent);

	/* Read the flags */
	strip_bytes(2); /* Old "rest" */
	rd_s16b(&p_ptr->blind);
	rd_s16b(&p_ptr->paralyzed);
	rd_s16b(&p_ptr->confused);
	rd_s16b(&p_ptr->food);
	strip_bytes(4); /* Old "food_digested" / "protection" */

	rd_s16b(&p_ptr->energy_need);
	if (z_older_than(11, 0, 13))
		p_ptr->energy_need = 100 - p_ptr->energy_need;
	if (h_older_than(2, 1, 2, 0))
		p_ptr->enchant_energy_need = 0;
	else
		rd_s16b(&p_ptr->enchant_energy_need);

	rd_s16b(&p_ptr->fast);
	rd_s16b(&p_ptr->slow);
	rd_s16b(&p_ptr->afraid);
	rd_s16b(&p_ptr->cut);
	rd_s16b(&p_ptr->stun);
	rd_s16b(&p_ptr->poisoned);
	rd_s16b(&p_ptr->image);
	rd_s16b(&p_ptr->protevil);
	rd_s16b(&p_ptr->invuln);
	if(z_older_than(10, 0, 0))
		p_ptr->ult_res = 0;
	else
		rd_s16b(&p_ptr->ult_res);
	rd_s16b(&p_ptr->hero);
	rd_s16b(&p_ptr->shero);
	rd_s16b(&p_ptr->shield);
	rd_s16b(&p_ptr->blessed);
	rd_s16b(&p_ptr->tim_invis);
	rd_s16b(&p_ptr->word_recall);
	if (z_older_than(10, 3, 8))
		p_ptr->recall_dungeon = DUNGEON_ANGBAND;
	else
	{
		rd_s16b(&tmp16s);
		p_ptr->recall_dungeon = (byte)tmp16s;
	}

	if (h_older_than(1, 5, 0, 0))
		p_ptr->alter_reality = 0;
	else
		rd_s16b(&p_ptr->alter_reality);

	rd_s16b(&p_ptr->see_infra);
	rd_s16b(&p_ptr->tim_infra);
	rd_s16b(&p_ptr->oppose_fire);
	rd_s16b(&p_ptr->oppose_cold);
	rd_s16b(&p_ptr->oppose_acid);
	rd_s16b(&p_ptr->oppose_elec);
	rd_s16b(&p_ptr->oppose_pois);
	if (z_older_than(10,0,2)) p_ptr->tsuyoshi = 0;
	else rd_s16b(&p_ptr->tsuyoshi);

	/* Old savefiles do not have the following fields... */
	if ((z_major == 2) && (z_minor == 0) && (z_patch == 6))
	{
		p_ptr->tim_esp = 0;
		p_ptr->wraith_form = 0;
		p_ptr->resist_magic = 0;
		p_ptr->tim_regen = 0;
		p_ptr->kabenuke = 0;
		p_ptr->tim_stealth = 0;
		p_ptr->tim_levitation = 0;
		p_ptr->tim_sh_touki = 0;
		p_ptr->lightspeed = 0;
		p_ptr->tsubureru = 0;
		p_ptr->tim_res_nether = 0;
		p_ptr->tim_res_time = 0;
		p_ptr->mimic_form = 0;
		p_ptr->tim_mimic = 0;
		p_ptr->tim_sh_fire = 0;

		/* by henkma */
		p_ptr->tim_reflect = 0;
		p_ptr->multishadow = 0;
		p_ptr->dustrobe = 0;

		p_ptr->chaos_patron = ((p_ptr->age + p_ptr->sc) % MAX_PATRON);
		p_ptr->muta1 = 0;
		p_ptr->muta2 = 0;
		p_ptr->muta3 = 0;
		get_virtues();
	}
	else
	{
		rd_s16b(&p_ptr->tim_esp);
		rd_s16b(&p_ptr->wraith_form);
		rd_s16b(&p_ptr->resist_magic);
		rd_s16b(&p_ptr->tim_regen);
		rd_s16b(&p_ptr->kabenuke);
		rd_s16b(&p_ptr->tim_stealth);
		rd_s16b(&p_ptr->tim_levitation);
		rd_s16b(&p_ptr->tim_sh_touki);
		rd_s16b(&p_ptr->lightspeed);
		rd_s16b(&p_ptr->tsubureru);
		if (z_older_than(10, 4, 7))
			p_ptr->magicdef = 0;
		else
			rd_s16b(&p_ptr->magicdef);
		rd_s16b(&p_ptr->tim_res_nether);
		if (z_older_than(10, 4, 11))
		{
			p_ptr->tim_res_time = 0;
			p_ptr->mimic_form = 0;
			p_ptr->tim_mimic = 0;
			p_ptr->tim_sh_fire = 0;
		}
		else
		{
			rd_s16b(&p_ptr->tim_res_time);
			rd_byte(&p_ptr->mimic_form);
			rd_s16b(&p_ptr->tim_mimic);
			rd_s16b(&p_ptr->tim_sh_fire);
		}

		if (z_older_than(11, 0, 99))
		{
			p_ptr->tim_sh_holy = 0;
			p_ptr->tim_eyeeye = 0;
		}
		else
		{
			rd_s16b(&p_ptr->tim_sh_holy);
			rd_s16b(&p_ptr->tim_eyeeye);
		}

		/* by henkma */
		if ( z_older_than(11,0,3) ){
		  p_ptr->tim_reflect=0;
		  p_ptr->multishadow=0;
		  p_ptr->dustrobe=0;
		}
		else {
		  rd_s16b(&p_ptr->tim_reflect);
		  rd_s16b(&p_ptr->multishadow);
		  rd_s16b(&p_ptr->dustrobe);
		}

		rd_s16b(&p_ptr->chaos_patron);
		rd_u32b(&p_ptr->muta1);
		rd_u32b(&p_ptr->muta2);
		rd_u32b(&p_ptr->muta3);

		for (i = 0; i < 8; i++)
			rd_s16b(&p_ptr->virtues[i]);
		for (i = 0; i < 8; i++)
			rd_s16b(&p_ptr->vir_types[i]);
	}

	/* Calc the regeneration modifier for mutations */
	mutant_regenerate_mod = calc_mutant_regenerate_mod();

	if (z_older_than(10,0,9))
	{
		rd_byte(&tmp8u);
		if (tmp8u) p_ptr->special_attack = ATTACK_CONFUSE;
		p_ptr->ele_attack = 0;
	}
	else
	{
		rd_s16b(&p_ptr->ele_attack);
		rd_u32b(&p_ptr->special_attack);
	}
	if (p_ptr->special_attack & KAMAE_MASK) p_ptr->action = ACTION_KAMAE;
	else if (p_ptr->special_attack & KATA_MASK) p_ptr->action = ACTION_KATA;
	if (z_older_than(10,0,12))
	{
		p_ptr->ele_immune = 0;
		p_ptr->special_defense = 0;
	}
	else
	{
		rd_s16b(&p_ptr->ele_immune);
		rd_u32b(&p_ptr->special_defense);
	}
	rd_byte(&p_ptr->knowledge);

	rd_byte(&tmp8u);
	p_ptr->autopick_autoregister = tmp8u ? TRUE : FALSE;

	rd_byte(&tmp8u); /* oops */
	rd_byte(&p_ptr->action);
	if (!z_older_than(10, 4, 3))
	{
		rd_byte(&tmp8u);
		if (tmp8u) p_ptr->action = ACTION_LEARN;
	}
	rd_byte((byte *)&preserve_mode);
	rd_byte((byte *)&p_ptr->wait_report_score);

	/* Future use */
	for (i = 0; i < 48; i++) rd_byte(&tmp8u);

	/* Skip the flags */
	strip_bytes(12);


	/* Hack -- the two "special seeds" */
	rd_u32b(&seed_flavor);
	rd_u32b(&seed_town);


	/* Special stuff */
	rd_u16b(&p_ptr->panic_save);
	rd_u16b(&p_ptr->total_winner);
	rd_u16b(&p_ptr->noscore);


	/* Read "death" */
	rd_byte(&tmp8u);
	p_ptr->is_dead = tmp8u;

	/* Read "feeling" */
	rd_byte(&p_ptr->feeling);

	switch (p_ptr->start_race)
	{
	case RACE_VAMPIRE:
	case RACE_SKELETON:
	case RACE_ZOMBIE:
	case RACE_SPECTRE:
		turn_limit = TURNS_PER_TICK * TOWN_DAWN * MAX_DAYS + TURNS_PER_TICK * TOWN_DAWN * 3 / 4;
		break;
	default:
		turn_limit = TURNS_PER_TICK * TOWN_DAWN * (MAX_DAYS - 1) + TURNS_PER_TICK * TOWN_DAWN * 3 / 4;
		break;
	}
	dungeon_turn_limit = TURNS_PER_TICK * TOWN_DAWN * (MAX_DAYS - 1) + TURNS_PER_TICK * TOWN_DAWN * 3 / 4;

	/* Turn when level began */
	rd_s32b(&old_turn);

	if (h_older_than(1, 7, 0, 4))
	{
		p_ptr->feeling_turn = old_turn;
	}
	else
	{
		/* Turn of last "feeling" */
		rd_s32b(&p_ptr->feeling_turn);
	}

	/* Current turn */
	rd_s32b(&turn);

	if (z_older_than(10, 3, 12))
	{
		dungeon_turn = turn;
	}
	else rd_s32b(&dungeon_turn);

	if (z_older_than(11, 0, 13))
	{
		old_turn /= 2;
		p_ptr->feeling_turn /= 2;
		turn /= 2;
		dungeon_turn /= 2;
	}

	if (z_older_than(10, 3, 13))
	{
		old_battle = turn;
	}
	else rd_s32b(&old_battle);

	if (z_older_than(10,0,3))
	{
		determine_today_mon(TRUE);
	}
	else
	{
		rd_s16b(&today_mon);
		rd_s16b(&p_ptr->today_mon);
	}

	if (z_older_than(10,0,7))
	{
		p_ptr->riding = 0;
	}
	else
	{
		rd_s16b(&p_ptr->riding);
	}

	/* Current floor_id */
	if (h_older_than(1, 5, 0, 0))
	{
		p_ptr->floor_id = 0;
	}
	else
	{
		rd_s16b(&p_ptr->floor_id);
	}

	if (h_older_than(1, 5, 0, 2))
	{
		/* Nothing to do */
	}
	else
	{
		/* Get number of party_mon array */
		rd_s16b(&tmp16s);

		/* Strip old temporary preserved pets */
		for (i = 0; i < tmp16s; i++)
		{
			monster_type dummy_mon;

			rd_monster(&dummy_mon);
		}
	}

	if (z_older_than(10,1,2))
	{
		playtime = 0;
	}
	else
	{
		rd_u32b(&playtime);
	}

	if (z_older_than(10,3,9))
	{
		p_ptr->visit = 1L;
	}
	else if (z_older_than(10, 3, 10))
	{
		s32b tmp32s;
		rd_s32b(&tmp32s);
		p_ptr->visit = 1L;
	}
	else
	{
		rd_s32b(&p_ptr->visit);
	}
	if (!z_older_than(11, 0, 5))
	{
		rd_u32b(&p_ptr->count);
	}
}




/*
 * Read the player inventory
 *
 * Note that the inventory changed in Angband 2.7.4.  Two extra
 * pack slots were added and the equipment was rearranged.  Note
 * that these two features combine when parsing old save-files, in
 * which items from the old "aux" slot are "carried", perhaps into
 * one of the two new "inventory" slots.
 *
 * Note that the inventory is "re-sorted" later by "dungeon()".
 */
static errr rd_inventory(void)
{
	int slot = 0;

	object_type forge;
	object_type *q_ptr;

	/* No weight */
	p_ptr->total_weight = 0;

	/* No items */
	inven_cnt = 0;
	equip_cnt = 0;

	/* Read until done */
	while (1)
	{
		u16b n;

		/* Get the next item index */
		rd_u16b(&n);

		/* Nope, we reached the end */
		if (n == 0xFFFF) break;

		/* Get local object */
		q_ptr = &forge;

		/* Wipe the object */
		object_wipe(q_ptr);

		/* Read the item */
		rd_item(q_ptr);

		/* Hack -- verify item */
		if (!q_ptr->k_idx) return (53);

		/* Wield equipment */
		if (n >= INVEN_RARM)
		{
			/* Player touches it */
			q_ptr->marked |= OM_TOUCHED;

			/* Copy object */
			object_copy(&inventory[n], q_ptr);

			/* Add the weight */
			p_ptr->total_weight += (q_ptr->number * q_ptr->weight);

			/* One more item */
			equip_cnt++;
		}

		/* Warning -- backpack is full */
		else if (inven_cnt == INVEN_PACK)
		{
			/* Oops */
			note(_("持ち物の中のアイテムが多すぎる！", "Too many items in the inventory!"));

			/* Fail */
			return (54);
		}

		/* Carry inventory */
		else
		{
			/* Get a slot */
			n = slot++;

			/* Player touches it */
			q_ptr->marked |= OM_TOUCHED;

			/* Copy object */
			object_copy(&inventory[n], q_ptr);

			/* Add the weight */
			p_ptr->total_weight += (q_ptr->number * q_ptr->weight);

			/* One more item */
			inven_cnt++;
		}
	}

	/* Success */
	return (0);
}



/*
 * Read the saved messages
 */
static void rd_messages(void)
{
	int i;
	char buf[128];

	s16b num;

	/* Total */
	rd_s16b(&num);

	/* Read the messages */
	for (i = 0; i < num; i++)
	{
		/* Read the message */
		rd_string(buf, sizeof(buf));

		/* Save the message */
		message_add(buf);
	}
}



/* Old hidden trap flag */
#define CAVE_TRAP       0x8000

/*** Terrain Feature Indexes (see "lib/edit/f_info.txt") ***/
#define OLD_FEAT_INVIS              0x02
#define OLD_FEAT_GLYPH              0x03
#define OLD_FEAT_QUEST_ENTER        0x08
#define OLD_FEAT_QUEST_EXIT         0x09
#define OLD_FEAT_MINOR_GLYPH        0x40
#define OLD_FEAT_BLDG_1             0x81
#define OLD_FEAT_MIRROR             0xc3

/* Old quests */
#define OLD_QUEST_WATER_CAVE 18

/* Quest constants */
#define QUEST_OLD_CASTLE  27
#define QUEST_ROYAL_CRYPT 28

/*
 * Read the dungeon (old method)
 *
 * The monsters/objects must be loaded in the same order
 * that they were stored, since the actual indexes matter.
 */
static errr rd_dungeon_old(void)
{
	int i, y, x;
	int ymax, xmax;
	byte count;
	byte tmp8u;
	s16b tmp16s;
	u16b limit;
	cave_type *c_ptr;


	/*** Basic info ***/

	/* Header info */
	rd_s16b(&dun_level);
	if (z_older_than(10, 3, 8)) dungeon_type = DUNGEON_ANGBAND;
	else rd_byte(&dungeon_type);

	/* Set the base level for old versions */
	base_level = dun_level;

	rd_s16b(&base_level);

	rd_s16b(&num_repro);
	rd_s16b(&tmp16s);
	py = (int)tmp16s;
	rd_s16b(&tmp16s);
	px = (int)tmp16s;
	if (z_older_than(10, 3, 13) && !dun_level && !p_ptr->inside_arena) {py = 33;px = 131;}
	rd_s16b(&cur_hgt);
	rd_s16b(&cur_wid);
	rd_s16b(&tmp16s); /* max_panel_rows */
	rd_s16b(&tmp16s); /* max_panel_cols */

#if 0
	if (!py || !px) {py = 10;px = 10;}/* ダンジョン生成に失敗してセグメンテったときの復旧用 */
#endif

	/* Maximal size */
	ymax = cur_hgt;
	xmax = cur_wid;


	/*** Run length decoding ***/

	/* Load the dungeon data */
	for (x = y = 0; y < ymax; )
	{
		u16b info;

		/* Grab RLE info */
		rd_byte(&count);
		if (z_older_than(10,3,6))
		{
			rd_byte(&tmp8u);
			info = (u16b)tmp8u;
		}
		else
		{
			rd_u16b(&info);

			/* Decline invalid flags */
			info &= ~(CAVE_LITE | CAVE_VIEW | CAVE_MNLT | CAVE_MNDK);
		}

		/* Apply the RLE info */
		for (i = count; i > 0; i--)
		{
			/* Access the cave */
			c_ptr = &cave[y][x];

			/* Extract "info" */
			c_ptr->info = info;

			/* Advance/Wrap */
			if (++x >= xmax)
			{
				/* Wrap */
				x = 0;

				/* Advance/Wrap */
				if (++y >= ymax) break;
			}
		}
	}


	/*** Run length decoding ***/

	/* Load the dungeon data */
	for (x = y = 0; y < ymax; )
	{
		/* Grab RLE info */
		rd_byte(&count);
		rd_byte(&tmp8u);

		/* Apply the RLE info */
		for (i = count; i > 0; i--)
		{
			/* Access the cave */
			c_ptr = &cave[y][x];

			/* Extract "feat" */
			c_ptr->feat = (s16b)tmp8u;

			/* Advance/Wrap */
			if (++x >= xmax)
			{
				/* Wrap */
				x = 0;

				/* Advance/Wrap */
				if (++y >= ymax) break;
			}
		}
	}

	/*** Run length decoding ***/

	/* Load the dungeon data */
	for (x = y = 0; y < ymax; )
	{
		/* Grab RLE info */
		rd_byte(&count);
		rd_byte(&tmp8u);

		/* Apply the RLE info */
		for (i = count; i > 0; i--)
		{
			/* Access the cave */
			c_ptr = &cave[y][x];

			/* Extract "mimic" */
			c_ptr->mimic = (s16b)tmp8u;

			/* Advance/Wrap */
			if (++x >= xmax)
			{
				/* Wrap */
				x = 0;

				/* Advance/Wrap */
				if (++y >= ymax) break;
			}
		}
	}

	/*** Run length decoding ***/

	/* Load the dungeon data */
	for (x = y = 0; y < ymax; )
	{
		/* Grab RLE info */
		rd_byte(&count);
		rd_s16b(&tmp16s);

		/* Apply the RLE info */
		for (i = count; i > 0; i--)
		{
			/* Access the cave */
			c_ptr = &cave[y][x];

			/* Extract "feat" */
			c_ptr->special = tmp16s;

			/* Advance/Wrap */
			if (++x >= xmax)
			{
				/* Wrap */
				x = 0;

				/* Advance/Wrap */
				if (++y >= ymax) break;
			}
		}
	}

	/* Convert cave data */
	if (z_older_than(11, 0, 99))
	{
		for (y = 0; y < ymax; y++) for (x = 0; x < xmax; x++)
		{
			/* Wipe old unused flags */
			cave[y][x].info &= ~(CAVE_MASK);
		}
	}

	if (h_older_than(1, 1, 1, 0))
	{
		for (y = 0; y < ymax; y++) for (x = 0; x < xmax; x++)
		{
			/* Access the cave */
			c_ptr = &cave[y][x];

			/* Very old */
			if (c_ptr->feat == OLD_FEAT_INVIS)
			{
				c_ptr->feat = feat_floor;
				c_ptr->info |= CAVE_TRAP;
			}

			/* Older than 1.1.1 */
			if (c_ptr->feat == OLD_FEAT_MIRROR)
			{
				c_ptr->feat = feat_floor;
				c_ptr->info |= CAVE_OBJECT;
			}
		}
	}

	if (h_older_than(1, 3, 1, 0))
	{
		for (y = 0; y < ymax; y++) for (x = 0; x < xmax; x++)
		{
			/* Access the cave */
			c_ptr = &cave[y][x];

			/* Old CAVE_IN_MIRROR flag */
			if (c_ptr->info & CAVE_OBJECT)
			{
				c_ptr->mimic = feat_mirror;
			}

			/* Runes will be mimics and flags */
			else if ((c_ptr->feat == OLD_FEAT_MINOR_GLYPH) ||
			         (c_ptr->feat == OLD_FEAT_GLYPH))
			{
				c_ptr->info |= CAVE_OBJECT;
				c_ptr->mimic = c_ptr->feat;
				c_ptr->feat = feat_floor;
			}

			/* Hidden traps will be trap terrains mimicing floor */
			else if (c_ptr->info & CAVE_TRAP)
			{
				c_ptr->info &= ~CAVE_TRAP;
				c_ptr->mimic = c_ptr->feat;
				c_ptr->feat = choose_random_trap();
			}

			/* Another hidden trap */
			else if (c_ptr->feat == OLD_FEAT_INVIS)
			{
				c_ptr->mimic = feat_floor;
				c_ptr->feat = feat_trap_open;
			}
		}
	}

	/* Quest 18 was removed */
	if (h_older_than(1, 7, 0, 6) && !vanilla_town)
	{
		for (y = 0; y < ymax; y++) for (x = 0; x < xmax; x++)
		{
			/* Access the cave */
			c_ptr = &cave[y][x];

			if ((c_ptr->special == OLD_QUEST_WATER_CAVE) && !dun_level)
			{
				if (c_ptr->feat == OLD_FEAT_QUEST_ENTER)
				{
					c_ptr->feat = feat_tree;
					c_ptr->special = 0;
				}
				else if (c_ptr->feat == OLD_FEAT_BLDG_1)
				{
					c_ptr->special = lite_town ? QUEST_OLD_CASTLE : QUEST_ROYAL_CRYPT;
				}
			}
			else if ((c_ptr->feat == OLD_FEAT_QUEST_EXIT) &&
			         (p_ptr->inside_quest == OLD_QUEST_WATER_CAVE))
			{
				c_ptr->feat = feat_up_stair;
				c_ptr->special = 0;
			}
		}
	}

	/*** Objects ***/

	/* Read the item count */
	rd_u16b(&limit);

	/* Verify maximum */
	if (limit > max_o_idx)
	{
		note(format(_("アイテムの配列が大きすぎる(%d)！", "Too many (%d) object entries!"), limit));
		return (151);
	}

	/* Read the dungeon items */
	for (i = 1; i < limit; i++)
	{
		int o_idx;

		object_type *o_ptr;


		/* Get a new record */
		o_idx = o_pop();

		/* Oops */
		if (i != o_idx)
		{
			note(format(_("アイテム配置エラー (%d <> %d)", "Object allocation error (%d <> %d)"), i, o_idx));
			return (152);
		}


		/* Acquire place */
		o_ptr = &o_list[o_idx];

		/* Read the item */
		rd_item(o_ptr);


		/* XXX XXX XXX XXX XXX */

		/* Monster */
		if (o_ptr->held_m_idx)
		{
			monster_type *m_ptr;

			/* Monster */
			m_ptr = &m_list[o_ptr->held_m_idx];

			/* Build a stack */
			o_ptr->next_o_idx = m_ptr->hold_o_idx;

			/* Place the object */
			m_ptr->hold_o_idx = o_idx;
		}

		/* Dungeon */
		else
		{
			/* Access the item location */
			c_ptr = &cave[o_ptr->iy][o_ptr->ix];

			/* Build a stack */
			o_ptr->next_o_idx = c_ptr->o_idx;

			/* Place the object */
			c_ptr->o_idx = o_idx;
		}
	}


	/*** Monsters ***/

	/* Read the monster count */
	rd_u16b(&limit);

	/* Hack -- verify */
	if (limit > max_m_idx)
	{
		note(format(_("モンスターの配列が大きすぎる(%d)！", "Too many (%d) monster entries!"), limit));
		return (161);
	}

	/* Read the monsters */
	for (i = 1; i < limit; i++)
	{
		int m_idx;
		monster_type *m_ptr;

		/* Get a new record */
		m_idx = m_pop();

		/* Oops */
		if (i != m_idx)
		{
			note(format(_("モンスター配置エラー (%d <> %d)", "Monster allocation error (%d <> %d)"), i, m_idx));
			return (162);
		}


		/* Acquire monster */
		m_ptr = &m_list[m_idx];

		/* Read the monster */
		rd_monster(m_ptr);


		/* Access grid */
		c_ptr = &cave[m_ptr->fy][m_ptr->fx];

		/* Mark the location */
		c_ptr->m_idx = m_idx;

		/* Count */
		real_r_ptr(m_ptr)->cur_num++;
	}

	/*** Success ***/

	/* The dungeon is ready */
	if (z_older_than(10, 3, 13) && !dun_level && !p_ptr->inside_arena)
		character_dungeon = FALSE;
	else
		character_dungeon = TRUE;

	/* Success */
	return (0);
}



/*
 * Read the saved floor
 *
 * The monsters/objects must be loaded in the same order
 * that they were stored, since the actual indexes matter.
 */
static errr rd_saved_floor(saved_floor_type *sf_ptr)
{
	int ymax, xmax;
	int i, y, x;
	byte count;
	byte tmp8u;
	s16b tmp16s;
	u16b tmp16u;
	s32b tmp32s;
	u32b tmp32u;
	u16b limit;

	cave_template_type *templates;


	/*** Wipe all cave ***/
	clear_cave();


	/*** Basic info ***/

	/* Dungeon floor specific info follows */

	if (!sf_ptr)
	{
		/*** Not a saved floor ***/

		rd_s16b(&dun_level);
		base_level = dun_level;
	}
	else
	{
		/*** The saved floor ***/

		rd_s16b(&tmp16s);
		if (tmp16s != sf_ptr->floor_id) return 171;

		rd_byte(&tmp8u);
		if (tmp8u != sf_ptr->savefile_id) return 171;

		rd_s16b(&tmp16s);
		if (tmp16s != sf_ptr->dun_level) return 171;
		dun_level = sf_ptr->dun_level;

		rd_s32b(&tmp32s);
		if (tmp32s != sf_ptr->last_visit) return 171;

		rd_u32b(&tmp32u);
		if (tmp32u != sf_ptr->visit_mark) return 171;

		rd_s16b(&tmp16s);
		if (tmp16s != sf_ptr->upper_floor_id) return 171;

		rd_s16b(&tmp16s);
		if (tmp16s != sf_ptr->lower_floor_id) return 171;
	}

	rd_s16b(&base_level);
	rd_s16b(&num_repro);

	rd_u16b(&tmp16u);
	py = (int)tmp16u;

	rd_u16b(&tmp16u);
	px = (int)tmp16u;

	rd_s16b(&cur_hgt);
	rd_s16b(&cur_wid);

	rd_byte(&p_ptr->feeling);



	/*** Read template for cave_type ***/

	/* Read the template count */
	rd_u16b(&limit);

	/* Allocate the "template" array */
	C_MAKE(templates, limit, cave_template_type);

	/* Read the templates */
	for (i = 0; i < limit; i++)
	{
		cave_template_type *ct_ptr = &templates[i];

		/* Read it */
		rd_u16b(&ct_ptr->info);
		if (h_older_than(1, 7, 0, 2))
		{
			rd_byte(&tmp8u);
			ct_ptr->feat = (s16b)tmp8u;
			rd_byte(&tmp8u);
			ct_ptr->mimic = (s16b)tmp8u;
		}
		else
		{
			rd_s16b(&ct_ptr->feat);
			rd_s16b(&ct_ptr->mimic);
		}
		rd_s16b(&ct_ptr->special);
	}

	/* Maximal size */
	ymax = cur_hgt;
	xmax = cur_wid;


	/*** Run length decoding ***/

	/* Load the dungeon data */
	for (x = y = 0; y < ymax; )
	{
		u16b id;

		/* Grab RLE info */
		rd_byte(&count);

		id = 0;
		do 
		{
			rd_byte(&tmp8u);
			id += tmp8u;
		} while (tmp8u == MAX_UCHAR);

		/* Apply the RLE info */
		for (i = count; i > 0; i--)
		{
			/* Access the cave */
			cave_type *c_ptr = &cave[y][x];

			/* Extract cave data */
			c_ptr->info = templates[id].info;
			c_ptr->feat = templates[id].feat;
			c_ptr->mimic = templates[id].mimic;
			c_ptr->special = templates[id].special;

			/* Advance/Wrap */
			if (++x >= xmax)
			{
				/* Wrap */
				x = 0;

				/* Advance/Wrap */
				if (++y >= ymax) break;
			}
		}
	}

	/* Quest 18 was removed */
	if (h_older_than(1, 7, 0, 6) && !vanilla_town)
	{
		for (y = 0; y < ymax; y++) for (x = 0; x < xmax; x++)
		{
			/* Access the cave */
			cave_type *c_ptr = &cave[y][x];

			if ((c_ptr->special == OLD_QUEST_WATER_CAVE) && !dun_level)
			{
				if (c_ptr->feat == OLD_FEAT_QUEST_ENTER)
				{
					c_ptr->feat = feat_tree;
					c_ptr->special = 0;
				}
				else if (c_ptr->feat == OLD_FEAT_BLDG_1)
				{
					c_ptr->special = lite_town ? QUEST_OLD_CASTLE : QUEST_ROYAL_CRYPT;
				}
			}
			else if ((c_ptr->feat == OLD_FEAT_QUEST_EXIT) &&
			         (p_ptr->inside_quest == OLD_QUEST_WATER_CAVE))
			{
				c_ptr->feat = feat_up_stair;
				c_ptr->special = 0;
			}
		}
	}

	/* Free the "template" array */
	C_KILL(templates, limit, cave_template_type);


	/*** Objects ***/

	/* Read the item count */
	rd_u16b(&limit);

	/* Verify maximum */
	if (limit > max_o_idx) return 151;


	/* Read the dungeon items */
	for (i = 1; i < limit; i++)
	{
		int o_idx;
		object_type *o_ptr;


		/* Get a new record */
		o_idx = o_pop();

		/* Oops */
		if (i != o_idx) return 152;

		/* Acquire place */
		o_ptr = &o_list[o_idx];

		/* Read the item */
		rd_item(o_ptr);


		/* Monster */
		if (o_ptr->held_m_idx)
		{
			monster_type *m_ptr;

			/* Monster */
			m_ptr = &m_list[o_ptr->held_m_idx];

			/* Build a stack */
			o_ptr->next_o_idx = m_ptr->hold_o_idx;

			/* Place the object */
			m_ptr->hold_o_idx = o_idx;
		}

		/* Dungeon */
		else
		{
			/* Access the item location */
			cave_type *c_ptr = &cave[o_ptr->iy][o_ptr->ix];

			/* Build a stack */
			o_ptr->next_o_idx = c_ptr->o_idx;

			/* Place the object */
			c_ptr->o_idx = o_idx;
		}
	}


	/*** Monsters ***/

	/* Read the monster count */
	rd_u16b(&limit);

	/* Hack -- verify */
	if (limit > max_m_idx) return 161;

	/* Read the monsters */
	for (i = 1; i < limit; i++)
	{
		cave_type *c_ptr;
		int m_idx;
		monster_type *m_ptr;

		/* Get a new record */
		m_idx = m_pop();

		/* Oops */
		if (i != m_idx) return 162;


		/* Acquire monster */
		m_ptr = &m_list[m_idx];

		/* Read the monster */
		rd_monster(m_ptr);


		/* Access grid */
		c_ptr = &cave[m_ptr->fy][m_ptr->fx];

		/* Mark the location */
		c_ptr->m_idx = m_idx;

		/* Count */
		real_r_ptr(m_ptr)->cur_num++;
	}

	/* Success */
	return 0;
}


/*
 * Read the dungeon (new method)
 *
 * The monsters/objects must be loaded in the same order
 * that they were stored, since the actual indexes matter.
 */
static errr rd_dungeon(void)
{
	errr err = 0;
	byte num;
	int i;

	/* Initialize saved_floors array and temporal files */
	init_saved_floors(FALSE);

	/* Older method */
	if (h_older_than(1, 5, 0, 0))
	{
		err = rd_dungeon_old();

		/* Prepare floor_id of current floor */
		if (dungeon_type)
		{
			p_ptr->floor_id = get_new_floor_id();
			get_sf_ptr(p_ptr->floor_id)->dun_level = dun_level;
		}

		return err;
	}


	/*** Meta info ***/

        /* Number of floor_id used from birth */
	rd_s16b(&max_floor_id);

	/* Current dungeon type */
	rd_byte(&dungeon_type);


	/* Number of the saved_floors array elements */
	rd_byte(&num);

	/*** No saved floor (On the surface etc.) ***/
	if (!num)
	{
		/* Read the current floor data */
		err = rd_saved_floor(NULL);
	}

	/*** In the dungeon ***/
	else
	{

		/* Read the saved_floors array */
		for (i = 0; i < num; i++)
		{
			saved_floor_type *sf_ptr = &saved_floors[i];

			rd_s16b(&sf_ptr->floor_id);
			rd_byte(&sf_ptr->savefile_id);
			rd_s16b(&sf_ptr->dun_level);
			rd_s32b(&sf_ptr->last_visit);
			rd_u32b(&sf_ptr->visit_mark);
			rd_s16b(&sf_ptr->upper_floor_id);
			rd_s16b(&sf_ptr->lower_floor_id);
		}


		/* Move saved floors data to temporal files */
		for (i = 0; i < num; i++)
		{
			saved_floor_type *sf_ptr = &saved_floors[i];
			byte tmp8u;

			/* Unused element */
			if (!sf_ptr->floor_id) continue;

			/* Read the failure mark */
			rd_byte(&tmp8u);
			if (tmp8u) continue;

			/* Read from the save file */
			err = rd_saved_floor(sf_ptr);

			/* Error? */
			if (err) break;

			/* Re-save as temporal saved floor file */
			if (!save_floor(sf_ptr, SLF_SECOND)) err = 182;

			/* Error? */
			if (err) break;
		}

		/* Finally load current floor data from temporal file */
		if (!err)
		{
			if (!load_floor(get_sf_ptr(p_ptr->floor_id), SLF_SECOND)) err = 183;
		}
	}


	/*** Error messages ***/
	switch (err)
	{
	case 151:
		note(_("アイテムの配列が大きすぎる！", "Too many object entries!"));
		break;

	case 152:
		note(_("アイテム配置エラー", "Object allocation error"));
		break;

	case 161:
		note(_("モンスターの配列が大きすぎる！", "Too many monster entries!"));
		break;

	case 162:
		note(_("モンスター配置エラー", "Monster allocation error"));
		break;

	case 171:
		note(_("保存されたフロアのダンジョンデータが壊れています！", "Dungeon data of saved floors are broken!"));
		break;

	case 182:
		note(_("テンポラリ・ファイルを作成できません！", "Failed to make temporal files!"));
		break;

	case 183:
		note(_("Error 183", "Error 183"));
		break;
	}

	/* The dungeon is ready */
	character_dungeon = TRUE;

	/* Success or Error */
	return err;
}


/*
 * Actually read the savefile
 */
static errr rd_savefile_new_aux(void)
{
	int i, j;
	int town_count;

	s32b wild_x_size;
	s32b wild_y_size;

	byte tmp8u;
	u16b tmp16u;
	u32b tmp32u;

#ifdef VERIFY_CHECKSUMS
	u32b n_x_check, n_v_check;
	u32b o_x_check, o_v_check;
#endif


	/* Mention the savefile version */
	note(format(
		     _("バージョン %d.%d.%d のセーブ・ファイルをロード中...", "Loading a %d.%d.%d savefile..."),
		     (z_major > 9) ? z_major - 10 : z_major, z_minor, z_patch));


	/* Strip the version bytes */
	strip_bytes(4);

	/* Hack -- decrypt */
	xor_byte = sf_extra;


	/* Clear the checksums */
	v_check = 0L;
	x_check = 0L;

	/* Read the version number of the savefile */
	/* Old savefile will be version 0.0.0.3 */
	rd_byte(&h_ver_extra);
	rd_byte(&h_ver_patch);
	rd_byte(&h_ver_minor);
	rd_byte(&h_ver_major);

	/* Operating system info */
	rd_u32b(&sf_system);

	/* Time of savefile creation */
	rd_u32b(&sf_when);

	/* Number of resurrections */
	rd_u16b(&sf_lives);

	/* Number of times played */
	rd_u16b(&sf_saves);


	/* Later use (always zero) */
	rd_u32b(&tmp32u);

	/* Later use (always zero) */
	rd_u16b(&tmp16u);

	/* Later use (always zero) */
	rd_byte(&tmp8u);

	/* Kanji code */
	rd_byte(&kanji_code);

	/* Read RNG state */
	rd_randomizer();
	if (arg_fiddle) note(_("乱数情報をロードしました", "Loaded Randomizer Info"));

	/* Then the options */
	rd_options();
	if (arg_fiddle) note(_("オプションをロードしました", "Loaded Option Flags"));

	/* Then the "messages" */
	rd_messages();
	if (arg_fiddle) note(_("メッセージをロードしました", "Loaded Messages"));

	for (i = 0; i < max_r_idx; i++)
	{
		/* Access that monster */
		monster_race *r_ptr = &r_info[i];

		/* Hack -- Reset the death counter */
		r_ptr->max_num = 100;

		if (r_ptr->flags1 & RF1_UNIQUE) r_ptr->max_num = 1;

		/* Hack -- Non-unique Nazguls are semi-unique */
		else if (r_ptr->flags7 & RF7_NAZGUL) r_ptr->max_num = MAX_NAZGUL_NUM;
	}

	/* Monster Memory */
	rd_u16b(&tmp16u);

	/* Incompatible save files */
	if (tmp16u > max_r_idx)
	{
		note(format(_("モンスターの種族が多すぎる(%u)！", "Too many (%u) monster races!"), tmp16u));
		return (21);
	}

	/* Read the available records */
	for (i = 0; i < tmp16u; i++)
	{
		/* Read the lore */
		rd_lore(i);
	}

	if (arg_fiddle) note(_("モンスターの思い出をロードしました", "Loaded Monster Memory"));

	/* Object Memory */
	rd_u16b(&tmp16u);

	/* Incompatible save files */
	if (tmp16u > max_k_idx)
	{
		note(format(_("アイテムの種類が多すぎる(%u)！", "Too many (%u) object kinds!"), tmp16u));
		return (22);
	}

	/* Read the object memory */
	for (i = 0; i < tmp16u; i++)
	{
		byte tmp8u;
		object_kind *k_ptr = &k_info[i];

		rd_byte(&tmp8u);

		k_ptr->aware = (tmp8u & 0x01) ? TRUE: FALSE;
		k_ptr->tried = (tmp8u & 0x02) ? TRUE: FALSE;
	}
	if (arg_fiddle) note(_("アイテムの記録をロードしました", "Loaded Object Memory"));

	/* 2.1.3 or newer version */
	{
		u16b max_towns_load;
		u16b max_quests_load;
		byte max_rquests_load;
		s16b old_inside_quest = p_ptr->inside_quest;

		/* Number of towns */
		rd_u16b(&max_towns_load);

		/* Incompatible save files */
		if (max_towns_load > max_towns)
		{
			note(format(_("町が多すぎる(%u)！", "Too many (%u) towns!"), max_towns_load));
			return (23);
		}

		/* Number of quests */
		rd_u16b(&max_quests_load);

		if (z_older_than(11, 0, 7))
		{
			max_rquests_load = 10;
		}
		else
		{
			rd_byte(&max_rquests_load);
		}

		/* Incompatible save files */
		if (max_quests_load > max_quests)
		{
			note(format(_("クエストが多すぎる(%u)！", "Too many (%u) quests!"), max_quests_load));
			return (23);
		}

		for (i = 0; i < max_quests_load; i++)
		{
			if (i < max_quests)
			{
				quest_type* const q_ptr = &quest[i];
				
				rd_s16b(&q_ptr->status);
				rd_s16b(&q_ptr->level);

				if (z_older_than(11, 0, 6))
				{
					q_ptr->complev = 0;
				}
				else
				{
					rd_byte(&q_ptr->complev);
				}
				if(h_older_than(2, 1, 2, 2))
				{
					q_ptr->comptime = 0;
				}
				else
				{
					rd_u32b(&q_ptr->comptime);
				}

				/* Load quest status if quest is running */
				if ((q_ptr->status == QUEST_STATUS_TAKEN) ||
				    (!z_older_than(10, 3, 14) && (q_ptr->status == QUEST_STATUS_COMPLETED)) ||
				    (!z_older_than(11, 0, 7) && (i >= MIN_RANDOM_QUEST) && (i <= (MIN_RANDOM_QUEST + max_rquests_load))))
				{
					rd_s16b(&q_ptr->cur_num);
					rd_s16b(&q_ptr->max_num);
					rd_s16b(&q_ptr->type);

					/* Load quest monster index */
					rd_s16b(&q_ptr->r_idx);

					if ((q_ptr->type == QUEST_TYPE_RANDOM) && (!q_ptr->r_idx))
					{
						determine_random_questor(&quest[i]);
					}

					/* Load quest item index */
					rd_s16b(&q_ptr->k_idx);

					if (q_ptr->k_idx)
						a_info[q_ptr->k_idx].gen_flags |= TRG_QUESTITEM;

					rd_byte(&q_ptr->flags);

					if (z_older_than(10, 3, 11))
					{
						if (q_ptr->flags & QUEST_FLAG_PRESET)
						{
							q_ptr->dungeon = 0;
						}
						else
						{
							init_flags = INIT_ASSIGN;
							p_ptr->inside_quest = i;

							process_dungeon_file("q_info.txt", 0, 0, 0, 0);
							p_ptr->inside_quest = old_inside_quest;
						}
					}
					else
					{
						rd_byte(&q_ptr->dungeon);
					}
					/* Mark uniques */
					if (q_ptr->status == QUEST_STATUS_TAKEN || q_ptr->status == QUEST_STATUS_UNTAKEN)
						if (r_info[q_ptr->r_idx].flags1 & RF1_UNIQUE)
							r_info[q_ptr->r_idx].flags1 |= RF1_QUESTOR;
				}
			}
			/* Ignore the empty quests from old versions */
			else
			{
				/* Ignore quest status */
				strip_bytes(2);

				/* Ignore quest level */
				strip_bytes(2);

				/*
				 * We don't have to care about the other info,
				 * since status should be 0 for these quests anyway
				 */
			}
		}

		/* Quest 18 was removed */
		if (h_older_than(1, 7, 0, 6))
		{
			(void)WIPE(&quest[OLD_QUEST_WATER_CAVE], quest_type);
			quest[OLD_QUEST_WATER_CAVE].status = QUEST_STATUS_UNTAKEN;
		}

		/* Position in the wilderness */
		rd_s32b(&p_ptr->wilderness_x);
		rd_s32b(&p_ptr->wilderness_y);
		if (z_older_than(10, 3, 13))
		{
			p_ptr->wilderness_x = 5;
			p_ptr->wilderness_y = 48;
		}

		if (z_older_than(10, 3, 7)) p_ptr->wild_mode = FALSE;
		else rd_byte((byte *)&p_ptr->wild_mode);
		if (z_older_than(10, 3, 7)) ambush_flag = FALSE;
		else rd_byte((byte *)&ambush_flag);

		/* Size of the wilderness */
		rd_s32b(&wild_x_size);
		rd_s32b(&wild_y_size);

		/* Incompatible save files */
		if ((wild_x_size > max_wild_x) || (wild_y_size > max_wild_y))
		{
			note(format(_("荒野が大きすぎる(%u/%u)！", "Wilderness is too big (%u/%u)!"), wild_x_size, wild_y_size));
			return (23);
		}

		/* Load the wilderness seeds */
		for (i = 0; i < wild_x_size; i++)
		{
			for (j = 0; j < wild_y_size; j++)
			{
				rd_u32b(&wilderness[j][i].seed);
			}
		}
	}

	if (arg_fiddle) note(_("クエスト情報をロードしました", "Loaded Quests"));

	/* Load the Artifacts */
	rd_u16b(&tmp16u);

	/* Incompatible save files */
	if (tmp16u > max_a_idx)
	{
		note(format(_("伝説のアイテムが多すぎる(%u)！", "Too many (%u) artifacts!"), tmp16u));
		return (24);
	}

	/* Read the artifact flags */
	for (i = 0; i < tmp16u; i++)
	{
		artifact_type *a_ptr = &a_info[i];

		rd_byte(&tmp8u);
		a_ptr->cur_num = tmp8u;

		if (h_older_than(1, 5, 0, 0))
		{
			a_ptr->floor_id = 0;

			rd_byte(&tmp8u);
			rd_byte(&tmp8u);
			rd_byte(&tmp8u);
		}
		else
		{
			rd_s16b(&a_ptr->floor_id);
		}
	}
	if (arg_fiddle) note(_("伝説のアイテムをロードしました", "Loaded Artifacts"));

	/* Read the extra stuff */
	rd_extra();
	if (p_ptr->energy_need < -999) world_player = TRUE;

	if (arg_fiddle) note(_("特別情報をロードしました", "Loaded extra information"));


	/* Read the player_hp array */
	rd_u16b(&tmp16u);

	/* Incompatible save files */
	if (tmp16u > PY_MAX_LEVEL)
	{
		note(format(_("ヒットポイント配列が大きすぎる(%u)！", "Too many (%u) hitpoint entries!"), tmp16u));
		return (25);
	}

	/* Read the player_hp array */
	for (i = 0; i < tmp16u; i++)
	{
		rd_s16b(&p_ptr->player_hp[i]);
	}

	/* Important -- Initialize the sex */
	sp_ptr = &sex_info[p_ptr->psex];

	/* Important -- Initialize the race/class */
	rp_ptr = &race_info[p_ptr->prace];
	cp_ptr = &class_info[p_ptr->pclass];
	ap_ptr = &seikaku_info[p_ptr->pseikaku];

	if(z_older_than(10, 2, 2) && (p_ptr->pclass == CLASS_BEASTMASTER) && !p_ptr->is_dead)
	{
		p_ptr->hitdie = rp_ptr->r_mhp + cp_ptr->c_mhp + ap_ptr->a_mhp;
		do_cmd_rerate(FALSE);
	}
	if(z_older_than(10, 3, 2) && (p_ptr->pclass == CLASS_ARCHER) && !p_ptr->is_dead)
	{
		p_ptr->hitdie = rp_ptr->r_mhp + cp_ptr->c_mhp + ap_ptr->a_mhp;
		do_cmd_rerate(FALSE);
	}
	if(z_older_than(10, 2, 6) && (p_ptr->pclass == CLASS_SORCERER) && !p_ptr->is_dead)
	{
		p_ptr->hitdie = rp_ptr->r_mhp/2 + cp_ptr->c_mhp + ap_ptr->a_mhp;
		do_cmd_rerate(FALSE);
	}
	if(z_older_than(10, 4, 7) && (p_ptr->pclass == CLASS_BLUE_MAGE) && !p_ptr->is_dead)
	{
		p_ptr->hitdie = rp_ptr->r_mhp + cp_ptr->c_mhp + ap_ptr->a_mhp;
		do_cmd_rerate(FALSE);
	}

	/* Important -- Initialize the magic */
	mp_ptr = &m_info[p_ptr->pclass];


	/* Read spell info */
	rd_u32b(&p_ptr->spell_learned1);
	rd_u32b(&p_ptr->spell_learned2);
	rd_u32b(&p_ptr->spell_worked1);
	rd_u32b(&p_ptr->spell_worked2);
	rd_u32b(&p_ptr->spell_forgotten1);
	rd_u32b(&p_ptr->spell_forgotten2);

	if (z_older_than(10,0,5))
	{
		p_ptr->learned_spells = 0;
		for (i = 0; i < 64; i++)
		{
			/* Count known spells */
			if ((i < 32) ?
			    (p_ptr->spell_learned1 & (1L << i)) :
			    (p_ptr->spell_learned2 & (1L << (i - 32))))
			{
				p_ptr->learned_spells++;
			}
		}
	}
	else rd_s16b(&p_ptr->learned_spells);

	if (z_older_than(10,0,6))
	{
		p_ptr->add_spells = 0;
	}
	else rd_s16b(&p_ptr->add_spells);
	if (p_ptr->pclass == CLASS_MINDCRAFTER) p_ptr->add_spells = 0;

	for (i = 0; i < 64; i++)
	{
		rd_byte(&p_ptr->spell_order[i]);
	}


	/* Read the inventory */
	if (rd_inventory())
	{
		note(_("持ち物情報を読み込むことができません", "Unable to read inventory"));
		return (21);
	}

	/* Read number of towns */
	rd_u16b(&tmp16u);
	town_count = tmp16u;

	/* Read the stores */
	rd_u16b(&tmp16u);
	for (i = 1; i < town_count; i++)
	{
		for (j = 0; j < tmp16u; j++)
		{
			if (rd_store(i, j)) return (22);
		}
	}

	rd_s16b(&p_ptr->pet_follow_distance);
	if (z_older_than(10, 4, 10))
	{
		p_ptr->pet_extra_flags = 0;
		rd_byte(&tmp8u);
		if (tmp8u) p_ptr->pet_extra_flags |= PF_OPEN_DOORS;
		rd_byte(&tmp8u);
		if (tmp8u) p_ptr->pet_extra_flags |= PF_PICKUP_ITEMS;

		if (z_older_than(10,0,4)) p_ptr->pet_extra_flags |= PF_TELEPORT;
		else
		{
			rd_byte(&tmp8u);
			if (tmp8u) p_ptr->pet_extra_flags |= PF_TELEPORT;
		}

		if (z_older_than(10,0,7)) p_ptr->pet_extra_flags |= PF_ATTACK_SPELL;
		else
		{
			rd_byte(&tmp8u);
			if (tmp8u) p_ptr->pet_extra_flags |= PF_ATTACK_SPELL;
		}

		if (z_older_than(10,0,8)) p_ptr->pet_extra_flags |= PF_SUMMON_SPELL;
		else
		{
			rd_byte(&tmp8u);
			if (tmp8u) p_ptr->pet_extra_flags |= PF_SUMMON_SPELL;
		}

		if (!z_older_than(10,0,8))
		{
			rd_byte(&tmp8u);
			if (tmp8u) p_ptr->pet_extra_flags |= PF_BALL_SPELL;
		}
	}
	else
	{
		rd_s16b(&p_ptr->pet_extra_flags);
	}

	if (!z_older_than(11, 0, 9))
	{
		char buf[SCREEN_BUF_SIZE];
		rd_string(buf, sizeof(buf));
		if (buf[0]) screen_dump = string_make(buf);
	}

	if (p_ptr->is_dead)
	{
		for (i = MIN_RANDOM_QUEST; i < MAX_RANDOM_QUEST + 1; i++)
		{
			r_info[quest[i].r_idx].flags1 &= ~(RF1_QUESTOR);
		}
	}


	/* I'm not dead yet... */
	if (!p_ptr->is_dead)
	{
		/* Dead players have no dungeon */
		note(_("ダンジョン復元中...", "Restoring Dungeon..."));

		if (rd_dungeon())
		{
			note(_("ダンジョンデータ読み込み失敗", "Error reading dungeon data"));
			return (34);
		}

		/* Read the ghost info */
		rd_ghost();

		{
			s32b tmp32s;

			rd_s32b(&tmp32s);
			strip_bytes(tmp32s);
		}
	}

	/* Quest 18 was removed */
	if (h_older_than(1, 7, 0, 6))
	{
		if (p_ptr->inside_quest == OLD_QUEST_WATER_CAVE)
		{
			dungeon_type = lite_town ? DUNGEON_ANGBAND : DUNGEON_GALGALS;
			dun_level = 1;
			p_ptr->inside_quest = 0;
		}
	}


#ifdef VERIFY_CHECKSUMS

	/* Save the checksum */
	n_v_check = v_check;

	/* Read the old checksum */
	rd_u32b(&o_v_check);

	/* Verify */
	if (o_v_check != n_v_check)
	{
		note(_("チェックサムがおかしい", "Invalid checksum"));
		return (11);
	}


	/* Save the encoded checksum */
	n_x_check = x_check;

	/* Read the checksum */
	rd_u32b(&o_x_check);


	/* Verify */
	if (o_x_check != n_x_check)
	{
		note(_("エンコードされたチェックサムがおかしい", "Invalid encoded checksum"));
		return (11);
	}

#endif

	/* Success */
	return (0);
}


/*
 * Actually read the savefile
 */
errr rd_savefile_new(void)
{
	errr err;

	/* Grab permissions */
	safe_setuid_grab();

	/* The savefile is a binary file */
	fff = my_fopen(savefile, "rb");

	/* Drop permissions */
	safe_setuid_drop();

	/* Paranoia */
	if (!fff) return (-1);

	/* Call the sub-function */
	err = rd_savefile_new_aux();

	/* Check for errors */
	if (ferror(fff)) err = -1;

	/* Close the file */
	my_fclose(fff);

	/* Result */
	return (err);
}


/*
 * Actually load and verify a floor save data
 */
static bool load_floor_aux(saved_floor_type *sf_ptr)
{
	byte tmp8u;
	u32b tmp32u;

#ifdef VERIFY_CHECKSUMS
	u32b n_x_check, n_v_check;
	u32b o_x_check, o_v_check;
#endif

	/* Hack -- decrypt (read xor_byte) */
	xor_byte = 0;
	rd_byte(&tmp8u);

	/* Clear the checksums */
	v_check = 0L;
	x_check = 0L;

	/* Set the version number to current version */
	/* Never load old temporal files */
	h_ver_extra = H_VER_EXTRA;
	h_ver_patch = H_VER_PATCH;
	h_ver_minor = H_VER_MINOR;
	h_ver_major = H_VER_MAJOR;

	/* Verify the sign */
	rd_u32b(&tmp32u);
	if (saved_floor_file_sign != tmp32u) return FALSE;

	/* Read -- have error? */
	if (rd_saved_floor(sf_ptr)) return FALSE;


#ifdef VERIFY_CHECKSUMS
	/* Save the checksum */
	n_v_check = v_check;

	/* Read the old checksum */
	rd_u32b(&o_v_check);

	/* Verify */
	if (o_v_check != n_v_check) return FALSE;

	/* Save the encoded checksum */
	n_x_check = x_check;

	/* Read the checksum */
	rd_u32b(&o_x_check);

	/* Verify */
	if (o_x_check != n_x_check) return FALSE;
#endif

	/* Success */
	return TRUE;
}


/*
 * Attempt to load the temporally saved-floor data
 */
bool load_floor(saved_floor_type *sf_ptr, u32b mode)
{
	FILE *old_fff = NULL;
	byte old_xor_byte = 0;
	u32b old_v_check = 0;
	u32b old_x_check = 0;
	byte old_h_ver_major = 0;
	byte old_h_ver_minor = 0;
	byte old_h_ver_patch = 0;
	byte old_h_ver_extra = 0;
 
	bool ok = TRUE;
	char floor_savefile[1024];

	byte old_kanji_code = kanji_code;

	/*
	 * Temporal files are always written in system depended kanji
	 * code.
	 */
#ifdef JP
# ifdef EUC
	/* EUC kanji code */
	kanji_code = 2;
# endif
# ifdef SJIS
	/* SJIS kanji code */
	kanji_code = 3;
# endif
#else
	/* ASCII */
	kanji_code = 1;
#endif


	/* We have one file already opened */
	if (mode & SLF_SECOND)
	{
		/* Backup original values */
		old_fff = fff;
		old_xor_byte = xor_byte;
		old_v_check = v_check;
		old_x_check = x_check;
		old_h_ver_major = h_ver_major;
		old_h_ver_minor = h_ver_minor;
		old_h_ver_patch = h_ver_patch;
		old_h_ver_extra = h_ver_extra;
	}

	/* floor savefile */
	sprintf(floor_savefile, "%s.F%02d", savefile, (int)sf_ptr->savefile_id);

	/* Grab permissions */
	safe_setuid_grab();

	/* The savefile is a binary file */
	fff = my_fopen(floor_savefile, "rb");

	/* Drop permissions */
	safe_setuid_drop();

	/* Couldn't read */
	if (!fff) ok = FALSE;

	/* Attempt to load */
	if (ok)
	{
		/* Load saved floor data from file */
		ok = load_floor_aux(sf_ptr);

		/* Check for errors */
		if (ferror(fff)) ok = FALSE;

		/* Close the file */
		my_fclose(fff);

		/* Grab permissions */
		safe_setuid_grab();

		/* Delete the file */
		if (!(mode & SLF_NO_KILL)) (void)fd_kill(floor_savefile);

		/* Drop permissions */
		safe_setuid_drop();
	}

	/* We have one file already opened */
	if (mode & SLF_SECOND)
	{
		/* Restore original values */
		fff = old_fff;
		xor_byte = old_xor_byte;
		v_check = old_v_check;
		x_check = old_x_check;
		h_ver_major = old_h_ver_major;
		h_ver_minor = old_h_ver_minor;
		h_ver_patch = old_h_ver_patch;
		h_ver_extra = old_h_ver_extra;
	}

	/* Restore old knowledge */
	kanji_code = old_kanji_code;

	/* Result */
	return ok;
}
