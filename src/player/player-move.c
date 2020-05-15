/*!
 *  @file cmd1.c
 *  @brief プレイヤーのコマンド処理1 / Movement commands (part 1)
 *  @date 2014/01/02
 *  @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * @note
 * <pre>
 * The running algorithm:                       -CJS-
 *
 * In the diagrams below, the player has just arrived in the
 * grid marked as '@', and he has just come from a grid marked
 * as 'o', and he is about to enter the grid marked as 'x'.
 *
 * Of course, if the "requested" move was impossible, then you
 * will of course be blocked, and will stop.
 *
 * Overview: You keep moving until something interesting happens.
 * If you are in an enclosed space, you follow corners. This is
 * the usual corridor scheme. If you are in an open space, you go
 * straight, but stop before entering enclosed space. This is
 * analogous to reaching doorways. If you have enclosed space on
 * one side only (that is, running along side a wall) stop if
 * your wall opens out, or your open space closes in. Either case
 * corresponds to a doorway.
 *
 * What happens depends on what you can really SEE. (i.e. if you
 * have no light, then running along a dark corridor is JUST like
 * running in a dark room.) The algorithm works equally well in
 * corridors, rooms, mine tailings, earthquake rubble, etc, etc.
 *
 * These conditions are kept in static memory:
 * find_openarea         You are in the open on at least one
 * side.
 * find_breakleft        You have a wall on the left, and will
 * stop if it opens
 * find_breakright       You have a wall on the right, and will
 * stop if it opens
 *
 * To initialize these conditions, we examine the grids adjacent
 * to the grid marked 'x', two on each side (marked 'L' and 'R').
 * If either one of the two grids on a given side is seen to be
 * closed, then that side is considered to be closed. If both
 * sides are closed, then it is an enclosed (corridor) run.
 *
 * LL           L
 * @@x          LxR
 * RR          @@R
 *
 * Looking at more than just the immediate squares is
 * significant. Consider the following case. A run along the
 * corridor will stop just before entering the center point,
 * because a choice is clearly established. Running in any of
 * three available directions will be defined as a corridor run.
 * Note that a minor hack is inserted to make the angled corridor
 * entry (with one side blocked near and the other side blocked
 * further away from the runner) work correctly. The runner moves
 * diagonally, but then saves the previous direction as being
 * straight into the gap. Otherwise, the tail end of the other
 * entry would be perceived as an alternative on the next move.
 *
 * \#.\#
 * \#\#.\#\#
 * \.\@x..
 * \#\#.\#\#
 * \#.\#
 *
 * Likewise, a run along a wall, and then into a doorway (two
 * runs) will work correctly. A single run rightwards from \@ will
 * stop at 1. Another run right and down will enter the corridor
 * and make the corner, stopping at the 2.
 *
 * \#\#\#\#\#\#\#\#\#\#\#\#\#\#\#\#\#\#
 * o@@x       1
 * \#\#\#\#\#\#\#\#\#\#\# \#\#\#\#\#\#
 * \#2          \#
 * \#\#\#\#\#\#\#\#\#\#\#\#\#
 *
 * After any move, the function area_affect is called to
 * determine the new surroundings, and the direction of
 * subsequent moves. It examines the current player location
 * (at which the runner has just arrived) and the previous
 * direction (from which the runner is considered to have come).
 *
 * Moving one square in some direction places you adjacent to
 * three or five new squares (for straight and diagonal moves
 * respectively) to which you were not previously adjacent,
 * marked as '!' in the diagrams below.
 *
 *   ...!              ...
 *   .o@@!  (normal)    .o.!  (diagonal)
 *   ...!  (east)      ..@@!  (south east)
 *                      !!!
 *
 * You STOP if any of the new squares are interesting in any way:
 * for example, if they contain visible monsters or treasure.
 *
 * You STOP if any of the newly adjacent squares seem to be open,
 * and you are also looking for a break on that side. (that is,
 * find_openarea AND find_break).
 *
 * You STOP if any of the newly adjacent squares do NOT seem to be
 * open and you are in an open area, and that side was previously
 * entirely open.
 *
 * Corners: If you are not in the open (i.e. you are in a corridor)
 * and there is only one way to go in the new squares, then turn in
 * that direction. If there are more than two new ways to go, STOP.
 * If there are two ways to go, and those ways are separated by a
 * square which does not seem to be open, then STOP.
 *
 * Otherwise, we have a potential corner. There are two new open
 * squares, which are also adjacent. One of the new squares is
 * diagonally located, the other is straight on (as in the diagram).
 * We consider two more squares further out (marked below as ?).
 *
 * We assign "option" to the straight-on grid, and "option2" to the
 * diagonal grid, and "check_dir" to the grid marked 's'.
 *
 * \#\#s
 * @@x?
 * \#.?
 *
 * If they are both seen to be closed, then it is seen that no benefit
 * is gained from moving straight. It is a known corner.  To cut the
 * corner, go diagonally, otherwise go straight, but pretend you
 * stepped diagonally into that next location for a full view next
 * time. Conversely, if one of the ? squares is not seen to be closed,
 * then there is a potential choice. We check to see whether it is a
 * potential corner or an intersection/room entrance.  If the square
 * two spaces straight ahead, and the space marked with 's' are both
 * unknown space, then it is a potential corner and enter if
 * find_examine is set, otherwise must stop because it is not a
 * corner. (find_examine option is removed and always is TRUE.)
 * </pre>
 */

#include "system/angband.h"
#include "core/stuff-handler.h"
#include "core/special-internal-keys.h"
#include "util.h"
#include "main/sound-definitions-table.h"

#include "realm/realm-song.h"
#include "autopick/autopick.h"
#include "dungeon/dungeon.h"
#include "effect/effect-characteristics.h"
#include "melee.h"
#include "grid/grid.h"
#include "grid/trap.h"
#include "quest.h"
#include "object/artifact.h"
#include "player/player-move.h"
#include "player/player-status.h"
#include "player/player-effects.h"
#include "player/player-race.h"
#include "player/player-class.h"
#include "inventory/player-inventory.h"
#include "player/player-personality.h"
#include "spell/spells-floor.h"
#include "grid/feature.h"
#include "warning.h"
#include "monster/monster.h"
#include "spell/monster-spell.h"
#include "monster/monster-status.h"
#include "object/object-hook.h"
#include "object/object-flavor.h"
#include "spell/spells-type.h"
#include "cmd-basic.h"
#include "view/display-main-window.h"
#include "world/world.h"
#include "object/object-kind.h"
#include "autopick/autopick.h"
#include "targeting.h"
#include "spell/process-effect.h"
#include "spell/spells3.h"

travel_type travel;

/*!
 * @brief 地形やその上のアイテムの隠された要素を全て明かす /
 * Search for hidden things
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param y 対象となるマスのY座標
 * @param x 対象となるマスのX座標
 * @return なし
 */
static void discover_hidden_things(player_type *creature_ptr, POSITION y, POSITION x)
{
	grid_type *g_ptr;
	floor_type *floor_ptr = creature_ptr->current_floor_ptr;
	g_ptr = &floor_ptr->grid_array[y][x];

	if (g_ptr->mimic && is_trap(creature_ptr, g_ptr->feat))
	{
		disclose_grid(creature_ptr, y, x);
		msg_print(_("トラップを発見した。", "You have found a trap."));
		disturb(creature_ptr, FALSE, TRUE);
	}

	if (is_hidden_door(creature_ptr, g_ptr))
	{
		msg_print(_("隠しドアを発見した。", "You have found a secret door."));
		disclose_grid(creature_ptr, y, x);
		disturb(creature_ptr, FALSE, FALSE);
	}

	/* Scan all objects in the grid */
	OBJECT_IDX next_o_idx = 0;
	for (OBJECT_IDX this_o_idx = g_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;
		o_ptr = &floor_ptr->o_list[this_o_idx];
		next_o_idx = o_ptr->next_o_idx;
		if (o_ptr->tval != TV_CHEST) continue;
		if (!chest_traps[o_ptr->pval]) continue;
		if (!object_is_known(o_ptr))
		{
			msg_print(_("箱に仕掛けられたトラップを発見した！", "You have discovered a trap on the chest!"));
			object_known(o_ptr);
			disturb(creature_ptr, FALSE, FALSE);
		}
	}
}


/*!
 * @brief プレイヤーの探索処理判定
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void search(player_type *creature_ptr)
{
	/* Start with base search ability */
	PERCENTAGE chance = creature_ptr->skill_srh;

	/* Penalize various conditions */
	if (creature_ptr->blind || no_lite(creature_ptr)) chance = chance / 10;
	if (creature_ptr->confused || creature_ptr->image) chance = chance / 10;

	/* Search the nearby grids, which are always in bounds */
	for (DIRECTION i = 0; i < 9; ++ i)
	{
		/* Sometimes, notice things */
		if (randint0(100) < chance)
		{
			discover_hidden_things(creature_ptr, creature_ptr->y + ddy_ddd[i], creature_ptr->x + ddx_ddd[i]);
		}
	}
}


/*!
 * @brief プレイヤーがオブジェクトを拾った際のメッセージ表示処理 /
 * Helper routine for py_pickup() and py_pickup_floor().
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param o_idx 取得したオブジェクトの参照ID
 * @return なし
 * @details
 * アイテムを拾った際に「２つのケーキを持っている」\n
 * "You have two cakes." とアイテムを拾った後の合計のみの表示がオリジナル\n
 * だが、違和感が\n
 * あるという指摘をうけたので、「～を拾った、～を持っている」という表示\n
 * にかえてある。そのための配列。\n
 * Add the given dungeon object to the character's inventory.\n
 * Delete the object afterwards.\n
 */
