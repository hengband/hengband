/*!
 *  @brief プレイヤーのコマンド処理2 / Movement commands (part 2)
 *  @date 2014/01/02
 *  @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */

#include "cmd/cmd-basic.h"
#include "art-definition/art-bow-types.h"
#include "art-definition/art-weapon-types.h"
#include "cmd-action/cmd-attack.h"
#include "cmd-io/cmd-dump.h"
#include "cmd-io/cmd-save.h"
#include "combat/attack-power-table.h"
#include "combat/shoot.h"
#include "combat/slaying.h"
#include "core/output-updater.h"
#include "core/stuff-handler.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "effect/spells-effect-util.h"
#include "floor/floor-object.h"
#include "floor/geometry.h"
#include "floor/wild.h"
#include "grid/grid.h"
#include "grid/trap.h"
#include "info-reader/fixed-map-parser.h"
#include "inventory/inventory-object.h"
#include "inventory/player-inventory.h"
#include "io/files-util.h"
#include "io/targeting.h"
#include "io/write-diary.h"
#include "main/music-definitions-table.h"
#include "main/sound-definitions-table.h"
#include "mind/snipe-types.h"
#include "monster/monster-status.h"
#include "monster/smart-learn-types.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-types.h"
#include "object/item-use-flags.h"
#include "object/object-broken.h"
#include "object/object-flags.h"
#include "object/object-flavor.h"
#include "object/object-generator.h"
#include "object/object-hook.h"
#include "object/object-kind.h"
#include "object/object-stack.h"
#include "object/object-info.h"
#include "perception/object-perception.h"
#include "player/avatar.h"
#include "player/player-effects.h"
#include "player/player-move.h"
#include "player/player-personalities-table.h"
#include "player/player-status.h"
#include "specific-object/chest.h"
#include "specific-object/torch.h"
#include "spell-kind/spells-teleport.h"
#include "spell-realm/spells-hex.h"
#include "sv-definition/sv-bow-types.h"
#include "system/system-variables.h"
#include "view/display-main-window.h"
#include "view/object-describer.h"
#include "world/world.h"

/*!
 * @brief フロア脱出時に出戻りが不可能だった場合に警告を加える処理
 * @param down_stair TRUEならば階段を降りる処理、FALSEなら階段を昇る処理による内容
 * @return フロア移動を実際に行うならTRUE、キャンセルする場合はFALSE
 */
static bool confirm_leave_level(player_type *creature_ptr, bool down_stair)
{
	quest_type *q_ptr = &quest[creature_ptr->current_floor_ptr->inside_quest];

	/* Confirm leaving from once only quest */
	if (confirm_quest && creature_ptr->current_floor_ptr->inside_quest &&
	    (q_ptr->type == QUEST_TYPE_RANDOM ||
	     (q_ptr->flags & QUEST_FLAG_ONCE &&
						q_ptr->status != QUEST_STATUS_COMPLETED) ||
		 (q_ptr->flags & QUEST_FLAG_TOWER &&
						((q_ptr->status != QUEST_STATUS_STAGE_COMPLETED) ||
						 (down_stair && (quest[QUEST_TOWER1].status != QUEST_STATUS_COMPLETED))))))
	{
		msg_print(_("この階を一度去ると二度と戻って来られません。", "You can't come back here once you leave this floor."));
		if (get_check(_("本当にこの階を去りますか？", "Really leave this floor? "))) return TRUE;
	}
	else
	{
		return TRUE;
	}
	return FALSE;
}

/*!
 * @brief 魔法系コマンドが制限されているかを返す。
 * @return 魔法系コマンドを使用可能ならFALSE、不可能ならば理由をメッセージ表示してTRUEを返す。
 */
bool cmd_limit_cast(player_type *creature_ptr)
{
	if (creature_ptr->current_floor_ptr->dun_level && (d_info[creature_ptr->dungeon_idx].flags1 & DF1_NO_MAGIC))
	{
		msg_print(_("ダンジョンが魔法を吸収した！", "The dungeon absorbs all attempted magic!"));
		msg_print(NULL);
		return TRUE;
	}
	else if (creature_ptr->anti_magic)
	{
		msg_print(_("反魔法バリアが魔法を邪魔した！", "An anti-magic shell disrupts your magic!"));
		return TRUE;
	}
	else if (creature_ptr->shero)
	{
		msg_format(_("狂戦士化していて頭が回らない！", "You cannot think directly!"));
		return TRUE;
	}
	else
		return FALSE;
}

bool cmd_limit_confused(player_type *creature_ptr)
{
	if (creature_ptr->confused)
	{
		msg_print(_("混乱していてできない！", "You are too confused!"));
		return TRUE;
	}
	return FALSE;
}

bool cmd_limit_image(player_type *creature_ptr)
{
	if (creature_ptr->image)
	{
		msg_print(_("幻覚が見えて集中できない！", "Your hallucinations prevent you from concentrating!"));
		return TRUE;
	}
	return FALSE;
}

bool cmd_limit_stun(player_type *creature_ptr)
{
	if (creature_ptr->stun)
	{
		msg_print(_("頭が朦朧としていて集中できない！", "You are too stunned!"));
		return TRUE;
	}
	return FALSE;
}

bool cmd_limit_arena(player_type *creature_ptr)
{
	if (creature_ptr->current_floor_ptr->inside_arena)
	{
		msg_print(_("アリーナが魔法を吸収した！", "The arena absorbs all attempted magic!"));
		msg_print(NULL);
		return TRUE;
	}
	return FALSE;
}

bool cmd_limit_blind(player_type *creature_ptr)
{
	if (creature_ptr->blind)
	{
		msg_print(_("目が見えない。", "You can't see anything."));
		return TRUE;
	}
	if (no_lite(creature_ptr))
	{
		msg_print(_("明かりがないので見えない。", "You have no light."));
		return TRUE;
	}
	return FALSE;
}

bool cmd_limit_time_walk(player_type *creature_ptr)
{
	if (creature_ptr->timewalk)
	{
		if (flush_failure) flush();
		msg_print(_("止まった時の中ではうまく働かないようだ。", "It shows no reaction."));
		sound(SOUND_FAIL);
		return TRUE;
	}
	return FALSE;
}

/*!
 * @brief 階段を使って階層を昇る処理 / Go up one level
 * @return なし
 */
void do_cmd_go_up(player_type *creature_ptr)
{
	bool go_up = FALSE;

	/* Player grid */
	grid_type *g_ptr = &creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x];
	feature_type *f_ptr = &f_info[g_ptr->feat];

	int up_num = 0;

	if (creature_ptr->special_defense & KATA_MUSOU)
	{
		set_action(creature_ptr, ACTION_NONE);
	}

	/* Verify stairs */
	if (!have_flag(f_ptr->flags, FF_LESS))
	{
		msg_print(_("ここには上り階段が見当たらない。", "I see no up staircase here."));
		return;
	}

	/* Quest up stairs */
	if (have_flag(f_ptr->flags, FF_QUEST))
	{
		if (!confirm_leave_level(creature_ptr, FALSE)) return;
	
		/* Success */
		if (IS_ECHIZEN(creature_ptr))
			msg_print(_("なんだこの階段は！", "What's this STAIRWAY!"));
		else
			msg_print(_("上の階に登った。", "You enter the up staircase."));

		leave_quest_check(creature_ptr);

		creature_ptr->current_floor_ptr->inside_quest = g_ptr->special;

		/* Activate the quest */
		if (!quest[creature_ptr->current_floor_ptr->inside_quest].status)
		{
			if (quest[creature_ptr->current_floor_ptr->inside_quest].type != QUEST_TYPE_RANDOM)
			{
				init_flags = INIT_ASSIGN;
				parse_fixed_map(creature_ptr, "q_info.txt", 0, 0, 0, 0);
			}
			quest[creature_ptr->current_floor_ptr->inside_quest].status = QUEST_STATUS_TAKEN;
		}

		/* Leaving a quest */
		if (!creature_ptr->current_floor_ptr->inside_quest)
		{
			creature_ptr->current_floor_ptr->dun_level = 0;
		}
		creature_ptr->leaving = TRUE;

		creature_ptr->oldpx = 0;
		creature_ptr->oldpy = 0;
		
		take_turn(creature_ptr, 100);

		/* End the command */
		return;
	}

	if (!creature_ptr->current_floor_ptr->dun_level)
	{
		go_up = TRUE;
	}
	else
	{
		go_up = confirm_leave_level(creature_ptr, FALSE);
	}

	if (!go_up) return;

	take_turn(creature_ptr, 100);

	if (autosave_l) do_cmd_save_game(creature_ptr, TRUE);

	/* For a random quest */
	if (creature_ptr->current_floor_ptr->inside_quest &&
	    quest[creature_ptr->current_floor_ptr->inside_quest].type == QUEST_TYPE_RANDOM)
	{
		leave_quest_check(creature_ptr);

		creature_ptr->current_floor_ptr->inside_quest = 0;
	}

	/* For a fixed quest */
	if (creature_ptr->current_floor_ptr->inside_quest &&
	    quest[creature_ptr->current_floor_ptr->inside_quest].type != QUEST_TYPE_RANDOM)
	{
		leave_quest_check(creature_ptr);

		creature_ptr->current_floor_ptr->inside_quest = g_ptr->special;
		creature_ptr->current_floor_ptr->dun_level = 0;
		up_num = 0;
	}

	/* For normal dungeon and random quest */
	else
	{
		/* New depth */
		if (have_flag(f_ptr->flags, FF_SHAFT))
		{
			/* Create a way back */
			prepare_change_floor_mode(creature_ptr, CFM_SAVE_FLOORS | CFM_UP | CFM_SHAFT);

			up_num = 2;
		}
		else
		{
			/* Create a way back */
			prepare_change_floor_mode(creature_ptr, CFM_SAVE_FLOORS | CFM_UP);

			up_num = 1;
		}

		/* Get out from current dungeon */
		if (creature_ptr->current_floor_ptr->dun_level - up_num < d_info[creature_ptr->dungeon_idx].mindepth)
			up_num = creature_ptr->current_floor_ptr->dun_level;
	}
	if (record_stair) exe_write_diary(creature_ptr, DIARY_STAIR, 0-up_num, _("階段を上った", "climbed up the stairs to"));

	/* Success */
	if (IS_ECHIZEN(creature_ptr))
		msg_print(_("なんだこの階段は！", "What's this STAIRWAY!"));
	else if (up_num == creature_ptr->current_floor_ptr->dun_level)
		msg_print(_("地上に戻った。", "You go back to the surface."));
	else
		msg_print(_("階段を上って新たなる迷宮へと足を踏み入れた。", "You enter a maze of up staircases."));
	creature_ptr->leaving = TRUE;
}


