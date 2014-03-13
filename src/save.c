/* File: save.c */

/*
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */

/* Purpose: interact with savefiles */

#include "angband.h"



/*
 * Some "local" parameters, used to help write savefiles
 */

static FILE     *fff;           /* Current save "file" */

static byte     xor_byte;       /* Simple encryption */

static u32b     v_stamp = 0L;   /* A simple "checksum" on the actual values */
static u32b     x_stamp = 0L;   /* A simple "checksum" on the encoded bytes */



/*
 * These functions place information into a savefile a byte at a time
 */

static void sf_put(byte v)
{
	/* Encode the value, write a character */
	xor_byte ^= v;
	(void)putc((int)xor_byte, fff);

	/* Maintain the checksum info */
	v_stamp += v;
	x_stamp += xor_byte;
}

static void wr_byte(byte v)
{
	sf_put(v);
}

static void wr_u16b(u16b v)
{
	sf_put((byte)(v & 0xFF));
	sf_put((byte)((v >> 8) & 0xFF));
}

static void wr_s16b(s16b v)
{
	wr_u16b((u16b)v);
}

static void wr_u32b(u32b v)
{
	sf_put((byte)(v & 0xFF));
	sf_put((byte)((v >> 8) & 0xFF));
	sf_put((byte)((v >> 16) & 0xFF));
	sf_put((byte)((v >> 24) & 0xFF));
}

static void wr_s32b(s32b v)
{
	wr_u32b((u32b)v);
}

static void wr_string(cptr str)
{
	while (*str)
	{
		wr_byte(*str);
		str++;
	}
	wr_byte(*str);
}


/*
 * These functions write info in larger logical records
 */


/*
 * Write an "item" record
 */
static void wr_item(object_type *o_ptr)
{
	u32b flags = 0x00000000;

	if (o_ptr->pval) flags |= SAVE_ITEM_PVAL;
	if (o_ptr->discount) flags |= SAVE_ITEM_DISCOUNT;
	if (o_ptr->number != 1) flags |= SAVE_ITEM_NUMBER;
	if (o_ptr->name1) flags |= SAVE_ITEM_NAME1;
	if (o_ptr->name2) flags |= SAVE_ITEM_NAME2;
	if (o_ptr->timeout) flags |= SAVE_ITEM_TIMEOUT;
	if (o_ptr->to_h) flags |= SAVE_ITEM_TO_H;
	if (o_ptr->to_d) flags |= SAVE_ITEM_TO_D;
	if (o_ptr->to_a) flags |= SAVE_ITEM_TO_A;
	if (o_ptr->ac) flags |= SAVE_ITEM_AC;
	if (o_ptr->dd) flags |= SAVE_ITEM_DD;
	if (o_ptr->ds) flags |= SAVE_ITEM_DS;
	if (o_ptr->ident) flags |= SAVE_ITEM_IDENT;
	if (o_ptr->marked) flags |= SAVE_ITEM_MARKED;
	if (o_ptr->art_flags[0]) flags |= SAVE_ITEM_ART_FLAGS0;
	if (o_ptr->art_flags[1]) flags |= SAVE_ITEM_ART_FLAGS1;
	if (o_ptr->art_flags[2]) flags |= SAVE_ITEM_ART_FLAGS2;
	if (o_ptr->art_flags[3]) flags |= SAVE_ITEM_ART_FLAGS3;
	if (o_ptr->art_flags[4]) flags |= SAVE_ITEM_ART_FLAGS4;
	if (o_ptr->curse_flags) flags |= SAVE_ITEM_CURSE_FLAGS;
	if (o_ptr->held_m_idx) flags |= SAVE_ITEM_HELD_M_IDX;
	if (o_ptr->xtra1) flags |= SAVE_ITEM_XTRA1;
	if (o_ptr->xtra2) flags |= SAVE_ITEM_XTRA2;
	if (o_ptr->xtra3) flags |= SAVE_ITEM_XTRA3;
	if (o_ptr->xtra4) flags |= SAVE_ITEM_XTRA4;
	if (o_ptr->xtra5) flags |= SAVE_ITEM_XTRA5;
	if (o_ptr->feeling) flags |= SAVE_ITEM_FEELING;
	if (o_ptr->inscription) flags |= SAVE_ITEM_INSCRIPTION;
	if (o_ptr->art_name) flags |= SAVE_ITEM_ART_NAME;

	/*** Item save flags ***/
	wr_u32b(flags);

	/*** Write only un-obvious elements ***/
	wr_s16b(o_ptr->k_idx);

	/* Location */
	wr_byte(o_ptr->iy);
	wr_byte(o_ptr->ix);

	if (flags & SAVE_ITEM_PVAL) wr_s16b(o_ptr->pval);

	if (flags & SAVE_ITEM_DISCOUNT) wr_byte(o_ptr->discount);
	if (flags & SAVE_ITEM_NUMBER) wr_byte(o_ptr->number);

	wr_s16b(o_ptr->weight);

	if (flags & SAVE_ITEM_NAME1) wr_byte(o_ptr->name1);
	if (flags & SAVE_ITEM_NAME2) wr_byte(o_ptr->name2);
	if (flags & SAVE_ITEM_TIMEOUT) wr_s16b(o_ptr->timeout);

	if (flags & SAVE_ITEM_TO_H) wr_s16b(o_ptr->to_h);
	if (flags & SAVE_ITEM_TO_D) wr_s16b(o_ptr->to_d);
	if (flags & SAVE_ITEM_TO_A) wr_s16b(o_ptr->to_a);
	if (flags & SAVE_ITEM_AC) wr_s16b(o_ptr->ac);
	if (flags & SAVE_ITEM_DD) wr_byte(o_ptr->dd);
	if (flags & SAVE_ITEM_DS) wr_byte(o_ptr->ds);

	if (flags & SAVE_ITEM_IDENT) wr_byte(o_ptr->ident);

	if (flags & SAVE_ITEM_MARKED) wr_byte(o_ptr->marked);

	if (flags & SAVE_ITEM_ART_FLAGS0) wr_u32b(o_ptr->art_flags[0]);
	if (flags & SAVE_ITEM_ART_FLAGS1) wr_u32b(o_ptr->art_flags[1]);
	if (flags & SAVE_ITEM_ART_FLAGS2) wr_u32b(o_ptr->art_flags[2]);
	if (flags & SAVE_ITEM_ART_FLAGS3) wr_u32b(o_ptr->art_flags[3]);
	if (flags & SAVE_ITEM_ART_FLAGS4) wr_u32b(o_ptr->art_flags[4]);

	if (flags & SAVE_ITEM_CURSE_FLAGS) wr_u32b(o_ptr->curse_flags);

	/* Held by monster index */
	if (flags & SAVE_ITEM_HELD_M_IDX) wr_s16b(o_ptr->held_m_idx);

	/* Extra information */
	if (flags & SAVE_ITEM_XTRA1) wr_byte(o_ptr->xtra1);
	if (flags & SAVE_ITEM_XTRA2) wr_byte(o_ptr->xtra2);
	if (flags & SAVE_ITEM_XTRA3) wr_byte(o_ptr->xtra3);
	if (flags & SAVE_ITEM_XTRA4) wr_s16b(o_ptr->xtra4);
	if (flags & SAVE_ITEM_XTRA5) wr_s16b(o_ptr->xtra5);

	/* Feelings */
	if (flags & SAVE_ITEM_FEELING) wr_byte(o_ptr->feeling);

	if (flags & SAVE_ITEM_INSCRIPTION) wr_string(quark_str(o_ptr->inscription));
	if (flags & SAVE_ITEM_ART_NAME) wr_string(quark_str(o_ptr->art_name));
}