void py_pickup_aux(player_type *owner_ptr, OBJECT_IDX o_idx)
{
#ifdef JP
	GAME_TEXT o_name[MAX_NLEN];
	GAME_TEXT old_name[MAX_NLEN];
	char kazu_str[80];
	int hirottakazu;
#else
	GAME_TEXT o_name[MAX_NLEN];
#endif

	object_type *o_ptr;
	o_ptr = &owner_ptr->current_floor_ptr->o_list[o_idx];

#ifdef JP
	object_desc(owner_ptr, old_name, o_ptr, OD_NAME_ONLY);
	object_desc_kosuu(kazu_str, o_ptr);
	hirottakazu = o_ptr->number;
#endif

	/* Carry the object */
	INVENTORY_IDX slot = inven_carry(owner_ptr, o_ptr);

	/* Get the object again */
	o_ptr = &owner_ptr->inventory_list[slot];

	delete_object_idx(owner_ptr, o_idx);

	if (owner_ptr->pseikaku == SEIKAKU_MUNCHKIN)
	{
		bool old_known = identify_item(owner_ptr, o_ptr);

		/* Auto-inscription/destroy */
		autopick_alter_item(owner_ptr, slot, (bool)(destroy_identify && !old_known));

		/* If it is destroyed, don't pick it up */
		if (o_ptr->marked & OM_AUTODESTROY) return;
	}

	object_desc(owner_ptr, o_name, o_ptr, 0);

#ifdef JP
	if ((o_ptr->name1 == ART_CRIMSON) && (owner_ptr->pseikaku == SEIKAKU_COMBAT))
	{
		msg_format("こうして、%sは『クリムゾン』を手に入れた。", owner_ptr->name);
		msg_print("しかし今、『混沌のサーペント』の放ったモンスターが、");
		msg_format("%sに襲いかかる．．．", owner_ptr->name);
	}
	else
	{
		if (plain_pickup)
		{
			msg_format("%s(%c)を持っている。",o_name, index_to_label(slot));
		}
		else
		{
			if (o_ptr->number > hirottakazu) {
			    msg_format("%s拾って、%s(%c)を持っている。",
			       kazu_str, o_name, index_to_label(slot));
			} else {
				msg_format("%s(%c)を拾った。", o_name, index_to_label(slot));
			}
		}
	}

	strcpy(record_o_name, old_name);
#else
	msg_format("You have %s (%c).", o_name, index_to_label(slot));
	strcpy(record_o_name, o_name);
#endif
	record_turn = current_world_ptr->game_turn;
	check_find_art_quest_completion(owner_ptr, o_ptr);
}


/*!
 * @brief プレイヤーがオブジェクト上に乗った際の表示処理
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param pickup 自動拾い処理を行うならばTRUEとする
 * @return なし
 * @details
 * Player "wants" to pick up an object or gold.
 * Note that we ONLY handle things that can be picked up.
 * See "move_player(creature_ptr, )" for handling of other things.
 */
void carry(player_type *creature_ptr, bool pickup)
{
	/* Recenter the map around the player */
	verify_panel(creature_ptr);

	creature_ptr->update |= (PU_MONSTERS);
	creature_ptr->redraw |= (PR_MAP);
	creature_ptr->window |= (PW_OVERHEAD);
	handle_stuff(creature_ptr);

	/* Automatically pickup/destroy/inscribe items */
	grid_type *g_ptr = &creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x];
	autopick_pickup_items(creature_ptr, g_ptr);

	if (easy_floor)
	{
		py_pickup_floor(creature_ptr, pickup);
		return;
	}

	/* Scan the pile of objects */
	OBJECT_IDX next_o_idx = 0;
	for (OBJECT_IDX this_o_idx = g_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;
		o_ptr = &creature_ptr->current_floor_ptr->o_list[this_o_idx];
		GAME_TEXT o_name[MAX_NLEN];
		object_desc(creature_ptr, o_name, o_ptr, 0);
		next_o_idx = o_ptr->next_o_idx;

		disturb(creature_ptr, FALSE, FALSE);

		if (o_ptr->tval == TV_GOLD)
		{
			int value = (long)o_ptr->pval;
			delete_object_idx(creature_ptr, this_o_idx);
			msg_format(_(" $%ld の価値がある%sを見つけた。", "You collect %ld gold pieces worth of %s."),
			   (long)value, o_name);

			sound(SOUND_SELL);

			creature_ptr->au += value;
			creature_ptr->redraw |= (PR_GOLD);
			creature_ptr->window |= (PW_PLAYER);
			continue;
		}

		/* Hack - some objects were handled in autopick_pickup_items(). */
		if (o_ptr->marked & OM_NOMSG)
		{
			o_ptr->marked &= ~OM_NOMSG;
			continue;
		}
		
		if (!pickup)
		{
			msg_format(_("%sがある。", "You see %s."), o_name);
			continue;
		}
		
		if (!inven_carry_okay(o_ptr))
		{
			msg_format(_("ザックには%sを入れる隙間がない。", "You have no room for %s."), o_name);
			continue;
		}

		int is_pickup_successful = TRUE;
		if (carry_query_flag)
		{
			char out_val[MAX_NLEN + 20];
			sprintf(out_val, _("%sを拾いますか? ", "Pick up %s? "), o_name);
			is_pickup_successful = get_check(out_val);
		}

		if (is_pickup_successful)
		{
			py_pickup_aux(creature_ptr, this_o_idx);
		}
	}
}


/*!
 * @brief パターンによる移動制限処理
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param c_y プレイヤーの移動元Y座標
 * @param c_x プレイヤーの移動元X座標
 * @param n_y プレイヤーの移動先Y座標
 * @param n_x プレイヤーの移動先X座標
 * @return 移動処理が可能である場合（可能な場合に選択した場合）TRUEを返す。
 */
bool pattern_seq(player_type *creature_ptr, POSITION c_y, POSITION c_x, POSITION n_y, POSITION n_x)
{
	feature_type *cur_f_ptr = &f_info[creature_ptr->current_floor_ptr->grid_array[c_y][c_x].feat];
	feature_type *new_f_ptr = &f_info[creature_ptr->current_floor_ptr->grid_array[n_y][n_x].feat];
	bool is_pattern_tile_cur = have_flag(cur_f_ptr->flags, FF_PATTERN);
	bool is_pattern_tile_new = have_flag(new_f_ptr->flags, FF_PATTERN);

	if (!is_pattern_tile_cur && !is_pattern_tile_new) return TRUE;

	int pattern_type_cur = is_pattern_tile_cur ? cur_f_ptr->subtype : NOT_PATTERN_TILE;
	int pattern_type_new = is_pattern_tile_new ? new_f_ptr->subtype : NOT_PATTERN_TILE;

	if (pattern_type_new == PATTERN_TILE_START)
	{
		if (!is_pattern_tile_cur && !creature_ptr->confused && !creature_ptr->stun && !creature_ptr->image)
		{
			if (get_check(_("パターンの上を歩き始めると、全てを歩かなければなりません。いいですか？", 
							"If you start walking the Pattern, you must walk the whole way. Ok? ")))
				return TRUE;
			else
				return FALSE;
		}
		else
			return TRUE;
	}
	
	if ((pattern_type_new == PATTERN_TILE_OLD) ||
		 (pattern_type_new == PATTERN_TILE_END) ||
		 (pattern_type_new == PATTERN_TILE_WRECKED))
	{
		if (is_pattern_tile_cur)
		{
			return TRUE;
		}
		else
		{
			msg_print(_("パターンの上を歩くにはスタート地点から歩き始めなくてはなりません。",
						"You must start walking the Pattern from the startpoint."));

			return FALSE;
		}
	}
	
	if ((pattern_type_new == PATTERN_TILE_TELEPORT) ||
		 (pattern_type_cur == PATTERN_TILE_TELEPORT))
	{
		return TRUE;
	}
	
	if (pattern_type_cur == PATTERN_TILE_START)
	{
		if (is_pattern_tile_new)
			return TRUE;
		else
		{
			msg_print(_("パターンの上は正しい順序で歩かねばなりません。", "You must walk the Pattern in correct order."));
			return FALSE;
		}
	}
	
	if ((pattern_type_cur == PATTERN_TILE_OLD) ||
		 (pattern_type_cur == PATTERN_TILE_END) ||
		 (pattern_type_cur == PATTERN_TILE_WRECKED))
	{
		if (!is_pattern_tile_new)
		{
			msg_print(_("パターンを踏み外してはいけません。", "You may not step off from the Pattern."));
			return FALSE;
		}
		else
		{
			return TRUE;
		}
	}
	
	if (!is_pattern_tile_cur)
	{
		msg_print(_("パターンの上を歩くにはスタート地点から歩き始めなくてはなりません。",
			"You must start walking the Pattern from the startpoint."));

		return FALSE;
	}
	
	byte ok_move = PATTERN_TILE_START;
	switch (pattern_type_cur)
	{
	case PATTERN_TILE_1:
		ok_move = PATTERN_TILE_2;
		break;
	case PATTERN_TILE_2:
		ok_move = PATTERN_TILE_3;
		break;
	case PATTERN_TILE_3:
		ok_move = PATTERN_TILE_4;
		break;
	case PATTERN_TILE_4:
		ok_move = PATTERN_TILE_1;
		break;
	default:
		if (current_world_ptr->wizard)
			msg_format(_("おかしなパターン歩行、%d。", "Funny Pattern walking, %d."), pattern_type_cur);
		return TRUE;
	}

	if ((pattern_type_new == ok_move) || (pattern_type_new == pattern_type_cur)) return TRUE;
	
	if (!is_pattern_tile_new)
		msg_print(_("パターンを踏み外してはいけません。", "You may not step off from the Pattern."));
	else
		msg_print(_("パターンの上は正しい順序で歩かねばなりません。", "You must walk the Pattern in correct order."));

	return FALSE;
}


