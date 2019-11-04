/*!
 * @file spells3.c
 * @brief 魔法効果の実装/ Spell code (part 3)
 * @date 2014/07/26
 * @author
 * <pre>
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * </pre>
 */

#include "angband.h"
#include "bldg.h"
#include "core.h"
#include "term.h"
#include "util.h"
#include "object-ego.h"

#include "creature.h"

#include "dungeon.h"
#include "floor.h"
#include "floor-town.h"
#include "object-boost.h"
#include "object-flavor.h"
#include "object-hook.h"
#include "melee.h"
#include "player-move.h"
#include "player-status.h"
#include "player-class.h"
#include "player-damage.h"
#include "player-inventory.h"
#include "spells-summon.h"
#include "quest.h"
#include "artifact.h"
#include "avatar.h"
#include "spells.h"
#include "spells-floor.h"
#include "grid.h"
#include "monster-process.h"
#include "monster-status.h"
#include "monster-spell.h"
#include "cmd-spell.h"
#include "cmd-dump.h"
#include "snipe.h"
#include "floor-save.h"
#include "files.h"
#include "player-effects.h"
#include "player-skill.h"
#include "player-personality.h"
#include "view-mainwindow.h"
#include "mind.h"
#include "wild.h"
#include "world.h"
#include "objectkind.h"
#include "autopick.h"
#include "targeting.h"


/*! テレポート先探索の試行数 / Maximum number of tries for teleporting */
#define MAX_TRIES 100


/*!
 * @brief モンスターのテレポートアウェイ処理 /
 * Teleport a monster, normally up to "dis" grids away.
 * @param m_idx モンスターID
 * @param dis テレポート距離
 * @param mode オプション
 * @return テレポートが実際に行われたらtrue
 * @details
 * Attempt to move the monster at least "dis/2" grids away.
 * But allow variation to prevent infinite loops.
 */
bool teleport_away(MONSTER_IDX m_idx, POSITION dis, BIT_FLAGS mode)
{
	POSITION oy, ox, d, i, min;
	int tries = 0;
	POSITION ny = 0, nx = 0;

	bool look = TRUE;

	monster_type *m_ptr = &p_ptr->current_floor_ptr->m_list[m_idx];
	if (!monster_is_valid(m_ptr)) return (FALSE);

	oy = m_ptr->fy;
	ox = m_ptr->fx;

	/* Minimum distance */
	min = dis / 2;

	if ((mode & TELEPORT_DEC_VALOUR) &&
	    (((p_ptr->chp * 10) / p_ptr->mhp) > 5) &&
		(4+randint1(5) < ((p_ptr->chp * 10) / p_ptr->mhp)))
	{
		chg_virtue(p_ptr, V_VALOUR, -1);
	}

	/* Look until done */
	while (look)
	{
		tries++;

		/* Verify max distance */
		if (dis > 200) dis = 200;

		/* Try several locations */
		for (i = 0; i < 500; i++)
		{
			/* Pick a (possibly illegal) location */
			while (1)
			{
				ny = rand_spread(oy, dis);
				nx = rand_spread(ox, dis);
				d = distance(oy, ox, ny, nx);
				if ((d >= min) && (d <= dis)) break;
			}

			/* Ignore illegal locations */
			if (!in_bounds(p_ptr->current_floor_ptr, ny, nx)) continue;

			if (!cave_monster_teleportable_bold(m_idx, ny, nx, mode)) continue;

			/* No teleporting into vaults and such */
			if (!(p_ptr->inside_quest || p_ptr->inside_arena))
				if (p_ptr->current_floor_ptr->grid_array[ny][nx].info & CAVE_ICKY) continue;

			/* This grid looks good */
			look = FALSE;

			/* Stop looking */
			break;
		}

		/* Increase the maximum distance */
		dis = dis * 2;

		/* Decrease the minimum distance */
		min = min / 2;

		/* Stop after MAX_TRIES tries */
		if (tries > MAX_TRIES) return (FALSE);
	}

	sound(SOUND_TPOTHER);

	/* Update the old location */
	p_ptr->current_floor_ptr->grid_array[oy][ox].m_idx = 0;

	/* Update the new location */
	p_ptr->current_floor_ptr->grid_array[ny][nx].m_idx = m_idx;

	/* Move the monster */
	m_ptr->fy = ny;
	m_ptr->fx = nx;

	/* Forget the counter target */
	reset_target(m_ptr);

	update_monster(m_idx, TRUE);
	lite_spot(oy, ox);
	lite_spot(ny, nx);

	if (r_info[m_ptr->r_idx].flags7 & (RF7_LITE_MASK | RF7_DARK_MASK))
		p_ptr->update |= (PU_MON_LITE);

	return (TRUE);
}


/*!
 * @brief モンスターを指定された座標付近にテレポートする /
 * Teleport monster next to a grid near the given location
 * @param m_idx モンスターID
 * @param ty 目安Y座標
 * @param tx 目安X座標
 * @param power テレポート成功確率
 * @param mode オプション
 * @return なし
 */
void teleport_monster_to(MONSTER_IDX m_idx, POSITION ty, POSITION tx, int power, BIT_FLAGS mode)
{
	POSITION ny, nx, oy, ox;
	int d, i, min;
	int attempts = 500;
	POSITION dis = 2;
	bool look = TRUE;
	monster_type *m_ptr = &p_ptr->current_floor_ptr->m_list[m_idx];
	if(!m_ptr->r_idx) return;

	/* "Skill" test */
	if(randint1(100) > power) return;

	ny = m_ptr->fy;
	nx = m_ptr->fx;
	oy = m_ptr->fy;
	ox = m_ptr->fx;

	/* Minimum distance */
	min = dis / 2;

	/* Look until done */
	while (look && --attempts)
	{
		/* Verify max distance */
		if (dis > 200) dis = 200;

		/* Try several locations */
		for (i = 0; i < 500; i++)
		{
			/* Pick a (possibly illegal) location */
			while (1)
			{
				ny = rand_spread(ty, dis);
				nx = rand_spread(tx, dis);
				d = distance(ty, tx, ny, nx);
				if ((d >= min) && (d <= dis)) break;
			}

			/* Ignore illegal locations */
			if (!in_bounds(p_ptr->current_floor_ptr, ny, nx)) continue;

			if (!cave_monster_teleportable_bold(m_idx, ny, nx, mode)) continue;

			/* No teleporting into vaults and such */
			/* if (p_ptr->current_floor_ptr->grid_array[ny][nx].info & (CAVE_ICKY)) continue; */

			/* This grid looks good */
			look = FALSE;

			/* Stop looking */
			break;
		}

		/* Increase the maximum distance */
		dis = dis * 2;

		/* Decrease the minimum distance */
		min = min / 2;
	}

	if (attempts < 1) return;

	sound(SOUND_TPOTHER);

	/* Update the old location */
	p_ptr->current_floor_ptr->grid_array[oy][ox].m_idx = 0;

	/* Update the new location */
	p_ptr->current_floor_ptr->grid_array[ny][nx].m_idx = m_idx;

	/* Move the monster */
	m_ptr->fy = ny;
	m_ptr->fx = nx;

	update_monster(m_idx, TRUE);
	lite_spot(oy, ox);
	lite_spot(ny, nx);

	if (r_info[m_ptr->r_idx].flags7 & (RF7_LITE_MASK | RF7_DARK_MASK))
		p_ptr->update |= (PU_MON_LITE);
}

/*!
 * @brief プレイヤーのテレポート先選定と移動処理 /
 * Teleport the player to a location up to "dis" grids away.
 * @param dis 基本移動距離
 * @param mode オプション
 * @return 実際にテレポート処理が行われたらtrue
 * @details
 * <pre>
 * If no such spaces are readily available, the distance may increase.
 * Try very hard to move the player at least a quarter that distance.
 *
 * There was a nasty tendency for a long time; which was causing the
 * player to "bounce" between two or three different spots because
 * these are the only spots that are "far enough" way to satisfy the
 * algorithm.
 *
 * But this tendency is now removed; in the new algorithm, a list of
 * candidates is selected first, which includes at least 50% of all
 * floor grids within the distance, and any single grid in this list
 * of candidates has equal possibility to be choosen as a destination.
 * </pre>
 */

bool teleport_player_aux(player_type *creature_ptr, POSITION dis, BIT_FLAGS mode)
{
	int candidates_at[MAX_TELEPORT_DISTANCE + 1];
	int total_candidates, cur_candidates;
	POSITION y = 0, x = 0;
	int min, pick, i;

	int left = MAX(1, creature_ptr->x - dis);
	int right = MIN(creature_ptr->current_floor_ptr->width - 2, creature_ptr->x + dis);
	int top = MAX(1, creature_ptr->y - dis);
	int bottom = MIN(creature_ptr->current_floor_ptr->height - 2, creature_ptr->y + dis);

	if (creature_ptr->wild_mode) return FALSE;

	if (creature_ptr->anti_tele && !(mode & TELEPORT_NONMAGICAL))
	{
		msg_print(_("不思議な力がテレポートを防いだ！", "A mysterious force prevents you from teleporting!"));
		return FALSE;
	}

	/* Initialize counters */
	total_candidates = 0;
	for (i = 0; i <= MAX_TELEPORT_DISTANCE; i++)
		candidates_at[i] = 0;

	/* Limit the distance */
	if (dis > MAX_TELEPORT_DISTANCE) dis = MAX_TELEPORT_DISTANCE;

	/* Search valid locations */
	for (y = top; y <= bottom; y++)
	{
		for (x = left; x <= right; x++)
		{
			int d;

			/* Skip illegal locations */
			if (!cave_player_teleportable_bold(y, x, mode)) continue;

			/* Calculate distance */
			d = distance(creature_ptr->y, creature_ptr->x, y, x);

			/* Skip too far locations */
			if (d > dis) continue;

			/* Count the total number of candidates */
			total_candidates++;

			/* Count the number of candidates in this circumference */
			candidates_at[d]++;
		}
	}

	/* No valid location! */
	if (0 == total_candidates) return FALSE;

	/* Fix the minimum distance */
	for (cur_candidates = 0, min = dis; min >= 0; min--)
	{
		cur_candidates += candidates_at[min];

		/* 50% of all candidates will have an equal chance to be choosen. */
		if (cur_candidates && (cur_candidates >= total_candidates / 2)) break;
	}

	/* Pick up a single location randomly */
	pick = randint1(cur_candidates);

	/* Search again the choosen location */
	for (y = top; y <= bottom; y++)
	{
		for (x = left; x <= right; x++)
		{
			int d;

			/* Skip illegal locations */
			if (!cave_player_teleportable_bold(y, x, mode)) continue;

			/* Calculate distance */
			d = distance(creature_ptr->y, creature_ptr->x, y, x);

			/* Skip too far locations */
			if (d > dis) continue;

			/* Skip too close locations */
			if (d < min) continue;

			/* This grid was picked up? */
			pick--;
			if (!pick) break;
		}

		/* Exit the loop */
		if (!pick) break;
	}

	if (player_bold(creature_ptr, y, x)) return FALSE;

	sound(SOUND_TELEPORT);

#ifdef JP
	if ((creature_ptr->pseikaku == SEIKAKU_COMBAT) || (creature_ptr->inventory_list[INVEN_BOW].name1 == ART_CRIMSON))
		msg_format("『こっちだぁ、%s』", creature_ptr->name);
#endif

	(void)move_player_effect(creature_ptr, y, x, MPE_FORGET_FLOW | MPE_HANDLE_STUFF | MPE_DONT_PICKUP);
	return TRUE;
}

/*!
 * @brief プレイヤーのテレポート処理メインルーチン
 * @param dis 基本移動距離
 * @param mode オプション
 * @return なし
 */
void teleport_player(POSITION dis, BIT_FLAGS mode)
{
	POSITION yy, xx;
	POSITION oy = p_ptr->y;
	POSITION ox = p_ptr->x;

	if (!teleport_player_aux(p_ptr, dis, mode)) return;

	/* Monsters with teleport ability may follow the player */
	for (xx = -1; xx < 2; xx++)
	{
		for (yy = -1; yy < 2; yy++)
		{
			MONSTER_IDX tmp_m_idx = p_ptr->current_floor_ptr->grid_array[oy+yy][ox+xx].m_idx;

			/* A monster except your mount may follow */
			if (tmp_m_idx && (p_ptr->riding != tmp_m_idx))
			{
				monster_type *m_ptr = &p_ptr->current_floor_ptr->m_list[tmp_m_idx];
				monster_race *r_ptr = &r_info[m_ptr->r_idx];

				/*
				 * The latter limitation is to avoid
				 * totally unkillable suckers...
				 */
				if ((r_ptr->a_ability_flags2 & RF6_TPORT) &&
				    !(r_ptr->flagsr & RFR_RES_TELE))
				{
					if (!MON_CSLEEP(m_ptr)) teleport_monster_to(tmp_m_idx, p_ptr->y, p_ptr->x, r_ptr->level, 0L);
				}
			}
		}
	}
}


/*!
 * @brief プレイヤーのテレポートアウェイ処理 /
 * @param m_idx アウェイを試みたプレイヤーID
 * @param dis テレポート距離
 * @return なし
 */
void teleport_player_away(MONSTER_IDX m_idx, POSITION dis)
{
	POSITION yy, xx;
	POSITION oy = p_ptr->y;
	POSITION ox = p_ptr->x;

	if (!teleport_player_aux(p_ptr, dis, TELEPORT_PASSIVE)) return;

	/* Monsters with teleport ability may follow the player */
	for (xx = -1; xx < 2; xx++)
	{
		for (yy = -1; yy < 2; yy++)
		{
			MONSTER_IDX tmp_m_idx = p_ptr->current_floor_ptr->grid_array[oy+yy][ox+xx].m_idx;

			/* A monster except your mount or caster may follow */
			if (tmp_m_idx && (p_ptr->riding != tmp_m_idx) && (m_idx != tmp_m_idx))
			{
				monster_type *m_ptr = &p_ptr->current_floor_ptr->m_list[tmp_m_idx];
				monster_race *r_ptr = &r_info[m_ptr->r_idx];

				/*
				 * The latter limitation is to avoid
				 * totally unkillable suckers...
				 */
				if ((r_ptr->a_ability_flags2 & RF6_TPORT) &&
				    !(r_ptr->flagsr & RFR_RES_TELE))
				{
					if (!MON_CSLEEP(m_ptr)) teleport_monster_to(tmp_m_idx, p_ptr->y, p_ptr->x, r_ptr->level, 0L);
				}
			}
		}
	}
}


/*!
 * @brief プレイヤーを指定位置近辺にテレポートさせる
 * Teleport player to a grid near the given location
 * @param ny 目標Y座標
 * @param nx 目標X座標
 * @param mode オプションフラグ
 * @return なし
 * @details
 * <pre>
 * This function is slightly obsessive about correctness.
 * This function allows teleporting into vaults (!)
 * </pre>
 */