/*!
 * @brief 階段を使って階層を降りる処理 / Go down one level
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void do_cmd_go_down(player_type *creature_ptr)
{
	bool fall_trap = FALSE;
	int down_num = 0;

	if (creature_ptr->special_defense & KATA_MUSOU)
	{
		set_action(creature_ptr, ACTION_NONE);
	}

	/* Verify stairs */
	grid_type *g_ptr = &creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x];
	feature_type *f_ptr = &f_info[g_ptr->feat];
	if (!have_flag(f_ptr->flags, FF_MORE))
	{
		msg_print(_("ここには下り階段が見当たらない。", "I see no down staircase here."));
		return;
	}

	if (have_flag(f_ptr->flags, FF_TRAP)) fall_trap = TRUE;

	/* Quest entrance */
	if (have_flag(f_ptr->flags, FF_QUEST_ENTER))
	{
		do_cmd_quest(creature_ptr);
		return;
	}

	/* Quest down stairs */
	if (have_flag(f_ptr->flags, FF_QUEST))
	{
		/* Confirm Leaving */
		if(!confirm_leave_level(creature_ptr, TRUE)) return;
		
		if (IS_ECHIZEN(creature_ptr))
			msg_print(_("なんだこの階段は！", "What's this STAIRWAY!"));
		else
			msg_print(_("下の階に降りた。", "You enter the down staircase."));

		leave_quest_check(creature_ptr);
		leave_tower_check(creature_ptr);

		creature_ptr->current_floor_ptr->inside_quest = g_ptr->special;

		/* Activate the quest */
		if (!quest[creature_ptr->current_floor_ptr->inside_quest].status)
		{
			if (quest[creature_ptr->current_floor_ptr->inside_quest].type != QUEST_TYPE_RANDOM)
			{
				init_flags = INIT_ASSIGN;
				parse_fixed_map(creature_ptr, "q_info.txt", 0, 0, 0, 0);
			}
			quest[creature_ptr->current_floor_ptr->inside_quest].status = QUEST_STATUS_TAKEN;
		}

		/* Leaving a quest */
		if (!creature_ptr->current_floor_ptr->inside_quest)
		{
			creature_ptr->current_floor_ptr->dun_level = 0;
		}
		creature_ptr->leaving = TRUE;
		creature_ptr->oldpx = 0;
		creature_ptr->oldpy = 0;
		
		take_turn(creature_ptr, 100);
		return;
	}

	DUNGEON_IDX target_dungeon = 0;

	if (!creature_ptr->current_floor_ptr->dun_level)
	{
		target_dungeon = have_flag(f_ptr->flags, FF_ENTRANCE) ? g_ptr->special : DUNGEON_ANGBAND;

		if (ironman_downward && (target_dungeon != DUNGEON_ANGBAND))
		{
			msg_print(_("ダンジョンの入口は塞がれている！", "The entrance of this dungeon is closed!"));
			return;
		}
		if (!max_dlv[target_dungeon])
		{
			msg_format(_("ここには%sの入り口(%d階相当)があります", "There is the entrance of %s (Danger level: %d)"),
				d_name + d_info[target_dungeon].name, d_info[target_dungeon].mindepth);
			if (!get_check(_("本当にこのダンジョンに入りますか？", "Do you really get in this dungeon? "))) return;
		}

		/* Save old player position */
		creature_ptr->oldpx = creature_ptr->x;
		creature_ptr->oldpy = creature_ptr->y;
		creature_ptr->dungeon_idx = target_dungeon;

		/*
		 * Clear all saved floors
		 * and create a first saved floor
		 */
		prepare_change_floor_mode(creature_ptr, CFM_FIRST_FLOOR);
	}

	take_turn(creature_ptr, 100);

	if (autosave_l) do_cmd_save_game(creature_ptr, TRUE);

	/* Go down */
	if (have_flag(f_ptr->flags, FF_SHAFT)) down_num += 2;
	else down_num += 1;

	if (!creature_ptr->current_floor_ptr->dun_level)
	{
		/* Enter the dungeon just now */
		creature_ptr->enter_dungeon = TRUE;
		down_num = d_info[creature_ptr->dungeon_idx].mindepth;
	}

	if (record_stair)
	{
		if (fall_trap) exe_write_diary(creature_ptr, DIARY_STAIR, down_num, _("落とし戸に落ちた", "fell through a trap door"));
		else exe_write_diary(creature_ptr, DIARY_STAIR, down_num, _("階段を下りた", "climbed down the stairs to"));
	}

	if (fall_trap)
	{
		msg_print(_("わざと落とし戸に落ちた。", "You deliberately jump through the trap door."));
	}
	else
	{
		/* Success */
		if (target_dungeon)
		{
			msg_format(_("%sへ入った。", "You entered %s."), d_text + d_info[creature_ptr->dungeon_idx].text);
		}
		else
		{
			if (IS_ECHIZEN(creature_ptr))
				msg_print(_("なんだこの階段は！", "What's this STAIRWAY!"));
			else
				msg_print(_("階段を下りて新たなる迷宮へと足を踏み入れた。", "You enter a maze of down staircases."));
		}
	}

	creature_ptr->leaving = TRUE;

	if (fall_trap)
	{
		prepare_change_floor_mode(creature_ptr, CFM_SAVE_FLOORS | CFM_DOWN | CFM_RAND_PLACE | CFM_RAND_CONNECT);
		return;
	}
	
	/* Create a way back */
	if (have_flag(f_ptr->flags, FF_SHAFT))
	{
		prepare_change_floor_mode(creature_ptr, CFM_SAVE_FLOORS | CFM_DOWN | CFM_SHAFT);
	}
	else
	{
		prepare_change_floor_mode(creature_ptr, CFM_SAVE_FLOORS | CFM_DOWN);
	}
}


/*!
 * @brief 探索コマンドのメインルーチン / Simple command to "search" for one turn
 * @return なし
 */
void do_cmd_search(player_type * creature_ptr)
{
	/* Allow repeated command */
	if (command_arg)
	{
		/* Set repeat count */
		command_rep = command_arg - 1;
		creature_ptr->redraw |= (PR_STATE);

		/* Cancel the arg */
		command_arg = 0;
	}

	take_turn(creature_ptr, 100);
	search(creature_ptr);
}


/*!
 * @brief 該当のマスに存在している箱のオブジェクトIDを返す。
 * @param y 走査対象にしたいマスのY座標
 * @param x 走査対象にしたいマスのX座標
 * @param trapped TRUEならばトラップが存在する箱のみ、FALSEならば空でない箱全てを対象にする
 * @return 箱が存在する場合そのオブジェクトID、存在しない場合0を返す。
 */
static OBJECT_IDX chest_check(floor_type *floor_ptr, POSITION y, POSITION x, bool trapped)
{
	grid_type *g_ptr = &floor_ptr->grid_array[y][x];
	OBJECT_IDX this_o_idx, next_o_idx = 0;

	/* Scan all objects in the grid */
	for (this_o_idx = g_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;

		o_ptr = &floor_ptr->o_list[this_o_idx];
		next_o_idx = o_ptr->next_o_idx;

		/* Skip unknown chests XXX XXX */
		/* if (!(o_ptr->marked & OM_FOUND)) continue; */

		/* Check for non empty chest */
		if ((o_ptr->tval == TV_CHEST) &&
			(((!trapped) && (o_ptr->pval)) || /* non empty */
			((trapped) && (o_ptr->pval > 0)))) /* trapped only */
		{
			return (this_o_idx);
		}
	}

	return 0;
}

/*!
 * @brief 箱を開ける実行処理 /
 * Attempt to open the given chest at the given location
 * @param y 箱の存在するマスのY座標
 * @param x 箱の存在するマスのX座標
 * @param o_idx 箱のオブジェクトID
 * @return 箱が開かなかった場合TRUE / Returns TRUE if repeated commands may continue
 * @details
 * Assume there is no monster blocking the destination
 */
static bool exe_open_chest(player_type *creature_ptr, POSITION y, POSITION x, OBJECT_IDX o_idx)
{
	bool flag = TRUE;
	bool more = FALSE;
	object_type *o_ptr = &creature_ptr->current_floor_ptr->o_list[o_idx];

	take_turn(creature_ptr, 100);

	/* Attempt to unlock it */
	if (o_ptr->pval > 0)
	{
		/* Assume locked, and thus not open */
		flag = FALSE;

		/* Get the "disarm" factor */
		int i = creature_ptr->skill_dis;

		/* Penalize some conditions */
		if (creature_ptr->blind || no_lite(creature_ptr)) i = i / 10;
		if (creature_ptr->confused || creature_ptr->image) i = i / 10;

		/* Extract the difficulty */
		int j = i - o_ptr->pval;

		/* Always have a small chance of success */
		if (j < 2) j = 2;

		/* Success -- May still have traps */
		if (randint0(100) < j)
		{
			msg_print(_("鍵をはずした。", "You have picked the lock."));
			gain_exp(creature_ptr, 1);
			flag = TRUE;
		}

		/* Failure -- Keep trying */
		else
		{
			/* We may continue repeating */
			more = TRUE;
			if (flush_failure) flush();
			msg_print(_("鍵をはずせなかった。", "You failed to pick the lock."));

		}
	}

	/* Allowed to open */
	if (flag)
	{
		/* Apply chest traps, if any */
		chest_trap(creature_ptr, y, x, o_idx);

		/* Let the Chest drop items */
		chest_death(creature_ptr, FALSE, y, x, o_idx);
	}

	return more;
}

/*!
 * @brief プレイヤーの周辺9マスに該当する地形がいくつあるかを返す /
 * Attempt to open the given chest at the given location
 * @param y 該当する地形の中から1つのY座標を返す参照ポインタ
 * @param x 該当する地形の中から1つのX座標を返す参照ポインタ
 * @param test 地形条件を判定するための関数ポインタ
 * @param under TRUEならばプレイヤーの直下の座標も走査対象にする
 * @return 該当する地形の数
 * @details Return the number of features around (or under) the character.
 * Usually look for doors and floor traps.
 */
static int count_dt(player_type *creature_ptr, POSITION *y, POSITION *x, bool (*test)(player_type*, FEAT_IDX feat), bool under)
{
	/* Check around (and under) the character */
	int count = 0;
	for (DIRECTION d = 0; d < 9; d++)
	{
		grid_type *g_ptr;
		FEAT_IDX feat;

		/* if not searching under player continue */
		if ((d == 8) && !under) continue;

		/* Extract adjacent (legal) location */
		POSITION yy = creature_ptr->y + ddy_ddd[d];
		POSITION xx = creature_ptr->x + ddx_ddd[d];

		/* Get the creature_ptr->current_floor_ptr->grid_array */
		g_ptr = &creature_ptr->current_floor_ptr->grid_array[yy][xx];

		/* Must have knowledge */
		if (!(g_ptr->info & (CAVE_MARK))) continue;

		/* Feature code (applying "mimic" field) */
		feat = get_feat_mimic(g_ptr);

		/* Not looking for this feature */
		if (!((*test)(creature_ptr, feat))) continue;

		/* OK */
		++count;

		/* Remember the location. Only useful if only one match */
		*y = yy;
		*x = xx;
	}

	/* All done */
	return count;
}


/*!
 * @brief プレイヤーの周辺9マスに箱のあるマスがいくつあるかを返す /
 * Return the number of chests around (or under) the character.
 * @param y 該当するマスの中から1つのY座標を返す参照ポインタ
 * @param x 該当するマスの中から1つのX座標を返す参照ポインタ
 * @param trapped TRUEならばトラップの存在が判明している箱のみ対象にする
 * @return 該当する地形の数
 * @details
 * If requested, count only trapped chests.
 */
