/*!
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
 * @brief モンスター情報のヘッダを記述する
 * Hack -- Display the "name" and "attr/chars" of a monster race
 * @param r_idx モンスターの種族ID
 * @return なし
 */
void roff_top(MONRACE_IDX r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];
	char c1 = r_ptr->d_char;
	char c2 = r_ptr->x_char;

	TERM_COLOR a1 = r_ptr->d_attr;
	TERM_COLOR a2 = r_ptr->x_attr;

	Term_erase(0, 0, 255);
	Term_gotoxy(0, 0);

#ifdef JP
#else
	if (!(r_ptr->flags1 & RF1_UNIQUE))
	{
		Term_addstr(-1, TERM_WHITE, "The ");
	}
#endif

	Term_addstr(-1, TERM_WHITE, (r_name + r_ptr->name));

	Term_addstr(-1, TERM_WHITE, " ('");
	Term_add_bigch(a1, c1);
	Term_addstr(-1, TERM_WHITE, "')");

	Term_addstr(-1, TERM_WHITE, "/('");
	Term_add_bigch(a2, c2);
	Term_addstr(-1, TERM_WHITE, "'):");

	if (!current_world_ptr->wizard) return;

	char buf[16];
	sprintf(buf, "%d", r_idx);
	Term_addstr(-1, TERM_WHITE, " (");
	Term_addstr(-1, TERM_L_BLUE, buf);
	Term_addch(TERM_WHITE, ')');
}


/*!
 * @brief  モンスター情報の表示と共に画面を一時消去するサブルーチン /
 * Hack -- describe the given monster race at the top of the screen
 * @param r_idx モンスターの種族ID
 * @param mode 表示オプション
 * @return なし
 */
void screen_roff(player_type *player_ptr, MONRACE_IDX r_idx, BIT_FLAGS mode)
{
	msg_erase();
	Term_erase(0, 1, 255);
	hook_c_roff = c_roff;
	process_monster_lore(player_ptr, r_idx, mode);
	roff_top(r_idx);
}


/*!
 * @brief モンスター情報の現在のウィンドウに表示する /
 * Hack -- describe the given monster race in the current "term" window
 * @param r_idx モンスターの種族ID
 * @return なし
 */
void display_roff(player_type *player_ptr)
{
	for (int y = 0; y < Term->hgt; y++)
	{
		Term_erase(0, y, 255);
	}

	Term_gotoxy(0, 1);
	hook_c_roff = c_roff;
	MONRACE_IDX r_idx = player_ptr->monster_race_idx;
	process_monster_lore(player_ptr, r_idx, 0);
	roff_top(r_idx);
}


/*!
 * @brief モンスター詳細情報を自動スポイラー向けに出力する /
 * Hack -- output description of the given monster race
 * @param r_idx モンスターの種族ID
 * @param roff_func 出力処理を行う関数ポインタ
 * @return なし
 */
void output_monster_spoiler(player_type *player_ptr, MONRACE_IDX r_idx, void(*roff_func)(TERM_COLOR attr, concptr str))
{
	hook_c_roff = roff_func;
	process_monster_lore(player_ptr, r_idx, 0x03);
}


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


/*!
 * @brief モンスターを倒した際の財宝svalを返す
 * @param r_idx 倒したモンスターの種族ID
 * @return 財宝のsval
 * @details
 * Hack -- Return the "automatic coin type" of a monster race
 * Used to allocate proper treasure when "Creeping coins" die
 * Note the use of actual "monster names"
 */
static OBJECT_SUBTYPE_VALUE get_coin_type(MONRACE_IDX r_idx)
{
	switch (r_idx)
	{
	case MON_COPPER_COINS: return 2;
	case MON_SILVER_COINS: return 5;
	case MON_GOLD_COINS: return 10;
	case MON_MITHRIL_COINS:
	case MON_MITHRIL_GOLEM: return 16;
	case MON_ADAMANT_COINS: return 17;
	}

	return 0;
}


/*!
 * @brief モンスターが死亡した時の処理 /
 * Handle the "death" of a monster.
 * @param m_idx 死亡したモンスターのID
 * @param drop_item TRUEならばモンスターのドロップ処理を行う
 * @return 撃破されたモンスターの述語
 * @details
 * <pre>
 * Disperse treasures centered at the monster location based on the
 * various flags contained in the monster flags fields.
 * Check for "Quest" completion when a quest monster is killed.
 * Note that only the player can induce "monster_death()" on Uniques.
 * Thus (for now) all Quest monsters should be Uniques.
 * Note that monsters can now carry objects, and when a monster dies,
 * it drops all of its objects, which may disappear in crowded rooms.
 * </pre>
 */