/*
 * Write a "monster" record
 */
static void wr_monster(monster_type *m_ptr)
{
	u32b flags = 0x00000000;
	byte tmp8u;

	if (!is_original_ap(m_ptr)) flags |= SAVE_MON_AP_R_IDX;
	if (m_ptr->sub_align) flags |= SAVE_MON_SUB_ALIGN;
	if (MON_CSLEEP(m_ptr)) flags |= SAVE_MON_CSLEEP;
	if (MON_FAST(m_ptr)) flags |= SAVE_MON_FAST;
	if (MON_SLOW(m_ptr)) flags |= SAVE_MON_SLOW;
	if (MON_STUNNED(m_ptr)) flags |= SAVE_MON_STUNNED;
	if (MON_CONFUSED(m_ptr)) flags |= SAVE_MON_CONFUSED;
	if (MON_MONFEAR(m_ptr)) flags |= SAVE_MON_MONFEAR;
	if (m_ptr->target_y) flags |= SAVE_MON_TARGET_Y;
	if (m_ptr->target_x) flags |= SAVE_MON_TARGET_X;
	if (MON_INVULNER(m_ptr)) flags |= SAVE_MON_INVULNER;
	if (m_ptr->smart) flags |= SAVE_MON_SMART;
	if (m_ptr->exp) flags |= SAVE_MON_EXP;
	if (m_ptr->mflag2) flags |= SAVE_MON_MFLAG2;
	if (m_ptr->nickname) flags |= SAVE_MON_NICKNAME;
	if (m_ptr->parent_m_idx) flags |= SAVE_MON_PARENT;

	/*** Monster save flags ***/
	wr_u32b(flags);

	/*** Write only un-obvious elements ***/
	wr_s16b(m_ptr->r_idx);
	wr_byte(m_ptr->fy);
	wr_byte(m_ptr->fx);
	wr_s16b(m_ptr->hp);
	wr_s16b(m_ptr->maxhp);
	wr_s16b(m_ptr->max_maxhp);
	wr_u32b(m_ptr->dealt_damage);
	

	/* Monster race index of its appearance */
	if (flags & SAVE_MON_AP_R_IDX) wr_s16b(m_ptr->ap_r_idx);

	if (flags & SAVE_MON_SUB_ALIGN) wr_byte(m_ptr->sub_align);
	if (flags & SAVE_MON_CSLEEP) wr_s16b(m_ptr->mtimed[MTIMED_CSLEEP]);

	wr_byte(m_ptr->mspeed);
	wr_s16b(m_ptr->energy_need);

	if (flags & SAVE_MON_FAST)
	{
		tmp8u = (byte)m_ptr->mtimed[MTIMED_FAST];
		wr_byte(tmp8u);
	}
	if (flags & SAVE_MON_SLOW)
	{
		tmp8u = (byte)m_ptr->mtimed[MTIMED_SLOW];
		wr_byte(tmp8u);
	}
	if (flags & SAVE_MON_STUNNED)
	{
		tmp8u = (byte)m_ptr->mtimed[MTIMED_STUNNED];
		wr_byte(tmp8u);
	}
	if (flags & SAVE_MON_CONFUSED)
	{
		tmp8u = (byte)m_ptr->mtimed[MTIMED_CONFUSED];
		wr_byte(tmp8u);
	}
	if (flags & SAVE_MON_MONFEAR)
	{
		tmp8u = (byte)m_ptr->mtimed[MTIMED_MONFEAR];
		wr_byte(tmp8u);
	}
	if (flags & SAVE_MON_TARGET_Y) wr_s16b(m_ptr->target_y);
	if (flags & SAVE_MON_TARGET_X) wr_s16b(m_ptr->target_x);
	if (flags & SAVE_MON_INVULNER)
	{
		tmp8u = (byte)m_ptr->mtimed[MTIMED_INVULNER];
		wr_byte(tmp8u);
	}
	if (flags & SAVE_MON_SMART) wr_u32b(m_ptr->smart);
	if (flags & SAVE_MON_EXP) wr_u32b(m_ptr->exp);
	if (flags & SAVE_MON_MFLAG2) wr_byte(m_ptr->mflag2);
	if (flags & SAVE_MON_NICKNAME) wr_string(quark_str(m_ptr->nickname));
	if (flags & SAVE_MON_PARENT) wr_s16b(m_ptr->parent_m_idx);
}


/*
 * Write a "lore" record
 */
static void wr_lore(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	/* Count sights/deaths/kills */
	wr_s16b(r_ptr->r_sights);
	wr_s16b(r_ptr->r_deaths);
	wr_s16b(r_ptr->r_pkills);
	wr_s16b(r_ptr->r_akills);
	wr_s16b(r_ptr->r_tkills);

	/* Count wakes and ignores */
	wr_byte(r_ptr->r_wake);
	wr_byte(r_ptr->r_ignore);

	/* Extra stuff */
	wr_byte(r_ptr->r_xtra1);
	wr_byte(r_ptr->r_xtra2);

	/* Count drops */
	wr_byte(r_ptr->r_drop_gold);
	wr_byte(r_ptr->r_drop_item);

	/* Count spells */
	wr_byte(0); /* unused now */
	wr_byte(r_ptr->r_cast_spell);

	/* Count blows of each type */
	wr_byte(r_ptr->r_blows[0]);
	wr_byte(r_ptr->r_blows[1]);
	wr_byte(r_ptr->r_blows[2]);
	wr_byte(r_ptr->r_blows[3]);

	/* Memorize flags */
	wr_u32b(r_ptr->r_flags1);
	wr_u32b(r_ptr->r_flags2);
	wr_u32b(r_ptr->r_flags3);
	wr_u32b(r_ptr->r_flags4);
	wr_u32b(r_ptr->r_flags5);
	wr_u32b(r_ptr->r_flags6);
	wr_u32b(r_ptr->r_flagsr);


	/* Monster limit per level */
	wr_byte(r_ptr->max_num);

	/* Location in saved floor */
	wr_s16b(r_ptr->floor_id);

	/* Later (?) */
	wr_byte(0);
}


/*
 * Write an "xtra" record
 */
static void wr_xtra(int k_idx)
{
	byte tmp8u = 0;

	object_kind *k_ptr = &k_info[k_idx];

	if (k_ptr->aware) tmp8u |= 0x01;
	if (k_ptr->tried) tmp8u |= 0x02;

	wr_byte(tmp8u);
}


/*
 * Write a "store" record
 */
static void wr_store(store_type *st_ptr)
{
	int j;

	/* Save the "open" counter */
	wr_u32b(st_ptr->store_open);

	/* Save the "insults" */
	wr_s16b(st_ptr->insult_cur);

	/* Save the current owner */
	wr_byte(st_ptr->owner);

	/* Save the stock size */
	wr_s16b(st_ptr->stock_num);

	/* Save the "haggle" info */
	wr_s16b(st_ptr->good_buy);
	wr_s16b(st_ptr->bad_buy);

	wr_s32b(st_ptr->last_visit);

	/* Save the stock */
	for (j = 0; j < st_ptr->stock_num; j++)
	{
		/* Save each item in stock */
		wr_item(&st_ptr->stock[j]);
	}
}


/*
 * Write RNG state
 */
static errr wr_randomizer(void)
{
	int i;

	/* Zero */
	wr_u16b(0);

	/* Place */
	wr_u16b(Rand_place);

	/* State */
	for (i = 0; i < RAND_DEG; i++)
	{
		wr_u32b(Rand_state[i]);
	}

	/* Success */
	return (0);
}