static int count_chests(player_type *creature_ptr, POSITION *y, POSITION *x, bool trapped)
{
	/* Check around (and under) the character */
	int count = 0;
	for (DIRECTION d = 0; d < 9; d++)
	{
		/* Extract adjacent (legal) location */
		POSITION yy = creature_ptr->y + ddy_ddd[d];
		POSITION xx = creature_ptr->x + ddx_ddd[d];

		/* No (visible) chest is there */
		OBJECT_IDX o_idx = chest_check(creature_ptr->current_floor_ptr, yy, xx, FALSE);
		if (!o_idx) continue;

		/* Grab the object */
		object_type *o_ptr;
		o_ptr = &creature_ptr->current_floor_ptr->o_list[o_idx];

		/* Already open */
		if (o_ptr->pval == 0) continue;

		/* No (known) traps here */
		if (trapped && (!object_is_known(o_ptr) ||
			!chest_traps[o_ptr->pval])) continue;

		/* OK */
		++count;

		/* Remember the location. Only useful if only one match */
		*y = yy;
		*x = xx;
	}

	/* All done */
	return count;
}



/*!
 * @brief 「開ける」動作コマンドのサブルーチン /
 * Perform the basic "open" command on doors
 * @param y 対象を行うマスのY座標
 * @param x 対象を行うマスのX座標
 * @return 実際に処理が行われた場合TRUEを返す。
 * @details
 * Assume destination is a closed/locked/jammed door
 * Assume there is no monster blocking the destination
 * Returns TRUE if repeated commands may continue
 */
static bool exe_open(player_type *creature_ptr, POSITION y, POSITION x)
{
	/* Get requested grid */
	grid_type *g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];
	feature_type *f_ptr = &f_info[g_ptr->feat];
	bool more = FALSE;

	take_turn(creature_ptr, 100);

	/* Seeing true feature code (ignore mimic) */

	/* Jammed door */
	if (!have_flag(f_ptr->flags, FF_OPEN))
	{
		/* Stuck */
		msg_format(_("%sはがっちりと閉じられているようだ。", "The %s appears to be stuck."), f_name + f_info[get_feat_mimic(g_ptr)].name);
		return more;
	}

	if (!f_ptr->power)
	{
		cave_alter_feat(creature_ptr, y, x, FF_OPEN);
		sound(SOUND_OPENDOOR);
		return more;
	}
	
	/* Disarm factor */
	int i = creature_ptr->skill_dis;

	/* Penalize some conditions */
	if (creature_ptr->blind || no_lite(creature_ptr)) i = i / 10;
	if (creature_ptr->confused || creature_ptr->image) i = i / 10;

	/* Extract the difficulty */
	int j = f_ptr->power;
	j = i - (j * 4);

	/* Always have a small chance of success */
	if (j < 2) j = 2;

	if (randint0(100) >= j)
	{
		if (flush_failure) flush();
		msg_print(_("鍵をはずせなかった。", "You failed to pick the lock."));
		more = TRUE;
	}

	msg_print(_("鍵をはずした。", "You have picked the lock."));

	/* Open the door */
	cave_alter_feat(creature_ptr, y, x, FF_OPEN);

	sound(SOUND_OPENDOOR);

	/* Experience */
	gain_exp(creature_ptr, 1);
	return more;
}

/*!
 * @brief 「開ける」コマンドのメインルーチン /
 * Open a closed/locked/jammed door or a closed/locked chest.
 * @return なし
 * @details
 * Unlocking a locked door/chest is worth one experience point.
 */
void do_cmd_open(player_type *creature_ptr)
{
	POSITION y, x;
	DIRECTION dir;
	OBJECT_IDX o_idx;

	bool more = FALSE;

	if (creature_ptr->wild_mode) return;

	if (creature_ptr->special_defense & KATA_MUSOU)
	{
		set_action(creature_ptr, ACTION_NONE);
	}

	/* Option: Pick a direction */
	if (easy_open)
	{
		int num_doors, num_chests;

		num_doors = count_dt(creature_ptr, &y, &x, is_closed_door, FALSE);
		num_chests = count_chests(creature_ptr, &y, &x, FALSE);
		if (num_doors || num_chests)
		{
			bool too_many = (num_doors && num_chests) || (num_doors > 1) ||
			    (num_chests > 1);
			if (!too_many) command_dir = coords_to_dir(creature_ptr, y, x);
		}
	}

	/* Allow repeated command */
	if (command_arg)
	{
		/* Set repeat count */
		command_rep = command_arg - 1;
		creature_ptr->redraw |= (PR_STATE);

		/* Cancel the arg */
		command_arg = 0;
	}

	/* Get a "repeated" direction */
	if (get_rep_dir(creature_ptr, &dir, TRUE))
	{
		FEAT_IDX feat;
		grid_type *g_ptr;

		/* Get requested location */
		y = creature_ptr->y + ddy[dir];
		x = creature_ptr->x + ddx[dir];

		/* Get requested grid */
		g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];

		/* Feature code (applying "mimic" field) */
		feat = get_feat_mimic(g_ptr);

		/* Check for chest */
		o_idx = chest_check(creature_ptr->current_floor_ptr, y, x, FALSE);

		if (!have_flag(f_info[feat].flags, FF_OPEN) && !o_idx)
		{
			msg_print(_("そこには開けるものが見当たらない。", "You see nothing there to open."));
		}
		else if (g_ptr->m_idx && creature_ptr->riding != g_ptr->m_idx)
		{
			take_turn(creature_ptr, 100);
			msg_print(_("モンスターが立ちふさがっている！", "There is a monster in the way!"));
			do_cmd_attack(creature_ptr, y, x, 0);
		}
		else if (o_idx)
		{
			more = exe_open_chest(creature_ptr, y, x, o_idx);
		}
		else
		{
			more = exe_open(creature_ptr, y, x);
		}
	}

	/* Cancel repeat unless we may continue */
	if (!more) disturb(creature_ptr, FALSE, FALSE);
}



/*
 * todo 常にFALSEを返している
 * @brief 「閉じる」動作コマンドのサブルーチン /
 * Perform the basic "close" command
 * @param y 対象を行うマスのY座標
 * @param x 対象を行うマスのX座標
 * @return 実際に処理が行われた場合TRUEを返す。
 * @details
 * Assume destination is an open/broken door
 * Assume there is no monster blocking the destination
 * Returns TRUE if repeated commands may continue
 */
static bool exe_close(player_type *creature_ptr, POSITION y, POSITION x)
{
	grid_type *g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];
	FEAT_IDX old_feat = g_ptr->feat;
	bool more = FALSE;

	take_turn(creature_ptr, 100);

	/* Seeing true feature code (ignore mimic) */

	/* Open door */
	if (!have_flag(f_info[old_feat].flags, FF_CLOSE))
	{
		return more;
	}
	
	s16b closed_feat = feat_state(creature_ptr, old_feat, FF_CLOSE);

	/* Hack -- object in the way */
	if ((g_ptr->o_idx || (g_ptr->info & CAVE_OBJECT)) &&
		(closed_feat != old_feat) && !have_flag(f_info[closed_feat].flags, FF_DROP))
	{
		msg_print(_("何かがつっかえて閉まらない。", "Something prevents it from closing."));
	}
	else
	{
		/* Close the door */
		cave_alter_feat(creature_ptr, y, x, FF_CLOSE);

		/* Broken door */
		if (old_feat == g_ptr->feat)
		{
			msg_print(_("ドアは壊れてしまっている。", "The door appears to be broken."));
		}
		else
		{
			sound(SOUND_SHUTDOOR);
		}
	}

	return more;
}


/*!
 * @brief 「閉じる」コマンドのメインルーチン /
 * Close an open door.
 * @return なし
 * @details
 * Unlocking a locked door/chest is worth one experience point.
 */
void do_cmd_close(player_type *creature_ptr)
{
	POSITION y, x;
	DIRECTION dir;

	bool more = FALSE;

	if (creature_ptr->wild_mode) return;

	if (creature_ptr->special_defense & KATA_MUSOU)
	{
		set_action(creature_ptr, ACTION_NONE);
	}

	/* Option: Pick a direction */
	if (easy_open)
	{
		/* Count open doors */
		if (count_dt(creature_ptr, &y, &x, is_open, FALSE) == 1)
		{
			command_dir = coords_to_dir(creature_ptr, y, x);
		}
	}

	/* Allow repeated command */
	if (command_arg)
	{
		/* Set repeat count */
		command_rep = command_arg - 1;
		creature_ptr->redraw |= (PR_STATE);

		/* Cancel the arg */
		command_arg = 0;
	}

	/* Get a "repeated" direction */
	if (get_rep_dir(creature_ptr, &dir, FALSE))
	{
		grid_type *g_ptr;
		FEAT_IDX feat;

		y = creature_ptr->y + ddy[dir];
		x = creature_ptr->x + ddx[dir];
		g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];

		/* Feature code (applying "mimic" field) */
		feat = get_feat_mimic(g_ptr);

		/* Require open/broken door */
		if (!have_flag(f_info[feat].flags, FF_CLOSE))
		{
			msg_print(_("そこには閉じるものが見当たらない。", "You see nothing there to close."));
		}

		/* Monster in the way */
		else if (g_ptr->m_idx)
		{
			take_turn(creature_ptr, 100);

			msg_print(_("モンスターが立ちふさがっている！", "There is a monster in the way!"));

			/* Attack */
			do_cmd_attack(creature_ptr, y, x, 0);
		}

		/* Close the door */
		else
		{
			/* Close the door */
			more = exe_close(creature_ptr, y, x);
		}
	}

	/* Cancel repeat unless we may continue */
	if (!more) disturb(creature_ptr, FALSE, FALSE);
}


/*!
 * @brief 「掘る」コマンドを該当のマスに行えるかの判定と結果メッセージの表示 /
 * Determine if a given grid may be "tunneled"
 * @param y 対象を行うマスのY座標
 * @param x 対象を行うマスのX座標
 * @return 
 */
static bool do_cmd_tunnel_test(floor_type *floor_ptr, POSITION y, POSITION x)
{
	grid_type *g_ptr = &floor_ptr->grid_array[y][x];

	/* Must have knowledge */
	if (!(g_ptr->info & CAVE_MARK))
	{
		msg_print(_("そこには何も見当たらない。", "You see nothing there."));

		return FALSE;
	}

	/* Must be a wall/door/etc */
	if (!cave_have_flag_grid(g_ptr, FF_TUNNEL))
	{
		msg_print(_("そこには掘るものが見当たらない。", "You see nothing there to tunnel."));

		return FALSE;
	}

	return TRUE;
}


/*!
 * @brief 「掘る」動作コマンドのサブルーチン /
 * Perform the basic "tunnel" command
 * @param y 対象を行うマスのY座標
 * @param x 対象を行うマスのX座標
 * @return 実際に処理が行われた場合TRUEを返す。
 * @details
 * Assumes that no monster is blocking the destination
 * Do not use twall anymore
 * Returns TRUE if repeated commands may continue
 */