/*!
 * @brief 移動に伴うプレイヤーのステータス変化処理
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param ny 移動先Y座標
 * @param nx 移動先X座標
 * @param mpe_mode 移動オプションフラグ
 * @return プレイヤーが死亡やフロア離脱を行わず、実際に移動が可能ならばTRUEを返す。
 */
bool move_player_effect(player_type *creature_ptr, POSITION ny, POSITION nx, BIT_FLAGS mpe_mode)
{
	POSITION oy = creature_ptr->y;
	POSITION ox = creature_ptr->x;
	floor_type *floor_ptr = creature_ptr->current_floor_ptr;
	grid_type *g_ptr = &floor_ptr->grid_array[ny][nx];
	grid_type *oc_ptr = &floor_ptr->grid_array[oy][ox];
	feature_type *f_ptr = &f_info[g_ptr->feat];
	feature_type *of_ptr = &f_info[oc_ptr->feat];

	if (!(mpe_mode & MPE_STAYING))
	{
		MONSTER_IDX om_idx = oc_ptr->m_idx;
		MONSTER_IDX nm_idx = g_ptr->m_idx;

		creature_ptr->y = ny;
		creature_ptr->x = nx;

		/* Hack -- For moving monster or riding player's moving */
		if (!(mpe_mode & MPE_DONT_SWAP_MON))
		{
			/* Swap two monsters */
			g_ptr->m_idx = om_idx;
			oc_ptr->m_idx = nm_idx;

			if (om_idx > 0) /* Monster on old spot (or creature_ptr->riding) */
			{
				monster_type *om_ptr = &floor_ptr->m_list[om_idx];
				om_ptr->fy = ny;
				om_ptr->fx = nx;
				update_monster(creature_ptr, om_idx, TRUE);
			}

			if (nm_idx > 0) /* Monster on new spot */
			{
				monster_type *nm_ptr = &floor_ptr->m_list[nm_idx];
				nm_ptr->fy = oy;
				nm_ptr->fx = ox;
				update_monster(creature_ptr, nm_idx, TRUE);
			}
		}

		lite_spot(creature_ptr, oy, ox);
		lite_spot(creature_ptr, ny, nx);

		/* Check for new panel (redraw map) */
		verify_panel(creature_ptr);

		if (mpe_mode & MPE_FORGET_FLOW)
		{
			forget_flow(floor_ptr);

			creature_ptr->update |= (PU_UN_VIEW);
			creature_ptr->redraw |= (PR_MAP);
		}

		creature_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW | PU_MON_LITE | PU_DISTANCE);
		creature_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);

		/* Remove "unsafe" flag */
		if ((!creature_ptr->blind && !no_lite(creature_ptr)) || !is_trap(creature_ptr, g_ptr->feat)) g_ptr->info &= ~(CAVE_UNSAFE);

		/* For get everything when requested hehe I'm *NASTY* */
		if (floor_ptr->dun_level && (d_info[creature_ptr->dungeon_idx].flags1 & DF1_FORGET)) wiz_dark(creature_ptr);
		if (mpe_mode & MPE_HANDLE_STUFF) handle_stuff(creature_ptr);

		if (creature_ptr->pclass == CLASS_NINJA)
		{
			if (g_ptr->info & (CAVE_GLOW)) set_superstealth(creature_ptr, FALSE);
			else if (creature_ptr->cur_lite <= 0) set_superstealth(creature_ptr, TRUE);
		}

		if ((creature_ptr->action == ACTION_HAYAGAKE) &&
		    (!have_flag(f_ptr->flags, FF_PROJECT) ||
		     (!creature_ptr->levitation && have_flag(f_ptr->flags, FF_DEEP))))
		{
			msg_print(_("ここでは素早く動けない。", "You cannot run in here."));
			set_action(creature_ptr, ACTION_NONE);
		}
		if (creature_ptr->prace == RACE_MERFOLK)
		{
			if(have_flag(f_ptr->flags, FF_WATER) ^ have_flag(of_ptr->flags, FF_WATER))
			{
				creature_ptr->update |= PU_BONUS;
				update_creature(creature_ptr);
			}
		}
	}

	if (mpe_mode & MPE_ENERGY_USE)
	{
		if (music_singing(creature_ptr, MUSIC_WALL))
		{
			(void)project(creature_ptr, 0, 0, creature_ptr->y, creature_ptr->x, (60 + creature_ptr->lev), GF_DISINTEGRATE,
				PROJECT_KILL | PROJECT_ITEM, -1);

			if (!player_bold(creature_ptr, ny, nx) || creature_ptr->is_dead || creature_ptr->leaving) return FALSE;
		}

		/* Spontaneous Searching */
		if ((creature_ptr->skill_fos >= 50) || (0 == randint0(50 - creature_ptr->skill_fos)))
		{
			search(creature_ptr);
		}

		/* Continuous Searching */
		if (creature_ptr->action == ACTION_SEARCH)
		{
			search(creature_ptr);
		}
	}

	/* Handle "objects" */
	if (!(mpe_mode & MPE_DONT_PICKUP))
	{
		carry(creature_ptr, (mpe_mode & MPE_DO_PICKUP) ? TRUE : FALSE);
	}

	/* Handle "store doors" */
	if (have_flag(f_ptr->flags, FF_STORE))
	{
		disturb(creature_ptr, FALSE, TRUE);

		free_turn(creature_ptr);
		/* Hack -- Enter store */
		command_new = SPECIAL_KEY_STORE;
	}

	/* Handle "building doors" -KMW- */
	else if (have_flag(f_ptr->flags, FF_BLDG))
	{
		disturb(creature_ptr, FALSE, TRUE);

		free_turn(creature_ptr);
		/* Hack -- Enter building */
		command_new = SPECIAL_KEY_BUILDING;
	}

	/* Handle quest areas -KMW- */
	else if (have_flag(f_ptr->flags, FF_QUEST_ENTER))
	{
		disturb(creature_ptr, FALSE, TRUE);

		free_turn(creature_ptr);
		/* Hack -- Enter quest level */
		command_new = SPECIAL_KEY_QUEST;
	}

	else if (have_flag(f_ptr->flags, FF_QUEST_EXIT))
	{
		if (quest[floor_ptr->inside_quest].type == QUEST_TYPE_FIND_EXIT)
		{
			complete_quest(creature_ptr, floor_ptr->inside_quest);
		}

		leave_quest_check(creature_ptr);

		floor_ptr->inside_quest = g_ptr->special;
		floor_ptr->dun_level = 0;
		creature_ptr->oldpx = 0;
		creature_ptr->oldpy = 0;

		creature_ptr->leaving = TRUE;
	}

	/* Set off a trap */
	else if (have_flag(f_ptr->flags, FF_HIT_TRAP) && !(mpe_mode & MPE_STAYING))
	{
		disturb(creature_ptr, FALSE, TRUE);

		/* Hidden trap */
		if (g_ptr->mimic || have_flag(f_ptr->flags, FF_SECRET))
		{
			msg_print(_("トラップだ！", "You found a trap!"));

			/* Pick a trap */
			disclose_grid(creature_ptr, creature_ptr->y, creature_ptr->x);
		}

		/* Hit the trap */
		hit_trap(creature_ptr, (mpe_mode & MPE_BREAK_TRAP) ? TRUE : FALSE);

		if (!player_bold(creature_ptr, ny, nx) || creature_ptr->is_dead || creature_ptr->leaving) return FALSE;
	}

	/* Warn when leaving trap detected region */
	if (!(mpe_mode & MPE_STAYING) && (disturb_trap_detect || alert_trap_detect)
	    && creature_ptr->dtrap && !(g_ptr->info & CAVE_IN_DETECT))
	{
		/* No duplicate warning */
		creature_ptr->dtrap = FALSE;

		/* You are just on the edge */
		if (!(g_ptr->info & CAVE_UNSAFE))
		{
			if (alert_trap_detect)
			{
				msg_print(_("* 注意:この先はトラップの感知範囲外です！ *", "*Leaving trap detect region!*"));
			}

			if (disturb_trap_detect) disturb(creature_ptr, FALSE, TRUE);
		}
	}

	return player_bold(creature_ptr, ny, nx) && !creature_ptr->is_dead && !creature_ptr->leaving;
}


/*!
 * @brief 該当地形のトラップがプレイヤーにとって無効かどうかを判定して返す
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param feat 地形ID
 * @return トラップが自動的に無効ならばTRUEを返す
 */