void monster_death(player_type *player_ptr, MONSTER_IDX m_idx, bool drop_item)
{
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	monster_type *m_ptr = &floor_ptr->m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	bool do_gold = (!(r_ptr->flags1 & RF1_ONLY_ITEM));
	bool do_item = (!(r_ptr->flags1 & RF1_ONLY_GOLD));
	bool cloned = (m_ptr->smart & SM_CLONED) ? TRUE : FALSE;
	int force_coin = get_coin_type(m_ptr->r_idx);

	bool drop_chosen_item = drop_item && !cloned && !floor_ptr->inside_arena && !player_ptr->phase_out && !is_pet(m_ptr);

	if (current_world_ptr->timewalk_m_idx && current_world_ptr->timewalk_m_idx == m_idx)
	{
		current_world_ptr->timewalk_m_idx = 0;
	}

	if (r_ptr->flags7 & (RF7_LITE_MASK | RF7_DARK_MASK))
	{
		player_ptr->update |= (PU_MON_LITE);
	}

	POSITION y = m_ptr->fy;
	POSITION x = m_ptr->fx;

	if (record_named_pet && is_pet(m_ptr) && m_ptr->nickname)
	{
		GAME_TEXT m_name[MAX_NLEN];

		monster_desc(player_ptr, m_name, m_ptr, MD_INDEF_VISIBLE);
		exe_write_diary(player_ptr, DIARY_NAMED_PET, 3, m_name);
	}

	for (int i = 0; i < 4; i++)
	{
		if (r_ptr->blow[i].method != RBM_EXPLODE) continue;

		BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
		EFFECT_ID typ = mbe_info[r_ptr->blow[i].effect].explode_type;
		DICE_NUMBER d_dice = r_ptr->blow[i].d_dice;
		DICE_SID d_side = r_ptr->blow[i].d_side;
		HIT_POINT damage = damroll(d_dice, d_side);

		project(player_ptr, m_idx, 3, y, x, damage, typ, flg, -1);
		break;
	}

	if (m_ptr->mflag2 & MFLAG2_CHAMELEON)
	{
		choose_new_monster(player_ptr, m_idx, TRUE, MON_CHAMELEON);
		r_ptr = &r_info[m_ptr->r_idx];
	}

	check_quest_completion(player_ptr, m_ptr);

	object_type forge;
	object_type *q_ptr;
	if (floor_ptr->inside_arena && !is_pet(m_ptr))
	{
		player_ptr->exit_bldg = TRUE;
		if (player_ptr->arena_number > MAX_ARENA_MONS)
		{
			msg_print(_("素晴らしい！君こそ真の勝利者だ。", "You are a Genuine Champion!"));
		}
		else
		{
			msg_print(_("勝利！チャンピオンへの道を進んでいる。", "Victorious! You're on your way to becoming Champion."));
		}

		if (arena_info[player_ptr->arena_number].tval)
		{
			q_ptr = &forge;
			object_prep(q_ptr, lookup_kind(arena_info[player_ptr->arena_number].tval, arena_info[player_ptr->arena_number].sval));
			apply_magic(player_ptr, q_ptr, floor_ptr->object_level, AM_NO_FIXED_ART);
			(void)drop_near(player_ptr, q_ptr, -1, y, x);
		}

		if (player_ptr->arena_number > MAX_ARENA_MONS) player_ptr->arena_number++;
		player_ptr->arena_number++;
		if (record_arena)
		{
			GAME_TEXT m_name[MAX_NLEN];
			monster_desc(player_ptr, m_name, m_ptr, MD_WRONGDOER_NAME);
			exe_write_diary(player_ptr, DIARY_ARENA, player_ptr->arena_number, m_name);
		}
	}
	
	if (m_idx == player_ptr->riding && process_fall_off_horse(player_ptr, -1, FALSE))
	{
			msg_print(_("地面に落とされた。", "You have fallen from the pet you were riding."));
	}

	bool is_drop_corpse = one_in_(r_ptr->flags1 & RF1_UNIQUE ? 1 : 4);
	is_drop_corpse &= (r_ptr->flags9 & (RF9_DROP_CORPSE | RF9_DROP_SKELETON)) != 0;
	is_drop_corpse &= !(floor_ptr->inside_arena || player_ptr->phase_out || cloned || ((m_ptr->r_idx == today_mon) && is_pet(m_ptr)));
	if (is_drop_corpse)
	{
		bool corpse = FALSE;

		if (!(r_ptr->flags9 & RF9_DROP_SKELETON))
			corpse = TRUE;
		else if ((r_ptr->flags9 & RF9_DROP_CORPSE) && (r_ptr->flags1 & RF1_UNIQUE))
			corpse = TRUE;
		else if (r_ptr->flags9 & RF9_DROP_CORPSE)
		{
			if ((0 - ((m_ptr->maxhp) / 4)) > m_ptr->hp)
			{
				if (one_in_(5)) corpse = TRUE;
			}
			else
			{
				if (!one_in_(5)) corpse = TRUE;
			}
		}

		q_ptr = &forge;
		object_prep(q_ptr, lookup_kind(TV_CORPSE, (corpse ? SV_CORPSE : SV_SKELETON)));
		apply_magic(player_ptr, q_ptr, floor_ptr->object_level, AM_NO_FIXED_ART);
		q_ptr->pval = m_ptr->r_idx;
		(void)drop_near(player_ptr, q_ptr, -1, y, x);
	}

	monster_drop_carried_objects(player_ptr, m_ptr);

	u32b mo_mode = 0L;
	if (r_ptr->flags1 & RF1_DROP_GOOD) mo_mode |= AM_GOOD;
	if (r_ptr->flags1 & RF1_DROP_GREAT) mo_mode |= AM_GREAT;

	switch (m_ptr->r_idx)
	{
	case MON_PINK_HORROR:
	{
		if (floor_ptr->inside_arena || player_ptr->phase_out) break;

		bool notice = FALSE;
		for (int i = 0; i < 2; i++)
		{
			POSITION wy = y, wx = x;
			bool pet = is_pet(m_ptr);
			BIT_FLAGS mode = 0L;

			if (pet)
			{
				mode |= PM_FORCE_PET;
			}

			if (summon_specific(player_ptr, (pet ? -1 : m_idx), wy, wx, 100, SUMMON_BLUE_HORROR, mode))
			{
				if (player_can_see_bold(player_ptr, wy, wx)) notice = TRUE;
			}
		}

		if (notice)
		{
			msg_print(_("ピンク・ホラーは分裂した！", "The Pink horror divides!"));
		}

		break;
	}
	case MON_BLOODLETTER:
	{
		if (!drop_chosen_item || (randint1(100) >= 15)) break;

		q_ptr = &forge;
		object_prep(q_ptr, lookup_kind(TV_SWORD, SV_BLADE_OF_CHAOS));
		apply_magic(player_ptr, q_ptr, floor_ptr->object_level, AM_NO_FIXED_ART | mo_mode);
		(void)drop_near(player_ptr, q_ptr, -1, y, x);
		break;
	}
	case MON_RAAL:
	{
		if (!drop_chosen_item || (floor_ptr->dun_level <= 9)) break;

		q_ptr = &forge;
		object_wipe(q_ptr);
		if ((floor_ptr->dun_level > 49) && one_in_(5))
			get_obj_num_hook = kind_is_good_book;
		else
			get_obj_num_hook = kind_is_book;

		make_object(player_ptr, q_ptr, mo_mode);
		(void)drop_near(player_ptr, q_ptr, -1, y, x);
		break;
	}
	case MON_DAWN:
	{
		if (floor_ptr->inside_arena || player_ptr->phase_out) break;
		if (one_in_(7)) break;

		POSITION wy = y, wx = x;
		int attempts = 100;
		bool pet = is_pet(m_ptr);
		do
		{
			scatter(player_ptr, &wy, &wx, y, x, 20, 0);
		} while (!(in_bounds(floor_ptr, wy, wx) && is_cave_empty_bold2(player_ptr, wy, wx)) && --attempts);

		if (attempts <= 0) break;

		BIT_FLAGS mode = 0L;
		if (pet) mode |= PM_FORCE_PET;

		if (summon_specific(player_ptr, (pet ? -1 : m_idx), wy, wx, 100, SUMMON_DAWN, mode))
		{
			if (player_can_see_bold(player_ptr, wy, wx))
				msg_print(_("新たな戦士が現れた！", "A new warrior steps forth!"));
		}

		break;
	}
	case MON_UNMAKER:
	{
		BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
		(void)project(player_ptr, m_idx, 6, y, x, 100, GF_CHAOS, flg, -1);
		break;
	}
	case MON_UNICORN_ORD:
	case MON_MORGOTH:
	case MON_ONE_RING:
	{
		if (player_ptr->pseikaku != PERSONALITY_LAZY) break;
		if (!drop_chosen_item) break;

		ARTIFACT_IDX a_idx = 0;
		artifact_type *a_ptr = NULL;
		do
		{
			switch (randint0(3))
			{
			case 0:
				a_idx = ART_NAMAKE_HAMMER;
				break;
			case 1:
				a_idx = ART_NAMAKE_BOW;
				break;
			case 2:
				a_idx = ART_NAMAKE_ARMOR;
				break;
			}

			a_ptr = &a_info[a_idx];
		} while (a_ptr->cur_num);

		if (create_named_art(player_ptr, a_idx, y, x))
		{
			a_ptr->cur_num = 1;
			if (current_world_ptr->character_dungeon) a_ptr->floor_id = player_ptr->floor_id;
		}
		else if (!preserve_mode) a_ptr->cur_num = 1;

		break;
	}
	case MON_SERPENT:
	{
		if (!drop_chosen_item) break;

		q_ptr = &forge;
		object_prep(q_ptr, lookup_kind(TV_HAFTED, SV_GROND));
		q_ptr->name1 = ART_GROND;
		apply_magic(player_ptr, q_ptr, -1, AM_GOOD | AM_GREAT);
		(void)drop_near(player_ptr, q_ptr, -1, y, x);
		q_ptr = &forge;
		object_prep(q_ptr, lookup_kind(TV_CROWN, SV_CHAOS));
		q_ptr->name1 = ART_CHAOS;
		apply_magic(player_ptr, q_ptr, -1, AM_GOOD | AM_GREAT);
		(void)drop_near(player_ptr, q_ptr, -1, y, x);
		break;
	}
	case MON_B_DEATH_SWORD:
	{
		if (!drop_chosen_item) break;

		q_ptr = &forge;
		object_prep(q_ptr, lookup_kind(TV_SWORD, randint1(2)));
		(void)drop_near(player_ptr, q_ptr, -1, y, x);
		break;
	}
	case MON_A_GOLD:
	case MON_A_SILVER:
	{
		bool is_drop_can = drop_chosen_item;
		bool is_silver = m_ptr->r_idx == MON_A_SILVER;
		is_silver &= r_ptr->r_akills % 5 == 0;
		is_drop_can &= (m_ptr->r_idx == MON_A_GOLD) || is_silver;
		if (!is_drop_can) break;

		q_ptr = &forge;
		object_prep(q_ptr, lookup_kind(TV_CHEST, SV_CHEST_KANDUME));
		apply_magic(player_ptr, q_ptr, floor_ptr->object_level, AM_NO_FIXED_ART);
		(void)drop_near(player_ptr, q_ptr, -1, y, x);
		break;
	}

	case MON_ROLENTO:
	{
		BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
		(void)project(player_ptr, m_idx, 3, y, x, damroll(20, 10), GF_FIRE, flg, -1);
		break;
	}
	default:
	{
		if (!drop_chosen_item) break;

		switch (r_ptr->d_char)
		{
		case '(':
		{
			if (floor_ptr->dun_level <= 0) break;

			q_ptr = &forge;
			object_wipe(q_ptr);
			get_obj_num_hook = kind_is_cloak;
			make_object(player_ptr, q_ptr, mo_mode);
			(void)drop_near(player_ptr, q_ptr, -1, y, x);
			break;
		}
		case '/':
		{
			if (floor_ptr->dun_level <= 4) break;

			q_ptr = &forge;
			object_wipe(q_ptr);
			get_obj_num_hook = kind_is_polearm;
			make_object(player_ptr, q_ptr, mo_mode);
			(void)drop_near(player_ptr, q_ptr, -1, y, x);
			break;
		}
		case '[':
		{
			if (floor_ptr->dun_level <= 19) break;

			q_ptr = &forge;
			object_wipe(q_ptr);
			get_obj_num_hook = kind_is_armor;
			make_object(player_ptr, q_ptr, mo_mode);
			(void)drop_near(player_ptr, q_ptr, -1, y, x);
			break;
		}
		case '\\':
		{
			if (floor_ptr->dun_level <= 4) break;
			q_ptr = &forge;
			object_wipe(q_ptr);
			get_obj_num_hook = kind_is_hafted;
			make_object(player_ptr, q_ptr, mo_mode);
			(void)drop_near(player_ptr, q_ptr, -1, y, x);
			break;
		}
		case '|':
		{
			if (m_ptr->r_idx == MON_STORMBRINGER) break;

			q_ptr = &forge;
			object_wipe(q_ptr);
			get_obj_num_hook = kind_is_sword;
			make_object(player_ptr, q_ptr, mo_mode);
			(void)drop_near(player_ptr, q_ptr, -1, y, x);
			break;
		}
		}
	}
	}

	if (drop_chosen_item)
	{
		ARTIFACT_IDX a_idx = 0;
		PERCENTAGE chance = 0;
		for (int i = 0; i < 4; i++)
		{
			if (!r_ptr->artifact_id[i]) break;
			a_idx = r_ptr->artifact_id[i];
			chance = r_ptr->artifact_percent[i];
			if (randint0(100) < chance || current_world_ptr->wizard)
			{
				artifact_type *a_ptr = &a_info[a_idx];
				if (!a_ptr->cur_num)
				{
					if (create_named_art(player_ptr, a_idx, y, x))
					{
						a_ptr->cur_num = 1;
						if (current_world_ptr->character_dungeon) a_ptr->floor_id = player_ptr->floor_id;
					}
					else if (!preserve_mode)
					{
						a_ptr->cur_num = 1;
					}
				}
			}
		}

		if ((r_ptr->flags7 & RF7_GUARDIAN) && (d_info[player_ptr->dungeon_idx].final_guardian == m_ptr->r_idx))
		{
			KIND_OBJECT_IDX k_idx = d_info[player_ptr->dungeon_idx].final_object != 0
				? d_info[player_ptr->dungeon_idx].final_object
				: lookup_kind(TV_SCROLL, SV_SCROLL_ACQUIREMENT);

			if (d_info[player_ptr->dungeon_idx].final_artifact != 0)
			{
				a_idx = d_info[player_ptr->dungeon_idx].final_artifact;
				artifact_type *a_ptr = &a_info[a_idx];
				if (!a_ptr->cur_num)
				{
					if (create_named_art(player_ptr, a_idx, y, x))
					{
						a_ptr->cur_num = 1;
						if (current_world_ptr->character_dungeon) a_ptr->floor_id = player_ptr->floor_id;
					}
					else if (!preserve_mode)
					{
						a_ptr->cur_num = 1;
					}

					if (!d_info[player_ptr->dungeon_idx].final_object) k_idx = 0;
				}
			}

			if (k_idx != 0)
			{
				q_ptr = &forge;
				object_prep(q_ptr, k_idx);
				apply_magic(player_ptr, q_ptr, floor_ptr->object_level, AM_NO_FIXED_ART | AM_GOOD);
				(void)drop_near(player_ptr, q_ptr, -1, y, x);
			}

			msg_format(_("あなたは%sを制覇した！", "You have conquered %s!"), d_name + d_info[player_ptr->dungeon_idx].name);
		}
	}

	int number = 0;
	if ((r_ptr->flags1 & RF1_DROP_60) && (randint0(100) < 60)) number++;
	if ((r_ptr->flags1 & RF1_DROP_90) && (randint0(100) < 90)) number++;
	if (r_ptr->flags1 & RF1_DROP_1D2) number += damroll(1, 2);
	if (r_ptr->flags1 & RF1_DROP_2D2) number += damroll(2, 2);
	if (r_ptr->flags1 & RF1_DROP_3D2) number += damroll(3, 2);
	if (r_ptr->flags1 & RF1_DROP_4D2) number += damroll(4, 2);

	if (cloned && !(r_ptr->flags1 & RF1_UNIQUE))
		number = 0;

	if (is_pet(m_ptr) || player_ptr->phase_out || floor_ptr->inside_arena)
		number = 0;

	if (!drop_item && (r_ptr->d_char != '$'))
		number = 0;

	if ((r_ptr->flags2 & (RF2_MULTIPLY)) && (r_ptr->r_akills > 1024))
		number = 0;

	coin_type = force_coin;
	floor_ptr->object_level = (floor_ptr->dun_level + r_ptr->level) / 2;

	int dump_item = 0;
	int dump_gold = 0;
	for (int i = 0; i < number; i++)
	{
		q_ptr = &forge;
		object_wipe(q_ptr);

		if (do_gold && (!do_item || (randint0(100) < 50)))
		{
			if (!make_gold(floor_ptr, q_ptr)) continue;
			dump_gold++;
		}
		else
		{
			if (!make_object(player_ptr, q_ptr, mo_mode)) continue;
			dump_item++;
		}

		(void)drop_near(player_ptr, q_ptr, -1, y, x);
	}

	floor_ptr->object_level = floor_ptr->base_level;
	coin_type = 0;
	bool visible = (m_ptr->ml && !player_ptr->image) || ((r_ptr->flags1 & RF1_UNIQUE) != 0);
	if (visible && (dump_item || dump_gold))
	{
		lore_treasure(player_ptr, m_idx, dump_item, dump_gold);
	}

	if (!(r_ptr->flags1 & RF1_QUESTOR)) return;
	if (player_ptr->phase_out) return;
	if ((m_ptr->r_idx != MON_SERPENT) || cloned) return;

	current_world_ptr->total_winner = TRUE;
	player_ptr->redraw |= (PR_TITLE);
	play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_FINAL_QUEST_CLEAR);
	exe_write_diary(player_ptr, DIARY_DESCRIPTION, 0, _("見事に変愚蛮怒の勝利者となった！", "finally became *WINNER* of Hengband!"));
	admire_from_patron(player_ptr);
	msg_print(_("*** おめでとう ***", "*** CONGRATULATIONS ***"));
	msg_print(_("あなたはゲームをコンプリートしました。", "You have won the game!"));
	msg_print(_("準備が整ったら引退(自殺コマンド)しても結構です。", "You may retire (commit suicide) when you are ready."));
}