void teleport_player_to(POSITION ny, POSITION nx, BIT_FLAGS mode)
{
	POSITION y, x;
	POSITION dis = 0, ctr = 0;

	if (p_ptr->anti_tele && !(mode & TELEPORT_NONMAGICAL))
	{
		msg_print(_("不思議な力がテレポートを防いだ！", "A mysterious force prevents you from teleporting!"));
		return;
	}

	/* Find a usable location */
	while (1)
	{
		/* Pick a nearby legal location */
		while (1)
		{
			y = (POSITION)rand_spread(ny, dis);
			x = (POSITION)rand_spread(nx, dis);
			if (in_bounds(p_ptr->current_floor_ptr, y, x)) break;
		}

		/* Accept any grid when wizard mode */
		if (current_world_ptr->wizard && !(mode & TELEPORT_PASSIVE) && (!p_ptr->current_floor_ptr->grid_array[y][x].m_idx || (p_ptr->current_floor_ptr->grid_array[y][x].m_idx == p_ptr->riding))) break;

		/* Accept teleportable floor grids */
		if (cave_player_teleportable_bold(y, x, mode)) break;

		/* Occasionally advance the distance */
		if (++ctr > (4 * dis * dis + 4 * dis + 1))
		{
			ctr = 0;
			dis++;
		}
	}

	sound(SOUND_TELEPORT);
	(void)move_player_effect(p_ptr, y, x, MPE_FORGET_FLOW | MPE_HANDLE_STUFF | MPE_DONT_PICKUP);
}


void teleport_away_followable(MONSTER_IDX m_idx)
{
	monster_type *m_ptr = &p_ptr->current_floor_ptr->m_list[m_idx];
	POSITION oldfy = m_ptr->fy;
	POSITION oldfx = m_ptr->fx;
	bool old_ml = m_ptr->ml;
	POSITION old_cdis = m_ptr->cdis;

	teleport_away(m_idx, MAX_SIGHT * 2 + 5, 0L);

	if (old_ml && (old_cdis <= MAX_SIGHT) && !current_world_ptr->timewalk_m_idx && !p_ptr->phase_out && los(p_ptr->y, p_ptr->x, oldfy, oldfx))
	{
		bool follow = FALSE;

		if ((p_ptr->muta1 & MUT1_VTELEPORT) || (p_ptr->pclass == CLASS_IMITATOR)) follow = TRUE;
		else
		{
			BIT_FLAGS flgs[TR_FLAG_SIZE];
			object_type *o_ptr;
			INVENTORY_IDX i;

			for (i = INVEN_RARM; i < INVEN_TOTAL; i++)
			{
				o_ptr = &p_ptr->inventory_list[i];
				if (o_ptr->k_idx && !object_is_cursed(o_ptr))
				{
					object_flags(o_ptr, flgs);
					if (have_flag(flgs, TR_TELEPORT))
					{
						follow = TRUE;
						break;
					}
				}
			}
		}

		if (follow)
		{
			if (get_check_strict(_("ついていきますか？", "Do you follow it? "), CHECK_OKAY_CANCEL))
			{
				if (one_in_(3))
				{
					teleport_player(200, TELEPORT_PASSIVE);
					msg_print(_("失敗！", "Failed!"));
				}
				else teleport_player_to(m_ptr->fy, m_ptr->fx, 0L);
				p_ptr->energy_need += ENERGY_NEED();
			}
		}
	}
}


bool teleport_level_other(player_type *creature_ptr)
{
	MONSTER_IDX target_m_idx;
	monster_type *m_ptr;
	monster_race *r_ptr;
	GAME_TEXT m_name[MAX_NLEN];

	if (!target_set(TARGET_KILL)) return FALSE;
	target_m_idx = p_ptr->current_floor_ptr->grid_array[target_row][target_col].m_idx;
	if (!target_m_idx) return TRUE;
	if (!player_has_los_bold(target_row, target_col)) return TRUE;
	if (!projectable(creature_ptr->y, creature_ptr->x, target_row, target_col)) return TRUE;
	m_ptr = &p_ptr->current_floor_ptr->m_list[target_m_idx];
	r_ptr = &r_info[m_ptr->r_idx];
	monster_desc(m_name, m_ptr, 0);
	msg_format(_("%^sの足を指さした。", "You gesture at %^s's feet."), m_name);

	if ((r_ptr->flagsr & (RFR_EFF_RES_NEXU_MASK | RFR_RES_TELE)) ||
		(r_ptr->flags1 & RF1_QUESTOR) || (r_ptr->level + randint1(50) > creature_ptr->lev + randint1(60)))
	{
		msg_format(_("しかし効果がなかった！", "%^s is unaffected!"), m_name);
	}
	else teleport_level(creature_ptr, target_m_idx);
	return TRUE;
}

/*!
 * @brief プレイヤー及びモンスターをレベルテレポートさせる /
 * Teleport the player one level up or down (random when legal)
 * @param m_idx テレポートの対象となるモンスターID(0ならばプレイヤー) / If m_idx <= 0, target is player.
 * @return なし
 */
void teleport_level(player_type *creature_ptr, MONSTER_IDX m_idx)
{
	bool go_up;
	GAME_TEXT m_name[160];
	bool see_m = TRUE;

	if (m_idx <= 0) /* To player */
	{
		strcpy(m_name, _("あなた", "you"));
	}
	else /* To monster */
	{
		monster_type *m_ptr = &creature_ptr->current_floor_ptr->m_list[m_idx];

		/* Get the monster name (or "it") */
		monster_desc(m_name, m_ptr, 0);

		see_m = is_seen(m_ptr);
	}

	/* No effect in some case */
	if (TELE_LEVEL_IS_INEFF(m_idx))
	{
		if (see_m) msg_print(_("効果がなかった。", "There is no effect."));
		return;
	}

	if ((m_idx <= 0) && creature_ptr->anti_tele) /* To player */
	{
		msg_print(_("不思議な力がテレポートを防いだ！", "A mysterious force prevents you from teleporting!"));
		return;
	}

	/* Choose up or down */
	if (randint0(100) < 50) go_up = TRUE;
	else go_up = FALSE;

	if ((m_idx <= 0) && current_world_ptr->wizard)
	{
		if (get_check("Force to go up? ")) go_up = TRUE;
		else if (get_check("Force to go down? ")) go_up = FALSE;
	}

	/* Down only */ 
	if ((ironman_downward && (m_idx <= 0)) || (creature_ptr->current_floor_ptr->dun_level <= d_info[creature_ptr->dungeon_idx].mindepth))
	{
#ifdef JP
		if (see_m) msg_format("%^sは床を突き破って沈んでいく。", m_name);
#else
		if (see_m) msg_format("%^s sink%s through the floor.", m_name, (m_idx <= 0) ? "" : "s");
#endif
		if (m_idx <= 0) /* To player */
		{
			if (!creature_ptr->current_floor_ptr->dun_level)
			{
				creature_ptr->dungeon_idx = ironman_downward ? DUNGEON_ANGBAND : creature_ptr->recall_dungeon;
				creature_ptr->oldpy = creature_ptr->y;
				creature_ptr->oldpx = creature_ptr->x;
			}

			if (record_stair) exe_write_diary(creature_ptr, NIKKI_TELE_LEV, 1, NULL);

			if (autosave_l) do_cmd_save_game(TRUE);

			if (!creature_ptr->current_floor_ptr->dun_level)
			{
				creature_ptr->current_floor_ptr->dun_level = d_info[creature_ptr->dungeon_idx].mindepth;
				prepare_change_floor_mode(CFM_RAND_PLACE);
			}
			else
			{
				prepare_change_floor_mode(CFM_SAVE_FLOORS | CFM_DOWN | CFM_RAND_PLACE | CFM_RAND_CONNECT);
			}
			creature_ptr->leaving = TRUE;
		}
	}

	/* Up only */
	else if (quest_number(creature_ptr->current_floor_ptr->dun_level) || (creature_ptr->current_floor_ptr->dun_level >= d_info[creature_ptr->dungeon_idx].maxdepth))
	{
#ifdef JP
		if (see_m) msg_format("%^sは天井を突き破って宙へ浮いていく。", m_name);
#else
		if (see_m) msg_format("%^s rise%s up through the ceiling.", m_name, (m_idx <= 0) ? "" : "s");
#endif


		if (m_idx <= 0) /* To player */
		{
			if (record_stair) exe_write_diary(creature_ptr, NIKKI_TELE_LEV, -1, NULL);

			if (autosave_l) do_cmd_save_game(TRUE);

			prepare_change_floor_mode(CFM_SAVE_FLOORS | CFM_UP | CFM_RAND_PLACE | CFM_RAND_CONNECT);

			leave_quest_check();
			creature_ptr->inside_quest = 0;
			creature_ptr->leaving = TRUE;
		}
	}
	else if (go_up)
	{
#ifdef JP
		if (see_m) msg_format("%^sは天井を突き破って宙へ浮いていく。", m_name);
#else
		if (see_m) msg_format("%^s rise%s up through the ceiling.", m_name, (m_idx <= 0) ? "" : "s");
#endif


		if (m_idx <= 0) /* To player */
		{
			if (record_stair) exe_write_diary(creature_ptr, NIKKI_TELE_LEV, -1, NULL);

			if (autosave_l) do_cmd_save_game(TRUE);

			prepare_change_floor_mode(CFM_SAVE_FLOORS | CFM_UP | CFM_RAND_PLACE | CFM_RAND_CONNECT);
			creature_ptr->leaving = TRUE;
		}
	}
	else
	{
#ifdef JP
		if (see_m) msg_format("%^sは床を突き破って沈んでいく。", m_name);
#else
		if (see_m) msg_format("%^s sink%s through the floor.", m_name, (m_idx <= 0) ? "" : "s");
#endif

		if (m_idx <= 0) /* To player */
		{
			/* Never reach this code on the surface */
			/* if (!creature_ptr->current_floor_ptr->dun_level) creature_ptr->dungeon_idx = creature_ptr->recall_dungeon; */
			if (record_stair) exe_write_diary(creature_ptr, NIKKI_TELE_LEV, 1, NULL);
			if (autosave_l) do_cmd_save_game(TRUE);

			prepare_change_floor_mode(CFM_SAVE_FLOORS | CFM_DOWN | CFM_RAND_PLACE | CFM_RAND_CONNECT);
			creature_ptr->leaving = TRUE;
		}
	}

	/* Monster level teleportation is simple deleting now */
	if (m_idx > 0)
	{
		monster_type *m_ptr = &creature_ptr->current_floor_ptr->m_list[m_idx];

		check_quest_completion(m_ptr);

		if (record_named_pet && is_pet(m_ptr) && m_ptr->nickname)
		{
			char m2_name[MAX_NLEN];

			monster_desc(m2_name, m_ptr, MD_INDEF_VISIBLE);
			exe_write_diary(creature_ptr, NIKKI_NAMED_PET, RECORD_NAMED_PET_TELE_LEVEL, m2_name);
		}

		delete_monster_idx(m_idx);
	}

	sound(SOUND_TPLEVEL);
}

/*!
 * @brief プレイヤーの帰還発動及び中止処理 /
 * Recall the player to town or dungeon
 * @param turns 発動までのターン数
 * @return 常にTRUEを返す
 */
bool recall_player(player_type *creature_ptr, TIME_EFFECT turns)
{
	/*
	 * TODO: Recall the player to the last
	 * visited town when in the wilderness
	 */

	/* Ironman option */
	if (creature_ptr->inside_arena || ironman_downward)
	{
		msg_print(_("何も起こらなかった。", "Nothing happens."));
		return TRUE;
	}

	if (p_ptr->current_floor_ptr->dun_level && (max_dlv[p_ptr->dungeon_idx] > p_ptr->current_floor_ptr->dun_level) && !creature_ptr->inside_quest && !creature_ptr->word_recall)
	{
		if (get_check(_("ここは最深到達階より浅い階です。この階に戻って来ますか？ ", "Reset recall depth? ")))
		{
			max_dlv[p_ptr->dungeon_idx] = p_ptr->current_floor_ptr->dun_level;
			if (record_maxdepth)
				exe_write_diary(p_ptr, NIKKI_TRUMP, p_ptr->dungeon_idx, _("帰還のときに", "when recall from dungeon"));
		}

	}
	if (!creature_ptr->word_recall)
	{
		if (!p_ptr->current_floor_ptr->dun_level)
		{
			DUNGEON_IDX select_dungeon;
			select_dungeon = choose_dungeon(_("に帰還", "recall"), 2, 14);
			if (!select_dungeon) return FALSE;
			creature_ptr->recall_dungeon = select_dungeon;
		}
		creature_ptr->word_recall = turns;
		msg_print(_("回りの大気が張りつめてきた...", "The air about you becomes charged..."));
		creature_ptr->redraw |= (PR_STATUS);
	}
	else
	{
		creature_ptr->word_recall = 0;
		msg_print(_("張りつめた大気が流れ去った...", "A tension leaves the air around you..."));
		creature_ptr->redraw |= (PR_STATUS);
	}
	return TRUE;
}

bool free_level_recall(player_type *creature_ptr)
{
	DUNGEON_IDX select_dungeon;
	DEPTH max_depth;
	QUANTITY amt;

	select_dungeon = choose_dungeon(_("にテレポート", "teleport"), 4, 0);

	if (!select_dungeon) return FALSE;

	max_depth = d_info[select_dungeon].maxdepth;

	/* Limit depth in Angband */
	if (select_dungeon == DUNGEON_ANGBAND)
	{
		if (quest[QUEST_OBERON].status != QUEST_STATUS_FINISHED) max_depth = 98;
		else if (quest[QUEST_SERPENT].status != QUEST_STATUS_FINISHED) max_depth = 99;
	}
	amt = get_quantity(format(_("%sの何階にテレポートしますか？", "Teleport to which level of %s? "),
		d_name + d_info[select_dungeon].name), (QUANTITY)max_depth);

	if (amt > 0)
	{
		creature_ptr->word_recall = 1;
		creature_ptr->recall_dungeon = select_dungeon;
		max_dlv[creature_ptr->recall_dungeon] = ((amt > d_info[select_dungeon].maxdepth) ? d_info[select_dungeon].maxdepth : ((amt < d_info[select_dungeon].mindepth) ? d_info[select_dungeon].mindepth : amt));
		if (record_maxdepth)
			exe_write_diary(p_ptr, NIKKI_TRUMP, select_dungeon, _("トランプタワーで", "at Trump Tower"));

		msg_print(_("回りの大気が張りつめてきた...", "The air about you becomes charged..."));

		creature_ptr->redraw |= (PR_STATUS);
		return TRUE;
	}
	return FALSE;
}


/*!
 * @brief フロア・リセット処理
 * @return リセット処理が実際に行われたらTRUEを返す
 */
bool reset_recall(void)
{
	int select_dungeon, dummy = 0;
	char ppp[80];
	char tmp_val[160];

	select_dungeon = choose_dungeon(_("をセット", "reset"), 2, 14);

	/* Ironman option */
	if (ironman_downward)
	{
		msg_print(_("何も起こらなかった。", "Nothing happens."));
		return TRUE;
	}

	if (!select_dungeon) return FALSE;
	/* Prompt */
	sprintf(ppp, _("何階にセットしますか (%d-%d):", "Reset to which level (%d-%d): "),
		(int)d_info[select_dungeon].mindepth, (int)max_dlv[select_dungeon]);

	/* Default */
	sprintf(tmp_val, "%d", (int)MAX(p_ptr->current_floor_ptr->dun_level, 1));

	/* Ask for a level */
	if (get_string(ppp, tmp_val, 10))
	{
		/* Extract request */
		dummy = atoi(tmp_val);
		if (dummy < 1) dummy = 1;
		if (dummy > max_dlv[select_dungeon]) dummy = max_dlv[select_dungeon];
		if (dummy < d_info[select_dungeon].mindepth) dummy = d_info[select_dungeon].mindepth;

		max_dlv[select_dungeon] = dummy;

		if (record_maxdepth)
			exe_write_diary(p_ptr, NIKKI_TRUMP, select_dungeon, _("フロア・リセットで", "using a scroll of reset recall"));
					/* Accept request */
#ifdef JP
		msg_format("%sの帰還レベルを %d 階にセット。", d_name+d_info[select_dungeon].name, dummy, dummy * 50);
#else
		msg_format("Recall depth set to level %d (%d').", dummy, dummy * 50);
#endif

	}
	else
	{
		return FALSE;
	}
	return TRUE;
}