bool trap_can_be_ignored(player_type *creature_ptr, FEAT_IDX feat)
{
	feature_type *f_ptr = &f_info[feat];

	if (!have_flag(f_ptr->flags, FF_TRAP)) return TRUE;

	switch (f_ptr->subtype)
	{
	case TRAP_TRAPDOOR:
	case TRAP_PIT:
	case TRAP_SPIKED_PIT:
	case TRAP_POISON_PIT:
		if (creature_ptr->levitation) return TRUE;
		break;
	case TRAP_TELEPORT:
		if (creature_ptr->anti_tele) return TRUE;
		break;
	case TRAP_FIRE:
		if (creature_ptr->immune_fire) return TRUE;
		break;
	case TRAP_ACID:
		if (creature_ptr->immune_acid) return TRUE;
		break;
	case TRAP_BLIND:
		if (creature_ptr->resist_blind) return TRUE;
		break;
	case TRAP_CONFUSE:
		if (creature_ptr->resist_conf) return TRUE;
		break;
	case TRAP_POISON:
		if (creature_ptr->resist_pois) return TRUE;
		break;
	case TRAP_SLEEP:
		if (creature_ptr->free_act) return TRUE;
		break;
	}

	return FALSE;
}


/*
 * todo 負論理なので反転させたい
 * Determine if a "boundary" grid is "floor mimic"
 * @param grid_type *g_ptr
 * @param feature_type *f_ptr
 * @param feature_type  *mimic_f_ptr
 * @return 移動不能であればTRUE
 */
bool boundary_floor(grid_type *g_ptr, feature_type *f_ptr, feature_type *mimic_f_ptr)
{
	bool is_boundary_floor = g_ptr->mimic > 0;
	is_boundary_floor &= permanent_wall(f_ptr);
	is_boundary_floor &= have_flag((mimic_f_ptr)->flags, FF_MOVE) || have_flag((mimic_f_ptr)->flags, FF_CAN_FLY);
	is_boundary_floor &= have_flag((mimic_f_ptr)->flags, FF_PROJECT);
	is_boundary_floor &= !have_flag((mimic_f_ptr)->flags, FF_OPEN);
	return is_boundary_floor;
}


/*!
 * @brief 該当地形のトラップがプレイヤーにとって無効かどうかを判定して返す /
 * Move player in the given direction, with the given "pickup" flag.
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param dir 移動方向ID
 * @param do_pickup 罠解除を試みながらの移動ならばTRUE
 * @param break_trap トラップ粉砕処理を行うならばTRUE
 * @return 実際に移動が行われたならばTRUEを返す。
 * @note
 * This routine should (probably) always induce energy expenditure.\n
 * @details
 * Note that moving will *always* take a turn, and will *always* hit\n
 * any monster which might be in the destination grid.  Previously,\n
 * moving into walls was "free" and did NOT hit invisible monsters.\n
 */