/*!
 * @brief モンスターを撃破した際の述語メッセージを返す /
 * Return monster death string
 * @param r_ptr 撃破されたモンスターの種族情報を持つ構造体の参照ポインタ
 * @return 撃破されたモンスターの述語
 */
concptr extract_note_dies(MONRACE_IDX r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];
	if (monster_living(r_idx)) return _("は死んだ。", " dies.");

	for (int i = 0; i < 4; i++)
	{
		if (r_ptr->blow[i].method == RBM_EXPLODE)
		{
			return _("は爆発して粉々になった。", " explodes into tiny shreds.");
		}
	}

	return _("を倒した。", " is destroyed.");
}


/*
 * Monster health description
 */
concptr look_mon_desc(monster_type *m_ptr, BIT_FLAGS mode)
{
	bool living = monster_living(m_ptr->ap_r_idx);
	int perc = m_ptr->maxhp > 0 ? 100L * m_ptr->hp / m_ptr->maxhp : 0;

	concptr desc;
	if (m_ptr->hp >= m_ptr->maxhp)
	{
		desc = living ? _("無傷", "unhurt") : _("無ダメージ", "undamaged");
	}
	else if (perc >= 60)
	{
		desc = living ? _("軽傷", "somewhat wounded") : _("小ダメージ", "somewhat damaged");
	}
	else if (perc >= 25)
	{
		desc = living ? _("負傷", "wounded") : _("中ダメージ", "damaged");
	}
	else if (perc >= 10)
	{
		desc = living ? _("重傷", "badly wounded") : _("大ダメージ", "badly damaged");
	}
	else
	{
		desc = living ? _("半死半生", "almost dead") : _("倒れかけ", "almost destroyed");
	}

	concptr attitude;
	if (!(mode & 0x01))
	{
		attitude = "";
	}
	else if (is_pet(m_ptr))
	{
		attitude = _(", ペット", ", pet");
	}
	else if (is_friendly(m_ptr))
	{
		attitude = _(", 友好的", ", friendly");
	}
	else
	{
		attitude = _("", "");
	}

	concptr clone = (m_ptr->smart & SM_CLONED) ? ", clone" : "";
	monster_race *ap_r_ptr = &r_info[m_ptr->ap_r_idx];
	if (ap_r_ptr->r_tkills && !(m_ptr->mflag2 & MFLAG2_KAGE))
	{
		return format(_("レベル%d, %s%s%s", "Level %d, %s%s%s"), ap_r_ptr->level, desc, attitude, clone);
	}

	return format(_("レベル???, %s%s%s", "Level ???, %s%s%s"), desc, attitude, clone);
}


bool is_original_ap_and_seen(player_type *player_ptr, monster_type *m_ptr)
{
	return m_ptr->ml && !player_ptr->image && (m_ptr->ap_r_idx == m_ptr->r_idx);
}
