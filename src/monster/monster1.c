﻿/*!
 * @file monster1.c
 * @brief モンスター情報の記述 / describe monsters (using monster memory)
 * @date 2013/12/11
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * 2014 Deskull rearranged comment for Doxygen.
 */

#include "monster/monster1.h"
#include "art-definition/art-armor-types.h"
#include "art-definition/art-bow-types.h"
#include "art-definition/art-protector-types.h"
#include "art-definition/art-weapon-types.h"
#include "cmd-building/cmd-building.h"
#include "cmd-io/cmd-dump.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "effect/effect-characteristics.h"
#include "floor/floor-object.h"
#include "floor/wild.h"
#include "io/write-diary.h"
#include "main/music-definitions-table.h"
#include "market/arena-info-table.h"
#include "melee/melee-postprocess.h"
#include "monster-attack/monster-attack-effect.h"
#include "monster-attack/monster-attack-types.h"
#include "monster-lore/lore-store.h"
#include "monster-lore/monster-lore.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags-ability1.h"
#include "monster-race/race-flags-ability2.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags4.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-flags8.h"
#include "monster-race/race-flags9.h"
#include "monster-race/race-indice-types.h"
#include "monster-race/monster-race-hook.h"
#include "monster/monster-description-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster2.h" // todo 相互参照している、いずれ消す.
#include "monster/place-monster-types.h"
#include "monster/smart-learn-types.h"
#include "mspell/monster-spell.h"
#include "mspell/mspell-damage-calculator.h"
#include "mspell/mspell-type.h"
#include "object-enchant/apply-magic.h"
#include "object-enchant/artifact.h"
#include "object-enchant/item-apply-magic.h"
#include "object/object-generator.h"
#include "object/object-kind-hook.h"
#include "pet/pet-fall-off.h"
#include "player/avatar.h"
#include "player/patron.h"
#include "player/player-class.h"
#include "player/player-personalities-table.h"
#include "spell/process-effect.h"
#include "spell/spells-summon.h"
#include "spell/spells-type.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-protector-types.h"
#include "sv-definition/sv-scroll-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/system-variables.h"
#include "term/term-color-types.h"
#include "util/util.h"
#include "view/display-main-window.h"
#include "world/world.h"

/*!
 * @brief モンスターを友好的にする
 * @param m_ptr モンスター情報構造体の参照ポインタ
 * @return なし
 */
void set_friendly(monster_type *m_ptr)
{
	m_ptr->smart |= SM_FRIENDLY;
}


/*!
 * @brief モンスターをペットにする
 * @param player_type プレーヤーへの参照ポインタ
 * @param m_ptr モンスター情報構造体の参照ポインタ
 * @return なし
 */
void set_pet(player_type *player_ptr, monster_type *m_ptr)
{
	check_quest_completion(player_ptr, m_ptr);
	m_ptr->smart |= SM_PET;
	if (!(r_info[m_ptr->r_idx].flags3 & (RF3_EVIL | RF3_GOOD)))
		m_ptr->sub_align = SUB_ALIGN_NEUTRAL;
}


/*!
 * @brief モンスターを敵に回す
 * Makes the monster hostile towards the player
 * @param m_ptr モンスター情報構造体の参照ポインタ
 * @return なし
 */
void set_hostile(player_type *player_ptr, monster_type *m_ptr)
{
	if (player_ptr->phase_out) return;
	m_ptr->smart &= ~SM_PET;
	m_ptr->smart &= ~SM_FRIENDLY;
}


/*!
 * @brief モンスターを怒らせる
 * Anger the monster
 * @param m_ptr モンスター情報構造体の参照ポインタ
 * @return なし
 */
void anger_monster(player_type *player_ptr, monster_type *m_ptr)
{
	if (player_ptr->phase_out) return;
	if (!is_friendly(m_ptr)) return;

	GAME_TEXT m_name[MAX_NLEN];

	monster_desc(player_ptr, m_name, m_ptr, 0);
	msg_format(_("%^sは怒った！", "%^s gets angry!"), m_name);
	set_hostile(player_ptr, m_ptr);
	chg_virtue(player_ptr, V_INDIVIDUALISM, 1);
	chg_virtue(player_ptr, V_HONOUR, -1);
	chg_virtue(player_ptr, V_JUSTICE, -1);
	chg_virtue(player_ptr, V_COMPASSION, -1);
}