/*!
 * @brief プレイヤーの装備劣化処理 /
 * Apply disenchantment to the player's stuff
 * @param mode 最下位ビットが1ならば劣化処理が若干低減される
 * @return 劣化処理に関するメッセージが発せられた場合はTRUEを返す /
 * Return "TRUE" if the player notices anything
 */
bool apply_disenchant(BIT_FLAGS mode)
{
	int             t = 0;
	object_type *o_ptr;
	GAME_TEXT o_name[MAX_NLEN];
	int to_h, to_d, to_a, pval;

	/* Pick a random slot */
	switch (randint1(8))
	{
		case 1: t = INVEN_RARM; break;
		case 2: t = INVEN_LARM; break;
		case 3: t = INVEN_BOW; break;
		case 4: t = INVEN_BODY; break;
		case 5: t = INVEN_OUTER; break;
		case 6: t = INVEN_HEAD; break;
		case 7: t = INVEN_HANDS; break;
		case 8: t = INVEN_FEET; break;
	}

	o_ptr = &p_ptr->inventory_list[t];

	/* No item, nothing happens */
	if (!o_ptr->k_idx) return (FALSE);

	/* Disenchant equipments only -- No disenchant on monster ball */
	if (!object_is_weapon_armour_ammo(o_ptr))
		return FALSE;

	/* Nothing to disenchant */
	if ((o_ptr->to_h <= 0) && (o_ptr->to_d <= 0) && (o_ptr->to_a <= 0) && (o_ptr->pval <= 1))
	{
		/* Nothing to notice */
		return (FALSE);
	}

	object_desc(o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));

	/* Artifacts have 71% chance to resist */
	if (object_is_artifact(o_ptr) && (randint0(100) < 71))
	{
#ifdef JP
		msg_format("%s(%c)は劣化を跳ね返した！",o_name, index_to_label(t) );
#else
		msg_format("Your %s (%c) resist%s disenchantment!", o_name, index_to_label(t),
			((o_ptr->number != 1) ? "" : "s"));
#endif
		return (TRUE);
	}


	/* Memorize old value */
	to_h = o_ptr->to_h;
	to_d = o_ptr->to_d;
	to_a = o_ptr->to_a;
	pval = o_ptr->pval;

	/* Disenchant tohit */
	if (o_ptr->to_h > 0) o_ptr->to_h--;
	if ((o_ptr->to_h > 5) && (randint0(100) < 20)) o_ptr->to_h--;

	/* Disenchant todam */
	if (o_ptr->to_d > 0) o_ptr->to_d--;
	if ((o_ptr->to_d > 5) && (randint0(100) < 20)) o_ptr->to_d--;

	/* Disenchant toac */
	if (o_ptr->to_a > 0) o_ptr->to_a--;
	if ((o_ptr->to_a > 5) && (randint0(100) < 20)) o_ptr->to_a--;

	/* Disenchant pval (occasionally) */
	/* Unless called from wild_magic() */
	if ((o_ptr->pval > 1) && one_in_(13) && !(mode & 0x01)) o_ptr->pval--;

	if ((to_h != o_ptr->to_h) || (to_d != o_ptr->to_d) ||
	    (to_a != o_ptr->to_a) || (pval != o_ptr->pval))
	{
#ifdef JP
		msg_format("%s(%c)は劣化してしまった！", o_name, index_to_label(t) );
#else
		msg_format("Your %s (%c) %s disenchanted!", o_name, index_to_label(t),
			((o_ptr->number != 1) ? "were" : "was"));
#endif

		chg_virtue(p_ptr, V_HARMONY, 1);
		chg_virtue(p_ptr, V_ENCHANT, -2);
		p_ptr->update |= (PU_BONUS);
		p_ptr->window |= (PW_EQUIP | PW_PLAYER);

		calc_android_exp(p_ptr);
	}

	return (TRUE);
}


/*!
 * @brief 虚無招来によるフロア中の全壁除去処理 /
 * Vanish all walls in this floor
 * @return 実際に処理が反映された場合TRUE
 */
static bool vanish_dungeon(floor_type *floor_ptr)
{
	POSITION y, x;
	grid_type *g_ptr;
	feature_type *f_ptr;
	monster_type *m_ptr;
	GAME_TEXT m_name[MAX_NLEN];

	/* Prevent vasishing of quest levels and town */
	if ((p_ptr->inside_quest && is_fixed_quest_idx(p_ptr->inside_quest)) || !floor_ptr->dun_level)
	{
		return FALSE;
	}

	/* Scan all normal grids */
	for (y = 1; y < floor_ptr->height - 1; y++)
	{
		for (x = 1; x < floor_ptr->width - 1; x++)
		{
			g_ptr = &floor_ptr->grid_array[y][x];

			/* Seeing true feature code (ignore mimic) */
			f_ptr = &f_info[g_ptr->feat];

			/* Lose room and vault */
			g_ptr->info &= ~(CAVE_ROOM | CAVE_ICKY);

			m_ptr = &floor_ptr->m_list[g_ptr->m_idx];

			/* Awake monster */
			if (g_ptr->m_idx && MON_CSLEEP(m_ptr))
			{
				(void)set_monster_csleep(g_ptr->m_idx, 0);

				/* Notice the "waking up" */
				if (m_ptr->ml)
				{
					monster_desc(m_name, m_ptr, 0);
					msg_format(_("%^sが目を覚ました。", "%^s wakes up."), m_name);
				}
			}

			/* Process all walls, doors and patterns */
			if (have_flag(f_ptr->flags, FF_HURT_DISI)) cave_alter_feat(y, x, FF_HURT_DISI);
		}
	}

	/* Special boundary walls -- Top and bottom */
	for (x = 0; x < floor_ptr->width; x++)
	{
		g_ptr = &floor_ptr->grid_array[0][x];
		f_ptr = &f_info[g_ptr->mimic];

		/* Lose room and vault */
		g_ptr->info &= ~(CAVE_ROOM | CAVE_ICKY);

		/* Set boundary mimic if needed */
		if (g_ptr->mimic && have_flag(f_ptr->flags, FF_HURT_DISI))
		{
			g_ptr->mimic = feat_state(g_ptr->mimic, FF_HURT_DISI);

			/* Check for change to boring grid */
			if (!have_flag(f_info[g_ptr->mimic].flags, FF_REMEMBER)) g_ptr->info &= ~(CAVE_MARK);
		}

		g_ptr = &floor_ptr->grid_array[floor_ptr->height - 1][x];
		f_ptr = &f_info[g_ptr->mimic];

		/* Lose room and vault */
		g_ptr->info &= ~(CAVE_ROOM | CAVE_ICKY);

		/* Set boundary mimic if needed */
		if (g_ptr->mimic && have_flag(f_ptr->flags, FF_HURT_DISI))
		{
			g_ptr->mimic = feat_state(g_ptr->mimic, FF_HURT_DISI);

			/* Check for change to boring grid */
			if (!have_flag(f_info[g_ptr->mimic].flags, FF_REMEMBER)) g_ptr->info &= ~(CAVE_MARK);
		}
	}

	/* Special boundary walls -- Left and right */
	for (y = 1; y < (floor_ptr->height - 1); y++)
	{
		g_ptr = &floor_ptr->grid_array[y][0];
		f_ptr = &f_info[g_ptr->mimic];

		/* Lose room and vault */
		g_ptr->info &= ~(CAVE_ROOM | CAVE_ICKY);

		/* Set boundary mimic if needed */
		if (g_ptr->mimic && have_flag(f_ptr->flags, FF_HURT_DISI))
		{
			g_ptr->mimic = feat_state(g_ptr->mimic, FF_HURT_DISI);

			/* Check for change to boring grid */
			if (!have_flag(f_info[g_ptr->mimic].flags, FF_REMEMBER)) g_ptr->info &= ~(CAVE_MARK);
		}

		g_ptr = &floor_ptr->grid_array[y][floor_ptr->width - 1];
		f_ptr = &f_info[g_ptr->mimic];

		/* Lose room and vault */
		g_ptr->info &= ~(CAVE_ROOM | CAVE_ICKY);

		/* Set boundary mimic if needed */
		if (g_ptr->mimic && have_flag(f_ptr->flags, FF_HURT_DISI))
		{
			g_ptr->mimic = feat_state(g_ptr->mimic, FF_HURT_DISI);

			/* Check for change to boring grid */
			if (!have_flag(f_info[g_ptr->mimic].flags, FF_REMEMBER)) g_ptr->info &= ~(CAVE_MARK);
		}
	}

	/* Mega-Hack -- Forget the view and lite */
	p_ptr->update |= (PU_UN_VIEW | PU_UN_LITE | PU_VIEW | PU_LITE | PU_FLOW | PU_MON_LITE | PU_MONSTERS);
	p_ptr->redraw |= (PR_MAP);
	p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);

	return TRUE;
}

/*!
 * @brief 虚無招来処理 /
 * @return なし
 */
void call_the_(void)
{
	int i;
	grid_type *g_ptr;
	bool do_call = TRUE;

	for (i = 0; i < 9; i++)
	{
		g_ptr = &p_ptr->current_floor_ptr->grid_array[p_ptr->y + ddy_ddd[i]][p_ptr->x + ddx_ddd[i]];

		if (!cave_have_flag_grid(g_ptr, FF_PROJECT))
		{
			if (!g_ptr->mimic || !have_flag(f_info[g_ptr->mimic].flags, FF_PROJECT) ||
			    !permanent_wall(&f_info[g_ptr->feat]))
			{
				do_call = FALSE;
				break;
			}
		}
	}

	if (do_call)
	{
		for (i = 1; i < 10; i++)
		{
			if (i - 5) fire_ball(GF_ROCKET, i, 175, 2);
		}

		for (i = 1; i < 10; i++)
		{
			if (i - 5) fire_ball(GF_MANA, i, 175, 3);
		}

		for (i = 1; i < 10; i++)
		{
			if (i - 5) fire_ball(GF_NUKE, i, 175, 4);
		}
	}

	/* Prevent destruction of quest levels and town */
	else if ((p_ptr->inside_quest && is_fixed_quest_idx(p_ptr->inside_quest)) || !p_ptr->current_floor_ptr->dun_level)
	{
		msg_print(_("地面が揺れた。", "The ground trembles."));
	}

	else
	{
#ifdef JP
		msg_format("あなたは%sを壁に近すぎる場所で唱えてしまった！",
			((mp_ptr->spell_book == TV_LIFE_BOOK) ? "祈り" : "呪文"));
#else
		msg_format("You %s the %s too close to a wall!",
			((mp_ptr->spell_book == TV_LIFE_BOOK) ? "recite" : "cast"),
			((mp_ptr->spell_book == TV_LIFE_BOOK) ? "prayer" : "spell"));
#endif
		msg_print(_("大きな爆発音があった！", "There is a loud explosion!"));

		if (one_in_(666))
		{
			if (!vanish_dungeon(p_ptr->current_floor_ptr)) msg_print(_("ダンジョンは一瞬静まり返った。", "The dungeon silences a moment."));
		}
		else
		{
			if (destroy_area(p_ptr->current_floor_ptr, p_ptr->y, p_ptr->x, 15 + p_ptr->lev + randint0(11), FALSE))
				msg_print(_("ダンジョンが崩壊した...", "The dungeon collapses..."));
			else
				msg_print(_("ダンジョンは大きく揺れた。", "The dungeon trembles."));
		}

		take_hit(p_ptr, DAMAGE_NOESCAPE, 100 + randint1(150), _("自殺的な虚無招来", "a suicidal Call the Void"), -1);
	}
}


/*!
 * @brief アイテム引き寄せ処理 /
 * Fetch an item (teleport it right underneath the caster)
 * @param dir 魔法の発動方向
 * @param wgt 許容重量
 * @param require_los 射線の通りを要求するならばTRUE
 * @return なし
 */
void fetch(player_type *caster_ptr, DIRECTION dir, WEIGHT wgt, bool require_los)
{
	POSITION ty, tx;
	OBJECT_IDX i;
	grid_type *g_ptr;
	object_type *o_ptr;
	GAME_TEXT o_name[MAX_NLEN];

	/* Check to see if an object is already there */
	if (caster_ptr->current_floor_ptr->grid_array[caster_ptr->y][caster_ptr->x].o_idx)
	{
		msg_print(_("自分の足の下にある物は取れません。", "You can't fetch when you're already standing on something."));
		return;
	}

	/* Use a target */
	if (dir == 5 && target_okay())
	{
		tx = target_col;
		ty = target_row;

		if (distance(caster_ptr->y, caster_ptr->x, ty, tx) > MAX_RANGE)
		{
			msg_print(_("そんなに遠くにある物は取れません！", "You can't fetch something that far away!"));
			return;
		}

		g_ptr = &caster_ptr->current_floor_ptr->grid_array[ty][tx];

		/* We need an item to fetch */
		if (!g_ptr->o_idx)
		{
			msg_print(_("そこには何もありません。", "There is no object at this place."));
			return;
		}

		/* No fetching from vault */
		if (g_ptr->info & CAVE_ICKY)
		{
			msg_print(_("アイテムがコントロールを外れて落ちた。", "The item slips from your control."));
			return;
		}

		/* We need to see the item */
		if (require_los)
		{
			if (!player_has_los_bold(ty, tx))
			{
				msg_print(_("そこはあなたの視界に入っていません。", "You have no direct line of sight to that location."));
				return;
			}
			else if (!projectable(caster_ptr->y, caster_ptr->x, ty, tx))
			{
				msg_print(_("そこは壁の向こうです。", "You have no direct line of sight to that location."));
				return;
			}
		}
	}
	else
	{
		ty = caster_ptr->y; 
		tx = caster_ptr->x;
		do
		{
			ty += ddy[dir];
			tx += ddx[dir];
			g_ptr = &caster_ptr->current_floor_ptr->grid_array[ty][tx];

			if ((distance(caster_ptr->y, caster_ptr->x, ty, tx) > MAX_RANGE) ||
				!cave_have_flag_bold(ty, tx, FF_PROJECT)) return;
		}
		while (!g_ptr->o_idx);
	}

	o_ptr = &caster_ptr->current_floor_ptr->o_list[g_ptr->o_idx];

	if (o_ptr->weight > wgt)
	{
		/* Too heavy to 'fetch' */
		msg_print(_("そのアイテムは重過ぎます。", "The object is too heavy."));
		return;
	}

	i = g_ptr->o_idx;
	g_ptr->o_idx = o_ptr->next_o_idx;
	caster_ptr->current_floor_ptr->grid_array[caster_ptr->y][caster_ptr->x].o_idx = i; /* 'move' it */

	o_ptr->next_o_idx = 0;
	o_ptr->iy = caster_ptr->y;
	o_ptr->ix = caster_ptr->x;

	object_desc(o_name, o_ptr, OD_NAME_ONLY);
	msg_format(_("%^sがあなたの足元に飛んできた。", "%^s flies through the air to your feet."), o_name);

	note_spot(caster_ptr->y, caster_ptr->x);
	caster_ptr->redraw |= PR_MAP;
}

/*!
 * @brief 現実変容処理
 * @return なし
 */
