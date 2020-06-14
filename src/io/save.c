/*!
 * @file save.c
 * @brief セーブファイル書き込み処理 / Purpose: interact with savefiles
 * @date 2014/07/12
 * @author
 * <pre>
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * </pre>
 */

#include "io/save.h"
#include "birth/quick-start.h"
#include "cmd-building/cmd-building.h"
#include "util/sort.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "floor/floor-events.h"
#include "floor/floor-town.h"
#include "floor/floor.h"
#include "floor/wild.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-options.h"
#include "game-option/option-flags.h"
#include "game-option/option-types-table.h"
#include "game-option/runtime-arguments.h"
#include "game-option/special-options.h"
#include "game-option/text-display-options.h"
#include "io/files-util.h"
#include "io/load.h"
#include "io/report.h"
#include "io/uid-checker.h"
#include "monster-race/monster-race.h"
#include "monster/monster-compaction.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "object-enchant/artifact.h"
#include "object/object-kind.h"
#include "store/store-util.h"
#include "store/store.h"
#include "system/angband-version.h"
#include "util/angband-files.h"
#include "util/quarks.h"
#include "view/display-main-window.h"
#include "view/display-messages.h"
#include "world/world.h"

 /*
  * Some "local" parameters, used to help write savefiles
  */

static FILE *fff;           /* Current save "file" */
static byte xor_byte;       /* Simple encryption */
static u32b v_stamp = 0L;   /* A simple "checksum" on the actual values */
static u32b x_stamp = 0L;   /* A simple "checksum" on the encoded bytes */

/*!
 * @brief 1バイトをファイルに書き込む / These functions place information into a savefile a byte at a time
 * @param v 書き込むバイト値
 * @return なし
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


/*!
 * @brief 1バイトをファイルに書き込む(sf_put()の糖衣)
 * @param v 書き込むバイト
 * @return なし
 */
static void wr_byte(byte v)
{
	sf_put(v);
}


/*!
 * @brief 符号なし16ビットをファイルに書き込む
 * @param v 書き込む符号なし16bit値
 * @return なし
 */
static void wr_u16b(u16b v)
{
	sf_put((byte)(v & 0xFF));
	sf_put((byte)((v >> 8) & 0xFF));
}


/*!
 * @brief 符号あり16ビットをファイルに書き込む
 * @param v 書き込む符号あり16bit値
 * @return なし
 */
static void wr_s16b(s16b v)
{
	wr_u16b((u16b)v);
}


/*!
 * @brief 符号なし32ビットをファイルに書き込む
 * @param v 書き込む符号なし32bit値
 * @return なし
 */
static void wr_u32b(u32b v)
{
	sf_put((byte)(v & 0xFF));
	sf_put((byte)((v >> 8) & 0xFF));
	sf_put((byte)((v >> 16) & 0xFF));
	sf_put((byte)((v >> 24) & 0xFF));
}


/*!
 * @brief 符号あり32ビットをファイルに書き込む
 * @param v 書き込む符号あり32bit値
 * @return なし
 */
static void wr_s32b(s32b v)
{
	wr_u32b((u32b)v);
}


/*!
 * @brief 文字列をファイルに書き込む
 * @param str 書き込む文字列
 * @return なし
 */
static void wr_string(concptr str)
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


 /*!
  * @brief アイテムオブジェクトを書き込む / Write an "item" record
  * @param o_ptr アイテムオブジェクト保存元ポインタ
  * @return なし
  */