static bool exe_tunnel(player_type *creature_ptr, POSITION y, POSITION x)
{
	grid_type *g_ptr;
	feature_type *f_ptr, *mimic_f_ptr;
	int power;
	concptr name;
	bool more = FALSE;

	/* Verify legality */
	if (!do_cmd_tunnel_test(creature_ptr->current_floor_ptr, y, x)) return FALSE;

	take_turn(creature_ptr, 100);

	g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];
	f_ptr = &f_info[g_ptr->feat];
	power = f_ptr->power;

	/* Feature code (applying "mimic" field) */
	mimic_f_ptr = &f_info[get_feat_mimic(g_ptr)];

	name = f_name + mimic_f_ptr->name;

	sound(SOUND_DIG);

	if (have_flag(f_ptr->flags, FF_PERMANENT))
	{
		/* Titanium */
		if (have_flag(mimic_f_ptr->flags, FF_PERMANENT))
		{
			msg_print(_("この岩は硬すぎて掘れないようだ。", "This seems to be permanent rock."));
		}

		/* Map border (mimiccing Permanent wall) */
		else
		{
			msg_print(_("そこは掘れない!", "You can't tunnel through that!"));
		}
	}

	/* Dig or tunnel */
	else if (have_flag(f_ptr->flags, FF_CAN_DIG))
	{
		/* Dig */
		if (creature_ptr->skill_dig > randint0(20 * power))
		{
			msg_format(_("%sをくずした。", "You have removed the %s."), name);

			/* Remove the feature */
			cave_alter_feat(creature_ptr, y, x, FF_TUNNEL);
			creature_ptr->update |= (PU_FLOW);
		}
		else
		{
			/* Message, keep digging */
			msg_format(_("%sをくずしている。", "You dig into the %s."), name);
			
			more = TRUE;
		}
	}

	else
	{
		bool tree = have_flag(mimic_f_ptr->flags, FF_TREE);

		/* Tunnel */
		if (creature_ptr->skill_dig > power + randint0(40 * power))
		{
			if (tree) msg_format(_("%sを切り払った。", "You have cleared away the %s."), name);
			else
			{
				msg_print(_("穴を掘り終えた。", "You have finished the tunnel."));
				creature_ptr->update |= (PU_FLOW);
			}
			
			if (have_flag(f_ptr->flags, FF_GLASS)) sound(SOUND_GLASS);

			/* Remove the feature */
			cave_alter_feat(creature_ptr, y, x, FF_TUNNEL);

			chg_virtue(creature_ptr, V_DILIGENCE, 1);
			chg_virtue(creature_ptr, V_NATURE, -1);
		}

		/* Keep trying */
		else
		{
			if (tree)
			{
				/* We may continue chopping */
				msg_format(_("%sを切っている。", "You chop away at the %s."), name);
				/* Occasional Search XXX XXX */
				if (randint0(100) < 25) search(creature_ptr);
			}
			else
			{
				/* We may continue tunelling */
				msg_format(_("%sに穴を掘っている。", "You tunnel into the %s."), name);
			}

			more = TRUE;
		}
	}

	if (is_hidden_door(creature_ptr, g_ptr))
	{
		/* Occasional Search XXX XXX */
		if (randint0(100) < 25) search(creature_ptr);
	}

	return more;
}


/*!
 * @brief 「掘る」動作コマンドのメインルーチン /
 * Tunnels through "walls" (including rubble and closed doors)
 * @return なし
 * @details
 * <pre>
 * Note that you must tunnel in order to hit invisible monsters
 * in walls, though moving into walls still takes a turn anyway.
 *
 * Digging is very difficult without a "digger" weapon, but can be
 * accomplished by strong players using heavy weapons.
 * </pre>
 */
void do_cmd_tunnel(player_type *creature_ptr)
{
	POSITION y, x;
	DIRECTION dir;
	grid_type *g_ptr;
	FEAT_IDX feat;

	bool more = FALSE;

	if (creature_ptr->special_defense & KATA_MUSOU)
	{
		set_action(creature_ptr, ACTION_NONE);
	}

	/* Allow repeated command */
	if (command_arg)
	{
		/* Set repeat count */
		command_rep = command_arg - 1;
		creature_ptr->redraw |= (PR_STATE);

		/* Cancel the arg */
		command_arg = 0;
	}

	/* Get a direction to tunnel, or Abort */
	if (get_rep_dir(creature_ptr, &dir,FALSE))
	{
		/* Get location */
		y = creature_ptr->y + ddy[dir];
		x = creature_ptr->x + ddx[dir];

		g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];

		/* Feature code (applying "mimic" field) */
		feat = get_feat_mimic(g_ptr);

		/* No tunnelling through doors */
		if (have_flag(f_info[feat].flags, FF_DOOR))
		{
			msg_print(_("ドアは掘れない。", "You cannot tunnel through doors."));
		}

		/* No tunnelling through most features */
		else if (!have_flag(f_info[feat].flags, FF_TUNNEL))
		{
			msg_print(_("そこは掘れない。", "You can't tunnel through that."));
		}

		/* A monster is in the way */
		else if (g_ptr->m_idx)
		{
			take_turn(creature_ptr, 100);
			msg_print(_("モンスターが立ちふさがっている！", "There is a monster in the way!"));

			/* Attack */
			do_cmd_attack(creature_ptr, y, x, 0);
		}

		/* Try digging */
		else
		{
			/* Tunnel through walls */
			more = exe_tunnel(creature_ptr, y, x);
		}
	}

	/* Cancel repetition unless we can continue */
	if (!more) disturb(creature_ptr, FALSE, FALSE);
}

/*!
 * @brief 移動処理による簡易な「開く」処理 /
 * easy_open_door --
 * @return 開く処理が実際に試みられた場合TRUEを返す
 * @details
 * <pre>
 *	If there is a jammed/closed/locked door at the given location,
 *	then attempt to unlock/open it. Return TRUE if an attempt was
 *	made (successful or not), otherwise return FALSE.
 *
 *	The code here should be nearly identical to that in
 *	do_cmd_open_test() and exe_open().
 * </pre>
 */
bool easy_open_door(player_type *creature_ptr, POSITION y, POSITION x)
{
	int i, j;

	grid_type *g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];
	feature_type *f_ptr = &f_info[g_ptr->feat];

	/* Must be a closed door */
	if (!is_closed_door(creature_ptr, g_ptr->feat))
	{
		return FALSE;
	}

	/* Jammed door */
	if (!have_flag(f_ptr->flags, FF_OPEN))
	{
		/* Stuck */
		msg_format(_("%sはがっちりと閉じられているようだ。", "The %s appears to be stuck."), f_name + f_info[get_feat_mimic(g_ptr)].name);

	}

	/* Locked door */
	else if (f_ptr->power)
	{
		/* Disarm factor */
		i = creature_ptr->skill_dis;

		/* Penalize some conditions */
		if (creature_ptr->blind || no_lite(creature_ptr)) i = i / 10;
		if (creature_ptr->confused || creature_ptr->image) i = i / 10;

		/* Extract the lock power */
		j = f_ptr->power;

		/* Extract the difficulty */
		j = i - (j * 4);

		/* Always have a small chance of success */
		if (j < 2) j = 2;

		/* Success */
		if (randint0(100) < j)
		{
			msg_print(_("鍵をはずした。", "You have picked the lock."));

			/* Open the door */
			cave_alter_feat(creature_ptr, y, x, FF_OPEN);

			sound(SOUND_OPENDOOR);

			/* Experience */
			gain_exp(creature_ptr, 1);
		}

		/* Failure */
		else
		{
			/* Failure */
			if (flush_failure) flush();

			msg_print(_("鍵をはずせなかった。", "You failed to pick the lock."));

		}
	}

	/* Closed door */
	else
	{
		/* Open the door */
		cave_alter_feat(creature_ptr, y, x, FF_OPEN);

		sound(SOUND_OPENDOOR);
	}

	return TRUE;
}

/*!
 * @brief 箱のトラップを解除する実行処理 /
 * Perform the basic "disarm" command
 * @param y 解除を行うマスのY座標
 * @param x 解除を行うマスのX座標
 * @param o_idx 箱のオブジェクトID
 * @return ターンを消費する処理が行われた場合TRUEを返す
 * @details
 * <pre>
 * Assume destination is a visible trap
 * Assume there is no monster blocking the destination
 * Returns TRUE if repeated commands may continue
 * </pre>
 */
static bool exe_disarm_chest(player_type *creature_ptr, POSITION y, POSITION x, OBJECT_IDX o_idx)
{
	int i, j;
	bool more = FALSE;
	object_type *o_ptr = &creature_ptr->current_floor_ptr->o_list[o_idx];

	take_turn(creature_ptr, 100);

	/* Get the "disarm" factor */
	i = creature_ptr->skill_dis;

	/* Penalize some conditions */
	if (creature_ptr->blind || no_lite(creature_ptr)) i = i / 10;
	if (creature_ptr->confused || creature_ptr->image) i = i / 10;

	/* Extract the difficulty */
	j = i - o_ptr->pval;

	/* Always have a small chance of success */
	if (j < 2) j = 2;

	/* Must find the trap first. */
	if (!object_is_known(o_ptr))
	{
		msg_print(_("トラップが見あたらない。", "I don't see any traps."));

	}

	/* Already disarmed/unlocked */
	else if (o_ptr->pval <= 0)
	{
		msg_print(_("箱にはトラップが仕掛けられていない。", "The chest is not trapped."));
	}

	/* No traps to find. */
	else if (!chest_traps[o_ptr->pval])
	{
		msg_print(_("箱にはトラップが仕掛けられていない。", "The chest is not trapped."));
	}

	/* Success (get a lot of experience) */
	else if (randint0(100) < j)
	{
		msg_print(_("箱に仕掛けられていたトラップを解除した。", "You have disarmed the chest."));
		gain_exp(creature_ptr, o_ptr->pval);
		o_ptr->pval = (0 - o_ptr->pval);
	}

	/* Failure -- Keep trying */
	else if ((i > 5) && (randint1(i) > 5))
	{
		/* We may keep trying */
		more = TRUE;
		if (flush_failure) flush();
		msg_print(_("箱のトラップ解除に失敗した。", "You failed to disarm the chest."));
	}

	/* Failure -- Set off the trap */
	else
	{
		msg_print(_("トラップを作動させてしまった！", "You set off a trap!"));
		sound(SOUND_FAIL);
		chest_trap(creature_ptr, y, x, o_idx);
	}

	return (more);
}


/*!
 * @brief 箱のトラップを解除するコマンドのサブルーチン /
 * Perform the basic "disarm" command
 * @param y 解除を行うマスのY座標
 * @param x 解除を行うマスのX座標
 * @param dir プレイヤーからみた方向ID
 * @return ターンを消費する処理が行われた場合TRUEを返す
 * @details
 * <pre>
 * Assume destination is a visible trap
 * Assume there is no monster blocking the destination
 * Returns TRUE if repeated commands may continue
 * </pre>
 */