void move_player(player_type *creature_ptr, DIRECTION dir, bool do_pickup, bool break_trap)
{
	POSITION y = creature_ptr->y + ddy[dir];
	POSITION x = creature_ptr->x + ddx[dir];

	floor_type *floor_ptr = creature_ptr->current_floor_ptr;
	grid_type *g_ptr = &floor_ptr->grid_array[y][x];
	bool p_can_enter = player_can_enter(creature_ptr, g_ptr->feat, CEM_P_CAN_ENTER_PATTERN);
	bool stormbringer = FALSE;

	/* Exit the area */
	if (!floor_ptr->dun_level && !creature_ptr->wild_mode &&
		((x == 0) || (x == MAX_WID - 1) ||
		 (y == 0) || (y == MAX_HGT - 1)))
	{
		/* Can the player enter the grid? */
		if (g_ptr->mimic && player_can_enter(creature_ptr, g_ptr->mimic, 0))
		{
			/* Hack: move to new area */
			if ((y == 0) && (x == 0))
			{
				creature_ptr->wilderness_y--;
				creature_ptr->wilderness_x--;
				creature_ptr->oldpy = floor_ptr->height - 2;
				creature_ptr->oldpx = floor_ptr->width - 2;
				creature_ptr->ambush_flag = FALSE;
			}

			else if ((y == 0) && (x == MAX_WID - 1))
			{
				creature_ptr->wilderness_y--;
				creature_ptr->wilderness_x++;
				creature_ptr->oldpy = floor_ptr->height - 2;
				creature_ptr->oldpx = 1;
				creature_ptr->ambush_flag = FALSE;
			}

			else if ((y == MAX_HGT - 1) && (x == 0))
			{
				creature_ptr->wilderness_y++;
				creature_ptr->wilderness_x--;
				creature_ptr->oldpy = 1;
				creature_ptr->oldpx = floor_ptr->width - 2;
				creature_ptr->ambush_flag = FALSE;
			}

			else if ((y == MAX_HGT - 1) && (x == MAX_WID - 1))
			{
				creature_ptr->wilderness_y++;
				creature_ptr->wilderness_x++;
				creature_ptr->oldpy = 1;
				creature_ptr->oldpx = 1;
				creature_ptr->ambush_flag = FALSE;
			}

			else if (y == 0)
			{
				creature_ptr->wilderness_y--;
				creature_ptr->oldpy = floor_ptr->height - 2;
				creature_ptr->oldpx = x;
				creature_ptr->ambush_flag = FALSE;
			}

			else if (y == MAX_HGT - 1)
			{
				creature_ptr->wilderness_y++;
				creature_ptr->oldpy = 1;
				creature_ptr->oldpx = x;
				creature_ptr->ambush_flag = FALSE;
			}

			else if (x == 0)
			{
				creature_ptr->wilderness_x--;
				creature_ptr->oldpx = floor_ptr->width - 2;
				creature_ptr->oldpy = y;
				creature_ptr->ambush_flag = FALSE;
			}

			else if (x == MAX_WID - 1)
			{
				creature_ptr->wilderness_x++;
				creature_ptr->oldpx = 1;
				creature_ptr->oldpy = y;
				creature_ptr->ambush_flag = FALSE;
			}

			creature_ptr->leaving = TRUE;
			take_turn(creature_ptr, 100);

			return;
		}

		/* "Blocked" message appears later */
		/* oktomove = FALSE; */
		p_can_enter = FALSE;
	}

	monster_type *m_ptr;
	m_ptr = &floor_ptr->m_list[g_ptr->m_idx];

	if (creature_ptr->inventory_list[INVEN_RARM].name1 == ART_STORMBRINGER) stormbringer = TRUE;
	if (creature_ptr->inventory_list[INVEN_LARM].name1 == ART_STORMBRINGER) stormbringer = TRUE;

	/* Player can not walk through "walls"... */
	/* unless in Shadow Form */
	feature_type *f_ptr = &f_info[g_ptr->feat];
	bool p_can_kill_walls = creature_ptr->kill_wall && have_flag(f_ptr->flags, FF_HURT_DISI) &&
		(!p_can_enter || !have_flag(f_ptr->flags, FF_LOS)) &&
		!have_flag(f_ptr->flags, FF_PERMANENT);

	/* Hack -- attack monsters */
	GAME_TEXT m_name[MAX_NLEN];
	bool can_move = TRUE;
	bool do_past = FALSE;
	if (g_ptr->m_idx && (m_ptr->ml || p_can_enter || p_can_kill_walls))
	{
		monster_race *r_ptr = &r_info[m_ptr->r_idx];

		/* Attack -- only if we can see it OR it is not in a wall */
		if (!is_hostile(m_ptr) &&
		    !(creature_ptr->confused || creature_ptr->image || !m_ptr->ml || creature_ptr->stun ||
		    ((creature_ptr->muta2 & MUT2_BERS_RAGE) && creature_ptr->shero)) &&
		    pattern_seq(creature_ptr, creature_ptr->y, creature_ptr->x, y, x) && (p_can_enter || p_can_kill_walls))
		{
			/* Disturb the monster */
			(void)set_monster_csleep(creature_ptr, g_ptr->m_idx, 0);

			monster_desc(creature_ptr, m_name, m_ptr, 0);

			if (m_ptr->ml)
			{
				/* Auto-Recall if possible and visible */
				if (!creature_ptr->image) monster_race_track(creature_ptr, m_ptr->ap_r_idx);
				health_track(creature_ptr, g_ptr->m_idx);
			}

			/* displace? */
			if ((stormbringer && (randint1(1000) > 666)) || (creature_ptr->pclass == CLASS_BERSERKER))
			{
				py_attack(creature_ptr, y, x, 0);
				can_move = FALSE;
			}
			else if (monster_can_cross_terrain(creature_ptr, floor_ptr->grid_array[creature_ptr->y][creature_ptr->x].feat, r_ptr, 0))
			{
				do_past = TRUE;
			}
			else
			{
				msg_format(_("%^sが邪魔だ！", "%^s is in your way!"), m_name);
				free_turn(creature_ptr);
				can_move = FALSE;
			}

			/* now continue on to 'movement' */
		}
		else
		{
			py_attack(creature_ptr, y, x, 0);
			can_move = FALSE;
		}
	}

	monster_type *riding_m_ptr = &floor_ptr->m_list[creature_ptr->riding];
	monster_race *riding_r_ptr = &r_info[creature_ptr->riding ? riding_m_ptr->r_idx : 0];
	if (can_move && creature_ptr->riding)
	{
		if (riding_r_ptr->flags1 & RF1_NEVER_MOVE)
		{
			msg_print(_("動けない！", "Can't move!"));
			free_turn(creature_ptr);
			can_move = FALSE;
			disturb(creature_ptr, FALSE, TRUE);
		}
		else if (MON_MONFEAR(riding_m_ptr))
		{
			GAME_TEXT steed_name[MAX_NLEN];
			monster_desc(creature_ptr, steed_name, riding_m_ptr, 0);
			msg_format(_("%sが恐怖していて制御できない。", "%^s is too scared to control."), steed_name);
			can_move = FALSE;
			disturb(creature_ptr, FALSE, TRUE);
		}
		else if (creature_ptr->riding_ryoute)
		{
			can_move = FALSE;
			disturb(creature_ptr, FALSE, TRUE);
		}
		else if (have_flag(f_ptr->flags, FF_CAN_FLY) && (riding_r_ptr->flags7 & RF7_CAN_FLY))
		{
			/* Allow moving */
		}
		else if (have_flag(f_ptr->flags, FF_CAN_SWIM) && (riding_r_ptr->flags7 & RF7_CAN_SWIM))
		{
			/* Allow moving */
		}
		else if (have_flag(f_ptr->flags, FF_WATER) &&
			!(riding_r_ptr->flags7 & RF7_AQUATIC) &&
			(have_flag(f_ptr->flags, FF_DEEP) || (riding_r_ptr->flags2 & RF2_AURA_FIRE)))
		{
			msg_format(_("%sの上に行けない。", "Can't swim."), f_name + f_info[get_feat_mimic(g_ptr)].name);
			free_turn(creature_ptr);
			can_move = FALSE;
			disturb(creature_ptr, FALSE, TRUE);
		}
		else if (!have_flag(f_ptr->flags, FF_WATER) && (riding_r_ptr->flags7 & RF7_AQUATIC))
		{
			msg_format(_("%sから上がれない。", "Can't land."), f_name + f_info[get_feat_mimic(&floor_ptr->grid_array[creature_ptr->y][creature_ptr->x])].name);
			free_turn(creature_ptr);
			can_move = FALSE;
			disturb(creature_ptr, FALSE, TRUE);
		}
		else if (have_flag(f_ptr->flags, FF_LAVA) && !(riding_r_ptr->flagsr & RFR_EFF_IM_FIRE_MASK))
		{
			msg_format(_("%sの上に行けない。", "Too hot to go through."), f_name + f_info[get_feat_mimic(g_ptr)].name);
			free_turn(creature_ptr);
			can_move = FALSE;
			disturb(creature_ptr, FALSE, TRUE);
		}

		if (can_move && MON_STUNNED(riding_m_ptr) && one_in_(2))
		{
			GAME_TEXT steed_name[MAX_NLEN];
			monster_desc(creature_ptr, steed_name, riding_m_ptr, 0);
			msg_format(_("%sが朦朧としていてうまく動けない！", "You cannot control stunned %s!"), steed_name);
			can_move = FALSE;
			disturb(creature_ptr, FALSE, TRUE);
		}
	}

	if (!can_move)
	{
	}

	else if (!have_flag(f_ptr->flags, FF_MOVE) && have_flag(f_ptr->flags, FF_CAN_FLY) && !creature_ptr->levitation)
	{
		msg_format(_("空を飛ばないと%sの上には行けない。", "You need to fly to go through the %s."), f_name + f_info[get_feat_mimic(g_ptr)].name);
		free_turn(creature_ptr);
		creature_ptr->running = 0;
		can_move = FALSE;
	}

	/*
	 * Player can move through trees and
	 * has effective -10 speed
	 * Rangers can move without penality
	 */
	else if (have_flag(f_ptr->flags, FF_TREE) && !p_can_kill_walls)
	{
		if ((creature_ptr->pclass != CLASS_RANGER) && !creature_ptr->levitation && (!creature_ptr->riding || !(riding_r_ptr->flags8 & RF8_WILD_WOOD))) creature_ptr->energy_use *= 2;
	}

	/* Disarm a visible trap */
	else if ((do_pickup != easy_disarm) && have_flag(f_ptr->flags, FF_DISARM) && !g_ptr->mimic)
	{
		if (!trap_can_be_ignored(creature_ptr, g_ptr->feat))
		{
			(void)exe_disarm(creature_ptr, y, x, dir);
			return;
		}
	}

	/* Player can not walk through "walls" unless in wraith form...*/
	else if (!p_can_enter && !p_can_kill_walls)
	{
		/* Feature code (applying "mimic" field) */
		FEAT_IDX feat = get_feat_mimic(g_ptr);
		feature_type *mimic_f_ptr = &f_info[feat];
		concptr name = f_name + mimic_f_ptr->name;

		can_move = FALSE;

		/* Notice things in the dark */
		if (!(g_ptr->info & CAVE_MARK) && !player_can_see_bold(creature_ptr, y, x))
		{
			/* Boundary floor mimic */
			if (boundary_floor(g_ptr, f_ptr, mimic_f_ptr))
			{
				msg_print(_("それ以上先には進めないようだ。", "You feel you cannot go any more."));
			}

			/* Wall (or secret door) */
			else
			{
#ifdef JP
				msg_format("%sが行く手をはばんでいるようだ。", name);
#else
				msg_format("You feel %s %s blocking your way.",
					is_a_vowel(name[0]) ? "an" : "a", name);
#endif

				g_ptr->info |= (CAVE_MARK);
				lite_spot(creature_ptr, y, x);
			}
		}

		/* Notice things */
		else
		{
			/* Boundary floor mimic */
			if (boundary_floor(g_ptr, f_ptr, mimic_f_ptr))
			{
				msg_print(_("それ以上先には進めない。", "You cannot go any more."));
				if (!(creature_ptr->confused || creature_ptr->stun || creature_ptr->image))
					free_turn(creature_ptr);
			}

			/* Wall (or secret door) */
			else
			{
				/* Closed doors */
				if (easy_open && is_closed_door(creature_ptr, feat) && easy_open_door(creature_ptr, y, x)) return;

#ifdef JP
				msg_format("%sが行く手をはばんでいる。", name);
#else
				msg_format("There is %s %s blocking your way.",
					is_a_vowel(name[0]) ? "an" : "a", name);
#endif

				/*
				 * Well, it makes sense that you lose time bumping into
				 * a wall _if_ you are confused, stunned or blind; but
				 * typing mistakes should not cost you a turn...
				 */
				if (!(creature_ptr->confused || creature_ptr->stun || creature_ptr->image))
					free_turn(creature_ptr);
			}
		}

		disturb(creature_ptr, FALSE, TRUE);

		if (!boundary_floor(g_ptr, f_ptr, mimic_f_ptr)) sound(SOUND_HITWALL);
	}

	/* Normal movement */
	if (can_move && !pattern_seq(creature_ptr, creature_ptr->y, creature_ptr->x, y, x))
	{
		if (!(creature_ptr->confused || creature_ptr->stun || creature_ptr->image))
		{
			free_turn(creature_ptr);
		}

		/* To avoid a loop with running */
		disturb(creature_ptr, FALSE, TRUE);

		can_move = FALSE;
	}

	if (!can_move) return;

	if (creature_ptr->warning)
	{
		if (!process_warning(creature_ptr, x, y))
		{
			creature_ptr->energy_use = 25;
			return;
		}
	}

	if (do_past)
	{
		msg_format(_("%sを押し退けた。", "You push past %s."), m_name);
	}

	/* Change oldpx and oldpy to place the player well when going back to big mode */
	if (creature_ptr->wild_mode)
	{
		if (ddy[dir] > 0)  creature_ptr->oldpy = 1;
		if (ddy[dir] < 0)  creature_ptr->oldpy = MAX_HGT - 2;
		if (ddy[dir] == 0) creature_ptr->oldpy = MAX_HGT / 2;
		if (ddx[dir] > 0)  creature_ptr->oldpx = 1;
		if (ddx[dir] < 0)  creature_ptr->oldpx = MAX_WID - 2;
		if (ddx[dir] == 0) creature_ptr->oldpx = MAX_WID / 2;
	}

	if (p_can_kill_walls)
	{
		cave_alter_feat(creature_ptr, y, x, FF_HURT_DISI);

		/* Update some things -- similar to GF_KILL_WALL */
		creature_ptr->update |= (PU_FLOW);
	}

	u32b mpe_mode = MPE_ENERGY_USE;
	if (do_pickup != always_pickup) mpe_mode |= MPE_DO_PICKUP;
	if (break_trap) mpe_mode |= MPE_BREAK_TRAP;

	(void)move_player_effect(creature_ptr, y, x, mpe_mode);
}


static bool ignore_avoid_run;

/*!
 * @brief ダッシュ移動処理中、移動先のマスが既知の壁かどうかを判定する /
 * Hack -- Check for a "known wall" (see below)
 * @param creature_ptr	プレーヤーへの参照ポインタ
 * @param dir 想定する移動方向ID
 * @param y 移動元のY座標
 * @param x 移動元のX座標
 * @return 移動先が既知の壁ならばTRUE
 */
static bool see_wall(player_type *creature_ptr, DIRECTION dir, POSITION y, POSITION x)
{
	/* Get the new location */
	y += ddy[dir];
	x += ddx[dir];

	/* Illegal grids are not known walls */
	floor_type *floor_ptr = creature_ptr->current_floor_ptr;
	if (!in_bounds2(floor_ptr, y, x)) return FALSE;

	grid_type *g_ptr;
	g_ptr = &floor_ptr->grid_array[y][x];

	/* Must be known to the player */
	if (!(g_ptr->info & CAVE_MARK)) return FALSE;

	/* Feature code (applying "mimic" field) */
	s16b feat = get_feat_mimic(g_ptr);
	feature_type *f_ptr = &f_info[feat];

	/* Wall grids are known walls */
	if (!player_can_enter(creature_ptr, feat, 0))
		return !have_flag(f_ptr->flags, FF_DOOR);

	/* Don't run on a tree unless explicitly requested */
	if (have_flag(f_ptr->flags, FF_AVOID_RUN) && !ignore_avoid_run)
		return TRUE;

	/* Don't run in a wall */
	if (!have_flag(f_ptr->flags, FF_MOVE) && !have_flag(f_ptr->flags, FF_CAN_FLY))
		return !have_flag(f_ptr->flags, FF_DOOR);

	return FALSE;
}