/*
 * Write the "options"
 */
static void wr_options(void)
{
	int i;

	u16b c;


	/*** Oops ***/

	/* Oops */
	for (i = 0; i < 4; i++) wr_u32b(0L);


	/*** Special Options ***/

	/* Write "delay_factor" */
	wr_byte(delay_factor);

	/* Write "hitpoint_warn" */
	wr_byte(hitpoint_warn);

	/* Write "mana_warn" */
	wr_byte(mana_warn);

	/*** Cheating options ***/

	c = 0;

	if (p_ptr->wizard) c |= 0x0002;

	if (cheat_peek) c |= 0x0100;
	if (cheat_hear) c |= 0x0200;
	if (cheat_room) c |= 0x0400;
	if (cheat_xtra) c |= 0x0800;
	if (cheat_know) c |= 0x1000;
	if (cheat_live) c |= 0x2000;
	if (cheat_save) c |= 0x4000;

	wr_u16b(c);

	/* Autosave info */
	wr_byte(autosave_l);
	wr_byte(autosave_t);
	wr_s16b(autosave_freq);

	/*** Extract options ***/

	/* Analyze the options */
	for (i = 0; option_info[i].o_desc; i++)
	{
		int os = option_info[i].o_set;
		int ob = option_info[i].o_bit;

		/* Process real entries */
		if (option_info[i].o_var)
		{
			/* Set */
			if (*option_info[i].o_var)
			{
				/* Set */
				option_flag[os] |= (1L << ob);
			}

			/* Clear */
			else
			{
				/* Clear */
				option_flag[os] &= ~(1L << ob);
			}
		}
	}


	/*** Normal options ***/

	/* Dump the flags */
	for (i = 0; i < 8; i++) wr_u32b(option_flag[i]);

	/* Dump the masks */
	for (i = 0; i < 8; i++) wr_u32b(option_mask[i]);


	/*** Window options ***/

	/* Dump the flags */
	for (i = 0; i < 8; i++) wr_u32b(window_flag[i]);

	/* Dump the masks */
	for (i = 0; i < 8; i++) wr_u32b(window_mask[i]);
}


/*
 * Hack -- Write the "ghost" info
 */
static void wr_ghost(void)
{
	int i;

	/* Name */
	wr_string(_("不正なゴースト", "Broken Ghost"));

	/* Hack -- stupid data */
	for (i = 0; i < 60; i++) wr_byte(0);
}


/*
 * Save quick start data
 */
static void save_quick_start(void)
{
	int i;

	wr_byte(previous_char.psex);
	wr_byte(previous_char.prace);
	wr_byte(previous_char.pclass);
	wr_byte(previous_char.pseikaku);
	wr_byte(previous_char.realm1);
	wr_byte(previous_char.realm2);

	wr_s16b(previous_char.age);
	wr_s16b(previous_char.ht);
	wr_s16b(previous_char.wt);
	wr_s16b(previous_char.sc);
	wr_s32b(previous_char.au);

	for (i = 0; i < 6; i++) wr_s16b(previous_char.stat_max[i]);
	for (i = 0; i < 6; i++) wr_s16b(previous_char.stat_max_max[i]);

	for (i = 0; i < PY_MAX_LEVEL; i++) wr_s16b(previous_char.player_hp[i]);

	wr_s16b(previous_char.chaos_patron);

	for (i = 0; i < 8; i++) wr_s16b(previous_char.vir_types[i]);

	for (i = 0; i < 4; i++) wr_string(previous_char.history[i]);

	/* UNUSED : Was number of random quests */
	wr_byte(0);

	/* No quick start after using debug mode or cheat options */
	if (p_ptr->noscore) previous_char.quick_ok = FALSE;

	wr_byte((byte)previous_char.quick_ok);
}

/*
 * Write some "extra" info
 */