bool exe_disarm(player_type *creature_ptr, POSITION y, POSITION x, DIRECTION dir)
{
	grid_type *g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];

	/* Get feature */
	feature_type *f_ptr = &f_info[g_ptr->feat];

	/* Access trap name */
	concptr name = (f_name + f_ptr->name);

	/* Extract trap "power" */
	int power = f_ptr->power;
	bool more = FALSE;

	/* Get the "disarm" factor */
	int i = creature_ptr->skill_dis;
	int j;

	take_turn(creature_ptr, 100);

	/* Penalize some conditions */
	if (creature_ptr->blind || no_lite(creature_ptr)) i = i / 10;
	if (creature_ptr->confused || creature_ptr->image) i = i / 10;

	/* Extract the difficulty */
	j = i - power;

	/* Always have a small chance of success */
	if (j < 2) j = 2;

	/* Success */
	if (randint0(100) < j)
	{
		msg_format(_("%sを解除した。", "You have disarmed the %s."), name);
		
		/* Reward */
		gain_exp(creature_ptr, power);

		/* Remove the trap */
		cave_alter_feat(creature_ptr, y, x, FF_DISARM);

		/* Move the player onto the trap */
		move_player(creature_ptr, dir, easy_disarm, FALSE);
	}

	/* Failure -- Keep trying */
	else if ((i > 5) && (randint1(i) > 5))
	{
		/* Failure */
		if (flush_failure) flush();

		msg_format(_("%sの解除に失敗した。", "You failed to disarm the %s."), name);

		/* We may keep trying */
		more = TRUE;
	}

	/* Failure -- Set off the trap */
	else
	{
		msg_format(_("%sを作動させてしまった！", "You set off the %s!"), name);
		/* Move the player onto the trap */
		move_player(creature_ptr, dir, easy_disarm, FALSE);
	}

	return (more);
}


/*!
 * @brief 箱、床のトラップ解除処理双方の統合メインルーチン /
 * Disarms a trap, or chest
 * @return なし
 */
void do_cmd_disarm(player_type *creature_ptr)
{
	POSITION y, x;
	DIRECTION dir;
	OBJECT_IDX o_idx;

	bool more = FALSE;

	if (creature_ptr->wild_mode) return;

	if (creature_ptr->special_defense & KATA_MUSOU)
	{
		set_action(creature_ptr, ACTION_NONE);
	}

	/* Option: Pick a direction */
	if (easy_disarm)
	{
		int num_traps, num_chests;

		/* Count visible traps */
		num_traps = count_dt(creature_ptr, &y, &x, is_trap, TRUE);

		/* Count chests (trapped) */
		num_chests = count_chests(creature_ptr, &y, &x, TRUE);

		/* See if only one target */
		if (num_traps || num_chests)
		{
			bool too_many = (num_traps && num_chests) || (num_traps > 1) || (num_chests > 1);
			if (!too_many) command_dir = coords_to_dir(creature_ptr, y, x);
		}
	}
	
	/* Allow repeated command */
	if (command_arg)
	{
		/* Set repeat count */
		command_rep = command_arg - 1;
		creature_ptr->redraw |= (PR_STATE);

		/* Cancel the arg */
		command_arg = 0;
	}

	/* Get a direction (or abort) */
	if (get_rep_dir(creature_ptr, &dir,TRUE))
	{
		grid_type *g_ptr;
		FEAT_IDX feat;

		y = creature_ptr->y + ddy[dir];
		x = creature_ptr->x + ddx[dir];
		g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];

		/* Feature code (applying "mimic" field) */
		feat = get_feat_mimic(g_ptr);

		/* Check for chests */
		o_idx = chest_check(creature_ptr->current_floor_ptr, y, x, TRUE);

		/* Disarm a trap */
		if (!is_trap(creature_ptr, feat) && !o_idx)
		{
			msg_print(_("そこには解除するものが見当たらない。", "You see nothing there to disarm."));
		}

		/* Monster in the way */
		else if (g_ptr->m_idx && creature_ptr->riding != g_ptr->m_idx)
		{
			msg_print(_("モンスターが立ちふさがっている！", "There is a monster in the way!"));

			/* Attack */
			do_cmd_attack(creature_ptr, y, x, 0);
		}

		/* Disarm chest */
		else if (o_idx)
		{
			more = exe_disarm_chest(creature_ptr, y, x, o_idx);
		}

		/* Disarm trap */
		else
		{
			more = exe_disarm(creature_ptr, y, x, dir);
		}
	}

	/* Cancel repeat unless told not to */
	if (!more) disturb(creature_ptr, FALSE, FALSE);
}


/*!
 * @brief 「打ち破る」動作コマンドのサブルーチン /
 * Perform the basic "bash" command
 * @param y 対象を行うマスのY座標
 * @param x 対象を行うマスのX座標
 * @param dir プレイヤーから見たターゲットの方角ID
 * @return 実際に処理が行われた場合TRUEを返す。
 * @details
 * <pre>
 * Assume destination is a closed/locked/jammed door
 * Assume there is no monster blocking the destination
 * Returns TRUE if repeated commands may continue
 * </pre>
 */
static bool do_cmd_bash_aux(player_type *creature_ptr, POSITION y, POSITION x, DIRECTION dir)
{
	grid_type	*g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];

	/* Get feature */
	feature_type *f_ptr = &f_info[g_ptr->feat];

	/* Hack -- Bash power based on strength */
	/* (Ranges from 3 to 20 to 100 to 200) */
	int bash = adj_str_blow[creature_ptr->stat_ind[A_STR]];

	/* Extract door power */
	int temp = f_ptr->power;

	bool		more = FALSE;

	concptr name = f_name + f_info[get_feat_mimic(g_ptr)].name;

	take_turn(creature_ptr, 100);

	msg_format(_("%sに体当たりをした！", "You smash into the %s!"), name);

	/* Compare bash power to door power */
	temp = (bash - (temp * 10));

	if (creature_ptr->pclass == CLASS_BERSERKER) temp *= 2;

	/* Hack -- always have a chance */
	if (temp < 1) temp = 1;

	/* Hack -- attempt to bash down the door */
	if (randint0(100) < temp)
	{
		msg_format(_("%sを壊した！", "The %s crashes open!"), name);

		sound(have_flag(f_ptr->flags, FF_GLASS) ? SOUND_GLASS : SOUND_OPENDOOR);

		/* Break down the door */
		if ((randint0(100) < 50) || (feat_state(creature_ptr, g_ptr->feat, FF_OPEN) == g_ptr->feat) || have_flag(f_ptr->flags, FF_GLASS))
		{
			cave_alter_feat(creature_ptr, y, x, FF_BASH);
		}

		/* Open the door */
		else
		{
			cave_alter_feat(creature_ptr, y, x, FF_OPEN);
		}

		/* Hack -- Fall through the door */
		move_player(creature_ptr, dir, FALSE, FALSE);
	}

	/* Saving throw against stun */
	else if (randint0(100) < adj_dex_safe[creature_ptr->stat_ind[A_DEX]] +
		 creature_ptr->lev)
	{
		msg_format(_("この%sは頑丈だ。", "The %s holds firm."), name);

		/* Allow repeated bashing */
		more = TRUE;
	}

	/* High dexterity yields coolness */
	else
	{
		msg_print(_("体のバランスをくずしてしまった。", "You are off-balance."));

		/* Hack -- Lose balance ala paralysis */
		(void)set_paralyzed(creature_ptr, creature_ptr->paralyzed + 2 + randint0(2));
	}

	return (more);
}


/*!
 * @brief 「打ち破る」動作コマンドのメインルーチン /
 * Bash open a door, success based on character strength
 * @return なし
 * @details
 * <pre>
 * For a closed door, pval is positive if locked; negative if stuck.
 *
 * For an open door, pval is positive for a broken door.
 *
 * A closed door can be opened - harder if locked. Any door might be
 * bashed open (and thereby broken). Bashing a door is (potentially)
 * faster! You move into the door way. To open a stuck door, it must
 * be bashed. A closed door can be jammed (see do_cmd_spike()).
 *
 * Creatures can also open or bash doors, see elsewhere.
 * </pre>
 */
void do_cmd_bash(player_type *creature_ptr)
{
	POSITION y, x;
	DIRECTION dir;
	grid_type *g_ptr;
	bool more = FALSE;

	if (creature_ptr->wild_mode) return;

	if (creature_ptr->special_defense & KATA_MUSOU)
	{
		set_action(creature_ptr, ACTION_NONE);
	}

	/* Allow repeated command */
	if (command_arg)
	{
		/* Set repeat count */
		command_rep = command_arg - 1;
		creature_ptr->redraw |= (PR_STATE);

		/* Cancel the arg */
		command_arg = 0;
	}

	/* Get a "repeated" direction */
	if (get_rep_dir(creature_ptr, &dir,FALSE))
	{
		FEAT_IDX feat;

		/* Bash location */
		y = creature_ptr->y + ddy[dir];
		x = creature_ptr->x + ddx[dir];

		g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];

		/* Feature code (applying "mimic" field) */
		feat = get_feat_mimic(g_ptr);

		/* Nothing useful */
		if (!have_flag(f_info[feat].flags, FF_BASH))
		{
			msg_print(_("そこには体当たりするものが見当たらない。", "You see nothing there to bash."));
		}

		/* Monster in the way */
		else if (g_ptr->m_idx)
		{
			take_turn(creature_ptr, 100);

			msg_print(_("モンスターが立ちふさがっている！", "There is a monster in the way!"));

			/* Attack */
			do_cmd_attack(creature_ptr, y, x, 0);
		}

		/* Bash a closed door */
		else
		{
			/* Bash the door */
			more = do_cmd_bash_aux(creature_ptr, y, x, dir);
		}
	}

	/* Unless valid action taken, cancel bash */
	if (!more) disturb(creature_ptr, FALSE, FALSE);
}


/*!
 * @brief 特定のマスに影響を及ぼすための汎用的コマンド
 * @return なし
 * @details
 * <pre>
 * Manipulate an adjacent grid in some way
 *
 * Attack monsters, tunnel through walls, disarm traps, open doors.
 *
 * Consider confusion 
 *
 * This command must always take a turn, to prevent free detection
 * of invisible monsters.
 * </pre>
 */
void do_cmd_alter(player_type *creature_ptr)
{
	POSITION y, x;
	DIRECTION dir;
	grid_type *g_ptr;
	bool more = FALSE;

	if (creature_ptr->special_defense & KATA_MUSOU)
	{
		set_action(creature_ptr, ACTION_NONE);
	}

	/* Allow repeated command */
	if (command_arg)
	{
		/* Set repeat count */
		command_rep = command_arg - 1;
		creature_ptr->redraw |= (PR_STATE);

		/* Cancel the arg */
		command_arg = 0;
	}

	/* Get a direction */
	if (get_rep_dir(creature_ptr, &dir,TRUE))
	{
		FEAT_IDX feat;
		feature_type *f_ptr;

		y = creature_ptr->y + ddy[dir];
		x = creature_ptr->x + ddx[dir];

		g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];

		/* Feature code (applying "mimic" field) */
		feat = get_feat_mimic(g_ptr);
		f_ptr = &f_info[feat];

		take_turn(creature_ptr, 100);

		if (g_ptr->m_idx)
		{
			do_cmd_attack(creature_ptr, y, x, 0);
		}

		/* Locked doors */
		else if (have_flag(f_ptr->flags, FF_OPEN))
		{
			more = exe_open(creature_ptr, y, x);
		}

		/* Bash jammed doors */
		else if (have_flag(f_ptr->flags, FF_BASH))
		{
			more = do_cmd_bash_aux(creature_ptr, y, x, dir);
		}

		/* Tunnel through walls */
		else if (have_flag(f_ptr->flags, FF_TUNNEL))
		{
			more = exe_tunnel(creature_ptr, y, x);
		}

		/* Close open doors */
		else if (have_flag(f_ptr->flags, FF_CLOSE))
		{
			more = exe_close(creature_ptr, y, x);
		}

		/* Disarm traps */
		else if (have_flag(f_ptr->flags, FF_DISARM))
		{
			more = exe_disarm(creature_ptr, y, x, dir);
		}

		else
		{
			msg_print(_("何もない空中を攻撃した。", "You attack the empty air."));
		}
	}

	/* Cancel repetition unless we can continue */
	if (!more) disturb(creature_ptr, FALSE, FALSE);
}