/*!
 * @brief モンスターが地形を踏破できるかどうかを返す
 * Check if monster can cross terrain
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param feat 地形ID
 * @param r_ptr モンスター種族構造体の参照ポインタ
 * @param mode オプション
 * @return 踏破可能ならばTRUEを返す
 */
bool monster_can_cross_terrain(player_type *player_ptr, FEAT_IDX feat, monster_race *r_ptr, BIT_FLAGS16 mode)
{
	feature_type *f_ptr = &f_info[feat];

	if (have_flag(f_ptr->flags, FF_PATTERN))
	{
		if (!(mode & CEM_RIDING))
		{
			if (!(r_ptr->flags7 & RF7_CAN_FLY)) return FALSE;
		}
		else
		{
			if (!(mode & CEM_P_CAN_ENTER_PATTERN)) return FALSE;
		}
	}

	if (have_flag(f_ptr->flags, FF_CAN_FLY) && (r_ptr->flags7 & RF7_CAN_FLY)) return TRUE;
	if (have_flag(f_ptr->flags, FF_CAN_SWIM) && (r_ptr->flags7 & RF7_CAN_SWIM)) return TRUE;
	if (have_flag(f_ptr->flags, FF_CAN_PASS))
	{
		if ((r_ptr->flags2 & RF2_PASS_WALL) && (!(mode & CEM_RIDING) || player_ptr->pass_wall)) return TRUE;
	}

	if (!have_flag(f_ptr->flags, FF_MOVE)) return FALSE;

	if (have_flag(f_ptr->flags, FF_MOUNTAIN) && (r_ptr->flags8 & RF8_WILD_MOUNTAIN)) return TRUE;

	if (have_flag(f_ptr->flags, FF_WATER))
	{
		if (!(r_ptr->flags7 & RF7_AQUATIC))
		{
			if (have_flag(f_ptr->flags, FF_DEEP)) return FALSE;
			else if (r_ptr->flags2 & RF2_AURA_FIRE) return FALSE;
		}
	}
	else if (r_ptr->flags7 & RF7_AQUATIC) return FALSE;

	if (have_flag(f_ptr->flags, FF_LAVA))
	{
		if (!(r_ptr->flagsr & RFR_EFF_IM_FIRE_MASK)) return FALSE;
	}

	if (have_flag(f_ptr->flags, FF_COLD_PUDDLE))
	{
		if (!(r_ptr->flagsr & RFR_EFF_IM_COLD_MASK)) return FALSE;
	}

	if (have_flag(f_ptr->flags, FF_ELEC_PUDDLE))
	{
		if (!(r_ptr->flagsr & RFR_EFF_IM_ELEC_MASK)) return FALSE;
	}

	if (have_flag(f_ptr->flags, FF_ACID_PUDDLE))
	{
		if (!(r_ptr->flagsr & RFR_EFF_IM_ACID_MASK)) return FALSE;
	}

	if (have_flag(f_ptr->flags, FF_POISON_PUDDLE))
	{
		if (!(r_ptr->flagsr & RFR_EFF_IM_POIS_MASK)) return FALSE;
	}

	return TRUE;
}


/*!
 * @brief 指定された座標の地形をモンスターが踏破できるかどうかを返す
 * Strictly check if monster can enter the grid
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param y 地形のY座標
 * @param x 地形のX座標
 * @param r_ptr モンスター種族構造体の参照ポインタ
 * @param mode オプション
 * @return 踏破可能ならばTRUEを返す
 */
bool monster_can_enter(player_type *player_ptr, POSITION y, POSITION x, monster_race *r_ptr, BIT_FLAGS16 mode)
{
	grid_type *g_ptr = &player_ptr->current_floor_ptr->grid_array[y][x];
	if (player_bold(player_ptr, y, x)) return FALSE;
	if (g_ptr->m_idx) return FALSE;

	return monster_can_cross_terrain(player_ptr, g_ptr->feat, r_ptr, mode);
}