static void wr_extra(void)
{
	int i,j;
	byte tmp8u;

	wr_string(player_name);

	wr_string(p_ptr->died_from);

	wr_string(p_ptr->last_message ? p_ptr->last_message : "");

	save_quick_start();

	for (i = 0; i < 4; i++)
	{
		wr_string(p_ptr->history[i]);
	}

	/* Race/Class/Gender/Spells */
	wr_byte(p_ptr->prace);
	wr_byte(p_ptr->pclass);
	wr_byte(p_ptr->pseikaku);
	wr_byte(p_ptr->psex);
	wr_byte(p_ptr->realm1);
	wr_byte(p_ptr->realm2);
	wr_byte(0);	/* oops */

	wr_byte(p_ptr->hitdie);
	wr_u16b(p_ptr->expfact);

	wr_s16b(p_ptr->age);
	wr_s16b(p_ptr->ht);
	wr_s16b(p_ptr->wt);

	/* Dump the stats (maximum and current) */
	for (i = 0; i < 6; ++i) wr_s16b(p_ptr->stat_max[i]);
	for (i = 0; i < 6; ++i) wr_s16b(p_ptr->stat_max_max[i]);
	for (i = 0; i < 6; ++i) wr_s16b(p_ptr->stat_cur[i]);

	/* Ignore the transient stats */
	for (i = 0; i < 12; ++i) wr_s16b(0);

	wr_u32b(p_ptr->au);

	wr_u32b(p_ptr->max_exp);
	wr_u32b(p_ptr->max_max_exp);
	wr_u32b(p_ptr->exp);
	wr_u32b(p_ptr->exp_frac);
	wr_s16b(p_ptr->lev);

	for (i = 0; i < 64; i++) wr_s16b(p_ptr->spell_exp[i]);
	for (i = 0; i < 5; i++) for (j = 0; j < 64; j++) wr_s16b(p_ptr->weapon_exp[i][j]);
	for (i = 0; i < 10; i++) wr_s16b(p_ptr->skill_exp[i]);
	for (i = 0; i < 108; i++) wr_s32b(p_ptr->magic_num1[i]);
	for (i = 0; i < 108; i++) wr_byte(p_ptr->magic_num2[i]);

	wr_byte(p_ptr->start_race);
	wr_s32b(p_ptr->old_race1);
	wr_s32b(p_ptr->old_race2);
	wr_s16b(p_ptr->old_realm);

	for (i = 0; i < MAX_MANE; i++)
	{
		wr_s16b(p_ptr->mane_spell[i]);
		wr_s16b(p_ptr->mane_dam[i]);
	}
	wr_s16b(p_ptr->mane_num);

	for (i = 0; i < MAX_KUBI; i++)
	{
		wr_s16b(kubi_r_idx[i]);
	}

	for (i = 0; i < 4; i++)
	{
		wr_s16b(battle_mon[i]);
		wr_u32b(mon_odds[i]);
	}

	wr_s16b(p_ptr->town_num); /* -KMW- */

	/* Write arena and rewards information -KMW- */
	wr_s16b(p_ptr->arena_number);
	wr_s16b(p_ptr->inside_arena);
	wr_s16b(p_ptr->inside_quest);
	wr_s16b(p_ptr->inside_battle);
	wr_byte(p_ptr->exit_bldg);
	wr_byte(0); /* Unused */

	wr_s16b(p_ptr->oldpx);
	wr_s16b(p_ptr->oldpy);

	/* Was number of p_ptr->rewards[] */
	wr_s16b(0);

	wr_s32b(p_ptr->mhp);
	wr_s32b(p_ptr->chp);
	wr_u32b(p_ptr->chp_frac);

	wr_s32b(p_ptr->msp);
	wr_s32b(p_ptr->csp);
	wr_u32b(p_ptr->csp_frac);

	/* Max Player and Dungeon Levels */
	wr_s16b(p_ptr->max_plv);
	tmp8u = (byte)max_d_idx;
	wr_byte(tmp8u);
	for (i = 0; i < tmp8u; i++)
		wr_s16b(max_dlv[i]);

	/* More info */
	wr_s16b(0);     /* oops */
	wr_s16b(0);     /* oops */
	wr_s16b(0);     /* oops */
	wr_s16b(0);     /* oops */
	wr_s16b(p_ptr->sc);
	wr_s16b(p_ptr->concent);

	wr_s16b(0);             /* old "rest" */
	wr_s16b(p_ptr->blind);
	wr_s16b(p_ptr->paralyzed);
	wr_s16b(p_ptr->confused);
	wr_s16b(p_ptr->food);
	wr_s16b(0);     /* old "food_digested" */
	wr_s16b(0);     /* old "protection" */
	wr_s16b(p_ptr->energy_need);
	wr_s16b(p_ptr->enchant_energy_need);
	wr_s16b(p_ptr->fast);
	wr_s16b(p_ptr->slow);
	wr_s16b(p_ptr->afraid);
	wr_s16b(p_ptr->cut);
	wr_s16b(p_ptr->stun);
	wr_s16b(p_ptr->poisoned);
	wr_s16b(p_ptr->image);
	wr_s16b(p_ptr->protevil);
	wr_s16b(p_ptr->invuln);
	wr_s16b(p_ptr->ult_res);
	wr_s16b(p_ptr->hero);
	wr_s16b(p_ptr->shero);
	wr_s16b(p_ptr->shield);
	wr_s16b(p_ptr->blessed);
	wr_s16b(p_ptr->tim_invis);
	wr_s16b(p_ptr->word_recall);
	wr_s16b(p_ptr->recall_dungeon);
	wr_s16b(p_ptr->alter_reality);
	wr_s16b(p_ptr->see_infra);
	wr_s16b(p_ptr->tim_infra);
	wr_s16b(p_ptr->oppose_fire);
	wr_s16b(p_ptr->oppose_cold);
	wr_s16b(p_ptr->oppose_acid);
	wr_s16b(p_ptr->oppose_elec);
	wr_s16b(p_ptr->oppose_pois);
	wr_s16b(p_ptr->tsuyoshi);
	wr_s16b(p_ptr->tim_esp);
	wr_s16b(p_ptr->wraith_form);
	wr_s16b(p_ptr->resist_magic);
	wr_s16b(p_ptr->tim_regen);
	wr_s16b(p_ptr->kabenuke);
	wr_s16b(p_ptr->tim_stealth);
	wr_s16b(p_ptr->tim_levitation);
	wr_s16b(p_ptr->tim_sh_touki);
	wr_s16b(p_ptr->lightspeed);
	wr_s16b(p_ptr->tsubureru);
	wr_s16b(p_ptr->magicdef);
	wr_s16b(p_ptr->tim_res_nether);
	wr_s16b(p_ptr->tim_res_time);
	wr_byte(p_ptr->mimic_form);
	wr_s16b(p_ptr->tim_mimic);
	wr_s16b(p_ptr->tim_sh_fire);
	wr_s16b(p_ptr->tim_sh_holy);
	wr_s16b(p_ptr->tim_eyeeye);

	/* by henkma */
	wr_s16b(p_ptr->tim_reflect);
	wr_s16b(p_ptr->multishadow);
	wr_s16b(p_ptr->dustrobe);

	wr_s16b(p_ptr->chaos_patron);
	wr_u32b(p_ptr->muta1);
	wr_u32b(p_ptr->muta2);
	wr_u32b(p_ptr->muta3);

	for (i = 0; i<8; i++)
		wr_s16b(p_ptr->virtues[i]);
	for (i = 0; i<8; i++)
		wr_s16b(p_ptr->vir_types[i]);

	wr_s16b(p_ptr->ele_attack);
	wr_u32b(p_ptr->special_attack);
	wr_s16b(p_ptr->ele_immune);
	wr_u32b(p_ptr->special_defense);
	wr_byte(p_ptr->knowledge);
	wr_byte(p_ptr->autopick_autoregister);
	wr_byte(0);     /* oops */
	wr_byte(p_ptr->action);
	wr_byte(0);
	wr_byte(preserve_mode);
	wr_byte(p_ptr->wait_report_score);

	/* Future use */
	for (i = 0; i < 12; i++) wr_u32b(0L);

	/* Ignore some flags */
	wr_u32b(0L);    /* oops */
	wr_u32b(0L);    /* oops */
	wr_u32b(0L);    /* oops */


	/* Write the "object seeds" */
	wr_u32b(seed_flavor);
	wr_u32b(seed_town);


	/* Special stuff */
	wr_u16b(p_ptr->panic_save);
	wr_u16b(p_ptr->total_winner);
	wr_u16b(p_ptr->noscore);


	/* Write death */
	wr_byte(p_ptr->is_dead);

	/* Write feeling */
	wr_byte(p_ptr->feeling);

	/* Turn when level began */
	wr_s32b(old_turn);

	/* Turn of last "feeling" */
	wr_s32b(p_ptr->feeling_turn);

	/* Current turn */
	wr_s32b(turn);

	wr_s32b(dungeon_turn);

	wr_s32b(old_battle);

	wr_s16b(today_mon);
	wr_s16b(p_ptr->today_mon);
	wr_s16b(p_ptr->riding);

	/* Current floor_id */
	wr_s16b(p_ptr->floor_id);

	/* Save temporary preserved pets (obsolated) */
	wr_s16b(0);

	wr_u32b(playtime);

	wr_s32b(p_ptr->visit);

	wr_u32b(p_ptr->count);
}



/*
 * hook function to sort monsters by level
 */
static bool ang_sort_comp_cave_temp(vptr u, vptr v, int a, int b)
{
	cave_template_type *who = (cave_template_type *)(u);

	u16b o1 = who[a].occurrence;
	u16b o2 = who[b].occurrence;

	/* Unused */
	(void)v;

	return o2 <= o1;
}


/*
 * Sorting hook -- Swap function
 */
static void ang_sort_swap_cave_temp(vptr u, vptr v, int a, int b)
{
	cave_template_type *who = (cave_template_type *)(u);

	cave_template_type holder;

	/* Unused */
	(void)v;

	/* Swap */
	holder = who[a];
	who[a] = who[b];
	who[b] = holder;
}


/*
 * Actually write a saved floor data
 * using effectively compressed format.
 */