/*!
 * @brief 「くさびを打つ」ために必要なオブジェクトを所持しているかどうかの判定を返す /
 * Find the index of some "spikes", if possible.
 * @param ip くさびとして打てるオブジェクトのID
 * @return オブジェクトがある場合TRUEを返す
 * @details
 * <pre>
 * Let user choose a pile of spikes, perhaps?
 * </pre>
 */
static bool get_spike(player_type *creature_ptr, INVENTORY_IDX *ip)
{
	INVENTORY_IDX i;

	/* Check every item in the pack */
	for (i = 0; i < INVEN_PACK; i++)
	{
		object_type *o_ptr = &creature_ptr->inventory_list[i];
		if (!o_ptr->k_idx) continue;

		/* Check the "tval" code */
		if (o_ptr->tval == TV_SPIKE)
		{
			/* Save the spike index */
			(*ip) = i;

			/* Success */
			return TRUE;
		}
	}

	return FALSE;
}


/*!
 * @brief 「くさびを打つ」動作コマンドのメインルーチン /
 * Jam a closed door with a spike
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details
 * <pre>
 * This command may NOT be repeated
 * </pre>
 */
void do_cmd_spike(player_type *creature_ptr)
{
	DIRECTION dir;

	if (creature_ptr->wild_mode) return;
	if (creature_ptr->special_defense & KATA_MUSOU)
	{
		set_action(creature_ptr, ACTION_NONE);
	}

	/* Get a "repeated" direction */
	if (!get_rep_dir(creature_ptr, &dir, FALSE)) return;

	POSITION y = creature_ptr->y + ddy[dir];
	POSITION x = creature_ptr->x + ddx[dir];
	grid_type *g_ptr;
	g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];

	/* Feature code (applying "mimic" field) */
	FEAT_IDX feat = get_feat_mimic(g_ptr);

	/* Require closed door */
	INVENTORY_IDX item;
	if (!have_flag(f_info[feat].flags, FF_SPIKE))
	{
		msg_print(_("そこにはくさびを打てるものが見当たらない。", "You see nothing there to spike."));
	}

	/* Get a spike */
	else if (!get_spike(creature_ptr, &item))
	{
		msg_print(_("くさびを持っていない！", "You have no spikes!"));
	}

	/* Is a monster in the way? */
	else if (g_ptr->m_idx)
	{
		take_turn(creature_ptr, 100);

		msg_print(_("モンスターが立ちふさがっている！", "There is a monster in the way!"));

		/* Attack */
		do_cmd_attack(creature_ptr, y, x, 0);
	}

	/* Go for it */
	else
	{
		take_turn(creature_ptr, 100);

		/* Successful jamming */
		msg_format(_("%sにくさびを打ち込んだ。", "You jam the %s with a spike."), f_name + f_info[feat].name);
		cave_alter_feat(creature_ptr, y, x, FF_SPIKE);

		vary_item(creature_ptr, item, -1);
	}
}


/*!
 * @brief 「歩く」動作コマンドのメインルーチン /
 * Support code for the "Walk" and "Jump" commands
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param pickup アイテムの自動拾いを行うならTRUE
 * @return なし
 */
void do_cmd_walk(player_type *creature_ptr, bool pickup)
{
	/* Allow repeated command */
	if (command_arg)
	{
		/* Set repeat count */
		command_rep = command_arg - 1;
		creature_ptr->redraw |= (PR_STATE);

		/* Cancel the arg */
		command_arg = 0;
	}

	/* Get a "repeated" direction */
	bool more = FALSE;
	DIRECTION dir;
	if (get_rep_dir(creature_ptr, &dir, FALSE))
	{
		take_turn(creature_ptr, 100);

		if ((dir != 5) && (creature_ptr->special_defense & KATA_MUSOU))
		{
			set_action(creature_ptr, ACTION_NONE);
		}

		/* Hack -- In small scale wilderness it takes MUCH more time to move */
		if (creature_ptr->wild_mode) creature_ptr->energy_use *= ((MAX_HGT + MAX_WID) / 2);
		if (creature_ptr->action == ACTION_HAYAGAKE) creature_ptr->energy_use = creature_ptr->energy_use * (45-(creature_ptr->lev/2)) / 100;

		/* Actually move the character */
		move_player(creature_ptr, dir, pickup, FALSE);

		/* Allow more walking */
		more = TRUE;
	}

	/* Hack again -- Is there a special encounter ??? */
	if (creature_ptr->wild_mode && !cave_have_flag_bold(creature_ptr->current_floor_ptr, creature_ptr->y, creature_ptr->x, FF_TOWN))
	{
		int tmp = 120 + creature_ptr->lev*10 - wilderness[creature_ptr->y][creature_ptr->x].level + 5;
		if (tmp < 1) 
			tmp = 1;
		if (((wilderness[creature_ptr->y][creature_ptr->x].level + 5) > (creature_ptr->lev / 2)) && randint0(tmp) < (21-creature_ptr->skill_stl))
		{
			/* Inform the player of his horrible fate :=) */
			msg_print(_("襲撃だ！", "You are ambushed !"));

			/* Go into large wilderness view */
			creature_ptr->oldpy = randint1(MAX_HGT-2);
			creature_ptr->oldpx = randint1(MAX_WID-2);
			change_wild_mode(creature_ptr, TRUE);

			/* Give first move to monsters */
			take_turn(creature_ptr, 100);

		}
	}

	/* Cancel repeat unless we may continue */
	if (!more) disturb(creature_ptr, FALSE, FALSE);
}


/*!
 * @brief 「走る」動作コマンドのメインルーチン /
 * Start running.
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void do_cmd_run(player_type *creature_ptr)
{
	DIRECTION dir;
	if (cmd_limit_confused(creature_ptr)) return;

	if (creature_ptr->special_defense & KATA_MUSOU)
	{
		set_action(creature_ptr, ACTION_NONE);
	}

	/* Get a "repeated" direction */
	if (get_rep_dir(creature_ptr, &dir,FALSE))
	{
		/* Hack -- Set the run counter */
		creature_ptr->running = (command_arg ? command_arg : 1000);

		/* First step */
		run_step(creature_ptr, dir);
	}
}


/*!
 * @brief 「留まる」動作コマンドのメインルーチン /
 * Stay still.  Search.  Enter stores.
 * Pick up treasure if "pickup" is true.
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param pickup アイテムの自動拾いを行うならTRUE
 * @return なし
 */
void do_cmd_stay(player_type *creature_ptr, bool pickup)
{
	u32b mpe_mode = MPE_STAYING | MPE_ENERGY_USE;

	/* Allow repeated command */
	if (command_arg)
	{
		/* Set repeat count */
		command_rep = command_arg - 1;
		creature_ptr->redraw |= (PR_STATE);

		/* Cancel the arg */
		command_arg = 0;
	}

	take_turn(creature_ptr, 100);

	if (pickup) mpe_mode |= MPE_DO_PICKUP;
	(void)move_player_effect(creature_ptr, creature_ptr->y, creature_ptr->x, mpe_mode);
}


/*!
 * @brief 「休む」動作コマンドのメインルーチン /
 * Resting allows a player to safely restore his hp	-RAK-
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void do_cmd_rest(player_type *creature_ptr)
{
	set_action(creature_ptr, ACTION_NONE);

	if ((creature_ptr->pclass == CLASS_BARD) && (SINGING_SONG_EFFECT(creature_ptr) || INTERUPTING_SONG_EFFECT(creature_ptr)))
	{
		stop_singing(creature_ptr);
	}

	if (hex_spelling_any(creature_ptr)) stop_hex_spell_all(creature_ptr);

	/* Prompt for time if needed */
	if (command_arg <= 0)
	{
		concptr p = _("休憩 (0-9999, '*' で HP/MP全快, '&' で必要なだけ): ", 
				   "Rest (0-9999, '*' for HP/SP, '&' as needed): ");

char out_val[80];

		/* Default */
		strcpy(out_val, "&");

		/* Ask for duration */
		if (!get_string(p, out_val, 4)) return;

		/* Rest until done */
		if (out_val[0] == '&')
		{
			command_arg = COMMAND_ARG_REST_UNTIL_DONE;
		}

		/* Rest a lot */
		else if (out_val[0] == '*')
		{
			command_arg = COMMAND_ARG_REST_FULL_HEALING;
		}

		/* Rest some */
		else
		{
			command_arg = (COMMAND_ARG)atoi(out_val);
			if (command_arg <= 0) return;
		}
	}

	if (command_arg > 9999) command_arg = 9999;

	if (creature_ptr->special_defense & NINJA_S_STEALTH) set_superstealth(creature_ptr, FALSE);

	/* Take a turn (?) */
	take_turn(creature_ptr, 100);

	/* The sin of sloth */
	if (command_arg > 100) chg_virtue(creature_ptr, V_DILIGENCE, -1);
	
	/* Why are you sleeping when there's no need?  WAKE UP!*/
	if ((creature_ptr->chp == creature_ptr->mhp) &&
	    (creature_ptr->csp == creature_ptr->msp) &&
	    !creature_ptr->blind && !creature_ptr->confused &&
	    !creature_ptr->poisoned && !creature_ptr->afraid &&
	    !creature_ptr->stun && !creature_ptr->cut &&
	    !creature_ptr->slow && !creature_ptr->paralyzed &&
	    !creature_ptr->image && !creature_ptr->word_recall &&
	    !creature_ptr->alter_reality)
			chg_virtue(creature_ptr, V_DILIGENCE, -1);

	/* Save the rest code */
	creature_ptr->resting = command_arg;
	creature_ptr->action = ACTION_REST;
	creature_ptr->update |= (PU_BONUS);
	update_creature(creature_ptr);

	creature_ptr->redraw |= (PR_STATE);
	update_output(creature_ptr);

	Term_fresh();
}



/*
 * todo Doxygenの加筆求む
 * @brief 射撃処理のメインルーチン
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param snipe_type ？？？
 * @return なし
 */