void alter_reality(void)
{
	/* Ironman option */
	if (p_ptr->inside_arena || ironman_downward)
	{
		msg_print(_("何も起こらなかった。", "Nothing happens."));
		return;
	}

	if (!p_ptr->alter_reality)
	{
		TIME_EFFECT turns = randint0(21) + 15;

		p_ptr->alter_reality = turns;
		msg_print(_("回りの景色が変わり始めた...", "The view around you begins to change..."));

		p_ptr->redraw |= (PR_STATUS);
	}
	else
	{
		p_ptr->alter_reality = 0;
		msg_print(_("景色が元に戻った...", "The view around you got back..."));
		p_ptr->redraw |= (PR_STATUS);
	}
	return;
}

/*!
 * @brief 全所持アイテム鑑定処理 /
 * Identify everything being carried.
 * Done by a potion of "self knowledge".
 * @return なし
 */
void identify_pack(void)
{
	INVENTORY_IDX i;

	/* Simply identify and know every item */
	for (i = 0; i < INVEN_TOTAL; i++)
	{
		object_type *o_ptr = &p_ptr->inventory_list[i];
		if (!o_ptr->k_idx) continue;

		identify_item(o_ptr);

		/* Auto-inscription */
		autopick_alter_item(i, FALSE);
	}
}

/*!
 * @brief 装備の解呪処理 /
 * Removes curses from items in p_ptr->inventory_list
 * @param all 軽い呪いまでの解除ならば0
 * @return 解呪されたアイテムの数
 * @details
 * <pre>
 * Note that Items which are "Perma-Cursed" (The One Ring,
 * The Crown of Morgoth) can NEVER be uncursed.
 *
 * Note that if "all" is FALSE, then Items which are
 * "Heavy-Cursed" (Mormegil, Calris, and Weapons of Morgul)
 * will not be uncursed.
 * </pre>
 */
static int remove_curse_aux(player_type *creature_ptr, int all)
{
	int i, cnt = 0;

	/* Attempt to uncurse items being worn */
	for (i = INVEN_RARM; i < INVEN_TOTAL; i++)
	{
		object_type *o_ptr = &creature_ptr->inventory_list[i];
		if (!o_ptr->k_idx) continue;

		/* Uncursed already */
		if (!object_is_cursed(o_ptr)) continue;

		/* Heavily Cursed Items need a special spell */
		if (!all && (o_ptr->curse_flags & TRC_HEAVY_CURSE)) continue;

		/* Perma-Cursed Items can NEVER be uncursed */
		if (o_ptr->curse_flags & TRC_PERMA_CURSE)
		{
			o_ptr->curse_flags &= (TRC_CURSED | TRC_HEAVY_CURSE | TRC_PERMA_CURSE);
			continue;
		}

		o_ptr->curse_flags = 0L;
		o_ptr->ident |= (IDENT_SENSE);
		o_ptr->feeling = FEEL_NONE;

		creature_ptr->update |= (PU_BONUS);
		creature_ptr->window |= (PW_EQUIP);

		/* Count the uncursings */
		cnt++;
	}

	if (cnt)
	{
		msg_print(_("誰かに見守られているような気がする。", "You feel as if someone is watching over you."));
	}
	/* Return "something uncursed" */
	return (cnt);
}


/*!
 * @brief 装備の軽い呪い解呪処理 /
 * Remove most curses
 * @return 解呪に成功した装備数
 */
int remove_curse(player_type *caster_ptr)
{
	return (remove_curse_aux(caster_ptr, FALSE));
}

/*!
 * @brief 装備の重い呪い解呪処理 /
 * Remove all curses
 * @return 解呪に成功した装備数
 */
int remove_all_curse(player_type *caster_ptr)
{
	return (remove_curse_aux(caster_ptr, TRUE));
}


/*!
 * @brief アイテムの価値に応じた錬金術処理 /
 * Turns an object into gold, gain some of its value in a shop
 * @return 処理が実際に行われたらTRUEを返す
 */
bool alchemy(void)
{
	OBJECT_IDX item;
	int amt = 1;
	ITEM_NUMBER old_number;
	PRICE price;
	bool force = FALSE;
	object_type *o_ptr;
	GAME_TEXT o_name[MAX_NLEN];
	char out_val[MAX_NLEN+40];

	concptr q, s;

	/* Hack -- force destruction */
	if (command_arg > 0) force = TRUE;

	q = _("どのアイテムを金に変えますか？", "Turn which item to gold? ");
	s = _("金に変えられる物がありません。", "You have nothing to current_world_ptr->game_turn to gold.");

	o_ptr = choose_object(p_ptr, &item, q, s, (USE_INVEN | USE_FLOOR), 0);
	if (!o_ptr) return (FALSE);

	/* See how many items */
	if (o_ptr->number > 1)
	{
		amt = get_quantity(NULL, o_ptr->number);

		/* Allow user abort */
		if (amt <= 0) return FALSE;
	}

	old_number = o_ptr->number;
	o_ptr->number = amt;
	object_desc(o_name, o_ptr, 0);
	o_ptr->number = old_number;

	/* Verify unless quantity given */
	if (!force)
	{
		if (confirm_destroy || (object_value(o_ptr) > 0))
		{
			/* Make a verification */
			sprintf(out_val, _("本当に%sを金に変えますか？", "Really current_world_ptr->game_turn %s to gold? "), o_name);
			if (!get_check(out_val)) return FALSE;
		}
	}

	/* Artifacts cannot be destroyed */
	if (!can_player_destroy_object(o_ptr))
	{
		msg_format(_("%sを金に変えることに失敗した。", "You fail to current_world_ptr->game_turn %s to gold!"), o_name);

		return FALSE;
	}

	price = object_value_real(o_ptr);

	if (price <= 0)
	{
		msg_format(_("%sをニセの金に変えた。", "You current_world_ptr->game_turn %s to fool's gold."), o_name);
	}
	else
	{
		price /= 3;

		if (amt > 1) price *= amt;

		if (price > 30000) price = 30000;
		msg_format(_("%sを＄%d の金に変えた。", "You current_world_ptr->game_turn %s to %ld coins worth of gold."), o_name, price);

		p_ptr->au += price;
		p_ptr->redraw |= (PR_GOLD);
		p_ptr->window |= (PW_PLAYER);
	}

	/* Eliminate the item (from the pack) */
	if (item >= 0)
	{
		inven_item_increase(item, -amt);
		inven_item_describe(item);
		inven_item_optimize(item);
	}

	/* Eliminate the item (from the floor) */
	else
	{
		floor_item_increase(0 - item, -amt);
		floor_item_describe(0 - item);
		floor_item_optimize(0 - item);
	}

	return TRUE;
}


/*!
 * @brief アーティファクト生成の巻物処理 /
 * @return 生成が実際に試みられたらTRUEを返す
 */
bool artifact_scroll(void)
{
	OBJECT_IDX item;
	bool okay = FALSE;
	object_type *o_ptr;
	GAME_TEXT o_name[MAX_NLEN];
	concptr q, s;

	/* Enchant weapon/armour */
	item_tester_hook = item_tester_hook_nameless_weapon_armour;

	q = _("どのアイテムを強化しますか? ", "Enchant which item? ");
	s = _("強化できるアイテムがない。", "You have nothing to enchant.");

	o_ptr = choose_object(p_ptr, &item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT), 0);
	if (!o_ptr) return (FALSE);

	object_desc(o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
#ifdef JP
	msg_format("%s は眩い光を発した！",o_name);
#else
	msg_format("%s %s radiate%s a blinding light!", ((item >= 0) ? "Your" : "The"), o_name, ((o_ptr->number > 1) ? "" : "s"));
#endif

	if (object_is_artifact(o_ptr))
	{
#ifdef JP
		msg_format("%sは既に伝説のアイテムです！", o_name  );
#else
		msg_format("The %s %s already %s!", o_name, ((o_ptr->number > 1) ? "are" : "is"), ((o_ptr->number > 1) ? "artifacts" : "an artifact"));
#endif

		okay = FALSE;
	}

	else if (object_is_ego(o_ptr))
	{
#ifdef JP
		msg_format("%sは既に名のあるアイテムです！", o_name );
#else
		msg_format("The %s %s already %s!",
		    o_name, ((o_ptr->number > 1) ? "are" : "is"),
		    ((o_ptr->number > 1) ? "ego items" : "an ego item"));
#endif

		okay = FALSE;
	}

	else if (o_ptr->xtra3)
	{
#ifdef JP
		msg_format("%sは既に強化されています！", o_name );
#else
		msg_format("The %s %s already %s!", o_name, ((o_ptr->number > 1) ? "are" : "is"),
		    ((o_ptr->number > 1) ? "customized items" : "a customized item"));
#endif
	}

	else
	{
		if (o_ptr->number > 1)
		{
			msg_print(_("複数のアイテムに魔法をかけるだけのエネルギーはありません！", "Not enough energy to enchant more than one object!"));
#ifdef JP
			msg_format("%d 個の%sが壊れた！",(o_ptr->number)-1, o_name);
#else
			msg_format("%d of your %s %s destroyed!",(o_ptr->number)-1, o_name, (o_ptr->number>2?"were":"was"));
#endif

			if (item >= 0)
			{
				inven_item_increase(item, 1 - (o_ptr->number));
			}
			else
			{
				floor_item_increase(0 - item, 1 - (o_ptr->number));
			}
		}
		okay = become_random_artifact(o_ptr, TRUE);
	}

	/* Failure */
	if (!okay)
	{
		if (flush_failure) flush();
		msg_print(_("強化に失敗した。", "The enchantment failed."));
		if (one_in_(3)) chg_virtue(p_ptr, V_ENCHANT, -1);
	}
	else
	{
		if (record_rand_art)
		{
			object_desc(o_name, o_ptr, OD_NAME_ONLY);
			exe_write_diary(p_ptr, NIKKI_ART_SCROLL, 0, o_name);
		}
		chg_virtue(p_ptr, V_ENCHANT, 1);
	}

	calc_android_exp(p_ptr);

	/* Something happened */
	return (TRUE);
}


/*!
 * @brief アイテム鑑定処理 /
 * Identify an object
 * @param o_ptr 鑑定されるアイテムの情報参照ポインタ
 * @return 実際に鑑定できたらTRUEを返す
 */
bool identify_item(object_type *o_ptr)
{
	bool old_known = FALSE;
	GAME_TEXT o_name[MAX_NLEN];

	object_desc(o_name, o_ptr, 0);

	if (o_ptr->ident & IDENT_KNOWN)
		old_known = TRUE;

	if (!(o_ptr->ident & (IDENT_MENTAL)))
	{
		if (object_is_artifact(o_ptr) || one_in_(5))
			chg_virtue(p_ptr, V_KNOWLEDGE, 1);
	}

	object_aware(o_ptr);
	object_known(o_ptr);
	o_ptr->marked |= OM_TOUCHED;

	p_ptr->update |= (PU_BONUS | PU_COMBINE | PU_REORDER);
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	strcpy(record_o_name, o_name);
	record_turn = current_world_ptr->game_turn;

	object_desc(o_name, o_ptr, OD_NAME_ONLY);

	if(record_fix_art && !old_known && object_is_fixed_artifact(o_ptr))
		exe_write_diary(p_ptr, NIKKI_ART, 0, o_name);
	if(record_rand_art && !old_known && o_ptr->art_name)
		exe_write_diary(p_ptr, NIKKI_ART, 0, o_name);

	return old_known;
}

/*!
 * @brief アイテム鑑定のメインルーチン処理 /
 * Identify an object in the p_ptr->inventory_list (or on the floor)
 * @param only_equip 装備品のみを対象とするならばTRUEを返す
 * @return 実際に鑑定を行ったならばTRUEを返す
 * @details
 * This routine does *not* automatically combine objects.
 * Returns TRUE if something was identified, else FALSE.
 */
bool ident_spell(bool only_equip)
{
	OBJECT_IDX item;
	object_type *o_ptr;
	GAME_TEXT o_name[MAX_NLEN];
	concptr            q, s;
	bool old_known;

	if (only_equip)
		item_tester_hook = item_tester_hook_identify_weapon_armour;
	else
		item_tester_hook = item_tester_hook_identify;

	if (can_get_item(item_tester_tval))
	{
		q = _("どのアイテムを鑑定しますか? ", "Identify which item? ");
	}
	else
	{
		if (only_equip)
			item_tester_hook = object_is_weapon_armour_ammo;
		else
			item_tester_hook = NULL;

		q = _("すべて鑑定済みです。 ", "All items are identified. ");
	}

	s = _("鑑定するべきアイテムがない。", "You have nothing to identify.");

	o_ptr = choose_object(p_ptr, &item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT), 0);
	if (!o_ptr) return (FALSE);

	old_known = identify_item(o_ptr);

	object_desc(o_name, o_ptr, 0);
	if (item >= INVEN_RARM)
	{
		msg_format(_("%^s: %s(%c)。", "%^s: %s (%c)."), describe_use(item), o_name, index_to_label(item));
	}
	else if (item >= 0)
	{
		msg_format(_("ザック中: %s(%c)。", "In your pack: %s (%c)."), o_name, index_to_label(item));
	}
	else
	{
		msg_format(_("床上: %s。", "On the ground: %s."), o_name);
	}

	/* Auto-inscription/destroy */
	autopick_alter_item(item, (bool)(destroy_identify && !old_known));

	/* Something happened */
	return (TRUE);
}


/*!
 * @brief アイテム凡庸化のメインルーチン処理 /
 * Identify an object in the p_ptr->inventory_list (or on the floor)
 * @param only_equip 装備品のみを対象とするならばTRUEを返す
 * @return 実際に凡庸化をを行ったならばTRUEを返す
 * @details
 * <pre>
 * Mundanify an object in the p_ptr->inventory_list (or on the floor)
 * This routine does *not* automatically combine objects.
 * Returns TRUE if something was mundanified, else FALSE.
 * </pre>
 */
bool mundane_spell(bool only_equip)
{
	OBJECT_IDX item;
	object_type *o_ptr;
	concptr q, s;

	if (only_equip) item_tester_hook = object_is_weapon_armour_ammo;

	q = _("どれを使いますか？", "Use which item? ");
	s = _("使えるものがありません。", "You have nothing you can use.");

	o_ptr = choose_object(p_ptr, &item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT), 0);
	if (!o_ptr) return (FALSE);

	msg_print(_("まばゆい閃光が走った！", "There is a bright flash of light!"));
	{
		POSITION iy = o_ptr->iy;                 /* Y-position on map, or zero */
		POSITION ix = o_ptr->ix;                 /* X-position on map, or zero */
		OBJECT_IDX next_o_idx = o_ptr->next_o_idx; /* Next object in stack (if any) */
		byte marked = o_ptr->marked;         /* Object is marked */
		WEIGHT weight = o_ptr->number * o_ptr->weight;
		u16b inscription = o_ptr->inscription;

		/* Wipe it clean */
		object_prep(o_ptr, o_ptr->k_idx);

		o_ptr->iy = iy;
		o_ptr->ix = ix;
		o_ptr->next_o_idx = next_o_idx;
		o_ptr->marked = marked;
		o_ptr->inscription = inscription;
		if (item >= 0) p_ptr->total_weight += (o_ptr->weight - weight);
	}
	calc_android_exp(p_ptr);

	/* Something happened */
	return TRUE;
}

/*!
 * @brief アイテム*鑑定*のメインルーチン処理 /
 * Identify an object in the p_ptr->inventory_list (or on the floor)
 * @param only_equip 装備品のみを対象とするならばTRUEを返す
 * @return 実際に鑑定を行ったならばTRUEを返す
 * @details
 * Fully "identify" an object in the p_ptr->inventory_list  -BEN-
 * This routine returns TRUE if an item was identified.
 */