static void wr_saved_floor(saved_floor_type *sf_ptr)
{
	cave_template_type *templates;
	u16b max_num_temp;
	u16b num_temp = 0;
	int dummy_why;

	int i, y, x;

	u16b tmp16u;

	byte count;
	u16b prev_u16b;


	/*** Basic info ***/

	/* Dungeon floor specific info follows */

	if (!sf_ptr)
	{
		/*** Not a saved floor ***/

		wr_s16b(dun_level);
	}
	else
	{
		/*** The saved floor ***/

		wr_s16b(sf_ptr->floor_id);
		wr_byte(sf_ptr->savefile_id);
		wr_s16b(sf_ptr->dun_level);
		wr_s32b(sf_ptr->last_visit);
		wr_u32b(sf_ptr->visit_mark);
		wr_s16b(sf_ptr->upper_floor_id);
		wr_s16b(sf_ptr->lower_floor_id);
	}

	wr_u16b(base_level);
	wr_u16b(num_repro);
	wr_u16b((u16b)py);
	wr_u16b((u16b)px);
	wr_u16b(cur_hgt);
	wr_u16b(cur_wid);
	wr_byte(p_ptr->feeling);



	/*********** Make template for cave_type **********/

	/*
	 * Usually number of templates are fewer than 255.  Even if
	 * more than 254 are exist, the occurrence of each template
	 * with larger ID is very small when we sort templates by
	 * occurrence.  So we will use two (or more) bytes for
	 * templete ID larger than 254.
	 *
	 * Ex: 256 will be "0xff" "0x01".
	 *     515 will be "0xff" "0xff" "0x03"
	 */

	/* Fake max number */
	max_num_temp = 255;

	/* Allocate the "template" array */
	C_MAKE(templates, max_num_temp, cave_template_type);

	/* Extract template array */
	for (y = 0; y < cur_hgt; y++)
	{
		for (x = 0; x < cur_wid; x++)
		{
			cave_type *c_ptr = &cave[y][x];

			for (i = 0; i < num_temp; i++)
			{
				if (templates[i].info == c_ptr->info &&
				    templates[i].feat == c_ptr->feat &&
				    templates[i].mimic == c_ptr->mimic &&
				    templates[i].special == c_ptr->special)
				{
					/* Same terrain is exist */
					templates[i].occurrence++;
					break;
				}
			}

			/* Are there same one? */
			if (i < num_temp) continue;

			/* If the max_num_temp is too small, increase it. */
			if (num_temp >= max_num_temp)
			{
				cave_template_type *old_template = templates;

				/* Re-allocate the "template" array */
				C_MAKE(templates, max_num_temp + 255, cave_template_type);
				(void)C_COPY(templates, old_template, max_num_temp, cave_template_type);
				C_KILL(old_template, max_num_temp, cave_template_type);
				max_num_temp += 255;
			}

			/* Add new template */
			templates[num_temp].info = c_ptr->info;
			templates[num_temp].feat = c_ptr->feat;
			templates[num_temp].mimic = c_ptr->mimic;
			templates[num_temp].special = c_ptr->special;
			templates[num_temp].occurrence = 1;

			/* Increase number of template */
			num_temp++;
		}
	}

	/* Select the sort method */
	ang_sort_comp = ang_sort_comp_cave_temp;
	ang_sort_swap = ang_sort_swap_cave_temp;

	/* Sort by occurrence */
	ang_sort(templates, &dummy_why, num_temp);


	/*** Dump templates ***/

	/* Total templates */
	wr_u16b(num_temp);

	/* Dump the templates */
	for (i = 0; i < num_temp; i++)
	{
		cave_template_type *ct_ptr = &templates[i];

		/* Dump it */
		wr_u16b(ct_ptr->info);
		wr_s16b(ct_ptr->feat);
		wr_s16b(ct_ptr->mimic);
		wr_s16b(ct_ptr->special);
	}



	/*** "Run-Length-Encoding" of cave ***/

	/* Note that this will induce two wasted bytes */
	count = 0;
	prev_u16b = 0;

	/* Dump the cave */
	for (y = 0; y < cur_hgt; y++)
	{
		for (x = 0; x < cur_wid; x++)
		{
			cave_type *c_ptr = &cave[y][x];

			for (i = 0; i < num_temp; i++)
			{
				if (templates[i].info == c_ptr->info &&
				    templates[i].feat == c_ptr->feat &&
				    templates[i].mimic == c_ptr->mimic &&
				    templates[i].special == c_ptr->special)
					break;
			}

			/* Extract an ID */
			tmp16u = i;

			/* If the run is broken, or too full, flush it */
			if ((tmp16u != prev_u16b) || (count == MAX_UCHAR))
			{
				wr_byte((byte)count);

				while (prev_u16b >= MAX_UCHAR)
				{
					/* Mark as actual data is larger than 254 */
					wr_byte(MAX_UCHAR);
					prev_u16b -= MAX_UCHAR;
				}

				wr_byte((byte)prev_u16b);
				prev_u16b = tmp16u;
				count = 1;
			}

			/* Continue the run */
			else
			{
				count++;
			}
		}
	}

	/* Flush the data (if any) */
	if (count)
	{
		wr_byte((byte)count);

		while (prev_u16b >= MAX_UCHAR)
		{
			/* Mark as actual data is larger than 254 */
			wr_byte(MAX_UCHAR);
			prev_u16b -= MAX_UCHAR;
		}
		wr_byte((byte)prev_u16b);
	}


	/* Free the "template" array */
	C_KILL(templates, max_num_temp, cave_template_type);


	/*** Dump objects ***/

	/* Total objects */
	wr_u16b(o_max);

	/* Dump the objects */
	for (i = 1; i < o_max; i++)
	{
		object_type *o_ptr = &o_list[i];

		/* Dump it */
		wr_item(o_ptr);
	}


	/*** Dump the monsters ***/

	/* Total monsters */
	wr_u16b(m_max);

	/* Dump the monsters */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];

		/* Dump it */
		wr_monster(m_ptr);
	}
}


/*
 * Write the current dungeon (new method)
 */
static bool wr_dungeon(void)
{
	saved_floor_type *cur_sf_ptr;
	int i;

	/* Forget the lite */
	forget_lite();

	/* Forget the view */
	forget_view();

	/* Forget the view */
	clear_mon_lite();

	/* Update lite/view */
	p_ptr->update |= (PU_VIEW | PU_LITE | PU_MON_LITE);

	/* Update monsters */
	p_ptr->update |= (PU_MONSTERS | PU_DISTANCE | PU_FLOW);


	/*** Meta info ***/

	/* Number of floor_id used from birth */
	wr_s16b(max_floor_id);

	/* Current dungeon type */
	wr_byte(dungeon_type);


	/*** No saved floor (On the surface etc.) ***/
	if (!p_ptr->floor_id)
	{
		/* No array elements */
		wr_byte(0);

		/* Write the current floor data */
		wr_saved_floor(NULL);

		/* Success */
		return TRUE;
	}


	/*** In the dungeon ***/

	/* Number of array elements */
	wr_byte(MAX_SAVED_FLOORS);

	/* Write the saved_floors array */
	for (i = 0; i < MAX_SAVED_FLOORS; i++)
	{
		saved_floor_type *sf_ptr = &saved_floors[i];

		wr_s16b(sf_ptr->floor_id);
		wr_byte(sf_ptr->savefile_id);
		wr_s16b(sf_ptr->dun_level);
		wr_s32b(sf_ptr->last_visit);
		wr_u32b(sf_ptr->visit_mark);
		wr_s16b(sf_ptr->upper_floor_id);
		wr_s16b(sf_ptr->lower_floor_id);
	}

	/* Extract pointer to current floor */
	cur_sf_ptr = get_sf_ptr(p_ptr->floor_id);

	/* Save current floor to temporal file */
	if (!save_floor(cur_sf_ptr, (SLF_SECOND))) return FALSE;

	/* Move data in temporal files to the savefile */
	for (i = 0; i < MAX_SAVED_FLOORS; i++)
	{
		saved_floor_type *sf_ptr = &saved_floors[i];

		/* Unused element */
		if (!sf_ptr->floor_id) continue;

		/* Load temporal saved floor file */
		if (load_floor(sf_ptr, (SLF_SECOND | SLF_NO_KILL)))
		{
			/* Mark success */
			wr_byte(0);

			/* Write saved floor data to the save file */
			wr_saved_floor(sf_ptr);
		}
		else
		{
			/* Mark failure */
			wr_byte(1);
		}
	}

	/* Restore current floor */
	if (!load_floor(cur_sf_ptr, (SLF_SECOND))) return FALSE;

	/* Success */
	return TRUE;
}