/*!
 * @brief モンスターの属性の基づいた敵対関係の有無を返す（サブルーチン）
 * Check if this monster has "hostile" alignment (aux)
 * @param sub_align1 モンスター1のサブフラグ
 * @param sub_align2 モンスター2のサブフラグ
 * @return 敵対関係にあるならばTRUEを返す
 */
static bool check_hostile_align(byte sub_align1, byte sub_align2)
{
	if (sub_align1 != sub_align2)
	{
		if (((sub_align1 & SUB_ALIGN_EVIL) && (sub_align2 & SUB_ALIGN_GOOD)) ||
			((sub_align1 & SUB_ALIGN_GOOD) && (sub_align2 & SUB_ALIGN_EVIL)))
			return TRUE;
	}

	return FALSE;
}


/*!
 * @brief モンスターの属性の基づいた敵対関係の有無を返す
 * Check if two monsters are enemies
 * @param m_ptr モンスター1の構造体参照ポインタ
 * @param n_ptr モンスター2の構造体参照ポインタ
 * @return 敵対関係にあるならばTRUEを返す
 */
bool are_enemies(player_type *player_ptr, monster_type *m_ptr, monster_type *n_ptr)
{
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	monster_race *s_ptr = &r_info[n_ptr->r_idx];

	if (player_ptr->phase_out)
	{
		if (is_pet(m_ptr) || is_pet(n_ptr)) return FALSE;
		return TRUE;
	}

	if ((r_ptr->flags8 & (RF8_WILD_TOWN | RF8_WILD_ALL))
		&& (s_ptr->flags8 & (RF8_WILD_TOWN | RF8_WILD_ALL)))
	{
		if (!is_pet(m_ptr) && !is_pet(n_ptr)) return FALSE;
	}

	if (check_hostile_align(m_ptr->sub_align, n_ptr->sub_align))
	{
		if (!(m_ptr->mflag2 & MFLAG2_CHAMELEON) || !(n_ptr->mflag2 & MFLAG2_CHAMELEON)) return TRUE;
	}

	if (is_hostile(m_ptr) != is_hostile(n_ptr))
	{
		return TRUE;
	}

	return FALSE;
}


/*!
 * @brief モンスターがプレイヤーに対して敵意を抱くかどうかを返す
 * Check if this monster race has "hostile" alignment
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param m_ptr モンスター情報構造体の参照ポインタ
 * @param pa_good プレイヤーの善傾向値
 * @param pa_evil プレイヤーの悪傾向値
 * @param r_ptr モンスター種族情報の構造体参照ポインタ
 * @return プレイヤーに敵意を持つならばTRUEを返す
 * @details
 * If user is player, m_ptr == NULL.
 */
bool monster_has_hostile_align(player_type *player_ptr, monster_type *m_ptr, int pa_good, int pa_evil, monster_race *r_ptr)
{
	byte sub_align1 = SUB_ALIGN_NEUTRAL;
	byte sub_align2 = SUB_ALIGN_NEUTRAL;

	if (m_ptr) /* For a monster */
	{
		sub_align1 = m_ptr->sub_align;
	}
	else /* For player */
	{
		if (player_ptr->align >= pa_good) sub_align1 |= SUB_ALIGN_GOOD;
		if (player_ptr->align <= pa_evil) sub_align1 |= SUB_ALIGN_EVIL;
	}

	/* Racial alignment flags */
	if (r_ptr->flags3 & RF3_EVIL) sub_align2 |= SUB_ALIGN_EVIL;
	if (r_ptr->flags3 & RF3_GOOD) sub_align2 |= SUB_ALIGN_GOOD;

	if (check_hostile_align(sub_align1, sub_align2)) return TRUE;

	return FALSE;
}


bool is_original_ap_and_seen(player_type *player_ptr, monster_type *m_ptr)
{
	return m_ptr->ml && !player_ptr->image && (m_ptr->ap_r_idx == m_ptr->r_idx);
}