bool identify_fully(bool only_equip)
{
	OBJECT_IDX item;
	object_type *o_ptr;
	GAME_TEXT o_name[MAX_NLEN];
	concptr q, s;
	bool old_known;

	if (only_equip)
		item_tester_hook = item_tester_hook_identify_fully_weapon_armour;
	else
		item_tester_hook = item_tester_hook_identify_fully;

	if (can_get_item(item_tester_tval))
	{
		q = _("どのアイテムを*鑑定*しますか? ", "*Identify* which item? ");
	}
	else
	{
		if (only_equip)
			item_tester_hook = object_is_weapon_armour_ammo;
		else
			item_tester_hook = NULL;

		q = _("すべて*鑑定*済みです。 ", "All items are *identified*. ");
	}

	s = _("*鑑定*するべきアイテムがない。", "You have nothing to *identify*.");

	o_ptr = choose_object(p_ptr, &item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT), 0);
	if (!o_ptr) return (FALSE);

	old_known = identify_item(o_ptr);

	/* Mark the item as fully known */
	o_ptr->ident |= (IDENT_MENTAL);
	handle_stuff();

	object_desc(o_name, o_ptr, 0);
	if (item >= INVEN_RARM)
	{
		msg_format(_("%^s: %s(%c)。", "%^s: %s (%c)."), describe_use(item), o_name, index_to_label(item));
	}
	else if (item >= 0)
	{
		msg_format(_("ザック中: %s(%c)。", "In your pack: %s (%c)."), o_name, index_to_label(item));
	}
	else
	{
		msg_format(_("床上: %s。", "On the ground: %s."), o_name);
	}

	/* Describe it fully */
	(void)screen_object(o_ptr, 0L);

	/* Auto-inscription/destroy */
	autopick_alter_item(item, (bool)(destroy_identify && !old_known));

	/* Success */
	return (TRUE);
}



/*!
 * @brief 魔力充填処理 /
 * Recharge a wand/staff/rod from the pack or on the floor.
 * This function has been rewritten in Oangband and ZAngband.
 * @param power 充填パワー
 * @return ターン消費を要する処理まで進んだらTRUEを返す
 *
 * Sorcery/Arcane -- Recharge  --> recharge(plev * 4)
 * Chaos -- Arcane Binding     --> recharge(90)
 *
 * Scroll of recharging        --> recharge(130)
 * Artifact activation/Thingol --> recharge(130)
 *
 * It is harder to recharge high level, and highly charged wands,
 * staffs, and rods.  The more wands in a stack, the more easily and
 * strongly they recharge.  Staffs, however, each get fewer charges if
 * stacked.
 *
 * Beware of "sliding index errors".
 */
bool recharge(int power)
{
	OBJECT_IDX item;
	DEPTH lev;
	int recharge_strength;
	TIME_EFFECT recharge_amount;

	object_type *o_ptr;
	object_kind *k_ptr;

	bool fail = FALSE;
	byte fail_type = 1;

	concptr q, s;
	GAME_TEXT o_name[MAX_NLEN];

	/* Only accept legal items */
	item_tester_hook = item_tester_hook_recharge;

	q = _("どのアイテムに魔力を充填しますか? ", "Recharge which item? ");
	s = _("魔力を充填すべきアイテムがない。", "You have nothing to recharge.");

	o_ptr = choose_object(p_ptr, &item, q, s, (USE_INVEN | USE_FLOOR), 0);
	if (!o_ptr) return (FALSE);

	/* Get the object kind. */
	k_ptr = &k_info[o_ptr->k_idx];

	/* Extract the object "level" */
	lev = k_info[o_ptr->k_idx].level;


	/* Recharge a rod */
	if (o_ptr->tval == TV_ROD)
	{
		/* Extract a recharge strength by comparing object level to power. */
		recharge_strength = ((power > lev / 2) ? (power - lev / 2) : 0) / 5;


		/* Back-fire */
		if (one_in_(recharge_strength))
		{
			/* Activate the failure code. */
			fail = TRUE;
		}

		/* Recharge */
		else
		{
			/* Recharge amount */
			recharge_amount = (power * damroll(3, 2));

			/* Recharge by that amount */
			if (o_ptr->timeout > recharge_amount)
				o_ptr->timeout -= recharge_amount;
			else
				o_ptr->timeout = 0;
		}
	}


	/* Recharge wand/staff */
	else
	{
		/* Extract a recharge strength by comparing object level to power.
		 * Divide up a stack of wands' charges to calculate charge penalty.
		 */
		if ((o_ptr->tval == TV_WAND) && (o_ptr->number > 1))
			recharge_strength = (100 + power - lev - (8 * o_ptr->pval / o_ptr->number)) / 15;

		/* All staffs, unstacked wands. */
		else recharge_strength = (100 + power - lev - (8 * o_ptr->pval)) / 15;
		if (recharge_strength < 0) recharge_strength = 0;

		/* Back-fire */
		if (one_in_(recharge_strength))
		{
			/* Activate the failure code. */
			fail = TRUE;
		}

		/* If the spell didn't backfire, recharge the wand or staff. */
		else
		{
			/* Recharge based on the standard number of charges. */
			recharge_amount = randint1(1 + k_ptr->pval / 2);

			/* Multiple wands in a stack increase recharging somewhat. */
			if ((o_ptr->tval == TV_WAND) && (o_ptr->number > 1))
			{
				recharge_amount +=
					(randint1(recharge_amount * (o_ptr->number - 1))) / 2;
				if (recharge_amount < 1) recharge_amount = 1;
				if (recharge_amount > 12) recharge_amount = 12;
			}

			/* But each staff in a stack gets fewer additional charges,
			 * although always at least one.
			 */
			if ((o_ptr->tval == TV_STAFF) && (o_ptr->number > 1))
			{
				recharge_amount /= (TIME_EFFECT)o_ptr->number;
				if (recharge_amount < 1) recharge_amount = 1;
			}

			/* Recharge the wand or staff. */
			o_ptr->pval += recharge_amount;


			/* Hack -- we no longer "know" the item */
			o_ptr->ident &= ~(IDENT_KNOWN);

			/* Hack -- we no longer think the item is empty */
			o_ptr->ident &= ~(IDENT_EMPTY);
		}
	}


	/* Inflict the penalties for failing a recharge. */
	if (fail)
	{
		/* Artifacts are never destroyed. */
		if (object_is_fixed_artifact(o_ptr))
		{
			object_desc(o_name, o_ptr, OD_NAME_ONLY);
			msg_format(_("魔力が逆流した！%sは完全に魔力を失った。", "The recharging backfires - %s is completely drained!"), o_name);

			/* Artifact rods. */
			if ((o_ptr->tval == TV_ROD) && (o_ptr->timeout < 10000))
				o_ptr->timeout = (o_ptr->timeout + 100) * 2;

			/* Artifact wands and staffs. */
			else if ((o_ptr->tval == TV_WAND) || (o_ptr->tval == TV_STAFF))
				o_ptr->pval = 0;
		}
		else
		{
			/* Get the object description */
			object_desc(o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));

			/*** Determine Seriousness of Failure ***/

			/* Mages recharge objects more safely. */
			if (IS_WIZARD_CLASS(p_ptr) || p_ptr->pclass == CLASS_MAGIC_EATER || p_ptr->pclass == CLASS_BLUE_MAGE)
			{
				/* 10% chance to blow up one rod, otherwise draining. */
				if (o_ptr->tval == TV_ROD)
				{
					if (one_in_(10)) fail_type = 2;
					else fail_type = 1;
				}
				/* 75% chance to blow up one wand, otherwise draining. */
				else if (o_ptr->tval == TV_WAND)
				{
					if (!one_in_(3)) fail_type = 2;
					else fail_type = 1;
				}
				/* 50% chance to blow up one staff, otherwise no effect. */
				else if (o_ptr->tval == TV_STAFF)
				{
					if (one_in_(2)) fail_type = 2;
					else fail_type = 0;
				}
			}

			/* All other classes get no special favors. */
			else
			{
				/* 33% chance to blow up one rod, otherwise draining. */
				if (o_ptr->tval == TV_ROD)
				{
					if (one_in_(3)) fail_type = 2;
					else fail_type = 1;
				}
				/* 20% chance of the entire stack, else destroy one wand. */
				else if (o_ptr->tval == TV_WAND)
				{
					if (one_in_(5)) fail_type = 3;
					else fail_type = 2;
				}
				/* Blow up one staff. */
				else if (o_ptr->tval == TV_STAFF)
				{
					fail_type = 2;
				}
			}

			/*** Apply draining and destruction. ***/

			/* Drain object or stack of objects. */
			if (fail_type == 1)
			{
				if (o_ptr->tval == TV_ROD)
				{
					msg_print(_("魔力が逆噴射して、ロッドからさらに魔力を吸い取ってしまった！", "The recharge backfires, draining the rod further!"));

					if (o_ptr->timeout < 10000)
						o_ptr->timeout = (o_ptr->timeout + 100) * 2;
				}
				else if (o_ptr->tval == TV_WAND)
				{
					msg_format(_("%sは破損を免れたが、魔力が全て失われた。", "You save your %s from destruction, but all charges are lost."), o_name);
					o_ptr->pval = 0;
				}
				/* Staffs aren't drained. */
			}

			/* Destroy an object or one in a stack of objects. */
			if (fail_type == 2)
			{
				if (o_ptr->number > 1)
					msg_format(_("乱暴な魔法のために%sが一本壊れた！", "Wild magic consumes one of your %s!"), o_name);
				else
					msg_format(_("乱暴な魔法のために%sが壊れた！", "Wild magic consumes your %s!"), o_name);

				/* Reduce rod stack maximum timeout, drain wands. */
				if (o_ptr->tval == TV_ROD) o_ptr->timeout = (o_ptr->number - 1) * k_ptr->pval;
				if (o_ptr->tval == TV_WAND) o_ptr->pval = 0;

				/* Reduce and describe p_ptr->inventory_list */
				if (item >= 0)
				{
					inven_item_increase(item, -1);
					inven_item_describe(item);
					inven_item_optimize(item);
				}

				/* Reduce and describe floor item */
				else
				{
					floor_item_increase(0 - item, -1);
					floor_item_describe(0 - item);
					floor_item_optimize(0 - item);
				}
			}

			/* Destroy all members of a stack of objects. */
			if (fail_type == 3)
			{
				if (o_ptr->number > 1)
					msg_format(_("乱暴な魔法のために%sが全て壊れた！", "Wild magic consumes all your %s!"), o_name);
				else
					msg_format(_("乱暴な魔法のために%sが壊れた！", "Wild magic consumes your %s!"), o_name);

				/* Reduce and describe p_ptr->inventory_list */
				if (item >= 0)
				{
					inven_item_increase(item, -999);
					inven_item_describe(item);
					inven_item_optimize(item);
				}

				/* Reduce and describe floor item */
				else
				{
					floor_item_increase(0 - item, -999);
					floor_item_describe(0 - item);
					floor_item_optimize(0 - item);
				}
			}
		}
	}
	p_ptr->update |= (PU_COMBINE | PU_REORDER);
	p_ptr->window |= (PW_INVEN);

	/* Something was done */
	return (TRUE);
}


/*!
 * @brief クリーチャー全既知呪文を表示する /
 * Hack -- Display all known spells in a window
 * @param caster_ptr 術者の参照ポインタ
 * return なし
 * @details
 * Need to analyze size of the window.
 * Need more color coding.
 */
void display_spell_list(player_type *caster_ptr)
{
	int i, j;
	TERM_LEN y, x;
	int m[9];
	const magic_type *s_ptr;
	GAME_TEXT name[MAX_NLEN];
	char out_val[160];

	clear_from(0);

	/* They have too many spells to list */
	if (caster_ptr->pclass == CLASS_SORCERER) return;
	if (caster_ptr->pclass == CLASS_RED_MAGE) return;

	if (caster_ptr->pclass == CLASS_SNIPER)
	{
		display_snipe_list(caster_ptr);
		return;
	}

	/* mind.c type classes */
	if ((caster_ptr->pclass == CLASS_MINDCRAFTER) ||
	    (caster_ptr->pclass == CLASS_BERSERKER) ||
	    (caster_ptr->pclass == CLASS_NINJA) ||
	    (caster_ptr->pclass == CLASS_MIRROR_MASTER) ||
	    (caster_ptr->pclass == CLASS_FORCETRAINER))
	{
		PERCENTAGE minfail = 0;
		PLAYER_LEVEL plev = caster_ptr->lev;
		PERCENTAGE chance = 0;
		mind_type       spell;
		char            comment[80];
		char            psi_desc[80];
		int             use_mind;
		bool use_hp = FALSE;

		y = 1;
		x = 1;

		/* Display a list of spells */
		prt("", y, x);
		put_str(_("名前", "Name"), y, x + 5);
		put_str(_("Lv   MP 失率 効果", "Lv Mana Fail Info"), y, x + 35);

		switch(caster_ptr->pclass)
		{
		case CLASS_MINDCRAFTER: use_mind = MIND_MINDCRAFTER;break;
		case CLASS_FORCETRAINER:          use_mind = MIND_KI;break;
		case CLASS_BERSERKER: use_mind = MIND_BERSERKER; use_hp = TRUE; break;
		case CLASS_MIRROR_MASTER: use_mind = MIND_MIRROR_MASTER; break;
		case CLASS_NINJA: use_mind = MIND_NINJUTSU; use_hp = TRUE; break;
		default:                use_mind = 0;break;
		}

		/* Dump the spells */
		for (i = 0; i < MAX_MIND_POWERS; i++)
		{
			byte a = TERM_WHITE;

			/* Access the available spell */
			spell = mind_powers[use_mind].info[i];
			if (spell.min_lev > plev) break;

			/* Get the failure rate */
			chance = spell.fail;

			/* Reduce failure rate by "effective" level adjustment */
			chance -= 3 * (caster_ptr->lev - spell.min_lev);

			/* Reduce failure rate by INT/WIS adjustment */
			chance -= 3 * (adj_mag_stat[caster_ptr->stat_ind[mp_ptr->spell_stat]] - 1);

			if (!use_hp)
			{
				/* Not enough mana to cast */
				if (spell.mana_cost > caster_ptr->csp)
				{
					chance += 5 * (spell.mana_cost - caster_ptr->csp);
					a = TERM_ORANGE;
				}
			}
			else
			{
				/* Not enough hp to cast */
				if (spell.mana_cost > caster_ptr->chp)
				{
					chance += 100;
					a = TERM_RED;
				}
			}

			/* Extract the minimum failure rate */
			minfail = adj_mag_fail[caster_ptr->stat_ind[mp_ptr->spell_stat]];

			/* Minimum failure rate */
			if (chance < minfail) chance = minfail;

			/* Stunning makes spells harder */
			if (caster_ptr->stun > 50) chance += 25;
			else if (caster_ptr->stun) chance += 15;

			/* Always a 5 percent chance of working */
			if (chance > 95) chance = 95;

			/* Get info */
			mindcraft_info(comment, use_mind, i);

			/* Dump the spell */
			sprintf(psi_desc, "  %c) %-30s%2d %4d %3d%%%s",
			    I2A(i), spell.name,
			    spell.min_lev, spell.mana_cost, chance, comment);

			Term_putstr(x, y + i + 1, -1, a, psi_desc);
		}
		return;
	}

	/* Cannot read spellbooks */
	if (REALM_NONE == caster_ptr->realm1) return;

	/* Normal spellcaster with books */

	/* Scan books */
	for (j = 0; j < ((caster_ptr->realm2 > REALM_NONE) ? 2 : 1); j++)
	{
		int n = 0;

		/* Reset vertical */
		m[j] = 0;

		/* Vertical location */
		y = (j < 3) ? 0 : (m[j - 3] + 2);

		/* Horizontal location */
		x = 27 * (j % 3);

		/* Scan spells */
		for (i = 0; i < 32; i++)
		{
			byte a = TERM_WHITE;

			/* Access the spell */
			if (!is_magic((j < 1) ? caster_ptr->realm1 : caster_ptr->realm2))
			{
				s_ptr = &technic_info[((j < 1) ? caster_ptr->realm1 : caster_ptr->realm2) - MIN_TECHNIC][i % 32];
			}
			else
			{
				s_ptr = &mp_ptr->info[((j < 1) ? caster_ptr->realm1 : caster_ptr->realm2) - 1][i % 32];
			}

			strcpy(name, exe_spell(caster_ptr, (j < 1) ? caster_ptr->realm1 : caster_ptr->realm2, i % 32, SPELL_NAME));

			/* Illegible */
			if (s_ptr->slevel >= 99)
			{
				/* Illegible */
				strcpy(name, _("(判読不能)", "(illegible)"));

				/* Unusable */
				a = TERM_L_DARK;
			}

			/* Forgotten */
			else if ((j < 1) ?
				((caster_ptr->spell_forgotten1 & (1L << i))) :
				((caster_ptr->spell_forgotten2 & (1L << (i % 32)))))
			{
				/* Forgotten */
				a = TERM_ORANGE;
			}

			/* Unknown */
			else if (!((j < 1) ?
				(caster_ptr->spell_learned1 & (1L << i)) :
				(caster_ptr->spell_learned2 & (1L << (i % 32)))))
			{
				/* Unknown */
				a = TERM_RED;
			}

			/* Untried */
			else if (!((j < 1) ?
				(caster_ptr->spell_worked1 & (1L << i)) :
				(caster_ptr->spell_worked2 & (1L << (i % 32)))))
			{
				/* Untried */
				a = TERM_YELLOW;
			}

			/* Dump the spell --(-- */
			sprintf(out_val, "%c/%c) %-20.20s",
				I2A(n / 8), I2A(n % 8), name);

			/* Track maximum */
			m[j] = y + n;

			/* Dump onto the window */
			Term_putstr(x, m[j], -1, a, out_val);

			/* Next */
			n++;
		}
	}
}