/*
 * Actually write a save-file
 */
static bool wr_savefile_new(void)
{
	int        i, j;

	u32b              now;

	byte            tmp8u;
	u16b            tmp16u;


	/* Compact the objects */
	compact_objects(0);
	/* Compact the monsters */
	compact_monsters(0);

	/* Guess at the current time */
	now = time((time_t *)0);


	/* Note the operating system */
	sf_system = 0L;

	/* Note when the file was saved */
	sf_when = now;

	/* Note the number of saves */
	sf_saves++;


	/*** Actually write the file ***/

	/* Dump the file header */
	xor_byte = 0;
	wr_byte(FAKE_VER_MAJOR);
	xor_byte = 0;
	wr_byte(FAKE_VER_MINOR);
	xor_byte = 0;
	wr_byte(FAKE_VER_PATCH);
	xor_byte = 0;

	/* Initial value of xor_byte */
	tmp8u = (byte)Rand_external(256);
	wr_byte(tmp8u);


	/* Reset the checksum */
	v_stamp = 0L;
	x_stamp = 0L;

	/* Write the savefile version for Hengband 1.1.1 and later */
	wr_byte(H_VER_EXTRA);
	wr_byte(H_VER_PATCH);
	wr_byte(H_VER_MINOR);
	wr_byte(H_VER_MAJOR);

	/* Operating system */
	wr_u32b(sf_system);


	/* Time file last saved */
	wr_u32b(sf_when);

	/* Number of past lives */
	wr_u16b(sf_lives);

	/* Number of times saved */
	wr_u16b(sf_saves);


	/* Space */
	wr_u32b(0L);
	wr_u16b(0);
	wr_byte(0);

#ifdef JP
# ifdef EUC
	/* EUC kanji code */
	wr_byte(2);
# endif
# ifdef SJIS
	/* SJIS kanji code */
	wr_byte(3);
# endif
#else
	/* ASCII */
	wr_byte(1);
#endif

	/* Write the RNG state */
	wr_randomizer();


	/* Write the boolean "options" */
	wr_options();


	/* Dump the number of "messages" */
	tmp16u = message_num();
	if (compress_savefile && (tmp16u > 40)) tmp16u = 40;
	wr_u16b(tmp16u);

	/* Dump the messages (oldest first!) */
	for (i = tmp16u - 1; i >= 0; i--)
	{
		wr_string(message_str((s16b)i));
	}


	/* Dump the monster lore */
	tmp16u = max_r_idx;
	wr_u16b(tmp16u);
	for (i = 0; i < tmp16u; i++) wr_lore(i);


	/* Dump the object memory */
	tmp16u = max_k_idx;
	wr_u16b(tmp16u);
	for (i = 0; i < tmp16u; i++) wr_xtra(i);

	/* Dump the towns */
	tmp16u = max_towns;
	wr_u16b(tmp16u);

	/* Dump the quests */
	tmp16u = max_quests;
	wr_u16b(tmp16u);

	/* Dump the quests */
	tmp8u = MAX_RANDOM_QUEST-MIN_RANDOM_QUEST;
	wr_byte(tmp8u);

	for (i = 0; i < max_quests; i++)
	{
		quest_type* const q_ptr = &quest[i];

		/* Save status for every quest */
		wr_s16b(q_ptr->status);

		/* And the dungeon level too */
		/* (prevents problems with multi-level quests) */
		wr_s16b(q_ptr->level);

		wr_byte(q_ptr->complev);
		wr_u32b(q_ptr->comptime);

		/* Save quest status if quest is running */
		if (q_ptr->status == QUEST_STATUS_TAKEN || q_ptr->status == QUEST_STATUS_COMPLETED || !is_fixed_quest_idx(i))
		{
			wr_s16b(q_ptr->cur_num);
			wr_s16b(q_ptr->max_num);
			wr_s16b(q_ptr->type);
			wr_s16b(q_ptr->r_idx);
			wr_s16b(q_ptr->k_idx);
			wr_byte(q_ptr->flags);
			wr_byte(q_ptr->dungeon);
		}
	}

	/* Dump the position in the wilderness */
	wr_s32b(p_ptr->wilderness_x);
	wr_s32b(p_ptr->wilderness_y);

	wr_byte(p_ptr->wild_mode);
	wr_byte(ambush_flag);

	wr_s32b(max_wild_x);
	wr_s32b(max_wild_y);

	/* Dump the wilderness seeds */
	for (i = 0; i < max_wild_x; i++)
	{
		for (j = 0; j < max_wild_y; j++)
		{
			wr_u32b(wilderness[j][i].seed);
		}
	}

	/* Hack -- Dump the artifacts */
	tmp16u = max_a_idx;
	wr_u16b(tmp16u);
	for (i = 0; i < tmp16u; i++)
	{
		artifact_type *a_ptr = &a_info[i];
		wr_byte(a_ptr->cur_num);
		wr_s16b(a_ptr->floor_id);
	}



	/* Write the "extra" information */
	wr_extra();

	/* Dump the "player hp" entries */
	tmp16u = PY_MAX_LEVEL;
	wr_u16b(tmp16u);
	for (i = 0; i < tmp16u; i++)
	{
		wr_s16b(p_ptr->player_hp[i]);
	}


	/* Write spell data */
	wr_u32b(p_ptr->spell_learned1);
	wr_u32b(p_ptr->spell_learned2);
	wr_u32b(p_ptr->spell_worked1);
	wr_u32b(p_ptr->spell_worked2);
	wr_u32b(p_ptr->spell_forgotten1);
	wr_u32b(p_ptr->spell_forgotten2);

	wr_s16b(p_ptr->learned_spells);
	wr_s16b(p_ptr->add_spells);

	/* Dump the ordered spells */
	for (i = 0; i < 64; i++)
	{
		wr_byte(p_ptr->spell_order[i]);
	}


	/* Write the inventory */
	for (i = 0; i < INVEN_TOTAL; i++)
	{
		object_type *o_ptr = &inventory[i];

		/* Skip non-objects */
		if (!o_ptr->k_idx) continue;

		/* Dump index */
		wr_u16b((u16b)i);

		/* Dump object */
		wr_item(o_ptr);
	}

	/* Add a sentinel */
	wr_u16b(0xFFFF);

	/* Note the towns */
	tmp16u = max_towns;
	wr_u16b(tmp16u);

	/* Note the stores */
	tmp16u = MAX_STORES;
	wr_u16b(tmp16u);

	/* Dump the stores of all towns */
	for (i = 1; i < max_towns; i++)
	{
		for (j = 0; j < MAX_STORES; j++)
		{
			wr_store(&town[i].store[j]);
		}
	}

	/* Write the pet command settings */
	wr_s16b(p_ptr->pet_follow_distance);
	wr_s16b(p_ptr->pet_extra_flags);

	/* Write screen dump for sending score */
	if (screen_dump && (p_ptr->wait_report_score || !p_ptr->is_dead))
	{
		wr_string(screen_dump);
	}
	else
	{
		wr_string("");
	}

	/* Player is not dead, write the dungeon */
	if (!p_ptr->is_dead)
	{
		/* Dump the dungeon */
		if (!wr_dungeon()) return FALSE;

		/* Dump the ghost */
		wr_ghost();

		/* No scripts */
		wr_s32b(0);
	}


	/* Write the "value check-sum" */
	wr_u32b(v_stamp);

	/* Write the "encoded checksum" */
	wr_u32b(x_stamp);


	/* Error in save */
	if (ferror(fff) || (fflush(fff) == EOF)) return FALSE;

	/* Successful save */
	return TRUE;
}