static void wr_item(object_type *o_ptr)
{
	BIT_FLAGS flags = 0x00000000;

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

	wr_byte((byte)o_ptr->iy);
	wr_byte((byte)o_ptr->ix);

	if (flags & SAVE_ITEM_PVAL) wr_s16b(o_ptr->pval);

	if (flags & SAVE_ITEM_DISCOUNT) wr_byte(o_ptr->discount);
	if (flags & SAVE_ITEM_NUMBER) wr_byte((byte)o_ptr->number);

	wr_s16b((s16b)o_ptr->weight);

	if (flags & SAVE_ITEM_NAME1) wr_byte((byte)o_ptr->name1);
	if (flags & SAVE_ITEM_NAME2) wr_byte((byte)o_ptr->name2);
	if (flags & SAVE_ITEM_TIMEOUT) wr_s16b(o_ptr->timeout);

	if (flags & SAVE_ITEM_TO_H) wr_s16b(o_ptr->to_h);
	if (flags & SAVE_ITEM_TO_D) wr_s16b((s16b)o_ptr->to_d);
	if (flags & SAVE_ITEM_TO_A) wr_s16b(o_ptr->to_a);
	if (flags & SAVE_ITEM_AC) wr_s16b(o_ptr->ac);
	if (flags & SAVE_ITEM_DD) wr_byte((byte)o_ptr->dd);
	if (flags & SAVE_ITEM_DS) wr_byte((byte)o_ptr->ds);

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


/*!
 * @brief モンスター情報を書き込む / Write a "monster" record
 * @param m_ptr モンスター情報保存元ポインタ
 * @return なし
 */
static void wr_monster(monster_type *m_ptr)
{
	BIT_FLAGS flags = 0x00000000;
	if (!is_original_ap(m_ptr)) flags |= SAVE_MON_AP_R_IDX;
	if (m_ptr->sub_align) flags |= SAVE_MON_SUB_ALIGN;
	if (monster_csleep_remaining(m_ptr)) flags |= SAVE_MON_CSLEEP;
	if (monster_fast_remaining(m_ptr)) flags |= SAVE_MON_FAST;
	if (monster_slow_remaining(m_ptr)) flags |= SAVE_MON_SLOW;
	if (monster_stunned_remaining(m_ptr)) flags |= SAVE_MON_STUNNED;
	if (monster_confused_remaining(m_ptr)) flags |= SAVE_MON_CONFUSED;
	if (monster_fear_remaining(m_ptr)) flags |= SAVE_MON_MONFEAR;
	if (m_ptr->target_y) flags |= SAVE_MON_TARGET_Y;
	if (m_ptr->target_x) flags |= SAVE_MON_TARGET_X;
	if (monster_invulner_remaining(m_ptr)) flags |= SAVE_MON_INVULNER;
	if (m_ptr->smart) flags |= SAVE_MON_SMART;
	if (m_ptr->exp) flags |= SAVE_MON_EXP;
	if (m_ptr->mflag2) flags |= SAVE_MON_MFLAG2;
	if (m_ptr->nickname) flags |= SAVE_MON_NICKNAME;
	if (m_ptr->parent_m_idx) flags |= SAVE_MON_PARENT;

	/*** Monster save flags ***/
	wr_u32b(flags);

	/*** Write only un-obvious elements ***/
	wr_s16b(m_ptr->r_idx);
	wr_byte((byte)m_ptr->fy);
	wr_byte((byte)m_ptr->fx);
	wr_s16b((s16b)m_ptr->hp);
	wr_s16b((s16b)m_ptr->maxhp);
	wr_s16b((s16b)m_ptr->max_maxhp);
	wr_u32b(m_ptr->dealt_damage);

	/* Monster race index of its appearance */
	if (flags & SAVE_MON_AP_R_IDX) wr_s16b(m_ptr->ap_r_idx);

	if (flags & SAVE_MON_SUB_ALIGN) wr_byte(m_ptr->sub_align);
	if (flags & SAVE_MON_CSLEEP) wr_s16b(m_ptr->mtimed[MTIMED_CSLEEP]);

	wr_byte((byte)m_ptr->mspeed);
	wr_s16b(m_ptr->energy_need);

	byte tmp8u;
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
	if (flags & SAVE_MON_TARGET_Y) wr_s16b((s16b)m_ptr->target_y);
	if (flags & SAVE_MON_TARGET_X) wr_s16b((s16b)m_ptr->target_x);
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


/*!
 * @brief モンスターの思い出を書き込む / Write a "lore" record
 * @param r_idx モンスター種族ID
 * @return なし
 */
static void wr_lore(MONRACE_IDX r_idx)
{
	/* Count sights/deaths/kills */
	monster_race *r_ptr = &r_info[r_idx];
	wr_s16b((s16b)r_ptr->r_sights);
	wr_s16b((s16b)r_ptr->r_deaths);
	wr_s16b((s16b)r_ptr->r_pkills);
	wr_s16b((s16b)r_ptr->r_akills);
	wr_s16b((s16b)r_ptr->r_tkills);

	/* Count wakes and ignores */
	wr_byte(r_ptr->r_wake);
	wr_byte(r_ptr->r_ignore);

	/* Extra stuff */
	wr_byte(r_ptr->r_xtra1);
	wr_byte(r_ptr->r_xtra2);

	/* Count drops */
	wr_byte((byte)r_ptr->r_drop_gold);
	wr_byte((byte)r_ptr->r_drop_item);

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
	wr_byte((byte)r_ptr->max_num);

	/* Location in saved floor */
	wr_s16b(r_ptr->floor_id);

	wr_byte(0);
}


/*!
 * @brief その他のゲーム情報を書き込む(実質はアイテムの鑑定情報のみ) / Write an "xtra" record
 * @param k_idx ベースアイテムのID
 * @return なし
 */
static void wr_xtra(KIND_OBJECT_IDX k_idx)
{
	byte tmp8u = 0;

	object_kind *k_ptr = &k_info[k_idx];

	if (k_ptr->aware) tmp8u |= 0x01;
	if (k_ptr->tried) tmp8u |= 0x02;

	wr_byte(tmp8u);
}


/*!
 * @brief セーブデータに店舗情報を書き込む / Write a "store" record
 * @param store_ptr 店舗情報の参照ポインタ
 * @return なし
 */
static void wr_store(store_type *store_ptr)
{
	/* Save the "open" counter */
	wr_u32b(store_ptr->store_open);

	/* Save the "insults" */
	wr_s16b(store_ptr->insult_cur);

	/* Save the current owner */
	wr_byte(store_ptr->owner);

	/* Save the stock size */
	wr_s16b(store_ptr->stock_num);

	/* Save the "haggle" info */
	wr_s16b(store_ptr->good_buy);
	wr_s16b(store_ptr->bad_buy);

	wr_s32b(store_ptr->last_visit);

	/* Save the stock */
	for (int j = 0; j < store_ptr->stock_num; j++)
	{
		/* Save each item in stock */
		wr_item(&store_ptr->stock[j]);
	}
}


/*!
 * @brief セーブデータに乱数情報を書き込む / Write RNG state
 * @return 常に0(成功を返す)
 */
static errr wr_randomizer(void)
{
	wr_u16b(0);
	wr_u16b(Rand_place);

	for (int i = 0; i < RAND_DEG; i++)
	{
		wr_u32b(Rand_state[i]);
	}

	return 0;
}


/*!
 * @brief ゲームオプション情報を書き込む / Write the "options"
 * @return なし
 */
static void wr_options(void)
{
	for (int i = 0; i < 4; i++) wr_u32b(0L);

	/*** Special options ***/
	/* Write "delay_factor" */
	wr_byte(delay_factor);

	/* Write "hitpoint_warn" */
	wr_byte(hitpoint_warn);

	/* Write "mana_warn" */
	wr_byte(mana_warn);

	/*** Cheating options ***/

	u16b c = 0;
	if (current_world_ptr->wizard) c |= 0x0002;

	if (cheat_sight) c |= 0x0040;
	if (cheat_turn) c |= 0x0080;

	if (cheat_peek) c |= 0x0100;
	if (cheat_hear) c |= 0x0200;
	if (cheat_room) c |= 0x0400;
	if (cheat_xtra) c |= 0x0800;
	if (cheat_know) c |= 0x1000;
	if (cheat_live) c |= 0x2000;
	if (cheat_save) c |= 0x4000;
	if (cheat_diary_output) c |= 0x8000;

	wr_u16b(c);

	/* Autosave info */
	wr_byte(autosave_l);
	wr_byte(autosave_t);
	wr_s16b(autosave_freq);

	/*** Extract options ***/
	/* Analyze the options */
	for (int i = 0; option_info[i].o_desc; i++)
	{
		int os = option_info[i].o_set;
		int ob = option_info[i].o_bit;

		/* Process real entries */
		if (!option_info[i].o_var) continue;

		if (*option_info[i].o_var)
		{
			option_flag[os] |= (1L << ob);
		}
		else
		{
			option_flag[os] &= ~(1L << ob);
		}
	}

	/*** Normal options ***/
	/* Dump the flags */
	for (int i = 0; i < 8; i++) wr_u32b(option_flag[i]);

	/* Dump the masks */
	for (int i = 0; i < 8; i++) wr_u32b(option_mask[i]);

	/*** Window options ***/
	/* Dump the flags */
	for (int i = 0; i < 8; i++) wr_u32b(window_flag[i]);

	/* Dump the masks */
	for (int i = 0; i < 8; i++) wr_u32b(window_mask[i]);
}


/*!
 * @brief ダミー情報スキップを書き込む / Hack -- Write the "ghost" info
 * @return なし
 */
static void wr_ghost(void)
{
	wr_string(_("不正なゴースト", "Broken Ghost"));

	/* Hack -- stupid data */
	for (int i = 0; i < 60; i++) wr_byte(0);
}


/*!
 * @brief クイック・スタート情報を書き込む / Save quick start data
 * @return なし
 */
static void save_quick_start(void)
{
	wr_byte(previous_char.psex);
	wr_byte((byte)previous_char.prace);
	wr_byte((byte)previous_char.pclass);
	wr_byte((byte)previous_char.pseikaku);
	wr_byte((byte)previous_char.realm1);
	wr_byte((byte)previous_char.realm2);

	wr_s16b(previous_char.age);
	wr_s16b(previous_char.ht);
	wr_s16b(previous_char.wt);
	wr_s16b(previous_char.sc);
	wr_s32b(previous_char.au);

	for (int i = 0; i < A_MAX; i++) wr_s16b(previous_char.stat_max[i]);
	for (int i = 0; i < A_MAX; i++) wr_s16b(previous_char.stat_max_max[i]);

	for (int i = 0; i < PY_MAX_LEVEL; i++) wr_s16b((s16b)previous_char.player_hp[i]);

	wr_s16b(previous_char.chaos_patron);

	for (int i = 0; i < 8; i++) wr_s16b(previous_char.vir_types[i]);

	for (int i = 0; i < 4; i++) wr_string(previous_char.history[i]);

	/* UNUSED : Was number of random quests */
	wr_byte(0);

	/* No quick start after using debug mode or cheat options */
	if (current_world_ptr->noscore) previous_char.quick_ok = FALSE;

	wr_byte((byte)previous_char.quick_ok);
}


/*!
 * @brief その他の情報を書き込む / Write some "extra" info
 * @return なし
 */
static void wr_extra(player_type *creature_ptr)
{
	wr_string(creature_ptr->name);
	wr_string(creature_ptr->died_from);
	wr_string(creature_ptr->last_message ? creature_ptr->last_message : "");

	save_quick_start();

	for (int i = 0; i < 4; i++)
	{
		wr_string(creature_ptr->history[i]);
	}

	/* Race/Class/Gender/Spells */
	wr_byte((byte)creature_ptr->prace);
	wr_byte((byte)creature_ptr->pclass);
	wr_byte((byte)creature_ptr->pseikaku);
	wr_byte((byte)creature_ptr->psex);
	wr_byte((byte)creature_ptr->realm1);
	wr_byte((byte)creature_ptr->realm2);
	wr_byte(0);

	wr_byte((byte)creature_ptr->hitdie);
	wr_u16b(creature_ptr->expfact);

	wr_s16b(creature_ptr->age);
	wr_s16b(creature_ptr->ht);
	wr_s16b(creature_ptr->wt);

	/* Dump the stats (maximum and current) */
	for (int i = 0; i < A_MAX; ++i) wr_s16b(creature_ptr->stat_max[i]);
	for (int i = 0; i < A_MAX; ++i) wr_s16b(creature_ptr->stat_max_max[i]);
	for (int i = 0; i < A_MAX; ++i) wr_s16b(creature_ptr->stat_cur[i]);

	/* Ignore the transient stats */
	for (int i = 0; i < 12; ++i) wr_s16b(0);

	wr_u32b(creature_ptr->au);

	wr_u32b(creature_ptr->max_exp);
	wr_u32b(creature_ptr->max_max_exp);
	wr_u32b(creature_ptr->exp);
	wr_u32b(creature_ptr->exp_frac);
	wr_s16b(creature_ptr->lev);

	for (int i = 0; i < 64; i++) wr_s16b(creature_ptr->spell_exp[i]);
	for (int i = 0; i < 5; i++)
		for (int j = 0; j < 64; j++)
			wr_s16b(creature_ptr->weapon_exp[i][j]);

	for (int i = 0; i < GINOU_MAX; i++) wr_s16b(creature_ptr->skill_exp[i]);
	for (int i = 0; i < 108; i++) wr_s32b(creature_ptr->magic_num1[i]);
	for (int i = 0; i < 108; i++) wr_byte(creature_ptr->magic_num2[i]);

	wr_byte((byte)creature_ptr->start_race);
	wr_s32b(creature_ptr->old_race1);
	wr_s32b(creature_ptr->old_race2);
	wr_s16b(creature_ptr->old_realm);

	for (int i = 0; i < MAX_MANE; i++)
	{
		wr_s16b((s16b)creature_ptr->mane_spell[i]);
		wr_s16b((s16b)creature_ptr->mane_dam[i]);
	}

	wr_s16b(creature_ptr->mane_num);

	for (int i = 0; i < MAX_BOUNTY; i++)
	{
		wr_s16b(current_world_ptr->bounty_r_idx[i]);
	}

	for (int i = 0; i < 4; i++)
	{
		wr_s16b(battle_mon[i]);
		wr_u32b(mon_odds[i]);
	}

	wr_s16b(creature_ptr->town_num); /* -KMW- */

	/* Write arena and rewards information -KMW- */
	wr_s16b(creature_ptr->arena_number);
	wr_s16b(creature_ptr->current_floor_ptr->inside_arena);
	wr_s16b(creature_ptr->current_floor_ptr->inside_quest);
	wr_s16b(creature_ptr->phase_out);
	wr_byte(creature_ptr->exit_bldg);
	wr_byte(0); /* Unused */

	wr_s16b((s16b)creature_ptr->oldpx);
	wr_s16b((s16b)creature_ptr->oldpy);

	/* Was number of creature_ptr->rewards[] */
	wr_s16b(0);

	wr_s32b(creature_ptr->mhp);
	wr_s32b(creature_ptr->chp);
	wr_u32b(creature_ptr->chp_frac);

	wr_s32b(creature_ptr->msp);
	wr_s32b(creature_ptr->csp);
	wr_u32b(creature_ptr->csp_frac);

	/* Max Player and Dungeon Levels */
	wr_s16b(creature_ptr->max_plv);
	byte tmp8u = (byte)current_world_ptr->max_d_idx;
	wr_byte(tmp8u);
	for (int i = 0; i < tmp8u; i++)
		wr_s16b((s16b)max_dlv[i]);

	/* More info */
	wr_s16b(0);
	wr_s16b(0);
	wr_s16b(0);
	wr_s16b(0);
	wr_s16b(creature_ptr->sc);
	wr_s16b(creature_ptr->concent);

	wr_s16b(0);             /* old "rest" */
	wr_s16b(creature_ptr->blind);
	wr_s16b(creature_ptr->paralyzed);
	wr_s16b(creature_ptr->confused);
	wr_s16b(creature_ptr->food);
	wr_s16b(0);     /* old "food_digested" */
	wr_s16b(0);     /* old "protection" */
	wr_s16b(creature_ptr->energy_need);
	wr_s16b(creature_ptr->enchant_energy_need);
	wr_s16b(creature_ptr->fast);
	wr_s16b(creature_ptr->slow);
	wr_s16b(creature_ptr->afraid);
	wr_s16b(creature_ptr->cut);
	wr_s16b(creature_ptr->stun);
	wr_s16b(creature_ptr->poisoned);
	wr_s16b(creature_ptr->image);
	wr_s16b(creature_ptr->protevil);
	wr_s16b(creature_ptr->invuln);
	wr_s16b(creature_ptr->ult_res);
	wr_s16b(creature_ptr->hero);
	wr_s16b(creature_ptr->shero);
	wr_s16b(creature_ptr->shield);
	wr_s16b(creature_ptr->blessed);
	wr_s16b(creature_ptr->tim_invis);
	wr_s16b(creature_ptr->word_recall);
	wr_s16b(creature_ptr->recall_dungeon);
	wr_s16b(creature_ptr->alter_reality);
	wr_s16b(creature_ptr->see_infra);
	wr_s16b(creature_ptr->tim_infra);
	wr_s16b(creature_ptr->oppose_fire);
	wr_s16b(creature_ptr->oppose_cold);
	wr_s16b(creature_ptr->oppose_acid);
	wr_s16b(creature_ptr->oppose_elec);
	wr_s16b(creature_ptr->oppose_pois);
	wr_s16b(creature_ptr->tsuyoshi);
	wr_s16b(creature_ptr->tim_esp);
	wr_s16b(creature_ptr->wraith_form);
	wr_s16b(creature_ptr->resist_magic);
	wr_s16b(creature_ptr->tim_regen);
	wr_s16b(creature_ptr->kabenuke);
	wr_s16b(creature_ptr->tim_stealth);
	wr_s16b(creature_ptr->tim_levitation);
	wr_s16b(creature_ptr->tim_sh_touki);
	wr_s16b(creature_ptr->lightspeed);
	wr_s16b(creature_ptr->tsubureru);
	wr_s16b(creature_ptr->magicdef);
	wr_s16b(creature_ptr->tim_res_nether);
	wr_s16b(creature_ptr->tim_res_time);
	wr_byte((byte)creature_ptr->mimic_form);
	wr_s16b(creature_ptr->tim_mimic);
	wr_s16b(creature_ptr->tim_sh_fire);
	wr_s16b(creature_ptr->tim_sh_holy);
	wr_s16b(creature_ptr->tim_eyeeye);

	/* by henkma */
	wr_s16b(creature_ptr->tim_reflect);
	wr_s16b(creature_ptr->multishadow);
	wr_s16b(creature_ptr->dustrobe);

	wr_s16b(creature_ptr->chaos_patron);
	wr_u32b(creature_ptr->muta1);
	wr_u32b(creature_ptr->muta2);
	wr_u32b(creature_ptr->muta3);

	for (int i = 0; i < 8; i++)
		wr_s16b(creature_ptr->virtues[i]);
	for (int i = 0; i < 8; i++)
		wr_s16b(creature_ptr->vir_types[i]);

	wr_s16b(creature_ptr->ele_attack);
	wr_u32b(creature_ptr->special_attack);
	wr_s16b(creature_ptr->ele_immune);
	wr_u32b(creature_ptr->special_defense);
	wr_byte(creature_ptr->knowledge);
	wr_byte(creature_ptr->autopick_autoregister);
	wr_byte(0);
	wr_byte((byte)creature_ptr->action);
	wr_byte(0);
	wr_byte(preserve_mode);
	wr_byte(creature_ptr->wait_report_score);

	/* Future use */
	for (int i = 0; i < 12; i++) wr_u32b(0L);

	/* Ignore some flags */
	wr_u32b(0L);
	wr_u32b(0L);
	wr_u32b(0L);

	/* Write the "object seeds" */
	wr_u32b(current_world_ptr->seed_flavor);
	wr_u32b(current_world_ptr->seed_town);

	/* Special stuff */
	wr_u16b(creature_ptr->panic_save);
	wr_u16b(current_world_ptr->total_winner);
	wr_u16b(current_world_ptr->noscore);

	/* Write death */
	wr_byte(creature_ptr->is_dead);

	/* Write feeling */
	wr_byte(creature_ptr->feeling);

	/* Turn when level began */
	wr_s32b(creature_ptr->current_floor_ptr->generated_turn);

	/* Turn of last "feeling" */
	wr_s32b(creature_ptr->feeling_turn);

	/* Current turn */
	wr_s32b(current_world_ptr->game_turn);

	wr_s32b(current_world_ptr->dungeon_turn);

	wr_s32b(current_world_ptr->arena_start_turn);

	wr_s16b(today_mon);
	wr_s16b(creature_ptr->today_mon);
	wr_s16b(creature_ptr->riding);

	/* Current floor_id */
	wr_s16b(creature_ptr->floor_id);

	/* Save temporary preserved pets (obsolated) */
	wr_s16b(0);

	wr_u32b(current_world_ptr->play_time);
	wr_s32b(creature_ptr->visit);
	wr_u32b(creature_ptr->count);
}


/*!
 * @brief 保存フロアの書き込み / Actually write a saved floor data using effectively compressed format.
 * @param sf_ptr 保存したいフロアの参照ポインタ
 * @return なし
 */
static void wr_saved_floor(player_type *player_ptr, saved_floor_type *sf_ptr)
{
	/*** Basic info ***/
	/* Dungeon floor specific info follows */
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	if (!sf_ptr)
	{
		/*** Not a saved floor ***/

		wr_s16b((s16b)floor_ptr->dun_level);
	}
	else
	{
		/*** The saved floor ***/

		wr_s16b(sf_ptr->floor_id);
		wr_byte((byte)sf_ptr->savefile_id);
		wr_s16b((s16b)sf_ptr->dun_level);
		wr_s32b(sf_ptr->last_visit);
		wr_u32b(sf_ptr->visit_mark);
		wr_s16b(sf_ptr->upper_floor_id);
		wr_s16b(sf_ptr->lower_floor_id);
	}

	wr_u16b((u16b)floor_ptr->base_level);
	wr_u16b((s16b)player_ptr->current_floor_ptr->num_repro);
	wr_u16b((u16b)player_ptr->y);
	wr_u16b((u16b)player_ptr->x);
	wr_u16b((u16b)floor_ptr->height);
	wr_u16b((u16b)floor_ptr->width);
	wr_byte(player_ptr->feeling);

	/*********** Make template for grid_type **********/

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
	u16b max_num_temp = 255;

	/* Allocate the "template" array */
	grid_template_type *templates;
	C_MAKE(templates, max_num_temp, grid_template_type);

	/* Extract template array */
	u16b num_temp = 0;
	for (int y = 0; y < floor_ptr->height; y++)
	{
		for (int x = 0; x < floor_ptr->width; x++)
		{
			grid_type *g_ptr = &floor_ptr->grid_array[y][x];

			int i;
			for (i = 0; i < num_temp; i++)
			{
				if (templates[i].info == g_ptr->info &&
					templates[i].feat == g_ptr->feat &&
					templates[i].mimic == g_ptr->mimic &&
					templates[i].special == g_ptr->special)
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
				grid_template_type *old_template = templates;

				/* Re-allocate the "template" array */
				C_MAKE(templates, max_num_temp + 255, grid_template_type);
				(void)C_COPY(templates, old_template, max_num_temp, grid_template_type);
				C_KILL(old_template, max_num_temp, grid_template_type);
				max_num_temp += 255;
			}

			/* Add new template */
			templates[num_temp].info = g_ptr->info;
			templates[num_temp].feat = g_ptr->feat;
			templates[num_temp].mimic = g_ptr->mimic;
			templates[num_temp].special = g_ptr->special;
			templates[num_temp].occurrence = 1;

			/* Increase number of template */
			num_temp++;
		}
	}

	/* Sort by occurrence */
	int dummy_why;
	ang_sort(templates, &dummy_why, num_temp, ang_sort_comp_cave_temp, ang_sort_swap_cave_temp);

	/*** Dump templates ***/

	/* Total templates */
	wr_u16b(num_temp);

	/* Dump the templates */
	for (int i = 0; i < num_temp; i++)
	{
		grid_template_type *ct_ptr = &templates[i];
		wr_u16b((u16b)ct_ptr->info);
		wr_s16b(ct_ptr->feat);
		wr_s16b(ct_ptr->mimic);
		wr_s16b(ct_ptr->special);
	}

	/*** "Run-Length-Encoding" of floor ***/
	/* Note that this will induce two wasted bytes */
	byte count = 0;
	u16b prev_u16b = 0;

	for (int y = 0; y < floor_ptr->height; y++)
	{
		for (int x = 0; x < floor_ptr->width; x++)
		{
			grid_type *g_ptr = &floor_ptr->grid_array[y][x];
			int i;
			for (i = 0; i < num_temp; i++)
			{
				if (templates[i].info == g_ptr->info &&
					templates[i].feat == g_ptr->feat &&
					templates[i].mimic == g_ptr->mimic &&
					templates[i].special == g_ptr->special)
					break;
			}

			/* Extract an ID */
			u16b tmp16u = (u16b)i;

			/* If the run is broken, or too full, flush it */
			if ((tmp16u == prev_u16b) && (count != MAX_UCHAR))
			{
				count++;
				continue;
			}

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
	}

	/* Flush the data (if any) */
	if (count > 0)
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
	C_KILL(templates, max_num_temp, grid_template_type);

	/*** Dump objects ***/

	/* Total objects */
	wr_u16b(floor_ptr->o_max);

	/* Dump the objects */
	for (int i = 1; i < floor_ptr->o_max; i++)
	{
		object_type *o_ptr = &floor_ptr->o_list[i];
		wr_item(o_ptr);
	}

	/*** Dump the monsters ***/

	/* Total monsters */
	wr_u16b(floor_ptr->m_max);

	/* Dump the monsters */
	for (int i = 1; i < floor_ptr->m_max; i++)
	{
		monster_type *m_ptr = &floor_ptr->m_list[i];
		wr_monster(m_ptr);
	}
}


/*!
 * @brief 現在フロアの書き込み /
 * Write the current dungeon (new method)
 * @player_ptr プレーヤーへの参照ポインタ
 * @return 保存に成功したらTRUE
 */
static bool wr_dungeon(player_type *player_ptr)
{
	forget_lite(player_ptr->current_floor_ptr);
	forget_view(player_ptr->current_floor_ptr);
	clear_mon_lite(player_ptr->current_floor_ptr);

	/* Update lite/view */
	player_ptr->update |= (PU_VIEW | PU_LITE | PU_MON_LITE);
	player_ptr->update |= (PU_MONSTERS | PU_DISTANCE | PU_FLOW);

	/*** Meta info ***/

	/* Number of floor_id used from birth */
	wr_s16b(max_floor_id);

	/* Current dungeon type */
	wr_byte((byte)player_ptr->dungeon_idx);


	/*** No saved floor (On the surface etc.) ***/
	if (!player_ptr->floor_id)
	{
		/* No array elements */
		wr_byte(0);

		/* Write the current floor data */
		wr_saved_floor(player_ptr, NULL);

		/* Success */
		return TRUE;
	}


	/*** In the dungeon ***/

	/* Number of array elements */
	wr_byte(MAX_SAVED_FLOORS);

	/* Write the saved_floors array */
	for (int i = 0; i < MAX_SAVED_FLOORS; i++)
	{
		saved_floor_type *sf_ptr = &saved_floors[i];

		wr_s16b(sf_ptr->floor_id);
		wr_byte((byte)sf_ptr->savefile_id);
		wr_s16b((s16b)sf_ptr->dun_level);
		wr_s32b(sf_ptr->last_visit);
		wr_u32b(sf_ptr->visit_mark);
		wr_s16b(sf_ptr->upper_floor_id);
		wr_s16b(sf_ptr->lower_floor_id);
	}

	/* Extract pointer to current floor */
	saved_floor_type *cur_sf_ptr;
	cur_sf_ptr = get_sf_ptr(player_ptr->floor_id);

	/* Save current floor to temporary file */
	if (!save_floor(player_ptr, cur_sf_ptr, (SLF_SECOND))) return FALSE;

	/* Move data in temporary files to the savefile */
	for (int i = 0; i < MAX_SAVED_FLOORS; i++)
	{
		saved_floor_type *sf_ptr = &saved_floors[i];
		if (!sf_ptr->floor_id) continue;
		if (!load_floor(player_ptr, sf_ptr, (SLF_SECOND | SLF_NO_KILL)))
		{
			wr_byte(1);
			continue;
		}

		wr_byte(0);
		wr_saved_floor(player_ptr, sf_ptr);
	}

	if (!load_floor(player_ptr, cur_sf_ptr, (SLF_SECOND))) return FALSE;
	return TRUE;
}


/*!
 * @brief セーブデータの書き込み /
 * Actually write a save-file
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return 成功すればtrue
 */
static bool wr_savefile_new(player_type *player_ptr)
{
	/* Compact the objects */
	compact_objects(player_ptr, 0);

	/* Compact the monsters */
	compact_monsters(player_ptr, 0);

	/* Guess at the current time */
	u32b now = (u32b)time((time_t *)0);

	/* Note the operating system */
	current_world_ptr->sf_system = 0L;

	/* Note when the file was saved */
	current_world_ptr->sf_when = now;

	/* Note the number of saves */
	current_world_ptr->sf_saves++;

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
	byte tmp8u = (byte)Rand_external(256);
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
	wr_u32b(current_world_ptr->sf_system);

	/* Time file last saved */
	wr_u32b(current_world_ptr->sf_when);

	/* Number of past lives */
	wr_u16b(current_world_ptr->sf_lives);

	/* Number of times saved */
	wr_u16b(current_world_ptr->sf_saves);

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
	u32b tmp32u = message_num();
	if (compress_savefile && (tmp32u > 40)) tmp32u = 40;
	wr_u32b(tmp32u);

	/* Dump the messages (oldest first!) */
	for (int i = tmp32u - 1; i >= 0; i--)
	{
		wr_string(message_str((s16b)i));
	}

	/* Dump the monster lore */
	u16b tmp16u = max_r_idx;
	wr_u16b(tmp16u);
	for (MONRACE_IDX r_idx = 0; r_idx < tmp16u; r_idx++)
	{
		wr_lore(r_idx);
	}

	/* Dump the object memory */
	tmp16u = max_k_idx;
	wr_u16b(tmp16u);
	for (KIND_OBJECT_IDX k_idx = 0; k_idx < tmp16u; k_idx++)
	{
		wr_xtra(k_idx);
	}

	/* Dump the towns */
	tmp16u = max_towns;
	wr_u16b(tmp16u);

	/* Dump the quests */
	tmp16u = max_q_idx;
	wr_u16b(tmp16u);

	/* Dump the quests */
	tmp8u = MAX_RANDOM_QUEST - MIN_RANDOM_QUEST;
	wr_byte(tmp8u);

	for (int i = 0; i < max_q_idx; i++)
	{
		quest_type* const q_ptr = &quest[i];

		/* Save status for every quest */
		wr_s16b(q_ptr->status);

		/* And the dungeon level too */
		/* (prevents problems with multi-level quests) */
		wr_s16b((s16b)q_ptr->level);

		wr_byte((byte)q_ptr->complev);
		wr_u32b(q_ptr->comptime);

		bool is_quest_running = q_ptr->status == QUEST_STATUS_TAKEN;
		is_quest_running |= q_ptr->status == QUEST_STATUS_COMPLETED;
		is_quest_running |= !is_fixed_quest_idx(i);
		if (!is_quest_running) continue;

		wr_s16b((s16b)q_ptr->cur_num);
		wr_s16b((s16b)q_ptr->max_num);
		wr_s16b(q_ptr->type);
		wr_s16b(q_ptr->r_idx);
		wr_s16b(q_ptr->k_idx);
		wr_byte((byte)q_ptr->flags);
		wr_byte((byte)q_ptr->dungeon);
	}

	/* Dump the position in the wilderness */
	wr_s32b(player_ptr->wilderness_x);
	wr_s32b(player_ptr->wilderness_y);

	wr_byte(player_ptr->wild_mode);
	wr_byte(player_ptr->ambush_flag);

	wr_s32b(current_world_ptr->max_wild_x);
	wr_s32b(current_world_ptr->max_wild_y);

	/* Dump the wilderness seeds */
	for (int i = 0; i < current_world_ptr->max_wild_x; i++)
	{
		for (int j = 0; j < current_world_ptr->max_wild_y; j++)
		{
			wr_u32b(wilderness[j][i].seed);
		}
	}

	/* Hack -- Dump the artifacts */
	tmp16u = max_a_idx;
	wr_u16b(tmp16u);
	for (int i = 0; i < tmp16u; i++)
	{
		artifact_type *a_ptr = &a_info[i];
		wr_byte(a_ptr->cur_num);
		wr_s16b(a_ptr->floor_id);
	}

	/* Write the "extra" information */
	wr_extra(player_ptr);

	/* Dump the "player hp" entries */
	tmp16u = PY_MAX_LEVEL;
	wr_u16b(tmp16u);
	for (int i = 0; i < tmp16u; i++)
	{
		wr_s16b((s16b)player_ptr->player_hp[i]);
	}

	/* Write spell data */
	wr_u32b(player_ptr->spell_learned1);
	wr_u32b(player_ptr->spell_learned2);
	wr_u32b(player_ptr->spell_worked1);
	wr_u32b(player_ptr->spell_worked2);
	wr_u32b(player_ptr->spell_forgotten1);
	wr_u32b(player_ptr->spell_forgotten2);

	wr_s16b(player_ptr->learned_spells);
	wr_s16b(player_ptr->add_spells);

	/* Dump the ordered spells */
	for (int i = 0; i < 64; i++)
	{
		wr_byte((byte)player_ptr->spell_order[i]);
	}

	for (int i = 0; i < INVEN_TOTAL; i++)
	{
		object_type *o_ptr = &player_ptr->inventory_list[i];
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
	for (int i = 1; i < max_towns; i++)
	{
		for (int j = 0; j < MAX_STORES; j++)
		{
			wr_store(&town_info[i].store[j]);
		}
	}

	/* Write the pet command settings */
	wr_s16b(player_ptr->pet_follow_distance);
	wr_s16b(player_ptr->pet_extra_flags);

	/* Write screen dump for sending score */
	if (screen_dump && (player_ptr->wait_report_score || !player_ptr->is_dead))
	{
		wr_string(screen_dump);
	}
	else
	{
		wr_string("");
	}

	/* Player is not dead, write the dungeon */
	if (!player_ptr->is_dead)
	{
		/* Dump the dungeon */
		if (!wr_dungeon(player_ptr)) return FALSE;

		/* Dump the ghost */
		wr_ghost();

		/* No scripts */
		wr_s32b(0);
	}

	/* Write the "value check-sum" */
	wr_u32b(v_stamp);

	/* Write the "encoded checksum" */
	wr_u32b(x_stamp);

	if (ferror(fff) || (fflush(fff) == EOF)) return FALSE;
	return TRUE;
}


/*!
 * @brief セーブデータ書き込みのサブルーチン /
 * Medium level player saver
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return 成功すればtrue
 * @details
 * Angband 2.8.0 will use "fd" instead of "fff" if possible
 */
static bool save_player_aux(player_type *player_ptr, char *name)
{
	/* Grab permissions */
	safe_setuid_grab();

	/* Create the savefile */
	int file_permission = 0644;
	int fd = fd_make(name, file_permission);

	/* Drop permissions */
	safe_setuid_drop();

	bool is_save_successful = FALSE;
	fff = NULL;
	if (fd >= 0)
	{
		/* Close the "fd" */
		(void)fd_close(fd);

		/* Grab permissions */
		safe_setuid_grab();

		/* Open the savefile */
		fff = angband_fopen(name, "wb");

		/* Drop permissions */
		safe_setuid_drop();

		/* Successful open */
		if (fff)
		{
			/* Write the savefile */
			if (wr_savefile_new(player_ptr)) is_save_successful = TRUE;

			/* Attempt to close it */
			if (angband_fclose(fff)) is_save_successful = FALSE;
		}

		/* Grab permissions */
		safe_setuid_grab();

		/* Remove "broken" files */
		if (!is_save_successful) (void)fd_kill(name);

		/* Drop permissions */
		safe_setuid_drop();
	}

	if (!is_save_successful) return FALSE;

	counts_write(player_ptr, 0, current_world_ptr->play_time);
	current_world_ptr->character_saved = TRUE;
	return TRUE;
}


/*!
 * @brief セーブデータ書き込みのメインルーチン /
 * Attempt to save the player in a savefile
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return 成功すればtrue
 */
bool save_player(player_type *player_ptr)
{
	char safe[1024];
	strcpy(safe, savefile);
	strcat(safe, ".new");

	/* Grab permissions */
	safe_setuid_grab();

	fd_kill(safe);

	/* Drop permissions */
	safe_setuid_drop();
	update_playtime();

	/* Attempt to save the player */
	bool result = FALSE;
	if (save_player_aux(player_ptr, safe))
	{
		char temp[1024];

		/* Old savefile */
		strcpy(temp, savefile);
		strcat(temp, ".old");

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
		current_world_ptr->character_loaded = TRUE;

		result = TRUE;
	}

	/* Return the result */
	return result;
}


/*!
 * @brief セーブデータ読み込みのメインルーチン /
 * Attempt to Load a "savefile"
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return 成功すればtrue
 * @details
 * <pre>
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
 * flag "current_world_ptr->character_loaded" if a real, living, character was loaded.
 *
 * Note that we always try to load the "current" savefile, even if
 * there is no such file, so we must check for "empty" savefile names.
 * </pre>
 */
bool load_player(player_type *player_ptr)
{
	concptr    what = "generic";

	current_world_ptr->game_turn = 0;
	player_ptr->is_dead = FALSE;


	/* Allow empty savefile name */
	if (!savefile[0]) return TRUE;


#if !defined(WINDOWS)

	/* Fix this */

	/* Verify the existance of the savefile */
	if (access(savefile, 0) < 0)
	{
		/* Give a message */
		msg_print(_("セーブファイルがありません。", "Savefile does not exist."));

		msg_print(NULL);

		/* Allow this */
		return TRUE;
	}

#endif

	errr err = 0;
	int fd = -1;
	byte vvv[4];
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
		/* Read the first four bytes */
		if (fd_read(fd, (char*)(vvv), 4)) err = -1;

		/* What */
		if (err) what = _("セーブファイルを読めません。", "Cannot read savefile");
		(void)fd_close(fd);
	}

	/* Process file */
	if (!err)
	{
		/* Extract version */
		current_world_ptr->z_major = vvv[0];
		current_world_ptr->z_minor = vvv[1];
		current_world_ptr->z_patch = vvv[2];
		current_world_ptr->sf_extra = vvv[3];

		Term_clear();

		/* Attempt to load */
		err = rd_savefile_new(player_ptr);

		/* Message (below) */
		if (err) what = _("セーブファイルを解析出来ません。", "Cannot parse savefile");
	}

	if (!err)
	{
		/* Invalid turn */
		if (!current_world_ptr->game_turn) err = -1;

		/* Message (below) */
		if (err) what = _("セーブファイルが壊れています", "Broken savefile");
	}

	if (!err)
	{
		/* Give a conversion warning */
		if ((FAKE_VER_MAJOR != current_world_ptr->z_major) ||
			(FAKE_VER_MINOR != current_world_ptr->z_minor) ||
			(FAKE_VER_PATCH != current_world_ptr->z_patch))
		{
			if (current_world_ptr->z_major == 2 && current_world_ptr->z_minor == 0 && current_world_ptr->z_patch == 6)
			{
				msg_print(_("バージョン 2.0.* 用のセーブファイルを変換しました。", "Converted a 2.0.* savefile."));
			}
			else
			{
				msg_format(_("バージョン %d.%d.%d 用のセーブ・ファイルを変換しました。", "Converted a %d.%d.%d savefile."),
					(current_world_ptr->z_major > 9) ? current_world_ptr->z_major - 10 : current_world_ptr->z_major, current_world_ptr->z_minor, current_world_ptr->z_patch);
			}
			msg_print(NULL);
		}

		/* Player is dead */
		if (player_ptr->is_dead)
		{
			/* Cheat death */
			if (arg_wizard)
			{
				/* A character was loaded */
				current_world_ptr->character_loaded = TRUE;
				return TRUE;
			}

			/* Player is no longer "dead" */
			player_ptr->is_dead = FALSE;

			/* Count lives */
			current_world_ptr->sf_lives++;

			return TRUE;
		}

		/* A character was loaded */
		current_world_ptr->character_loaded = TRUE;

		{
			u32b tmp = counts_read(player_ptr, 2);
			if (tmp > player_ptr->count)
				player_ptr->count = tmp;
			if (counts_read(player_ptr, 0) > current_world_ptr->play_time || counts_read(player_ptr, 1) == current_world_ptr->play_time)
				counts_write(player_ptr, 2, ++player_ptr->count);
			counts_write(player_ptr, 1, current_world_ptr->play_time);
		}

		/* Success */
		return TRUE;
	}

	msg_format(_("エラー(%s)がバージョン%d.%d.%d 用セーブファイル読み込み中に発生。", "Error (%s) reading %d.%d.%d savefile."),
		what, (current_world_ptr->z_major > 9) ? current_world_ptr->z_major - 10 : current_world_ptr->z_major, current_world_ptr->z_minor, current_world_ptr->z_patch);
	msg_print(NULL);
	return FALSE;
}


/*!
 * @brief ゲームプレイ中のフロア一時保存出力処理サブルーチン / Actually write a temporary saved floor file
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param sf_ptr 保存フロア参照ポインタ
 * @return なし
 */
static bool save_floor_aux(player_type *player_ptr, saved_floor_type *sf_ptr)
{
	/* Compact the objects */
	compact_objects(player_ptr, 0);
	/* Compact the monsters */
	compact_monsters(player_ptr, 0);

	/*** Actually write the file ***/
	/* Initial value of xor_byte */
	byte tmp8u = (byte)randint0(256);
	xor_byte = 0;
	wr_byte(tmp8u);

	/* Reset the checksum */
	v_stamp = 0L;
	x_stamp = 0L;

	/* Write the sign of this process */
	wr_u32b(saved_floor_file_sign);

	/* Dump the dungeon floor */
	wr_saved_floor(player_ptr, sf_ptr);

	/* Write the "value check-sum" */
	wr_u32b(v_stamp);

	/* Write the "encoded checksum" */
	wr_u32b(x_stamp);

	if (ferror(fff) || (fflush(fff) == EOF)) return FALSE;
	return TRUE;
}


/*!
 * @brief ゲームプレイ中のフロア一時保存出力処理メインルーチン / Attempt to save the temporarily saved-floor data
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param sf_ptr 保存フロア参照ポインタ
 * @param mode 保存オプション
 * @return なし
 */
bool save_floor(player_type *player_ptr, saved_floor_type *sf_ptr, BIT_FLAGS mode)
{
	FILE *old_fff = NULL;
	byte old_xor_byte = 0;
	u32b old_v_stamp = 0;
	u32b old_x_stamp = 0;

	char floor_savefile[1024];
	if (!(mode & SLF_SECOND))
	{
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

	/* Grab permissions */
	safe_setuid_grab();

	/* Create the savefile */
	int fd = fd_make(floor_savefile, 0644);

	/* Drop permissions */
	safe_setuid_drop();

	bool is_save_successful = FALSE;
	if (fd >= 0)
	{
		/* Close the "fd" */
		(void)fd_close(fd);

		/* Grab permissions */
		safe_setuid_grab();

		/* Open the savefile */
		fff = angband_fopen(floor_savefile, "wb");

		/* Drop permissions */
		safe_setuid_drop();

		/* Successful open */
		if (fff)
		{
			/* Write the savefile */
			if (save_floor_aux(player_ptr, sf_ptr)) is_save_successful = TRUE;

			/* Attempt to close it */
			if (angband_fclose(fff)) is_save_successful = FALSE;
		}

		/* Remove "broken" files */
		if (!is_save_successful)
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

	return is_save_successful;
}