/*!
 * @brief ダッシュ移動処理中、移動先のマスか未知の地形かどうかを判定する /
 * Hack -- Check for an "unknown corner" (see below)
 * @param creature_ptr	プレーヤーへの参照ポインタ
 * @param dir 想定する移動方向ID
 * @param y 移動元のY座標
 * @param x 移動元のX座標
 * @return 移動先が未知の地形ならばTRUE
 */
static bool see_nothing(player_type *creature_ptr, DIRECTION dir, POSITION y, POSITION x)
{
	/* Get the new location */
	y += ddy[dir];
	x += ddx[dir];

	/* Illegal grids are unknown */
	floor_type *floor_ptr = creature_ptr->current_floor_ptr;
	if (!in_bounds2(floor_ptr, y, x)) return TRUE;

	/* Memorized grids are always known */
	if (floor_ptr->grid_array[y][x].info & (CAVE_MARK)) return FALSE;

	/* Viewable door/wall grids are known */
	if (player_can_see_bold(creature_ptr, y, x)) return FALSE;

	/* Default */
	return TRUE;
}


/*
 * Hack -- allow quick "cycling" through the legal directions
 */
static byte cycle[] = { 1, 2, 3, 6, 9, 8, 7, 4, 1, 2, 3, 6, 9, 8, 7, 4, 1 };

/*
 * Hack -- map each direction into the "middle" of the "cycle[]" array
 */
static byte chome[] = { 0, 8, 9, 10, 7, 0, 11, 6, 5, 4 };

/*
 * The direction we are running
 */
static DIRECTION find_current;

/*
 * The direction we came from
 */
static DIRECTION find_prevdir;

/*
 * We are looking for open area
 */
static bool find_openarea;

/*
 * We are looking for a break
 */
static bool find_breakright;
static bool find_breakleft;

/*!
 * @brief ダッシュ処理の導入 /
 * Initialize the running algorithm for a new direction.
 * @param creature_ptr	プレーヤーへの参照ポインタ
 * @param dir 導入の移動先
 * @details
 * Diagonal Corridor -- allow diaginal entry into corridors.\n
 *\n
 * Blunt Corridor -- If there is a wall two spaces ahead and\n
 * we seem to be in a corridor, then force a turn into the side\n
 * corridor, must be moving straight into a corridor here. ???\n
 *\n
 * Diagonal Corridor    Blunt Corridor (?)\n
 *       \# \#                  \#\n
 *       \#x\#                  \@x\#\n
 *       \@\@p.                  p\n
 */
static void run_init(player_type *creature_ptr, DIRECTION dir)
{
	/* Save the direction */
	find_current = dir;

	/* Assume running straight */
	find_prevdir = dir;

	/* Assume looking for open area */
	find_openarea = TRUE;

	/* Assume not looking for breaks */
	find_breakright = find_breakleft = FALSE;

	/* Assume no nearby walls */
	bool deepleft = FALSE;
	bool deepright = FALSE;
	bool shortright = FALSE;
	bool shortleft = FALSE;

	creature_ptr->run_py = creature_ptr->y;
	creature_ptr->run_px = creature_ptr->x;

	/* Find the destination grid */
	int row = creature_ptr->y + ddy[dir];
	int col = creature_ptr->x + ddx[dir];

	ignore_avoid_run = cave_have_flag_bold(creature_ptr->current_floor_ptr, row, col, FF_AVOID_RUN);

	/* Extract cycle index */
	int i = chome[dir];

	/* Check for walls */
	if (see_wall(creature_ptr, cycle[i+1], creature_ptr->y, creature_ptr->x))
	{
		find_breakleft = TRUE;
		shortleft = TRUE;
	}
	else if (see_wall(creature_ptr, cycle[i+1], row, col))
	{
		find_breakleft = TRUE;
		deepleft = TRUE;
	}

	/* Check for walls */
	if (see_wall(creature_ptr, cycle[i-1], creature_ptr->y, creature_ptr->x))
	{
		find_breakright = TRUE;
		shortright = TRUE;
	}
	else if (see_wall(creature_ptr, cycle[i-1], row, col))
	{
		find_breakright = TRUE;
		deepright = TRUE;
	}

	/* Looking for a break */
	if (!find_breakleft || !find_breakright) return;

	/* Not looking for open area */
	find_openarea = FALSE;

	/* Hack -- allow angled corridor entry */
	if (dir & 0x01)
	{
		if (deepleft && !deepright)
		{
			find_prevdir = cycle[i - 1];
		}
		else if (deepright && !deepleft)
		{
			find_prevdir = cycle[i + 1];
		}

		return;
	}

	/* Hack -- allow blunt corridor entry */
	if (!see_wall(creature_ptr, cycle[i], row, col)) return;

	if (shortleft && !shortright)
	{
		find_prevdir = cycle[i - 2];
	}
	else if (shortright && !shortleft)
	{
		find_prevdir = cycle[i + 2];
	}
}


/*!
 * @brief ダッシュ移動が継続できるかどうかの判定 /
 * Update the current "run" path
 * @param creature_ptr	プレーヤーへの参照ポインタ
 * @return 立ち止まるべき条件が満たされたらTRUE
 * ダッシュ移動が継続できるならばTRUEを返す。
 * Return TRUE if the running should be stopped
 */