void do_cmd_fire(player_type *creature_ptr, SPELL_IDX snipe_type)
{
	OBJECT_IDX item;
	object_type *j_ptr, *ammo_ptr;
	concptr q, s;

	if(creature_ptr->wild_mode) return;

	creature_ptr->is_fired = FALSE;	/* not fired yet */

	/* Get the "bow" (if any) */
	j_ptr = &creature_ptr->inventory_list[INVEN_BOW];

	/* Require a launcher */
	if (!j_ptr->tval)
	{
		msg_print(_("射撃用の武器を持っていない。", "You have nothing to fire with."));
		flush();
		return;
	}

	if (j_ptr->sval == SV_CRIMSON)
	{
		msg_print(_("この武器は発動して使うもののようだ。", "It's already activated."));
		flush();
		return;
	}

	if (j_ptr->sval == SV_HARP)
	{
		msg_print(_("この武器で射撃はできない。", "It's not for firing."));
		flush();
		return;
	}


	if (creature_ptr->special_defense & KATA_MUSOU)
	{
		set_action(creature_ptr, ACTION_NONE);
	}

	q = _("どれを撃ちますか? ", "Fire which item? ");
	s = _("発射されるアイテムがありません。", "You have nothing to fire.");

	ammo_ptr = choose_object(creature_ptr, &item, q, s, (USE_INVEN | USE_FLOOR), creature_ptr->tval_ammo);
	if (!ammo_ptr)
	{
		flush();
		return;
	}

	/* Fire the item */
	exe_fire(creature_ptr, item, j_ptr, snipe_type);

	if (!creature_ptr->is_fired || creature_ptr->pclass != CLASS_SNIPER) return;

	/* Sniper actions after some shootings */
	if (snipe_type == SP_AWAY)
	{
		teleport_player(creature_ptr, 10 + (creature_ptr->concent * 2), TELEPORT_SPONTANEOUS);
	}

	if (snipe_type == SP_FINAL)
	{
		msg_print(_("射撃の反動が体を襲った。", "The weapon's recoil stuns you. "));
		(void)set_slow(creature_ptr, creature_ptr->slow + randint0(7) + 7, FALSE);
		(void)set_stun(creature_ptr, creature_ptr->stun + randint1(25));
	}
}


/*!
 * @brief 投射処理メインルーチン /
 * Throw an object from the pack or floor.
 * @param mult 威力の倍率
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param boomerang ブーメラン処理ならばTRUE
 * @param shuriken 忍者の手裏剣処理ならばTRUE
 * @return ターンを消費した場合TRUEを返す
 * @details
 * <pre>
 * Note: "unseen" monsters are very hard to hit.
 *
 * Should throwing a weapon do full damage?  Should it allow the magic
 * to hit bonus of the weapon to have an effect?  Should it ever cause
 * the item to be destroyed?  Should it do any damage at all?
 * </pre>
 */