/*
 * Medium level player saver
 *
 * XXX XXX XXX Angband 2.8.0 will use "fd" instead of "fff" if possible
 */
static bool save_player_aux(char *name)
{
	bool    ok = FALSE;

	int             fd = -1;

	int             mode = 0644;


	/* No file yet */
	fff = NULL;


	/* File type is "SAVE" */
	FILE_TYPE(FILE_TYPE_SAVE);


	/* Grab permissions */
	safe_setuid_grab();

	/* Create the savefile */
	fd = fd_make(name, mode);

	/* Drop permissions */
	safe_setuid_drop();

	/* File is okay */
	if (fd >= 0)
	{
		/* Close the "fd" */
		(void)fd_close(fd);

		/* Grab permissions */
		safe_setuid_grab();

		/* Open the savefile */
		fff = my_fopen(name, "wb");

		/* Drop permissions */
		safe_setuid_drop();

		/* Successful open */
		if (fff)
		{
			/* Write the savefile */
			if (wr_savefile_new()) ok = TRUE;

			/* Attempt to close it */
			if (my_fclose(fff)) ok = FALSE;
		}

		/* Grab permissions */
		safe_setuid_grab();

		/* Remove "broken" files */
		if (!ok) (void)fd_kill(name);

		/* Drop permissions */
		safe_setuid_drop();
	}


	/* Failure */
	if (!ok) return (FALSE);

	counts_write(0, playtime);

	/* Successful save */
	character_saved = TRUE;

	/* Success */
	return (TRUE);
}



/*
 * Attempt to save the player in a savefile
 */
bool save_player(void)
{
	int             result = FALSE;

	char    safe[1024];


#ifdef SET_UID

# ifdef SECURE

	/* Get "games" permissions */
	beGames();

# endif

#endif


	/* New savefile */
	strcpy(safe, savefile);
	strcat(safe, ".new");

#ifdef VM
	/* Hack -- support "flat directory" usage on VM/ESA */
	strcpy(safe, savefile);
	strcat(safe, "n");
#endif /* VM */

	/* Grab permissions */
	safe_setuid_grab();

	/* Remove it */
	fd_kill(safe);

	/* Drop permissions */
	safe_setuid_drop();

	update_playtime();

	/* Attempt to save the player */
	if (save_player_aux(safe))
	{
		char temp[1024];

		/* Old savefile */
		strcpy(temp, savefile);
		strcat(temp, ".old");

#ifdef VM
		/* Hack -- support "flat directory" usage on VM/ESA */
		strcpy(temp, savefile);
		strcat(temp, "o");
#endif /* VM */

		/* Grab permissions */
		safe_setuid_grab();

		/* Remove it */
		fd_kill(temp);

		/* Preserve old savefile */
		fd_move(savefile, temp);

		/* Activate new savefile */
		fd_move(safe, savefile);

		/* Remove preserved savefile */
		fd_kill(temp);

		/* Drop permissions */
		safe_setuid_drop();

		/* Hack -- Pretend the character was loaded */
		character_loaded = TRUE;

#ifdef VERIFY_SAVEFILE

		/* Lock on savefile */
		strcpy(temp, savefile);
		strcat(temp, ".lok");

		/* Grab permissions */
		safe_setuid_grab();

		/* Remove lock file */
		fd_kill(temp);

		/* Drop permissions */
		safe_setuid_drop();

#endif

		/* Success */
		result = TRUE;
	}


#ifdef SET_UID

# ifdef SECURE

	/* Drop "games" permissions */
	bePlayer();

# endif

#endif

	/* Return the result */
	return (result);
}



/*
 * Attempt to Load a "savefile"
 *
 * Version 2.7.0 introduced a slightly different "savefile" format from
 * older versions, requiring a completely different parsing method.
 *
 * Note that savefiles from 2.7.0 - 2.7.2 are completely obsolete.
 *
 * Pre-2.8.0 savefiles lose some data, see "load2.c" for info.
 *
 * Pre-2.7.0 savefiles lose a lot of things, see "load1.c" for info.
 *
 * On multi-user systems, you may only "read" a savefile if you will be
 * allowed to "write" it later, this prevents painful situations in which
 * the player loads a savefile belonging to someone else, and then is not
 * allowed to save his game when he quits.
 *
 * We return "TRUE" if the savefile was usable, and we set the global
 * flag "character_loaded" if a real, living, character was loaded.
 *
 * Note that we always try to load the "current" savefile, even if
 * there is no such file, so we must check for "empty" savefile names.
 */