/*!
 * @brief 呪文の経験値を返す /
 * Returns experience of a spell
 * @param spell 呪文ID
 * @param use_realm 魔法領域
 * @return 経験値
 */
EXP experience_of_spell(SPELL_IDX spell, REALM_IDX use_realm)
{
	if (p_ptr->pclass == CLASS_SORCERER) return SPELL_EXP_MASTER;
	else if (p_ptr->pclass == CLASS_RED_MAGE) return SPELL_EXP_SKILLED;
	else if (use_realm == p_ptr->realm1) return p_ptr->spell_exp[spell];
	else if (use_realm == p_ptr->realm2) return p_ptr->spell_exp[spell + 32];
	else return 0;
}


/*!
 * @brief 呪文の消費MPを返す /
 * Modify mana consumption rate using spell exp and p_ptr->dec_mana
 * @param need_mana 基本消費MP
 * @param spell 呪文ID
 * @param realm 魔法領域
 * @return 消費MP
 */
MANA_POINT mod_need_mana(MANA_POINT need_mana, SPELL_IDX spell, REALM_IDX realm)
{
#define MANA_CONST   2400
#define MANA_DIV        4
#define DEC_MANA_DIV    3

	/* Realm magic */
	if ((realm > REALM_NONE) && (realm <= MAX_REALM))
	{
		/*
		 * need_mana defaults if spell exp equals SPELL_EXP_EXPERT and !p_ptr->dec_mana.
		 * MANA_CONST is used to calculate need_mana effected from spell proficiency.
		 */
		need_mana = need_mana * (MANA_CONST + SPELL_EXP_EXPERT - experience_of_spell(spell, realm)) + (MANA_CONST - 1);
		need_mana *= p_ptr->dec_mana ? DEC_MANA_DIV : MANA_DIV;
		need_mana /= MANA_CONST * MANA_DIV;
		if (need_mana < 1) need_mana = 1;
	}

	/* Non-realm magic */
	else
	{
		if (p_ptr->dec_mana) need_mana = (need_mana + 1) * DEC_MANA_DIV / MANA_DIV;
	}

#undef DEC_MANA_DIV
#undef MANA_DIV
#undef MANA_CONST

	return need_mana;
}


/*!
 * @brief 呪文の失敗率修正処理1(呪い、消費魔力減少、呪文簡易化) /
 * Modify spell fail rate
 * Using p_ptr->to_m_chance, p_ptr->dec_mana, p_ptr->easy_spell and p_ptr->heavy_spell
 * @param chance 修正前失敗率
 * @return 失敗率(%)
 * @todo 統合を検討
 */
PERCENTAGE mod_spell_chance_1(PERCENTAGE chance)
{
	chance += p_ptr->to_m_chance;

	if (p_ptr->heavy_spell) chance += 20;

	if (p_ptr->dec_mana && p_ptr->easy_spell) chance -= 4;
	else if (p_ptr->easy_spell) chance -= 3;
	else if (p_ptr->dec_mana) chance -= 2;

	return chance;
}


/*!
 * @brief 呪文の失敗率修正処理2(消費魔力減少、呪い、負値修正) /
 * Modify spell fail rate
 * Using p_ptr->to_m_chance, p_ptr->dec_mana, p_ptr->easy_spell and p_ptr->heavy_spell
 * @param chance 修正前失敗率
 * @return 失敗率(%)
 * Modify spell fail rate (as "suffix" process)
 * Using p_ptr->dec_mana, p_ptr->easy_spell and p_ptr->heavy_spell
 * Note: variable "chance" cannot be negative.
 * @todo 統合を検討
 */
PERCENTAGE mod_spell_chance_2(PERCENTAGE chance)
{
	if (p_ptr->dec_mana) chance--;

	if (p_ptr->heavy_spell) chance += 5;

	return MAX(chance, 0);
}


/*!
 * @brief 呪文の失敗率計算メインルーチン /
 * Returns spell chance of failure for spell -RAK-
 * @param spell 呪文ID
 * @param use_realm 魔法領域ID
 * @return 失敗率(%)
 */
PERCENTAGE spell_chance(player_type *caster_ptr, SPELL_IDX spell, REALM_IDX use_realm)
{
	PERCENTAGE chance, minfail;
	const magic_type *s_ptr;
	MANA_POINT need_mana;
	PERCENTAGE penalty = (mp_ptr->spell_stat == A_WIS) ? 10 : 4;


	/* Paranoia -- must be literate */
	if (!mp_ptr->spell_book) return (100);

	if (use_realm == REALM_HISSATSU) return 0;

	/* Access the spell */
	if (!is_magic(use_realm))
	{
		s_ptr = &technic_info[use_realm - MIN_TECHNIC][spell];
	}
	else
	{
		s_ptr = &mp_ptr->info[use_realm - 1][spell];
	}

	/* Extract the base spell failure rate */
	chance = s_ptr->sfail;

	/* Reduce failure rate by "effective" level adjustment */
	chance -= 3 * (caster_ptr->lev - s_ptr->slevel);

	/* Reduce failure rate by INT/WIS adjustment */
	chance -= 3 * (adj_mag_stat[caster_ptr->stat_ind[mp_ptr->spell_stat]] - 1);

	if (caster_ptr->riding)
		chance += (MAX(r_info[caster_ptr->current_floor_ptr->m_list[caster_ptr->riding].r_idx].level - caster_ptr->skill_exp[GINOU_RIDING] / 100 - 10, 0));

	/* Extract mana consumption rate */
	need_mana = mod_need_mana(s_ptr->smana, spell, use_realm);

	/* Not enough mana to cast */
	if (need_mana > caster_ptr->csp)
	{
		chance += 5 * (need_mana - caster_ptr->csp);
	}

	if ((use_realm != caster_ptr->realm1) && ((caster_ptr->pclass == CLASS_MAGE) || (caster_ptr->pclass == CLASS_PRIEST))) chance += 5;

	/* Extract the minimum failure rate */
	minfail = adj_mag_fail[caster_ptr->stat_ind[mp_ptr->spell_stat]];

	/*
	 * Non mage/priest characters never get too good
	 * (added high mage, mindcrafter)
	 */
	if (mp_ptr->spell_xtra & MAGIC_FAIL_5PERCENT)
	{
		if (minfail < 5) minfail = 5;
	}

	/* Hack -- Priest prayer penalty for "edged" weapons  -DGK */
	if (((caster_ptr->pclass == CLASS_PRIEST) || (caster_ptr->pclass == CLASS_SORCERER)) && caster_ptr->icky_wield[0]) chance += 25;
	if (((caster_ptr->pclass == CLASS_PRIEST) || (caster_ptr->pclass == CLASS_SORCERER)) && caster_ptr->icky_wield[1]) chance += 25;

	chance = mod_spell_chance_1(chance);

	/* Goodness or evilness gives a penalty to failure rate */
	switch (use_realm)
	{
	case REALM_NATURE:
		if ((caster_ptr->align > 50) || (caster_ptr->align < -50)) chance += penalty;
		break;
	case REALM_LIFE: case REALM_CRUSADE:
		if (caster_ptr->align < -20) chance += penalty;
		break;
	case REALM_DEATH: case REALM_DAEMON: case REALM_HEX:
		if (caster_ptr->align > 20) chance += penalty;
		break;
	}

	/* Minimum failure rate */
	if (chance < minfail) chance = minfail;

	/* Stunning makes spells harder */
	if (caster_ptr->stun > 50) chance += 25;
	else if (caster_ptr->stun) chance += 15;

	/* Always a 5 percent chance of working */
	if (chance > 95) chance = 95;

	if ((use_realm == caster_ptr->realm1) || (use_realm == caster_ptr->realm2)
	    || (caster_ptr->pclass == CLASS_SORCERER) || (caster_ptr->pclass == CLASS_RED_MAGE))
	{
		EXP exp = experience_of_spell(spell, use_realm);
		if (exp >= SPELL_EXP_EXPERT) chance--;
		if (exp >= SPELL_EXP_MASTER) chance--;
	}

	/* Return the chance */
	return mod_spell_chance_2(chance);
}



/*!
 * @brief 呪文情報の表示処理 /
 * Print a list of spells (for browsing or casting or viewing)
 * @param target_spell 呪文ID		    
 * @param spells 表示するスペルID配列の参照ポインタ
 * @param num 表示するスペルの数(spellsの要素数)
 * @param y 表示メッセージ左上Y座標
 * @param x 表示メッセージ左上X座標
 * @param use_realm 魔法領域ID
 * @return なし
 */
void print_spells(SPELL_IDX target_spell, SPELL_IDX *spells, int num, TERM_LEN y, TERM_LEN x, REALM_IDX use_realm)
{
	int i;
	SPELL_IDX spell;
	int  exp_level, increment = 64;
	const magic_type *s_ptr;
	concptr comment;
	char info[80];
	char out_val[160];
	byte line_attr;
	MANA_POINT need_mana;
	char ryakuji[5];
	char buf[256];
	bool max = FALSE;

	if (((use_realm <= REALM_NONE) || (use_realm > MAX_REALM)) && current_world_ptr->wizard)
	msg_print(_("警告！ print_spell が領域なしに呼ばれた", "Warning! print_spells called with null realm"));

	/* Title the list */
	prt("", y, x);
	if (use_realm == REALM_HISSATSU)
		strcpy(buf,_("  Lv   MP", "  Lv   SP"));
	else
		strcpy(buf,_("熟練度 Lv   MP 失率 効果", "Profic Lv   SP Fail Effect"));

	put_str(_("名前", "Name"), y, x + 5);
	put_str(buf, y, x + 29);

	if ((p_ptr->pclass == CLASS_SORCERER) || (p_ptr->pclass == CLASS_RED_MAGE)) increment = 0;
	else if (use_realm == p_ptr->realm1) increment = 0;
	else if (use_realm == p_ptr->realm2) increment = 32;

	/* Dump the spells */
	for (i = 0; i < num; i++)
	{
		spell = spells[i];

		if (!is_magic(use_realm))
		{
			s_ptr = &technic_info[use_realm - MIN_TECHNIC][spell];
		}
		else
		{
			s_ptr = &mp_ptr->info[use_realm - 1][spell];
		}

		if (use_realm == REALM_HISSATSU)
			need_mana = s_ptr->smana;
		else
		{
			EXP exp = experience_of_spell(spell, use_realm);

			/* Extract mana consumption rate */
			need_mana = mod_need_mana(s_ptr->smana, spell, use_realm);

			if ((increment == 64) || (s_ptr->slevel >= 99)) exp_level = EXP_LEVEL_UNSKILLED;
			else exp_level = spell_exp_level(exp);

			max = FALSE;
			if (!increment && (exp_level == EXP_LEVEL_MASTER)) max = TRUE;
			else if ((increment == 32) && (exp_level >= EXP_LEVEL_EXPERT)) max = TRUE;
			else if (s_ptr->slevel >= 99) max = TRUE;
			else if ((p_ptr->pclass == CLASS_RED_MAGE) && (exp_level >= EXP_LEVEL_SKILLED)) max = TRUE;

			strncpy(ryakuji, exp_level_str[exp_level], 4);
			ryakuji[3] = ']';
			ryakuji[4] = '\0';
		}

		if (use_menu && target_spell)
		{
			if (i == (target_spell-1))
				strcpy(out_val, _("  》 ", "  >  "));
			else
				strcpy(out_val, "     ");
		}
		else sprintf(out_val, "  %c) ", I2A(i));
		/* Skip illegible spells */
		if (s_ptr->slevel >= 99)
		{
			strcat(out_val, format("%-30s", _("(判読不能)", "(illegible)")));
			c_prt(TERM_L_DARK, out_val, y + i + 1, x);
			continue;
		}

		/* XXX XXX Could label spells above the players level */

		/* Get extra info */
		strcpy(info, exe_spell(p_ptr, use_realm, spell, SPELL_INFO));

		/* Use that info */
		comment = info;

		/* Assume spell is known and tried */
		line_attr = TERM_WHITE;

		/* Analyze the spell */
		if ((p_ptr->pclass == CLASS_SORCERER) || (p_ptr->pclass == CLASS_RED_MAGE))
		{
			if (s_ptr->slevel > p_ptr->max_plv)
			{
				comment = _("未知", "unknown");
				line_attr = TERM_L_BLUE;
			}
			else if (s_ptr->slevel > p_ptr->lev)
			{
				comment = _("忘却", "forgotten");
				line_attr = TERM_YELLOW;
			}
		}
		else if ((use_realm != p_ptr->realm1) && (use_realm != p_ptr->realm2))
		{
			comment = _("未知", "unknown");
			line_attr = TERM_L_BLUE;
		}
		else if ((use_realm == p_ptr->realm1) ?
		    ((p_ptr->spell_forgotten1 & (1L << spell))) :
		    ((p_ptr->spell_forgotten2 & (1L << spell))))
		{
			comment = _("忘却", "forgotten");
			line_attr = TERM_YELLOW;
		}
		else if (!((use_realm == p_ptr->realm1) ?
		    (p_ptr->spell_learned1 & (1L << spell)) :
		    (p_ptr->spell_learned2 & (1L << spell))))
		{
			comment = _("未知", "unknown");
			line_attr = TERM_L_BLUE;
		}
		else if (!((use_realm == p_ptr->realm1) ?
		    (p_ptr->spell_worked1 & (1L << spell)) :
		    (p_ptr->spell_worked2 & (1L << spell))))
		{
			comment = _("未経験", "untried");
			line_attr = TERM_L_GREEN;
		}

		/* Dump the spell --(-- */
		if (use_realm == REALM_HISSATSU)
		{
			strcat(out_val, format("%-25s %2d %4d",
			    exe_spell(p_ptr, use_realm, spell, SPELL_NAME), /* realm, spell */
			    s_ptr->slevel, need_mana));
		}
		else
		{
			strcat(out_val, format("%-25s%c%-4s %2d %4d %3d%% %s",
			    exe_spell(p_ptr, use_realm, spell, SPELL_NAME), /* realm, spell */
			    (max ? '!' : ' '), ryakuji,
			    s_ptr->slevel, need_mana, spell_chance(p_ptr, spell, use_realm), comment));
		}
		c_prt(line_attr, out_val, y + i + 1, x);
	}

	/* Clear the bottom line */
	prt("", y + i + 1, x);
}