bool do_cmd_throw(player_type *creature_ptr, int mult, bool boomerang, OBJECT_IDX shuriken)
{
	DIRECTION dir;
	OBJECT_IDX item;
	int i;
	POSITION y, x, ty, tx, prev_y, prev_x;
	POSITION ny[19], nx[19];
	int chance, tdam, tdis;
	int mul, div, dd, ds;
	int cur_dis, visible;
	PERCENTAGE j;

	object_type forge;
	object_type *q_ptr;
	object_type *o_ptr;

	bool hit_body = FALSE;
	bool hit_wall = FALSE;
	bool equiped_item = FALSE;
	bool return_when_thrown = FALSE;

	GAME_TEXT o_name[MAX_NLEN];

	int msec = delay_factor * delay_factor * delay_factor;

	BIT_FLAGS flgs[TR_FLAG_SIZE];
	concptr q, s;
	bool come_back = FALSE;
	bool do_drop = TRUE;

	if (creature_ptr->wild_mode) return FALSE;

	if (creature_ptr->special_defense & KATA_MUSOU)
	{
		set_action(creature_ptr, ACTION_NONE);
	}

	if (shuriken >= 0)
	{
		item = shuriken;
		o_ptr = &creature_ptr->inventory_list[item];
	}
	else if (boomerang)
	{
		if (has_melee_weapon(creature_ptr, INVEN_RARM) && has_melee_weapon(creature_ptr, INVEN_LARM))
		{
			item_tester_hook = item_tester_hook_boomerang;
			q = _("どの武器を投げますか? ", "Throw which item? ");
			s = _("投げる武器がない。", "You have nothing to throw.");
			o_ptr = choose_object(creature_ptr, &item, q, s, (USE_EQUIP), 0);
			if (!o_ptr)
			{
				flush();
				return FALSE;
			}
		}
		else if (has_melee_weapon(creature_ptr, INVEN_LARM))
		{
			item = INVEN_LARM;
			o_ptr = &creature_ptr->inventory_list[item];
		}
		else
		{
			item = INVEN_RARM;
			o_ptr = &creature_ptr->inventory_list[item];
		}
	}
	else
	{
		q = _("どのアイテムを投げますか? ", "Throw which item? ");
		s = _("投げるアイテムがない。", "You have nothing to throw.");
		o_ptr = choose_object(creature_ptr, &item, q, s, (USE_INVEN | USE_FLOOR | USE_EQUIP), 0);
		if (!o_ptr)
		{
			flush();
			return FALSE;
		}
	}

	/* Item is cursed */
	if (object_is_cursed(o_ptr) && (item >= INVEN_RARM))
	{
		msg_print(_("ふーむ、どうやら呪われているようだ。", "Hmmm, it seems to be cursed."));
		return FALSE;
	}

	if (creature_ptr->current_floor_ptr->inside_arena && !boomerang)
	{
		if (o_ptr->tval != TV_SPIKE)
		{
			msg_print(_("アリーナではアイテムを使えない！", "You're in the arena now. This is hand-to-hand!"));
			msg_print(NULL);

			return FALSE;
		}
	}

	q_ptr = &forge;
	object_copy(q_ptr, o_ptr);

	/* Extract the thrown object's flags. */
	object_flags(q_ptr, flgs);
	torch_flags(q_ptr, flgs);

	/* Distribute the charges of rods/wands between the stacks */
	distribute_charges(o_ptr, q_ptr, 1);

	/* Single object */
	q_ptr->number = 1;

	object_desc(creature_ptr, o_name, q_ptr, OD_OMIT_PREFIX);

	if (creature_ptr->mighty_throw) mult += 3;

	/* Extract a "distance multiplier" */
	/* Changed for 'launcher' mutation */
	mul = 10 + 2 * (mult - 1);

	/* Enforce a minimum "weight" of one pound */
	div = ((q_ptr->weight > 10) ? q_ptr->weight : 10);
	if ((have_flag(flgs, TR_THROW)) || boomerang) div /= 2;

	/* Hack -- Distance -- Reward strength, penalize weight */
	tdis = (adj_str_blow[creature_ptr->stat_ind[A_STR]] + 20) * mul / div;

	/* Max distance of 10-18 */
	if (tdis > mul) tdis = mul;

	if (shuriken >= 0)
	{
		ty = randint0(101) - 50 + creature_ptr->y;
		tx = randint0(101) - 50 + creature_ptr->x;
	}
	else
	{
		project_length = tdis + 1;

		/* Get a direction (or cancel) */
		if (!get_aim_dir(creature_ptr, &dir)) return FALSE;

		/* Predict the "target" location */
		tx = creature_ptr->x + 99 * ddx[dir];
		ty = creature_ptr->y + 99 * ddy[dir];

		/* Check for "target request" */
		if ((dir == 5) && target_okay(creature_ptr))
		{
			tx = target_col;
			ty = target_row;
		}

		project_length = 0;  /* reset to default */
	}

	if ((q_ptr->name1 == ART_MJOLLNIR) ||
	    (q_ptr->name1 == ART_AEGISFANG) || boomerang)
		return_when_thrown = TRUE;

	if (item >= 0)
	{
		inven_item_increase(creature_ptr, item, -1);
		if (!return_when_thrown)
			inven_item_describe(creature_ptr, item);
		inven_item_optimize(creature_ptr, item);
	}
	else
	{
		floor_item_increase(creature_ptr->current_floor_ptr, 0 - item, -1);
		floor_item_optimize(creature_ptr, 0 - item);
	}

	if (item >= INVEN_RARM)
	{
		equiped_item = TRUE;
		creature_ptr->redraw |= (PR_EQUIPPY);
	}

	take_turn(creature_ptr, 100);

	/* Rogue and Ninja gets bonus */
	if ((creature_ptr->pclass == CLASS_ROGUE) || (creature_ptr->pclass == CLASS_NINJA))
		creature_ptr->energy_use -= creature_ptr->lev;

	/* Start at the player */
	y = creature_ptr->y;
	x = creature_ptr->x;

	handle_stuff(creature_ptr);

	if ((creature_ptr->pclass == CLASS_NINJA) && ((q_ptr->tval == TV_SPIKE) || ((have_flag(flgs, TR_THROW)) && (q_ptr->tval == TV_SWORD)))) shuriken = TRUE;
	else shuriken = FALSE;

	/* Chance of hitting */
	if (have_flag(flgs, TR_THROW)) chance = ((creature_ptr->skill_tht) +
		((creature_ptr->to_h_b + q_ptr->to_h) * BTH_PLUS_ADJ));
	else chance = (creature_ptr->skill_tht + (creature_ptr->to_h_b * BTH_PLUS_ADJ));

	if (shuriken) chance *= 2;

	prev_y = y;
	prev_x = x;

	/* Travel until stopped */
	for (cur_dis = 0; cur_dis <= tdis; )
	{
		/* Hack -- Stop at the target */
		if ((y == ty) && (x == tx)) break;

		/* Calculate the new location (see "project()") */
		ny[cur_dis] = y;
		nx[cur_dis] = x;
		mmove2(&ny[cur_dis], &nx[cur_dis], creature_ptr->y, creature_ptr->x, ty, tx);

		/* Stopped by walls/doors */
		if (!cave_have_flag_bold(creature_ptr->current_floor_ptr, ny[cur_dis], nx[cur_dis], FF_PROJECT))
		{
			hit_wall = TRUE;
			if ((q_ptr->tval == TV_FIGURINE) || object_is_potion(q_ptr) || !creature_ptr->current_floor_ptr->grid_array[ny[cur_dis]][nx[cur_dis]].m_idx) break;
		}

		/* The player can see the (on screen) missile */
		if (panel_contains(ny[cur_dis], nx[cur_dis]) && player_can_see_bold(creature_ptr, ny[cur_dis], nx[cur_dis]))
		{
			SYMBOL_CODE c = object_char(q_ptr);
			TERM_COLOR a = object_attr(q_ptr);

			/* Draw, Hilite, Fresh, Pause, Erase */
			print_rel(creature_ptr, c, a, ny[cur_dis], nx[cur_dis]);
			move_cursor_relative(ny[cur_dis], nx[cur_dis]);
			Term_fresh();
			Term_xtra(TERM_XTRA_DELAY, msec);
			lite_spot(creature_ptr, ny[cur_dis], nx[cur_dis]);
			Term_fresh();
		}

		/* The player cannot see the missile */
		else
		{
			/* Pause anyway, for consistancy */
			Term_xtra(TERM_XTRA_DELAY, msec);
		}

		prev_y = y;
		prev_x = x;

		/* Save the new location */
		x = nx[cur_dis];
		y = ny[cur_dis];

		/* Advance the distance */
		cur_dis++;

		/* Monster here, Try to hit it */
		if (creature_ptr->current_floor_ptr->grid_array[y][x].m_idx)
		{
			grid_type *g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];
			monster_type *m_ptr = &creature_ptr->current_floor_ptr->m_list[g_ptr->m_idx];
			GAME_TEXT m_name[MAX_NLEN];
			monster_name(creature_ptr, g_ptr->m_idx, m_name);

			/* Check the visibility */
			visible = m_ptr->ml;

			/* Note the collision */
			hit_body = TRUE;

			/* Did we hit it (penalize range) */
			if (test_hit_fire(creature_ptr, chance - cur_dis, m_ptr, m_ptr->ml, o_name))
			{
				bool fear = FALSE;

				/* Handle unseen monster */
				if (!visible)
				{
					/* Invisible monster */
					msg_format(_("%sが敵を捕捉した。", "The %s finds a mark."), o_name);
				}

				/* Handle visible monster */
				else
				{
					msg_format(_("%sが%sに命中した。", "The %s hits %s."), o_name, m_name);

					if (m_ptr->ml)
					{
						if (!creature_ptr->image) monster_race_track(creature_ptr, m_ptr->ap_r_idx);
						health_track(creature_ptr, g_ptr->m_idx);
					}
				}

				/* Hack -- Base damage from thrown object */
				dd = q_ptr->dd;
				ds = q_ptr->ds;
				torch_dice(q_ptr, &dd, &ds); /* throwing a torch */
				tdam = damroll(dd, ds);
				/* Apply special damage */
				tdam = calc_attack_damage_with_slay(creature_ptr, q_ptr, tdam, m_ptr, 0, TRUE);
				tdam = critical_shot(creature_ptr, q_ptr->weight, q_ptr->to_h, 0, tdam);
				if (q_ptr->to_d > 0)
					tdam += q_ptr->to_d;
				else
					tdam += -q_ptr->to_d;

				if (boomerang)
				{
					tdam *= (mult+creature_ptr->num_blow[item - INVEN_RARM]);
					tdam += creature_ptr->to_d_m;
				}
				else if (have_flag(flgs, TR_THROW))
				{
					tdam *= (3+mult);
					tdam += creature_ptr->to_d_m;
				}
				else
				{
					tdam *= mult;
				}
				if (shuriken)
				{
					tdam += ((creature_ptr->lev+30)*(creature_ptr->lev+30)-900)/55;
				}

				/* No negative damage */
				if (tdam < 0) tdam = 0;

				/* Modify the damage */
				tdam = mon_damage_mod(creature_ptr, m_ptr, tdam, FALSE);

				msg_format_wizard(CHEAT_MONSTER, _("%dのダメージを与えた。(残りHP %d/%d(%d))", "You do %d damage. (left HP %d/%d(%d))"),
					tdam, m_ptr->hp - tdam, m_ptr->maxhp, m_ptr->max_maxhp);

				/* Hit the monster, check for death */
				if (mon_take_hit(creature_ptr, g_ptr->m_idx, tdam, &fear, extract_note_dies(real_r_idx(m_ptr))))
				{
					/* Dead monster */
				}

				/* No death */
				else
				{
					message_pain(creature_ptr, g_ptr->m_idx, tdam);

					/* Anger the monster */
					if ((tdam > 0) && !object_is_potion(q_ptr))
						anger_monster(creature_ptr, m_ptr);

					if (fear && m_ptr->ml)
					{
						sound(SOUND_FLEE);
						msg_format(_("%^sは恐怖して逃げ出した！", "%^s flees in terror!"), m_name);
					}
				}
			}

			/* Stop looking */
			break;
		}
	}

	/* decrease toach's fuel */
	if (hit_body) torch_lost_fuel(q_ptr);

	/* Chance of breakage (during attacks) */
	j = (hit_body ? breakage_chance(creature_ptr, q_ptr, creature_ptr->pclass == CLASS_ARCHER, 0) : 0);

	/* Figurines transform */
	if ((q_ptr->tval == TV_FIGURINE) && !(creature_ptr->current_floor_ptr->inside_arena))
	{
		j = 100;

		if (!(summon_named_creature(creature_ptr, 0, y, x, q_ptr->pval, !(object_is_cursed(q_ptr)) ? PM_FORCE_PET : 0L)))
			msg_print(_("人形は捻じ曲がり砕け散ってしまった！", "The Figurine writhes and then shatters."));
		else if (object_is_cursed(q_ptr))
			msg_print(_("これはあまり良くない気がする。", "You have a bad feeling about this."));

	}

	/* Potions smash open */
	if (object_is_potion(q_ptr))
	{
		if (hit_body || hit_wall || (randint1(100) < j))
		{
			msg_format(_("%sは砕け散った！", "The %s shatters!"), o_name);

			if (potion_smash_effect(creature_ptr, 0, y, x, q_ptr->k_idx))
			{
				monster_type *m_ptr = &creature_ptr->current_floor_ptr->m_list[creature_ptr->current_floor_ptr->grid_array[y][x].m_idx];
				if (creature_ptr->current_floor_ptr->grid_array[y][x].m_idx && is_friendly(m_ptr) && !MON_INVULNER(m_ptr))
				{
					GAME_TEXT m_name[MAX_NLEN];
					monster_desc(creature_ptr, m_name, m_ptr, 0);
					msg_format(_("%sは怒った！", "%^s gets angry!"), m_name);
					set_hostile(creature_ptr, &creature_ptr->current_floor_ptr->m_list[creature_ptr->current_floor_ptr->grid_array[y][x].m_idx]);
				}
			}
			do_drop = FALSE;
		}
		else
		{
			j = 0;
		}
	}

	if (return_when_thrown)
	{
		int back_chance = randint1(30)+20+((int)(adj_dex_th[creature_ptr->stat_ind[A_DEX]]) - 128);
		char o2_name[MAX_NLEN];
		bool super_boomerang = (((q_ptr->name1 == ART_MJOLLNIR) || (q_ptr->name1 == ART_AEGISFANG)) && boomerang);

		j = -1;
		if (boomerang) back_chance += 4+randint1(5);
		if (super_boomerang) back_chance += 100;
		object_desc(creature_ptr, o2_name, q_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));

		if((back_chance > 30) && (!one_in_(100) || super_boomerang))
		{
			for (i = cur_dis - 1; i > 0; i--)
			{
				if (panel_contains(ny[i], nx[i]) && player_can_see_bold(creature_ptr, ny[i], nx[i]))
				{
					char c = object_char(q_ptr);
					byte a = object_attr(q_ptr);

					/* Draw, Hilite, Fresh, Pause, Erase */
					print_rel(creature_ptr, c, a, ny[i], nx[i]);
					move_cursor_relative(ny[i], nx[i]);
					Term_fresh();
					Term_xtra(TERM_XTRA_DELAY, msec);
					lite_spot(creature_ptr, ny[i], nx[i]);
					Term_fresh();
				}
				else
				{
					/* Pause anyway, for consistancy */
					Term_xtra(TERM_XTRA_DELAY, msec);
				}
			}
			if((back_chance > 37) && !creature_ptr->blind && (item >= 0))
			{
				msg_format(_("%sが手元に返ってきた。", "%s comes back to you."), o2_name);
				come_back = TRUE;
			}
			else
			{
				if (item >= 0)
				{
					msg_format(_("%sを受け損ねた！", "%s comes back, but you can't catch!"), o2_name);
				}
				else
				{
					msg_format(_("%sが返ってきた。", "%s comes back."), o2_name);
				}
				y = creature_ptr->y;
				x = creature_ptr->x;
			}
		}
		else
		{
			msg_format(_("%sが返ってこなかった！", "%s doesn't come back!"), o2_name);
		}
	}

	if (come_back)
	{
		if (item == INVEN_RARM || item == INVEN_LARM)
		{
			/* Access the wield slot */
			o_ptr = &creature_ptr->inventory_list[item];

			/* Wear the new stuff */
			object_copy(o_ptr, q_ptr);

			creature_ptr->total_weight += q_ptr->weight;

			/* Increment the equip counter by hand */
			creature_ptr->equip_cnt++;

			creature_ptr->update |= (PU_BONUS | PU_TORCH | PU_MANA);
			creature_ptr->window |= (PW_EQUIP);
		}
		else
		{
			store_item_to_inventory(creature_ptr, q_ptr);
		}
		do_drop = FALSE;
	}
	else if (equiped_item)
	{
		verify_equip_slot(creature_ptr, item);
		calc_android_exp(creature_ptr);
	}

	if (do_drop)
	{
		if (cave_have_flag_bold(creature_ptr->current_floor_ptr, y, x, FF_PROJECT))
		{
			(void)drop_near(creature_ptr, q_ptr, j, y, x);
		}
		else
		{
			(void)drop_near(creature_ptr, q_ptr, j, prev_y, prev_x);
		}
	}

	return TRUE;
}

/*!
 * @brief 自殺するコマンドのメインルーチン
 * Hack -- commit suicide
 * @return なし
 * @details
 */
void do_cmd_suicide(player_type *creature_ptr)
{
	int i;

	/* Flush input */
	flush();

	/* Verify Retirement */
	if (current_world_ptr->total_winner)
	{
		/* Verify */
		if (!get_check_strict(_("引退しますか? ", "Do you want to retire? "), CHECK_NO_HISTORY)) return;
	}

	/* Verify Suicide */
	else
	{
		/* Verify */
		if (!get_check(_("本当に自殺しますか？", "Do you really want to commit suicide? "))) return;
	}

	if (!current_world_ptr->noscore)
	{
		/* Special Verification for suicide */
		prt(_("確認のため '@' を押して下さい。", "Please verify SUICIDE by typing the '@' sign: "), 0, 0);

		flush();
		i = inkey();
		prt("", 0, 0);
		if (i != '@') return;

		play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_GAMEOVER);
	}

	/* Initialize "last message" buffer */
	if (creature_ptr->last_message) string_free(creature_ptr->last_message);
	creature_ptr->last_message = NULL;

	/* Hack -- Note *winning* message */
	if (current_world_ptr->total_winner && last_words)
	{
		char buf[1024] = "";
		play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_WINNER);
		do
		{
			while (!get_string(_("*勝利*メッセージ: ", "*Winning* message: "), buf, sizeof buf));
		} while (!get_check_strict(_("よろしいですか？", "Are you sure? "), CHECK_NO_HISTORY));

		if (buf[0])
		{
			creature_ptr->last_message = string_make(buf);
			msg_print(creature_ptr->last_message);
		}
	}

	/* Stop playing */
	creature_ptr->playing = FALSE;

	/* Kill the player */
	creature_ptr->is_dead = TRUE;
	creature_ptr->leaving = TRUE;

	if (!current_world_ptr->total_winner)
	{
		exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 0, _("ダンジョンの探索に絶望して自殺した。", "gave up all hope to commit suicide."));
		exe_write_diary(creature_ptr, DIARY_GAMESTART, 1, _("-------- ゲームオーバー --------", "--------   Game  Over   --------"));
		exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 1, "\n\n\n\n");
	}

	/* Cause of death */
	(void)strcpy(creature_ptr->died_from, _("途中終了", "Quitting"));
}