bool load_player(void)
{
	int             fd = -1;

	errr    err = 0;

	byte    vvv[4];

#ifdef VERIFY_TIMESTAMP
	struct stat     statbuf;
#endif

	cptr    what = "generic";


	/* Paranoia */
	turn = 0;

	/* Paranoia */
	p_ptr->is_dead = FALSE;


	/* Allow empty savefile name */
	if (!savefile[0]) return (TRUE);


#if !defined(MACINTOSH) && !defined(WINDOWS) && !defined(VM)

	/* XXX XXX XXX Fix this */

	/* Verify the existance of the savefile */
	if (access(savefile, 0) < 0)
	{
		/* Give a message */
		msg_print(_("セーブファイルがありません。", "Savefile does not exist."));

		msg_print(NULL);

		/* Allow this */
		return (TRUE);
	}

#endif


#ifdef VERIFY_SAVEFILE

	/* Verify savefile usage */
	if (!err)
	{
		FILE *fkk;

		char temp[1024];

		/* Extract name of lock file */
		strcpy(temp, savefile);
		strcat(temp, ".lok");

		/* Check for lock */
		fkk = my_fopen(temp, "r");

		/* Oops, lock exists */
		if (fkk)
		{
			/* Close the file */
			my_fclose(fkk);

			/* Message */
			msg_print(_("セーブファイルは現在使用中です。", "Savefile is currently in use."));
			msg_print(NULL);

			/* Oops */
			return (FALSE);
		}

		/* Create a lock file */
		fkk = my_fopen(temp, "w");

		/* Dump a line of info */
		fprintf(fkk, "Lock file for savefile '%s'\n", savefile);

		/* Close the lock file */
		my_fclose(fkk);
	}

#endif


	/* Okay */
	if (!err)
	{
		/* Open the savefile */
		fd = fd_open(savefile, O_RDONLY);

		/* No file */
		if (fd < 0) err = -1;

		/* Message (below) */
		if (err) what = _("セーブファイルを開けません。", "Cannot open savefile");
	}

	/* Process file */
	if (!err)
	{

#ifdef VERIFY_TIMESTAMP
		/* Get the timestamp */
		(void)fstat(fd, &statbuf);
#endif

		/* Read the first four bytes */
		if (fd_read(fd, (char*)(vvv), 4)) err = -1;

		/* What */
		if (err) what = _("セーブファイルを読めません。", "Cannot read savefile");

		/* Close the file */
		(void)fd_close(fd);
	}

	/* Process file */
	if (!err)
	{

		/* Extract version */
		z_major = vvv[0];
		z_minor = vvv[1];
		z_patch = vvv[2];
		sf_extra = vvv[3];


		/* Clear screen */
		Term_clear();

		/* Attempt to load */
		err = rd_savefile_new();

		/* Message (below) */
		if (err) what = _("セーブファイルを解析出来ません。", "Cannot parse savefile");
	}

	/* Paranoia */
	if (!err)
	{
		/* Invalid turn */
		if (!turn) err = -1;

		/* Message (below) */
		if (err) what = _("セーブファイルが壊れています", "Broken savefile");
	}

#ifdef VERIFY_TIMESTAMP
	/* Verify timestamp */
	if (!err && !arg_wizard)
	{
		/* Hack -- Verify the timestamp */
		if (sf_when > (statbuf.st_ctime + 100) ||
		    sf_when < (statbuf.st_ctime - 100))
		{
			/* Message */
			what = _("無効なタイム・スタンプです", "Invalid timestamp");

			/* Oops */
			err = -1;
		}
	}
#endif


	/* Okay */
	if (!err)
	{
		/* Give a conversion warning */
		if ((FAKE_VER_MAJOR != z_major) ||
		    (FAKE_VER_MINOR != z_minor) ||
		    (FAKE_VER_PATCH != z_patch))
		{
			if (z_major == 2 && z_minor == 0 && z_patch == 6)
			{
				msg_print(_("バージョン 2.0.* 用のセーブファイルを変換しました。", "Converted a 2.0.* savefile."));
			}
			else
			{
				/* Message */
#ifdef JP
				msg_format("バージョン %d.%d.%d 用のセーブ・ファイルを変換しました。",
				    (z_major > 9) ? z_major-10 : z_major , z_minor, z_patch);
#else
				msg_format("Converted a %d.%d.%d savefile.",
				    (z_major > 9) ? z_major-10 : z_major , z_minor, z_patch);
#endif
			}
			msg_print(NULL);
		}

		/* Player is dead */
		if (p_ptr->is_dead)
		{
			/* Cheat death */
			if (arg_wizard)
			{
				/* A character was loaded */
				character_loaded = TRUE;

				/* Done */
				return (TRUE);
			}

			/* Player is no longer "dead" */
			p_ptr->is_dead = FALSE;

			/* Count lives */
			sf_lives++;

			/* Done */
			return (TRUE);
		}

		/* A character was loaded */
		character_loaded = TRUE;

		{
			u32b tmp = counts_read(2);
			if (tmp > p_ptr->count)
				p_ptr->count = tmp;
			if (counts_read(0) > playtime || counts_read(1) == playtime)
				counts_write(2, ++p_ptr->count);
			counts_write(1, playtime);
		}

		/* Success */
		return (TRUE);
	}


#ifdef VERIFY_SAVEFILE

	/* Verify savefile usage */
	if (TRUE)
	{
		char temp[1024];

		/* Extract name of lock file */
		strcpy(temp, savefile);
		strcat(temp, ".lok");

		/* Remove lock */
		fd_kill(temp);
	}

#endif


	/* Message */
#ifdef JP
	msg_format("エラー(%s)がバージョン%d.%d.%d 用セーブファイル読み込中に発生。",
		   what, (z_major>9) ? z_major - 10 : z_major, z_minor, z_patch);
#else
	msg_format("Error (%s) reading %d.%d.%d savefile.",
		   what, (z_major>9) ? z_major - 10 : z_major, z_minor, z_patch);
#endif
	msg_print(NULL);

	/* Oops */
	return (FALSE);
}


void remove_loc(void)
{
#ifdef VERIFY_SAVEFILE
	char temp[1024];
#endif /* VERIFY_SAVEFILE */

#ifdef SET_UID
# ifdef SECURE

	/* Get "games" permissions */
	beGames();

# endif /* SECURE */
#endif /* SET_UID */

#ifdef VERIFY_SAVEFILE

	/* Lock on savefile */
	strcpy(temp, savefile);
	strcat(temp, ".lok");

	/* Remove lock file */
	fd_kill(temp);

#endif /* VERIFY_SAVEFILE */

#ifdef SET_UID
# ifdef SECURE

	/* Drop "games" permissions */
	bePlayer();

# endif /* SECURE */
#endif /* SET_UID */

}


/*
 * Actually write a temporal saved floor file
 */
static bool save_floor_aux(saved_floor_type *sf_ptr)
{
	byte tmp8u;

	/* Compact the objects */
	compact_objects(0);
	/* Compact the monsters */
	compact_monsters(0);


	/*** Actually write the file ***/

	/* Initial value of xor_byte */
	tmp8u = (byte)randint0(256);
	xor_byte = 0;
	wr_byte(tmp8u);


	/* Reset the checksum */
	v_stamp = 0L;
	x_stamp = 0L;

	/* Write the sign of this process */
	wr_u32b(saved_floor_file_sign);

	/* Dump the dungeon floor */
	wr_saved_floor(sf_ptr);


	/* Write the "value check-sum" */
	wr_u32b(v_stamp);

	/* Write the "encoded checksum" */
	wr_u32b(x_stamp);


	/* Error in save */
	if (ferror(fff) || (fflush(fff) == EOF)) return FALSE;

	/* Successful save */
	return TRUE;
}


/*
 * Attempt to save the temporally saved-floor data
 */
bool save_floor(saved_floor_type *sf_ptr, u32b mode)
{
	FILE *old_fff = NULL;
	byte old_xor_byte = 0;
	u32b old_v_stamp = 0;
	u32b old_x_stamp = 0;

	char floor_savefile[1024];
	int fd = -1;
	bool ok = FALSE;

	if (!(mode & SLF_SECOND))
	{
#ifdef SET_UID
# ifdef SECURE
		/* Get "games" permissions */
		beGames();
# endif
#endif
	}

	/* We have one file already opened */
	else
	{
		/* Backup original values */
		old_fff = fff;
		old_xor_byte = xor_byte;
		old_v_stamp = v_stamp;
		old_x_stamp = x_stamp;
	}

	/* New savefile */
	sprintf(floor_savefile, "%s.F%02d", savefile, (int)sf_ptr->savefile_id);

	/* Grab permissions */
	safe_setuid_grab();

	/* Remove it */
	fd_kill(floor_savefile);

	/* Drop permissions */
	safe_setuid_drop();


	/* Attempt to save the player */

	/* No file yet */
	fff = NULL;

	/* File type is "SAVE" */
	FILE_TYPE(FILE_TYPE_SAVE);

	/* Grab permissions */
	safe_setuid_grab();

	/* Create the savefile */
	fd = fd_make(floor_savefile, 0644);

	/* Drop permissions */
	safe_setuid_drop();

	/* File is okay */
	if (fd >= 0)
	{
		/* Close the "fd" */
		(void)fd_close(fd);

		/* Grab permissions */
		safe_setuid_grab();

		/* Open the savefile */
		fff = my_fopen(floor_savefile, "wb");

		/* Drop permissions */
		safe_setuid_drop();

		/* Successful open */
		if (fff)
		{
			/* Write the savefile */
			if (save_floor_aux(sf_ptr)) ok = TRUE;

			/* Attempt to close it */
			if (my_fclose(fff)) ok = FALSE;
		}

		/* Remove "broken" files */
		if (!ok)
		{
			/* Grab permissions */
			safe_setuid_grab();

			(void)fd_kill(floor_savefile);

			/* Drop permissions */
			safe_setuid_drop();
		}
	}

	if (!(mode & SLF_SECOND))
	{
#ifdef SET_UID
# ifdef SECURE
		/* Drop "games" permissions */
		bePlayer();
# endif
#endif
	}

	/* We have one file already opened */
	else
	{
		/* Restore original values */
		fff = old_fff;
		xor_byte = old_xor_byte;
		v_stamp = old_v_stamp;
		x_stamp = old_x_stamp;
	}

	/* Return the result */
	return ok;
}