/*!
 * @brief 変身処理向けにモンスターの近隣レベル帯モンスターを返す /
 * Helper function -- return a "nearby" race for polymorphing
 * @param r_idx 基準となるモンスター種族ID
 * @return 変更先のモンスター種族ID
 * @details
 * Note that this function is one of the more "dangerous" ones...
 */
static MONRACE_IDX poly_r_idx(MONRACE_IDX r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	int i;
	MONRACE_IDX r;
	DEPTH lev1, lev2;

	/* Hack -- Uniques/Questors never polymorph */
	if ((r_ptr->flags1 & RF1_UNIQUE) || (r_ptr->flags1 & RF1_QUESTOR))
		return (r_idx);

	/* Allowable range of "levels" for resulting monster */
	lev1 = r_ptr->level - ((randint1(20) / randint1(9)) + 1);
	lev2 = r_ptr->level + ((randint1(20) / randint1(9)) + 1);

	/* Pick a (possibly new) non-unique race */
	for (i = 0; i < 1000; i++)
	{
		/* Pick a new race, using a level calculation */
		r = get_mon_num((p_ptr->current_floor_ptr->dun_level + r_ptr->level) / 2 + 5);

		/* Handle failure */
		if (!r) break;

		r_ptr = &r_info[r];

		/* Ignore unique monsters */
		if (r_ptr->flags1 & RF1_UNIQUE) continue;

		/* Ignore monsters with incompatible levels */
		if ((r_ptr->level < lev1) || (r_ptr->level > lev2)) continue;

		/* Use that index */
		r_idx = r;

		break;
	}
	return (r_idx);
}

/*!
 * @brief 指定座標にいるモンスターを変身させる /
 * Helper function -- return a "nearby" race for polymorphing
 * @param y 指定のY座標
 * @param x 指定のX座標
 * @return 実際に変身したらTRUEを返す
 */
bool polymorph_monster(player_type *caster_ptr, POSITION y, POSITION x)
{
	grid_type *g_ptr = &caster_ptr->current_floor_ptr->grid_array[y][x];
	monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[g_ptr->m_idx];
	bool polymorphed = FALSE;
	MONRACE_IDX new_r_idx;
	MONRACE_IDX old_r_idx = m_ptr->r_idx;
	bool targeted = (target_who == g_ptr->m_idx) ? TRUE : FALSE;
	bool health_tracked = (caster_ptr->health_who == g_ptr->m_idx) ? TRUE : FALSE;
	monster_type back_m;

	if (caster_ptr->inside_arena || caster_ptr->phase_out) return (FALSE);

	if ((caster_ptr->riding == g_ptr->m_idx) || (m_ptr->mflag2 & MFLAG2_KAGE)) return (FALSE);

	/* Memorize the monster before polymorphing */
	back_m = *m_ptr;

	/* Pick a "new" monster race */
	new_r_idx = poly_r_idx(old_r_idx);

	/* Handle polymorph */
	if (new_r_idx != old_r_idx)
	{
		BIT_FLAGS mode = 0L;
		bool preserve_hold_objects = back_m.hold_o_idx ? TRUE : FALSE;
		OBJECT_IDX this_o_idx, next_o_idx = 0;

		/* Get the monsters attitude */
		if (is_friendly(m_ptr)) mode |= PM_FORCE_FRIENDLY;
		if (is_pet(m_ptr)) mode |= PM_FORCE_PET;
		if (m_ptr->mflag2 & MFLAG2_NOPET) mode |= PM_NO_PET;

		/* Mega-hack -- ignore held objects */
		m_ptr->hold_o_idx = 0;

		/* "Kill" the "old" monster */
		delete_monster_idx(g_ptr->m_idx);

		/* Create a new monster (no groups) */
		if (place_monster_aux(0, y, x, new_r_idx, mode))
		{
			caster_ptr->current_floor_ptr->m_list[hack_m_idx_ii].nickname = back_m.nickname;
			caster_ptr->current_floor_ptr->m_list[hack_m_idx_ii].parent_m_idx = back_m.parent_m_idx;
			caster_ptr->current_floor_ptr->m_list[hack_m_idx_ii].hold_o_idx = back_m.hold_o_idx;

			/* Success */
			polymorphed = TRUE;
		}
		else
		{
			/* Placing the new monster failed */
			if (place_monster_aux(0, y, x, old_r_idx, (mode | PM_NO_KAGE | PM_IGNORE_TERRAIN)))
			{
				caster_ptr->current_floor_ptr->m_list[hack_m_idx_ii] = back_m;

				/* Re-initialize monster process */
				mproc_init();
			}
			else preserve_hold_objects = FALSE;
		}

		/* Mega-hack -- preserve held objects */
		if (preserve_hold_objects)
		{
			for (this_o_idx = back_m.hold_o_idx; this_o_idx; this_o_idx = next_o_idx)
			{
				object_type *o_ptr = &caster_ptr->current_floor_ptr->o_list[this_o_idx];
				next_o_idx = o_ptr->next_o_idx;

				/* Held by new monster */
				o_ptr->held_m_idx = hack_m_idx_ii;
			}
		}
		else if (back_m.hold_o_idx) /* Failed (paranoia) */
		{
			for (this_o_idx = back_m.hold_o_idx; this_o_idx; this_o_idx = next_o_idx)
			{
				next_o_idx = caster_ptr->current_floor_ptr->o_list[this_o_idx].next_o_idx;
				delete_object_idx(this_o_idx);
			}
		}

		if (targeted) target_who = hack_m_idx_ii;
		if (health_tracked) health_track(hack_m_idx_ii);
	}

	return polymorphed;
}

/*!
 * @brief 次元の扉処理 /
 * Dimension Door
 * @param x テレポート先のX座標
 * @param y テレポート先のY座標
 * @return 目標に指定通りテレポートできたならばTRUEを返す
 */
static bool dimension_door_aux(DEPTH x, DEPTH y)
{
	PLAYER_LEVEL plev = p_ptr->lev;

	p_ptr->energy_need += (s16b)((s32b)(60 - plev) * ENERGY_NEED() / 100L);

	if (!cave_player_teleportable_bold(y, x, 0L) ||
	    (distance(y, x, p_ptr->y, p_ptr->x) > plev / 2 + 10) ||
	    (!randint0(plev / 10 + 10)))
	{
		p_ptr->energy_need += (s16b)((s32b)(60 - plev) * ENERGY_NEED() / 100L);
		teleport_player((plev + 2) * 2, TELEPORT_PASSIVE);

		/* Failed */
		return FALSE;
	}
	else
	{
		teleport_player_to(y, x, 0L);

		/* Success */
		return TRUE;
	}
}


/*!
 * @brief 次元の扉処理のメインルーチン /
 * Dimension Door
 * @return ターンを消費した場合TRUEを返す
 */
bool dimension_door(void)
{
	DEPTH x = 0, y = 0;

	/* Rerutn FALSE if cancelled */
	if (!tgt_pt(&x, &y)) return FALSE;

	if (dimension_door_aux(x, y)) return TRUE;

	msg_print(_("精霊界から物質界に戻る時うまくいかなかった！", "You fail to exit the astral plane correctly!"));

	return TRUE;
}


/*!
 * @brief 鏡抜け処理のメインルーチン /
 * Mirror Master's Dimension Door
 * @return ターンを消費した場合TRUEを返す
 */
bool mirror_tunnel(void)
{
	POSITION x = 0, y = 0;

	/* Rerutn FALSE if cancelled */
	if (!tgt_pt(&x, &y)) return FALSE;

	if (dimension_door_aux(x, y)) return TRUE;

	msg_print(_("鏡の世界をうまく通れなかった！", "You fail to pass the mirror plane correctly!"));

	return TRUE;
}

/*!
 * @brief 魔力食い処理
 * @param power 基本効力
 * @return ターンを消費した場合TRUEを返す
 */
bool eat_magic(int power)
{
	object_type *o_ptr;
	object_kind *k_ptr;
	DEPTH lev;
	OBJECT_IDX item;
	int recharge_strength = 0;

	bool fail = FALSE;
	byte fail_type = 1;

	concptr q, s;
	GAME_TEXT o_name[MAX_NLEN];

	item_tester_hook = item_tester_hook_recharge;

	q = _("どのアイテムから魔力を吸収しますか？", "Drain which item? ");
	s = _("魔力を吸収できるアイテムがありません。", "You have nothing to drain.");

	o_ptr = choose_object(p_ptr, &item, q, s, (USE_INVEN | USE_FLOOR), 0);
	if (!o_ptr) return FALSE;

	k_ptr = &k_info[o_ptr->k_idx];
	lev = k_info[o_ptr->k_idx].level;

	if (o_ptr->tval == TV_ROD)
	{
		recharge_strength = ((power > lev/2) ? (power - lev/2) : 0) / 5;

		/* Back-fire */
		if (one_in_(recharge_strength))
		{
			/* Activate the failure code. */
			fail = TRUE;
		}
		else
		{
			if (o_ptr->timeout > (o_ptr->number - 1) * k_ptr->pval)
			{
				msg_print(_("充填中のロッドから魔力を吸収することはできません。", "You can't absorb energy from a discharged rod."));
			}
			else
			{
				p_ptr->csp += lev;
				o_ptr->timeout += k_ptr->pval;
			}
		}
	}
	else
	{
		/* All staffs, wands. */
		recharge_strength = (100 + power - lev) / 15;
		if (recharge_strength < 0) recharge_strength = 0;

		/* Back-fire */
		if (one_in_(recharge_strength))
		{
			/* Activate the failure code. */
			fail = TRUE;
		}
		else
		{
			if (o_ptr->pval > 0)
			{
				p_ptr->csp += lev / 2;
				o_ptr->pval --;

				/* XXX Hack -- unstack if necessary */
				if ((o_ptr->tval == TV_STAFF) && (item >= 0) && (o_ptr->number > 1))
				{
					object_type forge;
					object_type *q_ptr;
					q_ptr = &forge;
					object_copy(q_ptr, o_ptr);

					/* Modify quantity */
					q_ptr->number = 1;

					/* Restore the charges */
					o_ptr->pval++;

					/* Unstack the used item */
					o_ptr->number--;
					p_ptr->total_weight -= q_ptr->weight;
					item = inven_carry(q_ptr);

					msg_print(_("杖をまとめなおした。", "You unstack your staff."));
				}
			}
			else
			{
				msg_print(_("吸収できる魔力がありません！", "There's no energy there to absorb!"));
			}
			if (!o_ptr->pval) o_ptr->ident |= IDENT_EMPTY;
		}
	}

	/* Inflict the penalties for failing a recharge. */
	if (fail)
	{
		/* Artifacts are never destroyed. */
		if (object_is_fixed_artifact(o_ptr))
		{
			object_desc(o_name, o_ptr, OD_NAME_ONLY);
			msg_format(_("魔力が逆流した！%sは完全に魔力を失った。", "The recharging backfires - %s is completely drained!"), o_name);

			/* Artifact rods. */
			if (o_ptr->tval == TV_ROD)
				o_ptr->timeout = k_ptr->pval * o_ptr->number;

			/* Artifact wands and staffs. */
			else if ((o_ptr->tval == TV_WAND) || (o_ptr->tval == TV_STAFF))
				o_ptr->pval = 0;
		}
		else
		{
			/* Get the object description */
			object_desc(o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));

			/*** Determine Seriousness of Failure ***/

			/* Mages recharge objects more safely. */
			if (IS_WIZARD_CLASS(p_ptr))
			{
				/* 10% chance to blow up one rod, otherwise draining. */
				if (o_ptr->tval == TV_ROD)
				{
					if (one_in_(10)) fail_type = 2;
					else fail_type = 1;
				}
				/* 75% chance to blow up one wand, otherwise draining. */
				else if (o_ptr->tval == TV_WAND)
				{
					if (!one_in_(3)) fail_type = 2;
					else fail_type = 1;
				}
				/* 50% chance to blow up one staff, otherwise no effect. */
				else if (o_ptr->tval == TV_STAFF)
				{
					if (one_in_(2)) fail_type = 2;
					else fail_type = 0;
				}
			}

			/* All other classes get no special favors. */
			else
			{
				/* 33% chance to blow up one rod, otherwise draining. */
				if (o_ptr->tval == TV_ROD)
				{
					if (one_in_(3)) fail_type = 2;
					else fail_type = 1;
				}
				/* 20% chance of the entire stack, else destroy one wand. */
				else if (o_ptr->tval == TV_WAND)
				{
					if (one_in_(5)) fail_type = 3;
					else fail_type = 2;
				}
				/* Blow up one staff. */
				else if (o_ptr->tval == TV_STAFF)
				{
					fail_type = 2;
				}
			}

			/*** Apply draining and destruction. ***/

			/* Drain object or stack of objects. */
			if (fail_type == 1)
			{
				if (o_ptr->tval == TV_ROD)
				{
					msg_format(_("ロッドは破損を免れたが、魔力は全て失なわれた。",
								 "You save your rod from destruction, but all charges are lost."), o_name);
					o_ptr->timeout = k_ptr->pval * o_ptr->number;
				}
				else if (o_ptr->tval == TV_WAND)
				{
					msg_format(_("%sは破損を免れたが、魔力が全て失われた。", "You save your %s from destruction, but all charges are lost."), o_name);
					o_ptr->pval = 0;
				}
				/* Staffs aren't drained. */
			}

			/* Destroy an object or one in a stack of objects. */
			if (fail_type == 2)
			{
				if (o_ptr->number > 1)
				{
					msg_format(_("乱暴な魔法のために%sが一本壊れた！", "Wild magic consumes one of your %s!"), o_name);
					/* Reduce rod stack maximum timeout, drain wands. */
					if (o_ptr->tval == TV_ROD) o_ptr->timeout = MIN(o_ptr->timeout, k_ptr->pval * (o_ptr->number - 1));
					else if (o_ptr->tval == TV_WAND) o_ptr->pval = o_ptr->pval * (o_ptr->number - 1) / o_ptr->number;
				}
				else
				{
					msg_format(_("乱暴な魔法のために%sが何本か壊れた！", "Wild magic consumes your %s!"), o_name);
				}
				
				/* Reduce and describe p_ptr->inventory_list */
				if (item >= 0)
				{
					inven_item_increase(item, -1);
					inven_item_describe(item);
					inven_item_optimize(item);
				}

				/* Reduce and describe floor item */
				else
				{
					floor_item_increase(0 - item, -1);
					floor_item_describe(0 - item);
					floor_item_optimize(0 - item);
				}
			}

			/* Destroy all members of a stack of objects. */
			if (fail_type == 3)
			{
				if (o_ptr->number > 1)
					msg_format(_("乱暴な魔法のために%sが全て壊れた！", "Wild magic consumes all your %s!"), o_name);
				else
					msg_format(_("乱暴な魔法のために%sが壊れた！", "Wild magic consumes your %s!"), o_name);

				/* Reduce and describe p_ptr->inventory_list */
				if (item >= 0)
				{
					inven_item_increase(item, -999);
					inven_item_describe(item);
					inven_item_optimize(item);
				}

				/* Reduce and describe floor item */
				else
				{
					floor_item_increase(0 - item, -999);
					floor_item_describe(0 - item);
					floor_item_optimize(0 - item);
				}
			}
		}
	}

	if (p_ptr->csp > p_ptr->msp)
	{
		p_ptr->csp = p_ptr->msp;
	}

	p_ptr->redraw |= (PR_MANA);
	p_ptr->update |= (PU_COMBINE | PU_REORDER);
	p_ptr->window |= (PW_INVEN);

	return TRUE;
}