static bool run_test(player_type *creature_ptr)
{
	DIRECTION prev_dir = find_prevdir;

	/* Range of newly adjacent grids */
	int max = (prev_dir & 0x01) + 1;

	/* break run when leaving trap detected region */
	floor_type *floor_ptr = creature_ptr->current_floor_ptr;
	if ((disturb_trap_detect || alert_trap_detect)
	    && creature_ptr->dtrap && !(floor_ptr->grid_array[creature_ptr->y][creature_ptr->x].info & CAVE_IN_DETECT))
	{
		/* No duplicate warning */
		creature_ptr->dtrap = FALSE;

		/* You are just on the edge */
		if (!(floor_ptr->grid_array[creature_ptr->y][creature_ptr->x].info & CAVE_UNSAFE))
		{
			if (alert_trap_detect)
			{
				msg_print(_("* 注意:この先はトラップの感知範囲外です！ *", "*Leaving trap detect region!*"));
			}

			if (disturb_trap_detect)
			{
				/* Break Run */
				return TRUE;
			}
		}
	}

	/* Look at every newly adjacent square. */
	DIRECTION check_dir = 0;
	int option = 0, option2 = 0;
	for (int i = -max; i <= max; i++)
	{
		OBJECT_IDX this_o_idx, next_o_idx = 0;

		/* New direction */
		DIRECTION new_dir = cycle[chome[prev_dir] + i];

		/* New location */
		int row = creature_ptr->y + ddy[new_dir];
		int col = creature_ptr->x + ddx[new_dir];

		grid_type *g_ptr;
		g_ptr = &floor_ptr->grid_array[row][col];

		/* Feature code (applying "mimic" field) */
		FEAT_IDX feat = get_feat_mimic(g_ptr);
		feature_type *f_ptr;
		f_ptr = &f_info[feat];

		/* Visible monsters abort running */
		if (g_ptr->m_idx)
		{
			monster_type *m_ptr = &floor_ptr->m_list[g_ptr->m_idx];

			/* Visible monster */
			if (m_ptr->ml) return TRUE;
		}

		/* Visible objects abort running */
		for (this_o_idx = g_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
		{
			object_type *o_ptr;
			o_ptr = &floor_ptr->o_list[this_o_idx];
			next_o_idx = o_ptr->next_o_idx;

			/* Visible object */
			if (o_ptr->marked & OM_FOUND) return TRUE;
		}

		/* Assume unknown */
		bool inv = TRUE;

		/* Check memorized grids */
		if (g_ptr->info & (CAVE_MARK))
		{
			bool notice = have_flag(f_ptr->flags, FF_NOTICE);

			if (notice && have_flag(f_ptr->flags, FF_MOVE))
			{
				/* Open doors */
				if (find_ignore_doors && have_flag(f_ptr->flags, FF_DOOR) && have_flag(f_ptr->flags, FF_CLOSE))
				{
					/* Option -- ignore */
					notice = FALSE;
				}

				/* Stairs */
				else if (find_ignore_stairs && have_flag(f_ptr->flags, FF_STAIRS))
				{
					/* Option -- ignore */
					notice = FALSE;
				}

				/* Lava */
				else if (have_flag(f_ptr->flags, FF_LAVA) && (creature_ptr->immune_fire || IS_INVULN(creature_ptr)))
				{
					/* Ignore */
					notice = FALSE;
				}

				/* Deep water */
				else if (have_flag(f_ptr->flags, FF_WATER) && have_flag(f_ptr->flags, FF_DEEP) &&
				         (creature_ptr->levitation || creature_ptr->can_swim || (creature_ptr->total_weight <= weight_limit(creature_ptr))))
				{
					/* Ignore */
					notice = FALSE;
				}
			}

			/* Interesting feature */
			if (notice) return TRUE;

			/* The grid is "visible" */
			inv = FALSE;
		}

		/* Analyze unknown grids and floors considering mimic */
		if (!inv && see_wall(creature_ptr, 0, row, col))
		{
			if (find_openarea)
			{
				if (i < 0)
				{
					/* Break to the right */
					find_breakright = TRUE;
				}

				else if (i > 0)
				{
					/* Break to the left */
					find_breakleft = TRUE;
				}
			}

			continue;
		}

		/* Looking for open area */
		if (find_openarea) continue;

		/* The first new direction. */
		if (!option)
		{
			option = new_dir;
			continue;
		}

		/* Three new directions. Stop running. */
		if (option2)
		{
			return TRUE;
		}

		/* Two non-adjacent new directions.  Stop running. */
		if (option != cycle[chome[prev_dir] + i - 1])
		{
			return TRUE;
		}

		/* Two new (adjacent) directions (case 1) */
		if (new_dir & 0x01)
		{
			check_dir = cycle[chome[prev_dir] + i - 2];
			option2 = new_dir;
			continue;
		}

		/* Two new (adjacent) directions (case 2) */
		check_dir = cycle[chome[prev_dir] + i + 1];
		option2 = option;
		option = new_dir;
	}

	/* Looking for open area */
	if (find_openarea)
	{
		/* Hack -- look again */
		for (int i = -max; i < 0; i++)
		{
			/* Unknown grid or non-wall */
			if (!see_wall(creature_ptr, cycle[chome[prev_dir] + i], creature_ptr->y, creature_ptr->x))
			{
				/* Looking to break right */
				if (find_breakright)
				{
					return TRUE;
				}
			}

			/* Obstacle */
			else
			{
				/* Looking to break left */
				if (find_breakleft)
				{
					return TRUE;
				}
			}
		}

		/* Hack -- look again */
		for (int i = max; i > 0; i--)
		{
			/* Unknown grid or non-wall */
			if (!see_wall(creature_ptr, cycle[chome[prev_dir] + i], creature_ptr->y, creature_ptr->x))
			{
				/* Looking to break left */
				if (find_breakleft)
				{
					return TRUE;
				}
			}

			/* Obstacle */
			else
			{
				/* Looking to break right */
				if (find_breakright)
				{
					return TRUE;
				}
			}
		}

		return see_wall(creature_ptr, find_current, creature_ptr->y, creature_ptr->x);
	}

	/* No options */
	if (!option)
	{
		return TRUE;
	}

	/* One option */
	if (!option2)
	{
		find_current = option;
		find_prevdir = option;
		return see_wall(creature_ptr, find_current, creature_ptr->y, creature_ptr->x);
	}

	/* Two options, examining corners */
	else if (!find_cut)
	{
		find_current = option;
		find_prevdir = option2;
		return see_wall(creature_ptr, find_current, creature_ptr->y, creature_ptr->x);
	}

	/* Two options, pick one */
	/* Get next location */
	int row = creature_ptr->y + ddy[option];
	int col = creature_ptr->x + ddx[option];

	/* Don't see that it is closed off. */
	/* This could be a potential corner or an intersection. */
	if (!see_wall(creature_ptr, option, row, col) ||
		!see_wall(creature_ptr, check_dir, row, col))
	{
		/* Can not see anything ahead and in the direction we */
		/* are turning, assume that it is a potential corner. */
		if (see_nothing(creature_ptr, option, row, col) &&
			see_nothing(creature_ptr, option2, row, col))
		{
			find_current = option;
			find_prevdir = option2;
			return see_wall(creature_ptr, find_current, creature_ptr->y, creature_ptr->x);
		}

		/* STOP: we are next to an intersection or a room */
		return TRUE;
	}

	/* This corner is seen to be enclosed; we cut the corner. */
	if (find_cut)
	{
		find_current = option2;
		find_prevdir = option2;
		return see_wall(creature_ptr, find_current, creature_ptr->y, creature_ptr->x);
	}

	/* This corner is seen to be enclosed, and we */
	/* deliberately go the long way. */
	find_current = option;
	find_prevdir = option2;
	return see_wall(creature_ptr, find_current, creature_ptr->y, creature_ptr->x);
}


/*!
 * @brief 継続的なダッシュ処理 /
 * Take one step along the current "run" path
 * @param creature_ptr	プレーヤーへの参照ポインタ
 * @param dir 移動を試みる方向ID
 * @return なし
 */
void run_step(player_type *creature_ptr, DIRECTION dir)
{
	/* Start running */
	if (dir)
	{
		/* Ignore AVOID_RUN on a first step */
		ignore_avoid_run = TRUE;

		/* Hack -- do not start silly run */
		if (see_wall(creature_ptr, dir, creature_ptr->y, creature_ptr->x))
		{
			sound(SOUND_HITWALL);

			msg_print(_("その方向には走れません。", "You cannot run in that direction."));

			disturb(creature_ptr, FALSE, FALSE);

			return;
		}

		run_init(creature_ptr, dir);
	}

	/* Keep running */
	else
	{
		/* Update run */
		if (run_test(creature_ptr))
		{
			disturb(creature_ptr, FALSE, FALSE);

			return;
		}
	}

	/* Decrease the run counter */
	if (--creature_ptr->running <= 0) return;

	/* Take time */
	take_turn(creature_ptr, 100);

	/* Move the player, using the "pickup" flag */
	move_player(creature_ptr, find_current, FALSE, FALSE);

	if (player_bold(creature_ptr, creature_ptr->run_py, creature_ptr->run_px))
	{
		creature_ptr->run_py = 0;
		creature_ptr->run_px = 0;
		disturb(creature_ptr, FALSE, FALSE);
	}
}


/*!
 * @brief トラベル機能の判定処理 /
 * Test for traveling
 * @param creature_ptr	プレーヤーへの参照ポインタ
 * @param prev_dir 前回移動を行った元の方角ID
 * @return 次の方向
 */
static DIRECTION travel_test(player_type *creature_ptr, DIRECTION prev_dir)
{
	/* Cannot travel when blind */
	if (creature_ptr->blind || no_lite(creature_ptr))
	{
		msg_print(_("目が見えない！", "You cannot see!"));
		return 0;
	}

	/* break run when leaving trap detected region */
	floor_type *floor_ptr = creature_ptr->current_floor_ptr;
	if ((disturb_trap_detect || alert_trap_detect)
	    && creature_ptr->dtrap && !(floor_ptr->grid_array[creature_ptr->y][creature_ptr->x].info & CAVE_IN_DETECT))
	{
		/* No duplicate warning */
		creature_ptr->dtrap = FALSE;

		/* You are just on the edge */
		if (!(floor_ptr->grid_array[creature_ptr->y][creature_ptr->x].info & CAVE_UNSAFE))
		{
			if (alert_trap_detect)
			{
				msg_print(_("* 注意:この先はトラップの感知範囲外です！ *", "*Leaving trap detect region!*"));
			}

			if (disturb_trap_detect)
			{
				/* Break Run */
				return 0;
			}
		}
	}

	/* Range of newly adjacent grids */
	int max = (prev_dir & 0x01) + 1;

	/* Look at every newly adjacent square. */
	const grid_type *g_ptr;
	for (int i = -max; i <= max; i++)
	{
		/* New direction */
		DIRECTION dir = cycle[chome[prev_dir] + i];

		/* New location */
		POSITION row = creature_ptr->y + ddy[dir];
		POSITION col = creature_ptr->x + ddx[dir];

		g_ptr = &floor_ptr->grid_array[row][col];

		/* Visible monsters abort running */
		if (g_ptr->m_idx)
		{
			monster_type *m_ptr = &floor_ptr->m_list[g_ptr->m_idx];

			/* Visible monster */
			if (m_ptr->ml) return 0;
		}

	}

	/* Travel cost of current grid */
	int cost = travel.cost[creature_ptr->y][creature_ptr->x];

	/* Determine travel direction */
	DIRECTION new_dir = 0;
	for (int i = 0; i < 8; ++ i)
	{
		int dir_cost = travel.cost[creature_ptr->y+ddy_ddd[i]][creature_ptr->x+ddx_ddd[i]];

		if (dir_cost < cost)
		{
			new_dir = ddd[i];
			cost = dir_cost;
		}
	}

	if (!new_dir) return 0;

	/* Access newly move grid */
	g_ptr = &floor_ptr->grid_array[creature_ptr->y+ddy[new_dir]][creature_ptr->x+ddx[new_dir]];

	/* Close door abort traveling */
	if (!easy_open && is_closed_door(creature_ptr, g_ptr->feat)) return 0;

	/* Visible and unignorable trap abort tarveling */
	if (!g_ptr->mimic && !trap_can_be_ignored(creature_ptr, g_ptr->feat)) return 0;

	/* Move new grid */
	return new_dir;
}


/*!
 * @brief トラベル機能の実装 /
 * Travel command
 * @param creature_ptr	プレーヤーへの参照ポインタ
 * @return なし
 */
void travel_step(player_type *creature_ptr)
{
	/* Get travel direction */
	travel.dir = travel_test(creature_ptr, travel.dir);

	if (!travel.dir)
	{
		if (travel.run == 255)
		{
			msg_print(_("道筋が見つかりません！", "No route is found!"));
			travel.y = travel.x = 0;
		}

		disturb(creature_ptr, FALSE, TRUE);
		return;
	}

	take_turn(creature_ptr, 100);

	move_player(creature_ptr, travel.dir, always_pickup, FALSE);

	if ((creature_ptr->y == travel.y) && (creature_ptr->x == travel.x))
	{
		travel.run = 0;
		travel.y = travel.x = 0;
	}
	else if (travel.run > 0)
	{
		travel.run--;
	}

	/* Travel Delay */
	Term_xtra(TERM_XTRA_DELAY, delay_factor);
}


#define TRAVEL_UNABLE 9999

static int flow_head = 0;
static int flow_tail = 0;
static POSITION temp2_x[MAX_SHORT];
static POSITION temp2_y[MAX_SHORT];

/*!
 * @brief トラベル処理の記憶配列を初期化する Hack: forget the "flow" information
 * @param creature_ptr	プレーヤーへの参照ポインタ
 * @return なし
 */
void forget_travel_flow(floor_type *floor_ptr)
{
	/* Check the entire dungeon / Forget the old data */
	for (POSITION y = 0; y < floor_ptr->height; y++)
	{
		for (POSITION x = 0; x < floor_ptr->width; x++)
		{
			travel.cost[y][x] = MAX_SHORT;
		}
	}

	travel.y = travel.x = 0;
}


/*!
 * @brief トラベル処理中に地形に応じた移動コスト基準を返す
 * @param creature_ptr	プレーヤーへの参照ポインタ
 * @param y 該当地点のY座標
 * @param x 該当地点のX座標
 * @return コスト値
 */
static int travel_flow_cost(player_type *creature_ptr, POSITION y, POSITION x)
{
	/* Avoid obstacles (ex. trees) */
	int cost = 1;
	feature_type *f_ptr = &f_info[creature_ptr->current_floor_ptr->grid_array[y][x].feat];
	if (have_flag(f_ptr->flags, FF_AVOID_RUN)) cost += 1;

	/* Water */
	if (have_flag(f_ptr->flags, FF_WATER))
	{
		if (have_flag(f_ptr->flags, FF_DEEP) && !creature_ptr->levitation) cost += 5;
	}

	/* Lava */
	if (have_flag(f_ptr->flags, FF_LAVA))
	{
		int lava = 2;
		if (!creature_ptr->resist_fire) lava *= 2;
		if (!creature_ptr->levitation) lava *= 2;
		if (have_flag(f_ptr->flags, FF_DEEP)) lava *= 2;

		cost += lava;
	}

	/* Detected traps and doors */
	if (creature_ptr->current_floor_ptr->grid_array[y][x].info & (CAVE_MARK))
	{
		if (have_flag(f_ptr->flags, FF_DOOR)) cost += 1;
		if (have_flag(f_ptr->flags, FF_TRAP)) cost += 10;
	}

	return cost;
}


/*!
 * @brief トラベル処理の到達地点までの行程を得る処理のサブルーチン
 * @param creature_ptr	プレーヤーへの参照ポインタ
 * @param y 目標地点のY座標
 * @param x 目標地点のX座標
 * @param n 現在のコスト
 * @param wall プレイヤーが壁の中にいるならばTRUE
 * @return なし
 */
static void travel_flow_aux(player_type *creature_ptr, POSITION y, POSITION x, int n, bool wall)
{
	/* Ignore out of bounds */
	floor_type *floor_ptr = creature_ptr->current_floor_ptr;
	grid_type *g_ptr = &floor_ptr->grid_array[y][x];
	feature_type *f_ptr = &f_info[g_ptr->feat];
	if (!in_bounds(floor_ptr, y, x)) return;

	/* Ignore unknown grid except in wilderness */
	if (floor_ptr->dun_level > 0 && !(g_ptr->info & CAVE_KNOWN)) return;

	/* Ignore "walls" and "rubble" (include "secret doors") */
	int add_cost = 1;
	int from_wall = (n / TRAVEL_UNABLE);
	if (have_flag(f_ptr->flags, FF_WALL) ||
		have_flag(f_ptr->flags, FF_CAN_DIG) ||
		(have_flag(f_ptr->flags, FF_DOOR) && floor_ptr->grid_array[y][x].mimic) ||
		(!have_flag(f_ptr->flags, FF_MOVE) && have_flag(f_ptr->flags, FF_CAN_FLY) && !creature_ptr->levitation))
	{
		if (!wall || !from_wall) return;
		add_cost += TRAVEL_UNABLE;
	}
	else
	{
		add_cost = travel_flow_cost(creature_ptr, y, x);
	}

	int base_cost = (n % TRAVEL_UNABLE);
	int cost = base_cost + add_cost;

	/* Ignore lower cost entries */
	if (travel.cost[y][x] <= cost) return;

	/* Save the flow cost */
	travel.cost[y][x] = cost;

	/* Enqueue that entry */
	int old_head = flow_head;
	temp2_y[flow_head] = y;
	temp2_x[flow_head] = x;

	/* Advance the queue */
	if (++flow_head == MAX_SHORT) flow_head = 0;

	/* Hack -- notice overflow by forgetting new entry */
	if (flow_head == flow_tail) flow_head = old_head;
}


/*!
 * @brief トラベル処理の到達地点までの行程を得る処理のメインルーチン
 * @param creature_ptr	プレーヤーへの参照ポインタ
 * @param ty 目標地点のY座標
 * @param tx 目標地点のX座標
 * @return なし
 */
static void travel_flow(player_type *creature_ptr, POSITION ty, POSITION tx)
{
	flow_head = flow_tail = 0;

	/* is player in the wall? */
	bool wall = FALSE;
	feature_type *f_ptr = &f_info[creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x].feat];
	if (!have_flag(f_ptr->flags, FF_MOVE)) wall = TRUE;

	/* Start at the target grid */
	travel_flow_aux(creature_ptr, ty, tx, 0, wall);

	/* Now process the queue */
	POSITION x, y;
	while (flow_head != flow_tail)
	{
		/* Extract the next entry */
		y = temp2_y[flow_tail];
		x = temp2_x[flow_tail];

		/* Forget that entry */
		if (++flow_tail == MAX_SHORT) flow_tail = 0;

		/* Add the "children" */
		for (DIRECTION d = 0; d < 8; d++)
		{
			/* Add that child if "legal" */
			travel_flow_aux(creature_ptr, y + ddy_ddd[d], x + ddx_ddd[d], travel.cost[y][x], wall);
		}
	}

	flow_head = flow_tail = 0;
}