/*!
 * @brief 皆殺し(全方向攻撃)処理
 * @param py プレイヤーY座標
 * @param px プレイヤーX座標
 * @return なし
 */
void massacre(void)
{
	POSITION x, y;
	grid_type *g_ptr;
	monster_type *m_ptr;
	DIRECTION dir;

	for (dir = 0; dir < 8; dir++)
	{
		y = p_ptr->y + ddy_ddd[dir];
		x = p_ptr->x + ddx_ddd[dir];
		g_ptr = &p_ptr->current_floor_ptr->grid_array[y][x];
		m_ptr = &p_ptr->current_floor_ptr->m_list[g_ptr->m_idx];

		/* Hack -- attack monsters */
		if (g_ptr->m_idx && (m_ptr->ml || cave_have_flag_bold(y, x, FF_PROJECT)))
			py_attack(p_ptr, y, x, 0);
	}
}

bool eat_lock(void)
{
	POSITION x, y;
	grid_type *g_ptr;
	feature_type *f_ptr, *mimic_f_ptr;
	DIRECTION dir;

	if (!get_direction(&dir, FALSE, FALSE)) return FALSE;
	y = p_ptr->y + ddy[dir];
	x = p_ptr->x + ddx[dir];
	g_ptr = &p_ptr->current_floor_ptr->grid_array[y][x];
	f_ptr = &f_info[g_ptr->feat];
	mimic_f_ptr = &f_info[get_feat_mimic(g_ptr)];

	stop_mouth();

	if (!have_flag(mimic_f_ptr->flags, FF_HURT_ROCK))
	{
		msg_print(_("この地形は食べられない。", "You cannot eat this feature."));
	}
	else if (have_flag(f_ptr->flags, FF_PERMANENT))
	{
		msg_format(_("いてっ！この%sはあなたの歯より硬い！", "Ouch!  This %s is harder than your teeth!"), f_name + mimic_f_ptr->name);
	}
	else if (g_ptr->m_idx)
	{
		monster_type *m_ptr = &p_ptr->current_floor_ptr->m_list[g_ptr->m_idx];
		msg_print(_("何かが邪魔しています！", "There's something in the way!"));

		if (!m_ptr->ml || !is_pet(m_ptr)) py_attack(p_ptr, y, x, 0);
	}
	else if (have_flag(f_ptr->flags, FF_TREE))
	{
		msg_print(_("木の味は好きじゃない！", "You don't like the woody taste!"));
	}
	else if (have_flag(f_ptr->flags, FF_GLASS))
	{
		msg_print(_("ガラスの味は好きじゃない！", "You don't like the glassy taste!"));
	}
	else if (have_flag(f_ptr->flags, FF_DOOR) || have_flag(f_ptr->flags, FF_CAN_DIG))
	{
		(void)set_food(p_ptr, p_ptr->food + 3000);
	}
	else if (have_flag(f_ptr->flags, FF_MAY_HAVE_GOLD) || have_flag(f_ptr->flags, FF_HAS_GOLD))
	{
		(void)set_food(p_ptr, p_ptr->food + 5000);
	}
	else
	{
		msg_format(_("この%sはとてもおいしい！", "This %s is very filling!"), f_name + mimic_f_ptr->name);
		(void)set_food(p_ptr, p_ptr->food + 10000);
	}

	/* Destroy the wall */
	cave_alter_feat(y, x, FF_HURT_ROCK);

	(void)move_player_effect(p_ptr, y, x, MPE_DONT_PICKUP);
	return TRUE;
}


bool shock_power(void)
{
	DIRECTION dir;
	POSITION y, x;
	HIT_POINT dam;
	PLAYER_LEVEL plev = p_ptr->lev;
	int boost = P_PTR_KI;
	if (heavy_armor(p_ptr)) boost /= 2;

	project_length = 1;
	if (!get_aim_dir(&dir)) return FALSE;

	y = p_ptr->y + ddy[dir];
	x = p_ptr->x + ddx[dir];
	dam = damroll(8 + ((plev - 5) / 4) + boost / 12, 8);
	fire_beam(GF_MISSILE, dir, dam);
	if (p_ptr->current_floor_ptr->grid_array[y][x].m_idx)
	{
		int i;
		POSITION ty = y, tx = x;
		POSITION oy = y, ox = x;
		MONSTER_IDX m_idx = p_ptr->current_floor_ptr->grid_array[y][x].m_idx;
		monster_type *m_ptr = &p_ptr->current_floor_ptr->m_list[m_idx];
		monster_race *r_ptr = &r_info[m_ptr->r_idx];
		GAME_TEXT m_name[MAX_NLEN];

		monster_desc(m_name, m_ptr, 0);

		if (randint1(r_ptr->level * 3 / 2) > randint0(dam / 2) + dam / 2)
		{
			msg_format(_("%sは飛ばされなかった。", "%^s was not blown away."), m_name);
		}
		else
		{
			for (i = 0; i < 5; i++)
			{
				y += ddy[dir];
				x += ddx[dir];
				if (cave_empty_bold(y, x))
				{
					ty = y;
					tx = x;
				}
				else break;
			}
			if ((ty != oy) || (tx != ox))
			{
				msg_format(_("%sを吹き飛ばした！", "You blow %s away!"), m_name);
				p_ptr->current_floor_ptr->grid_array[oy][ox].m_idx = 0;
				p_ptr->current_floor_ptr->grid_array[ty][tx].m_idx = m_idx;
				m_ptr->fy = ty;
				m_ptr->fx = tx;

				update_monster(m_idx, TRUE);
				lite_spot(oy, ox);
				lite_spot(ty, tx);

				if (r_ptr->flags7 & (RF7_LITE_MASK | RF7_DARK_MASK))
					p_ptr->update |= (PU_MON_LITE);
			}
		}
	}
	return TRUE;
}

bool booze(player_type *creature_ptr)
{
	bool ident = FALSE;
	if (creature_ptr->pclass != CLASS_MONK) chg_virtue(p_ptr, V_HARMONY, -1);
	else if (!creature_ptr->resist_conf) creature_ptr->special_attack |= ATTACK_SUIKEN;
	if (!creature_ptr->resist_conf)
	{
		if (set_confused(p_ptr, randint0(20) + 15))
		{
			ident = TRUE;
		}
	}

	if (!creature_ptr->resist_chaos)
	{
		if (one_in_(2))
		{
			if (set_image(p_ptr, creature_ptr->image + randint0(150) + 150))
			{
				ident = TRUE;
			}
		}
		if (one_in_(13) && (creature_ptr->pclass != CLASS_MONK))
		{
			ident = TRUE;
			if (one_in_(3)) lose_all_info(p_ptr);
			else wiz_dark(creature_ptr);
			(void)teleport_player_aux(creature_ptr, 100, TELEPORT_NONMAGICAL | TELEPORT_PASSIVE);
			wiz_dark(creature_ptr);
			msg_print(_("知らない場所で目が醒めた。頭痛がする。", "You wake up somewhere with a sore head..."));
			msg_print(_("何も思い出せない。どうやってここへ来たのかも分からない！", "You can't remember a thing, or how you got here!"));
		}
	}
	return ident;
}

bool detonation(player_type *creature_ptr)
{
	msg_print(_("体の中で激しい爆発が起きた！", "Massive explosions rupture your body!"));
	take_hit(p_ptr, DAMAGE_NOESCAPE, damroll(50, 20), _("爆発の薬", "a potion of Detonation"), -1);
	(void)set_stun(p_ptr, creature_ptr->stun + 75);
	(void)set_cut(p_ptr,creature_ptr->cut + 5000);
	return TRUE;
}

void blood_curse_to_enemy(MONSTER_IDX m_idx)
{
	monster_type *m_ptr = &p_ptr->current_floor_ptr->m_list[m_idx];
	grid_type *g_ptr = &p_ptr->current_floor_ptr->grid_array[m_ptr->fy][m_ptr->fx];
	BIT_FLAGS curse_flg = (PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP);
	int count = 0;
	do
	{
		switch (randint1(28))
		{
		case 1: case 2:
			if (!count)
			{
				msg_print(_("地面が揺れた...", "The ground trembles..."));
				earthquake(p_ptr, m_ptr->fy, m_ptr->fx, 4 + randint0(4), 0);
				if (!one_in_(6)) break;
			}
		case 3: case 4: case 5: case 6:
			if (!count)
			{
				int extra_dam = damroll(10, 10);
				msg_print(_("純粋な魔力の次元への扉が開いた！", "A portal opens to a plane of raw mana!"));

				project(0, 8, m_ptr->fy, m_ptr->fx, extra_dam, GF_MANA, curse_flg, -1);
				if (!one_in_(6)) break;
			}
		case 7: case 8:
			if (!count)
			{
				msg_print(_("空間が歪んだ！", "Space warps about you!"));

				if (m_ptr->r_idx) teleport_away(g_ptr->m_idx, damroll(10, 10), TELEPORT_PASSIVE);
				if (one_in_(13)) count += activate_hi_summon(m_ptr->fy, m_ptr->fx, TRUE);
				if (!one_in_(6)) break;
			}
		case 9: case 10: case 11:
			msg_print(_("エネルギーのうねりを感じた！", "You feel a surge of energy!"));
			project(0, 7, m_ptr->fy, m_ptr->fx, 50, GF_DISINTEGRATE, curse_flg, -1);
			if (!one_in_(6)) break;
		case 12: case 13: case 14: case 15: case 16:
			aggravate_monsters(0);
			if (!one_in_(6)) break;
		case 17: case 18:
			count += activate_hi_summon(m_ptr->fy, m_ptr->fx, TRUE);
			if (!one_in_(6)) break;
		case 19: case 20: case 21: case 22:
		{
			bool pet = !one_in_(3);
			BIT_FLAGS mode = PM_ALLOW_GROUP;

			if (pet) mode |= PM_FORCE_PET;
			else mode |= (PM_NO_PET | PM_FORCE_FRIENDLY);

			count += summon_specific((pet ? -1 : 0), p_ptr->y, p_ptr->x, (pet ? p_ptr->lev * 2 / 3 + randint1(p_ptr->lev / 2) : p_ptr->current_floor_ptr->dun_level), 0, mode);
			if (!one_in_(6)) break;
		}
		case 23: case 24: case 25:
			if (p_ptr->hold_exp && (randint0(100) < 75)) break;
			msg_print(_("経験値が体から吸い取られた気がする！", "You feel your experience draining away..."));

			if (p_ptr->hold_exp) lose_exp(p_ptr, p_ptr->exp / 160);
			else lose_exp(p_ptr, p_ptr->exp / 16);
			if (!one_in_(6)) break;
		case 26: case 27: case 28:
		{
			int i = 0;
			if (one_in_(13))
			{
				while (i < A_MAX)
				{
					do
					{
						(void)do_dec_stat(p_ptr, i);
					} while (one_in_(2));

					i++;
				}
			}
			else
			{
				(void)do_dec_stat(p_ptr, randint0(6));
			}
			break;
		}
		}
	} while (one_in_(5));
}

/*!
 * @brief クリムゾンを発射する / Fire Crimson, evoluting gun.
 @ @param shooter_ptr 射撃を行うクリーチャー参照
 * @return キャンセルした場合 false.
 * @details
 * Need to analyze size of the window.
 * Need more color coding.
 */
bool fire_crimson(player_type *shooter_ptr)
{
	int num = 1;
	int i;
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
	POSITION tx, ty;
	DIRECTION dir;

	if (!get_aim_dir(&dir)) return FALSE;

	/* Use the given direction */
	tx = shooter_ptr->x + 99 * ddx[dir];
	ty = shooter_ptr->y + 99 * ddy[dir];

	/* Hack -- Use an actual "target" */
	if ((dir == 5) && target_okay())
	{
		tx = target_col;
		ty = target_row;
	}

	if (shooter_ptr->pclass == CLASS_ARCHER)
	{
		/* Extra shot at level 10 */
		if (shooter_ptr->lev >= 10) num++;

		/* Extra shot at level 30 */
		if (shooter_ptr->lev >= 30) num++;

		/* Extra shot at level 45 */
		if (shooter_ptr->lev >= 45) num++;
	}

	for (i = 0; i < num; i++)
		project(0, shooter_ptr->lev / 20 + 1, ty, tx, shooter_ptr->lev*shooter_ptr->lev * 6 / 50, GF_ROCKET, flg, -1);

	return TRUE;
}


/*!
 * @brief 町間のテレポートを行うメインルーチン。
 * @return テレポート処理を決定したか否か
 */
bool tele_town(void)
{
	int i;
	POSITION x, y;
	int num = 0;

	if (p_ptr->current_floor_ptr->dun_level)
	{
		msg_print(_("この魔法は地上でしか使えない！", "This spell can only be used on the surface!"));
		return FALSE;
	}

	if (p_ptr->inside_arena || p_ptr->phase_out)
	{
		msg_print(_("この魔法は外でしか使えない！", "This spell can only be used outside!"));
		return FALSE;
	}

	screen_save();
	clear_bldg(4, 10);

	for (i = 1; i < max_towns; i++)
	{
		char buf[80];

		if ((i == NO_TOWN) || (i == SECRET_TOWN) || (i == p_ptr->town_num) || !(p_ptr->visit & (1L << (i - 1)))) continue;

		sprintf(buf, "%c) %-20s", I2A(i - 1), town_info[i].name);
		prt(buf, 5 + i, 5);
		num++;
	}

	if (!num)
	{
		msg_print(_("まだ行けるところがない。", "You have not yet visited any town."));
		msg_print(NULL);
		screen_load();
		return FALSE;
	}

	prt(_("どこに行きますか:", "Which town you go: "), 0, 0);
	while (1)
	{
		i = inkey();

		if (i == ESCAPE)
		{
			screen_load();
			return FALSE;
		}
		else if ((i < 'a') || (i > ('a' + max_towns - 2))) continue;
		else if (((i - 'a' + 1) == p_ptr->town_num) || ((i - 'a' + 1) == NO_TOWN) || ((i - 'a' + 1) == SECRET_TOWN) || !(p_ptr->visit & (1L << (i - 'a')))) continue;
		break;
	}

	for (y = 0; y < current_world_ptr->max_wild_y; y++)
	{
		for (x = 0; x < current_world_ptr->max_wild_x; x++)
		{
			if (wilderness[y][x].town == (i - 'a' + 1))
			{
				p_ptr->wilderness_y = y;
				p_ptr->wilderness_x = x;
			}
		}
	}

	p_ptr->leaving = TRUE;
	p_ptr->leave_bldg = TRUE;
	p_ptr->teleport_town = TRUE;
	screen_load();
	return TRUE;
}