/*!
 * @brief トラベル処理のメインルーチン
 * @return なし
 */
void do_cmd_travel(player_type *creature_ptr)
{
	POSITION x, y;
	if (travel.x != 0 && travel.y != 0 &&
		get_check(_("トラベルを継続しますか？", "Do you continue to travel?")))
	{
		y = travel.y;
		x = travel.x;
	}
	else if (!tgt_pt(creature_ptr, &x, &y)) return;

	if ((x == creature_ptr->x) && (y == creature_ptr->y))
	{
		msg_print(_("すでにそこにいます！", "You are already there!!"));
		return;
	}

	floor_type *floor_ptr = creature_ptr->current_floor_ptr;
	feature_type *f_ptr;
	f_ptr = &f_info[floor_ptr->grid_array[y][x].feat];

	if ((floor_ptr->grid_array[y][x].info & CAVE_MARK) &&
		(have_flag(f_ptr->flags, FF_WALL) ||
			have_flag(f_ptr->flags, FF_CAN_DIG) ||
			(have_flag(f_ptr->flags, FF_DOOR) && floor_ptr->grid_array[y][x].mimic)))
	{
		msg_print(_("そこには行くことができません！", "You cannot travel there!"));
		return;
	}

	forget_travel_flow(creature_ptr->current_floor_ptr);
	travel_flow(creature_ptr, y, x);

	travel.x = x;
	travel.y = y;

	/* Travel till 255 steps */
	travel.run = 255;
	travel.dir = 0;

	/* Decides first direction */
	POSITION dx = abs(creature_ptr->x - x);
	POSITION dy = abs(creature_ptr->y - y);
	POSITION sx = ((x == creature_ptr->x) || (dx < dy)) ? 0 : ((x > creature_ptr->x) ? 1 : -1);
	POSITION sy = ((y == creature_ptr->y) || (dy < dx)) ? 0 : ((y > creature_ptr->y) ? 1 : -1);

	for (int i = 1; i <= 9; i++)
	{
		if ((sx == ddx[i]) && (sy == ddy[i])) travel.dir = i;
	}
}


/*
 * Something has happened to disturb the player.
 * The first arg indicates a major disturbance, which affects search.
 * The second arg is currently unused, but could induce output flush.
 * All disturbance cancels repeated commands, resting, and running.
 */
void disturb(player_type *creature_ptr, bool stop_search, bool stop_travel)
{
	/* Cancel repeated commands */
	if (command_rep)
	{
		/* Cancel */
		command_rep = 0;

		/* Redraw the state (later) */
		creature_ptr->redraw |= (PR_STATE);
	}

	/* Cancel Resting */
	if ((creature_ptr->action == ACTION_REST) || (creature_ptr->action == ACTION_FISH) || (stop_search && (creature_ptr->action == ACTION_SEARCH)))
	{
		/* Cancel */
		set_action(creature_ptr, ACTION_NONE);
	}

	/* Cancel running */
	if (creature_ptr->running)
	{
		/* Cancel */
		creature_ptr->running = 0;

		/* Check for new panel if appropriate */
		if (center_player && !center_running) verify_panel(creature_ptr);

		/* Calculate torch radius */
		creature_ptr->update |= (PU_TORCH);

		/* Update monster flow */
		creature_ptr->update |= (PU_FLOW);
	}

	if (stop_travel)
	{
		/* Cancel */
		travel.run = 0;

		/* Check for new panel if appropriate */
		if (center_player && !center_running) verify_panel(creature_ptr);

		/* Calculate torch radius */
		creature_ptr->update |= (PU_TORCH);
	}

	if (flush_disturb) flush();
}
