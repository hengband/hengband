/*!
 *  @file cmd2.c
 *  @brief プレイヤーのコマンド処理2 / Movement commands (part 2)
 *  @date 2014/01/02
 *  @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */

#include "angband.h"
#include "util.h"
#include "core.h"
#include "term.h"

#include "cmd-dump.h"
#include "chest.h"
#include "trap.h"
#include "dungeon.h"
#include "feature.h"
#include "floor.h"
#include "melee.h"
#include "object-hook.h"
#include "spells.h"
#include "spells-summon.h"
#include "spells-status.h"
#include "monster.h"
#include "monster-status.h"
#include "quest.h"
#include "artifact.h"
#include "avatar.h"
#include "player-status.h"
#include "realm-hex.h"
#include "realm-song.h"
#include "geometry.h"
#include "wild.h"
#include "grid.h"
#include "feature.h"
#include "player-move.h"
#include "player-effects.h"
#include "player-class.h"
#include "player-personality.h"
#include "objectkind.h"
#include "object-broken.h"
#include "object-flavor.h"
#include "shoot.h"
#include "snipe.h"

#include "cmd-basic.h"
#include "cmd-item.h"
#include "floor-save.h"
#include "dungeon-file.h"
#include "files.h"

#include "view-mainwindow.h"
#include "targeting.h"

/*!
 * @brief フロア脱出時に出戻りが不可能だった場合に警告を加える処理
 * @param down_stair TRUEならば階段を降りる処理、FALSEなら階段を昇る処理による内容
 * @return フロア移動を実際に行うならTRUE、キャンセルする場合はFALSE
 */
static bool confirm_leave_level(bool down_stair)
{
	quest_type *q_ptr = &quest[p_ptr->inside_quest];

	/* Confirm leaving from once only quest */
	if (confirm_quest && p_ptr->inside_quest &&
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
	if (current_floor_ptr->dun_level && (d_info[p_ptr->dungeon_idx].flags1 & DF1_NO_MAGIC))
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
		msg_print(_("幻覚が見えて集中できない！", "You are too hallucinated!"));
		return TRUE;
	}
	return FALSE;
}

bool cmd_limit_stun(player_type *creature_ptr)
{
	if (creature_ptr->stun)
	{
		msg_print(_("頭が朦朧としていて集中できない！", "You are too stuned!"));
		return TRUE;
	}
	return FALSE;
}

bool cmd_limit_arena(player_type *creature_ptr)
{
	if (creature_ptr->inside_arena)
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
	if (no_lite())
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
void do_cmd_go_up(void)
{
	bool go_up = FALSE;

	/* Player grid */
	grid_type *g_ptr = &current_floor_ptr->grid_array[p_ptr->y][p_ptr->x];
	feature_type *f_ptr = &f_info[g_ptr->feat];

	int up_num = 0;

	if (p_ptr->special_defense & KATA_MUSOU)
	{
		set_action(ACTION_NONE);
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
		/* Cancel the command */
		if (!confirm_leave_level(FALSE)) return;
	
		
		/* Success */
		if ((p_ptr->pseikaku == SEIKAKU_COMBAT) || (p_ptr->inventory_list[INVEN_BOW].name1 == ART_CRIMSON))
			msg_print(_("なんだこの階段は！", "What's this STAIRWAY!"));
		else
			msg_print(_("上の階に登った。", "You enter the up staircase."));

		leave_quest_check();

		p_ptr->inside_quest = g_ptr->special;

		/* Activate the quest */
		if (!quest[p_ptr->inside_quest].status)
		{
			if (quest[p_ptr->inside_quest].type != QUEST_TYPE_RANDOM)
			{
				init_flags = INIT_ASSIGN;
				process_dungeon_file("q_info.txt", 0, 0, 0, 0);
			}
			quest[p_ptr->inside_quest].status = QUEST_STATUS_TAKEN;
		}

		/* Leaving a quest */
		if (!p_ptr->inside_quest)
		{
			current_floor_ptr->dun_level = 0;
		}
		p_ptr->leaving = TRUE;

		p_ptr->oldpx = 0;
		p_ptr->oldpy = 0;
		
		take_turn(p_ptr, 100);

		/* End the command */
		return;
	}

	if (!current_floor_ptr->dun_level)
	{
		go_up = TRUE;
	}
	else
	{
		go_up = confirm_leave_level(FALSE);
	}

	/* Cancel the command */
	if (!go_up) return;

	take_turn(p_ptr, 100);

	if (autosave_l) do_cmd_save_game(TRUE);

	/* For a random quest */
	if (p_ptr->inside_quest &&
	    quest[p_ptr->inside_quest].type == QUEST_TYPE_RANDOM)
	{
		leave_quest_check();

		p_ptr->inside_quest = 0;
	}

	/* For a fixed quest */
	if (p_ptr->inside_quest &&
	    quest[p_ptr->inside_quest].type != QUEST_TYPE_RANDOM)
	{
		leave_quest_check();

		p_ptr->inside_quest = g_ptr->special;
		current_floor_ptr->dun_level = 0;
		up_num = 0;
	}

	/* For normal dungeon and random quest */
	else
	{
		/* New depth */
		if (have_flag(f_ptr->flags, FF_SHAFT))
		{
			/* Create a way back */
			prepare_change_floor_mode(CFM_SAVE_FLOORS | CFM_UP | CFM_SHAFT);

			up_num = 2;
		}
		else
		{
			/* Create a way back */
			prepare_change_floor_mode(CFM_SAVE_FLOORS | CFM_UP);

			up_num = 1;
		}

		/* Get out from current dungeon */
		if (current_floor_ptr->dun_level - up_num < d_info[p_ptr->dungeon_idx].mindepth)
			up_num = current_floor_ptr->dun_level;
	}
	if (record_stair) do_cmd_write_nikki(NIKKI_STAIR, 0-up_num, _("階段を上った", "climbed up the stairs to"));

	/* Success */
	if ((p_ptr->pseikaku == SEIKAKU_COMBAT) || (p_ptr->inventory_list[INVEN_BOW].name1 == ART_CRIMSON))
		msg_print(_("なんだこの階段は！", "What's this STAIRWAY!"));
	else if (up_num == current_floor_ptr->dun_level)
		msg_print(_("地上に戻った。", "You go back to the surface."));
	else
		msg_print(_("階段を上って新たなる迷宮へと足を踏み入れた。", "You enter a maze of up staircases."));
	p_ptr->leaving = TRUE;
}


/*!
 * @brief 階段を使って階層を降りる処理 / Go down one level
 * @return なし
 */
void do_cmd_go_down(void)
{
	/* Player grid */
	grid_type *g_ptr = &current_floor_ptr->grid_array[p_ptr->y][p_ptr->x];
	feature_type *f_ptr = &f_info[g_ptr->feat];

	bool fall_trap = FALSE;
	int down_num = 0;

	if (p_ptr->special_defense & KATA_MUSOU)
	{
		set_action(ACTION_NONE);
	}

	/* Verify stairs */
	if (!have_flag(f_ptr->flags, FF_MORE))
	{
		msg_print(_("ここには下り階段が見当たらない。", "I see no down staircase here."));
		return;
	}

	if (have_flag(f_ptr->flags, FF_TRAP)) fall_trap = TRUE;

	/* Quest entrance */
	if (have_flag(f_ptr->flags, FF_QUEST_ENTER))
	{
		do_cmd_quest();
	}

	/* Quest down stairs */
	else if (have_flag(f_ptr->flags, FF_QUEST))
	{
		/* Confirm Leaving */
		if(!confirm_leave_level(TRUE)) return;
		
		if ((p_ptr->pseikaku == SEIKAKU_COMBAT) || (p_ptr->inventory_list[INVEN_BOW].name1 == ART_CRIMSON))
			msg_print(_("なんだこの階段は！", "What's this STAIRWAY!"));
		else
			msg_print(_("下の階に降りた。", "You enter the down staircase."));

		leave_quest_check();
		leave_tower_check();

		p_ptr->inside_quest = g_ptr->special;

		/* Activate the quest */
		if (!quest[p_ptr->inside_quest].status)
		{
			if (quest[p_ptr->inside_quest].type != QUEST_TYPE_RANDOM)
			{
				init_flags = INIT_ASSIGN;
				process_dungeon_file("q_info.txt", 0, 0, 0, 0);
			}
			quest[p_ptr->inside_quest].status = QUEST_STATUS_TAKEN;
		}

		/* Leaving a quest */
		if (!p_ptr->inside_quest)
		{
			current_floor_ptr->dun_level = 0;
		}
		p_ptr->leaving = TRUE;
		p_ptr->oldpx = 0;
		p_ptr->oldpy = 0;
		
		take_turn(p_ptr, 100);
	}

	else
	{
		DUNGEON_IDX target_dungeon = 0;

		if (!current_floor_ptr->dun_level)
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
							d_name+d_info[target_dungeon].name, d_info[target_dungeon].mindepth);
				if (!get_check(_("本当にこのダンジョンに入りますか？", "Do you really get in this dungeon? "))) return;
			}

			/* Save old player position */
			p_ptr->oldpx = p_ptr->x;
			p_ptr->oldpy = p_ptr->y;
			p_ptr->dungeon_idx = target_dungeon;

			/*
			 * Clear all saved floors
			 * and create a first saved floor
			 */
			prepare_change_floor_mode(CFM_FIRST_FLOOR);
		}

		take_turn(p_ptr, 100);

		if (autosave_l) do_cmd_save_game(TRUE);

		/* Go down */
		if (have_flag(f_ptr->flags, FF_SHAFT)) down_num += 2;
		else down_num += 1;

		if (!current_floor_ptr->dun_level)
		{
			/* Enter the dungeon just now */
			p_ptr->enter_dungeon = TRUE;
			down_num = d_info[p_ptr->dungeon_idx].mindepth;
		}

		if (record_stair)
		{
			if (fall_trap) do_cmd_write_nikki(NIKKI_STAIR, down_num, _("落とし戸に落ちた", "fell through a trap door"));
			else do_cmd_write_nikki(NIKKI_STAIR, down_num, _("階段を下りた", "climbed down the stairs to"));
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
				msg_format(_("%sへ入った。", "You entered %s."), d_text + d_info[p_ptr->dungeon_idx].text);
			}
			else
			{
				if ((p_ptr->pseikaku == SEIKAKU_COMBAT) || (p_ptr->inventory_list[INVEN_BOW].name1 == ART_CRIMSON))
					msg_print(_("なんだこの階段は！", "What's this STAIRWAY!"));
				else
					msg_print(_("階段を下りて新たなる迷宮へと足を踏み入れた。", "You enter a maze of down staircases."));
			}
		}

		p_ptr->leaving = TRUE;

		if (fall_trap)
		{
			prepare_change_floor_mode(CFM_SAVE_FLOORS | CFM_DOWN | CFM_RAND_PLACE | CFM_RAND_CONNECT);
		}
		else
		{
			if (have_flag(f_ptr->flags, FF_SHAFT))
			{
				/* Create a way back */
				prepare_change_floor_mode(CFM_SAVE_FLOORS | CFM_DOWN | CFM_SHAFT);
			}
			else
			{
				/* Create a way back */
				prepare_change_floor_mode(CFM_SAVE_FLOORS | CFM_DOWN);
			}
		}
	}
}


/*!
 * @brief 探索コマンドのメインルーチン / Simple command to "search" for one current_world_ptr->game_turn
 * @return なし
 */
void do_cmd_search(void)
{
	/* Allow repeated command */
	if (command_arg)
	{
		/* Set repeat count */
		command_rep = command_arg - 1;
		p_ptr->redraw |= (PR_STATE);

		/* Cancel the arg */
		command_arg = 0;
	}
	take_turn(p_ptr, 100);

	/* Search */
	search();
}


/*!
 * @brief 該当のマスに存在している箱のオブジェクトIDを返す。
 * @param y 走査対象にしたいマスのY座標
 * @param x 走査対象にしたいマスのX座標
 * @param trapped TRUEならばトラップが存在する箱のみ、FALSEならば空でない箱全てを対象にする
 * @return 箱が存在する場合そのオブジェクトID、存在しない場合0を返す。
 */
static OBJECT_IDX chest_check(POSITION y, POSITION x, bool trapped)
{
	grid_type *g_ptr = &current_floor_ptr->grid_array[y][x];
	OBJECT_IDX this_o_idx, next_o_idx = 0;

	/* Scan all objects in the grid */
	for (this_o_idx = g_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;

		o_ptr = &current_floor_ptr->o_list[this_o_idx];
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
	return (0);
}

/*!
 * @brief 箱を開けるコマンドのメインルーチン /
 * Attempt to open the given chest at the given location
 * @param y 箱の存在するマスのY座標
 * @param x 箱の存在するマスのX座標
 * @param o_idx 箱のオブジェクトID
 * @return 箱が開かなかった場合TRUE / Returns TRUE if repeated commands may continue
 * @details
 * Assume there is no monster blocking the destination
 */
static bool do_cmd_open_chest(POSITION y, POSITION x, OBJECT_IDX o_idx)
{
	int i, j;
	bool flag = TRUE;
	bool more = FALSE;
	object_type *o_ptr = &current_floor_ptr->o_list[o_idx];

	take_turn(p_ptr, 100);

	/* Attempt to unlock it */
	if (o_ptr->pval > 0)
	{
		/* Assume locked, and thus not open */
		flag = FALSE;

		/* Get the "disarm" factor */
		i = p_ptr->skill_dis;

		/* Penalize some conditions */
		if (p_ptr->blind || no_lite()) i = i / 10;
		if (p_ptr->confused || p_ptr->image) i = i / 10;

		/* Extract the difficulty */
		j = i - o_ptr->pval;

		/* Always have a small chance of success */
		if (j < 2) j = 2;

		/* Success -- May still have traps */
		if (randint0(100) < j)
		{
			msg_print(_("鍵をはずした。", "You have picked the lock."));
			gain_exp(1);
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
		chest_trap(y, x, o_idx);

		/* Let the Chest drop items */
		chest_death(FALSE, y, x, o_idx);
	}
	return (more);
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
static int count_dt(POSITION *y, POSITION *x, bool (*test)(FEAT_IDX feat), bool under)
{
	DIRECTION d;
	int count;
	POSITION xx, yy;

	/* Count how many matches */
	count = 0;

	/* Check around (and under) the character */
	for (d = 0; d < 9; d++)
	{
		grid_type *g_ptr;
		FEAT_IDX feat;

		/* if not searching under player continue */
		if ((d == 8) && !under) continue;

		/* Extract adjacent (legal) location */
		yy = p_ptr->y + ddy_ddd[d];
		xx = p_ptr->x + ddx_ddd[d];

		/* Get the current_floor_ptr->grid_array */
		g_ptr = &current_floor_ptr->grid_array[yy][xx];

		/* Must have knowledge */
		if (!(g_ptr->info & (CAVE_MARK))) continue;

		/* Feature code (applying "mimic" field) */
		feat = get_feat_mimic(g_ptr);

		/* Not looking for this feature */
		if (!((*test)(feat))) continue;

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
	DIRECTION d;
	int count;
	OBJECT_IDX o_idx;
	object_type *o_ptr;

	/* Count how many matches */
	count = 0;

	/* Check around (and under) the character */
	for (d = 0; d < 9; d++)
	{
		/* Extract adjacent (legal) location */
		POSITION yy = creature_ptr->y + ddy_ddd[d];
		POSITION xx = creature_ptr->x + ddx_ddd[d];

		/* No (visible) chest is there */
		if ((o_idx = chest_check(yy, xx, FALSE)) == 0) continue;

		/* Grab the object */
		o_ptr = &current_floor_ptr->o_list[o_idx];

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
static bool do_cmd_open_aux(POSITION y, POSITION x)
{
	int i, j;

	/* Get requested grid */
	grid_type *g_ptr = &current_floor_ptr->grid_array[y][x];
	feature_type *f_ptr = &f_info[g_ptr->feat];
	bool more = FALSE;

	take_turn(p_ptr, 100);

	/* Seeing true feature code (ignore mimic) */

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
		i = p_ptr->skill_dis;

		/* Penalize some conditions */
		if (p_ptr->blind || no_lite()) i = i / 10;
		if (p_ptr->confused || p_ptr->image) i = i / 10;

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
			cave_alter_feat(y, x, FF_OPEN);

			sound(SOUND_OPENDOOR);

			/* Experience */
			gain_exp(1);
		}

		/* Failure */
		else
		{
			/* Failure */
			if (flush_failure) flush();

			msg_print(_("鍵をはずせなかった。", "You failed to pick the lock."));

			/* We may keep trying */
			more = TRUE;
		}
	}

	/* Closed door */
	else
	{
		/* Open the door */
		cave_alter_feat(y, x, FF_OPEN);

		sound(SOUND_OPENDOOR);
	}
	return (more);
}

/*!
 * @brief 「開ける」コマンドのメインルーチン /
 * Open a closed/locked/jammed door or a closed/locked chest.
 * @return なし
 * @details
 * Unlocking a locked door/chest is worth one experience point.
 */
void do_cmd_open(void)
{
	POSITION y, x;
	DIRECTION dir;
	OBJECT_IDX o_idx;

	bool more = FALSE;

	if (p_ptr->wild_mode) return;

	if (p_ptr->special_defense & KATA_MUSOU)
	{
		set_action(ACTION_NONE);
	}

	/* Option: Pick a direction */
	if (easy_open)
	{
		int num_doors, num_chests;

		/* Count closed doors (locked or jammed) */
		num_doors = count_dt(&y, &x, is_closed_door, FALSE);

		/* Count chests (locked) */
		num_chests = count_chests(p_ptr, &y, &x, FALSE);

		/* See if only one target */
		if (num_doors || num_chests)
		{
			bool too_many = (num_doors && num_chests) || (num_doors > 1) ||
			    (num_chests > 1);
			if (!too_many) command_dir = coords_to_dir(y, x);
		}
	}

	/* Allow repeated command */
	if (command_arg)
	{
		/* Set repeat count */
		command_rep = command_arg - 1;
		p_ptr->redraw |= (PR_STATE);

		/* Cancel the arg */
		command_arg = 0;
	}

	/* Get a "repeated" direction */
	if (get_rep_dir(&dir, TRUE))
	{
		FEAT_IDX feat;
		grid_type *g_ptr;

		/* Get requested location */
		y = p_ptr->y + ddy[dir];
		x = p_ptr->x + ddx[dir];

		/* Get requested grid */
		g_ptr = &current_floor_ptr->grid_array[y][x];

		/* Feature code (applying "mimic" field) */
		feat = get_feat_mimic(g_ptr);

		/* Check for chest */
		o_idx = chest_check(y, x, FALSE);

		/* Nothing useful */
		if (!have_flag(f_info[feat].flags, FF_OPEN) && !o_idx)
		{
			msg_print(_("そこには開けるものが見当たらない。", "You see nothing there to open."));
		}

		/* Monster in the way */
		else if (g_ptr->m_idx && p_ptr->riding != g_ptr->m_idx)
		{
			take_turn(p_ptr, 100);
			msg_print(_("モンスターが立ちふさがっている！", "There is a monster in the way!"));
			py_attack(y, x, 0);
		}

		/* Handle chests */
		else if (o_idx)
		{
			/* Open the chest */
			more = do_cmd_open_chest(y, x, o_idx);
		}

		/* Handle doors */
		else
		{
			/* Open the door */
			more = do_cmd_open_aux(y, x);
		}
	}

	/* Cancel repeat unless we may continue */
	if (!more) disturb(FALSE, FALSE);
}



/*!
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
static bool do_cmd_close_aux(POSITION y, POSITION x)
{
	grid_type *g_ptr = &current_floor_ptr->grid_array[y][x];
	FEAT_IDX old_feat = g_ptr->feat;
	bool more = FALSE;

	take_turn(p_ptr, 100);

	/* Seeing true feature code (ignore mimic) */

	/* Open door */
	if (have_flag(f_info[old_feat].flags, FF_CLOSE))
	{
		s16b closed_feat = feat_state(old_feat, FF_CLOSE);

		/* Hack -- object in the way */
		if ((g_ptr->o_idx || (g_ptr->info & CAVE_OBJECT)) &&
		    (closed_feat != old_feat) && !have_flag(f_info[closed_feat].flags, FF_DROP))
		{
			msg_print(_("何かがつっかえて閉まらない。", "There seems stuck."));
		}
		else
		{
			/* Close the door */
			cave_alter_feat(y, x, FF_CLOSE);

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
	}
	return (more);
}


/*!
 * @brief 「閉じる」コマンドのメインルーチン /
 * Close an open door.
 * @return なし
 * @details
 * Unlocking a locked door/chest is worth one experience point.
 */
void do_cmd_close(void)
{
	POSITION y, x;
	DIRECTION dir;

	bool more = FALSE;

	if (p_ptr->wild_mode) return;

	if (p_ptr->special_defense & KATA_MUSOU)
	{
		set_action(ACTION_NONE);
	}

	/* Option: Pick a direction */
	if (easy_open)
	{
		/* Count open doors */
		if (count_dt(&y, &x, is_open, FALSE) == 1)
		{
			command_dir = coords_to_dir(y, x);
		}
	}

	/* Allow repeated command */
	if (command_arg)
	{
		/* Set repeat count */
		command_rep = command_arg - 1;
		p_ptr->redraw |= (PR_STATE);

		/* Cancel the arg */
		command_arg = 0;
	}

	/* Get a "repeated" direction */
	if (get_rep_dir(&dir, FALSE))
	{
		grid_type *g_ptr;
		FEAT_IDX feat;

		y = p_ptr->y + ddy[dir];
		x = p_ptr->x + ddx[dir];
		g_ptr = &current_floor_ptr->grid_array[y][x];

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
			take_turn(p_ptr, 100);

			msg_print(_("モンスターが立ちふさがっている！", "There is a monster in the way!"));

			/* Attack */
			py_attack(y, x, 0);
		}

		/* Close the door */
		else
		{
			/* Close the door */
			more = do_cmd_close_aux(y, x);
		}
	}

	/* Cancel repeat unless we may continue */
	if (!more) disturb(FALSE, FALSE);
}


/*!
 * @brief 「掘る」コマンドを該当のマスに行えるかの判定と結果メッセージの表示 /
 * Determine if a given grid may be "tunneled"
 * @param y 対象を行うマスのY座標
 * @param x 対象を行うマスのX座標
 * @return 
 */
static bool do_cmd_tunnel_test(POSITION y, POSITION x)
{
	grid_type *g_ptr = &current_floor_ptr->grid_array[y][x];

	/* Must have knowledge */
	if (!(g_ptr->info & CAVE_MARK))
	{
		msg_print(_("そこには何も見当たらない。", "You see nothing there."));

		return (FALSE);
	}

	/* Must be a wall/door/etc */
	if (!cave_have_flag_grid(g_ptr, FF_TUNNEL))
	{
		msg_print(_("そこには掘るものが見当たらない。", "You see nothing there to tunnel."));

		return (FALSE);
	}

	return (TRUE);
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
static bool do_cmd_tunnel_aux(POSITION y, POSITION x)
{
	grid_type *g_ptr;
	feature_type *f_ptr, *mimic_f_ptr;
	int power;
	concptr name;
	bool more = FALSE;

	/* Verify legality */
	if (!do_cmd_tunnel_test(y, x)) return (FALSE);

	take_turn(p_ptr, 100);

	g_ptr = &current_floor_ptr->grid_array[y][x];
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
		if (p_ptr->skill_dig > randint0(20 * power))
		{
			msg_format(_("%sをくずした。", "You have removed the %s."), name);

			/* Remove the feature */
			cave_alter_feat(y, x, FF_TUNNEL);
			p_ptr->update |= (PU_FLOW);
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
		if (p_ptr->skill_dig > power + randint0(40 * power))
		{
			if (tree) msg_format(_("%sを切り払った。", "You have cleared away the %s."), name);
			else
			{
				msg_print(_("穴を掘り終えた。", "You have finished the tunnel."));
				p_ptr->update |= (PU_FLOW);
			}
			
			if (have_flag(f_ptr->flags, FF_GLASS)) sound(SOUND_GLASS);

			/* Remove the feature */
			cave_alter_feat(y, x, FF_TUNNEL);

			chg_virtue(V_DILIGENCE, 1);
			chg_virtue(V_NATURE, -1);
		}

		/* Keep trying */
		else
		{
			if (tree)
			{
				/* We may continue chopping */
				msg_format(_("%sを切っている。", "You chop away at the %s."), name);
				/* Occasional Search XXX XXX */
				if (randint0(100) < 25) search();
			}
			else
			{
				/* We may continue tunelling */
				msg_format(_("%sに穴を掘っている。", "You tunnel into the %s."), name);
			}

			more = TRUE;
		}
	}

	if (is_hidden_door(g_ptr))
	{
		/* Occasional Search XXX XXX */
		if (randint0(100) < 25) search();
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
 * in walls, though moving into walls still takes a current_world_ptr->game_turn anyway.
 *
 * Digging is very difficult without a "digger" weapon, but can be
 * accomplished by strong players using heavy weapons.
 * </pre>
 */
void do_cmd_tunnel(void)
{
	POSITION y, x;
	DIRECTION dir;
	grid_type *g_ptr;
	FEAT_IDX feat;

	bool more = FALSE;

	if (p_ptr->special_defense & KATA_MUSOU)
	{
		set_action(ACTION_NONE);
	}

	/* Allow repeated command */
	if (command_arg)
	{
		/* Set repeat count */
		command_rep = command_arg - 1;
		p_ptr->redraw |= (PR_STATE);

		/* Cancel the arg */
		command_arg = 0;
	}

	/* Get a direction to tunnel, or Abort */
	if (get_rep_dir(&dir,FALSE))
	{
		/* Get location */
		y = p_ptr->y + ddy[dir];
		x = p_ptr->x + ddx[dir];

		g_ptr = &current_floor_ptr->grid_array[y][x];

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
			take_turn(p_ptr, 100);

			msg_print(_("モンスターが立ちふさがっている！", "There is a monster in the way!"));

			/* Attack */
			py_attack(y, x, 0);
		}

		/* Try digging */
		else
		{
			/* Tunnel through walls */
			more = do_cmd_tunnel_aux(y, x);
		}
	}

	/* Cancel repetition unless we can continue */
	if (!more) disturb(FALSE, FALSE);
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
 *	do_cmd_open_test() and do_cmd_open_aux().
 * </pre>
 */
bool easy_open_door(POSITION y, POSITION x)
{
	int i, j;

	grid_type *g_ptr = &current_floor_ptr->grid_array[y][x];
	feature_type *f_ptr = &f_info[g_ptr->feat];

	/* Must be a closed door */
	if (!is_closed_door(g_ptr->feat))
	{
		return (FALSE);
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
		i = p_ptr->skill_dis;

		/* Penalize some conditions */
		if (p_ptr->blind || no_lite()) i = i / 10;
		if (p_ptr->confused || p_ptr->image) i = i / 10;

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
			cave_alter_feat(y, x, FF_OPEN);

			sound(SOUND_OPENDOOR);

			/* Experience */
			gain_exp(1);
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
		cave_alter_feat(y, x, FF_OPEN);

		sound(SOUND_OPENDOOR);
	}
	return (TRUE);
}

/*!
 * @brief 箱のトラップを解除するコマンドのメインルーチン /
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
static bool do_cmd_disarm_chest(POSITION y, POSITION x, OBJECT_IDX o_idx)
{
	int i, j;
	bool more = FALSE;
	object_type *o_ptr = &current_floor_ptr->o_list[o_idx];

	take_turn(p_ptr, 100);

	/* Get the "disarm" factor */
	i = p_ptr->skill_dis;

	/* Penalize some conditions */
	if (p_ptr->blind || no_lite()) i = i / 10;
	if (p_ptr->confused || p_ptr->image) i = i / 10;

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
		gain_exp(o_ptr->pval);
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
		chest_trap(y, x, o_idx);
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

bool do_cmd_disarm_aux(POSITION y, POSITION x, DIRECTION dir)
{
	grid_type *g_ptr = &current_floor_ptr->grid_array[y][x];

	/* Get feature */
	feature_type *f_ptr = &f_info[g_ptr->feat];

	/* Access trap name */
	concptr name = (f_name + f_ptr->name);

	/* Extract trap "power" */
	int power = f_ptr->power;
	bool more = FALSE;

	/* Get the "disarm" factor */
	int i = p_ptr->skill_dis;
	int j;

	take_turn(p_ptr, 100);

	/* Penalize some conditions */
	if (p_ptr->blind || no_lite()) i = i / 10;
	if (p_ptr->confused || p_ptr->image) i = i / 10;

	/* Extract the difficulty */
	j = i - power;

	/* Always have a small chance of success */
	if (j < 2) j = 2;

	/* Success */
	if (randint0(100) < j)
	{
		msg_format(_("%sを解除した。", "You have disarmed the %s."), name);
		
		/* Reward */
		gain_exp(power);

		/* Remove the trap */
		cave_alter_feat(y, x, FF_DISARM);

		/* Move the player onto the trap */
		move_player(dir, easy_disarm, FALSE);
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
		move_player(dir, easy_disarm, FALSE);
	}
	return (more);
}


/*!
 * @brief 箱、床のトラップ解除処理双方の統合メインルーチン /
 * Disarms a trap, or chest
 * @return なし
 */
void do_cmd_disarm(void)
{
	POSITION y, x;
	DIRECTION dir;
	OBJECT_IDX o_idx;

	bool more = FALSE;

	if (p_ptr->wild_mode) return;

	if (p_ptr->special_defense & KATA_MUSOU)
	{
		set_action(ACTION_NONE);
	}

	/* Option: Pick a direction */
	if (easy_disarm)
	{
		int num_traps, num_chests;

		/* Count visible traps */
		num_traps = count_dt(&y, &x, is_trap, TRUE);

		/* Count chests (trapped) */
		num_chests = count_chests(p_ptr, &y, &x, TRUE);

		/* See if only one target */
		if (num_traps || num_chests)
		{
			bool too_many = (num_traps && num_chests) || (num_traps > 1) || (num_chests > 1);
			if (!too_many) command_dir = coords_to_dir(y, x);
		}
	}


	/* Allow repeated command */
	if (command_arg)
	{
		/* Set repeat count */
		command_rep = command_arg - 1;
		p_ptr->redraw |= (PR_STATE);

		/* Cancel the arg */
		command_arg = 0;
	}

	/* Get a direction (or abort) */
	if (get_rep_dir(&dir,TRUE))
	{
		grid_type *g_ptr;
		FEAT_IDX feat;

		y = p_ptr->y + ddy[dir];
		x = p_ptr->x + ddx[dir];
		g_ptr = &current_floor_ptr->grid_array[y][x];

		/* Feature code (applying "mimic" field) */
		feat = get_feat_mimic(g_ptr);

		/* Check for chests */
		o_idx = chest_check(y, x, TRUE);

		/* Disarm a trap */
		if (!is_trap(feat) && !o_idx)
		{
			msg_print(_("そこには解除するものが見当たらない。", "You see nothing there to disarm."));
		}

		/* Monster in the way */
		else if (g_ptr->m_idx && p_ptr->riding != g_ptr->m_idx)
		{
			msg_print(_("モンスターが立ちふさがっている！", "There is a monster in the way!"));

			/* Attack */
			py_attack(y, x, 0);
		}

		/* Disarm chest */
		else if (o_idx)
		{
			more = do_cmd_disarm_chest(y, x, o_idx);
		}

		/* Disarm trap */
		else
		{
			more = do_cmd_disarm_aux(y, x, dir);
		}
	}

	/* Cancel repeat unless told not to */
	if (!more) disturb(FALSE, FALSE);
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
static bool do_cmd_bash_aux(POSITION y, POSITION x, DIRECTION dir)
{
	grid_type	*g_ptr = &current_floor_ptr->grid_array[y][x];

	/* Get feature */
	feature_type *f_ptr = &f_info[g_ptr->feat];

	/* Hack -- Bash power based on strength */
	/* (Ranges from 3 to 20 to 100 to 200) */
	int bash = adj_str_blow[p_ptr->stat_ind[A_STR]];

	/* Extract door power */
	int temp = f_ptr->power;

	bool		more = FALSE;

	concptr name = f_name + f_info[get_feat_mimic(g_ptr)].name;

	take_turn(p_ptr, 100);

	msg_format(_("%sに体当たりをした！", "You smash into the %s!"), name);

	/* Compare bash power to door power */
	temp = (bash - (temp * 10));

	if (p_ptr->pclass == CLASS_BERSERKER) temp *= 2;

	/* Hack -- always have a chance */
	if (temp < 1) temp = 1;

	/* Hack -- attempt to bash down the door */
	if (randint0(100) < temp)
	{
		msg_format(_("%sを壊した！", "The %s crashes open!"), name);

		sound(have_flag(f_ptr->flags, FF_GLASS) ? SOUND_GLASS : SOUND_OPENDOOR);

		/* Break down the door */
		if ((randint0(100) < 50) || (feat_state(g_ptr->feat, FF_OPEN) == g_ptr->feat) || have_flag(f_ptr->flags, FF_GLASS))
		{
			cave_alter_feat(y, x, FF_BASH);
		}

		/* Open the door */
		else
		{
			cave_alter_feat(y, x, FF_OPEN);
		}

		/* Hack -- Fall through the door */
		move_player(dir, FALSE, FALSE);
	}

	/* Saving throw against stun */
	else if (randint0(100) < adj_dex_safe[p_ptr->stat_ind[A_DEX]] +
		 p_ptr->lev)
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
		(void)set_paralyzed(p_ptr->paralyzed + 2 + randint0(2));
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
void do_cmd_bash(void)
{
	int	y, x, dir;
	grid_type	*g_ptr;
	bool		more = FALSE;

	if (p_ptr->wild_mode) return;

	if (p_ptr->special_defense & KATA_MUSOU)
	{
		set_action(ACTION_NONE);
	}

	/* Allow repeated command */
	if (command_arg)
	{
		/* Set repeat count */
		command_rep = command_arg - 1;
		p_ptr->redraw |= (PR_STATE);

		/* Cancel the arg */
		command_arg = 0;
	}

	/* Get a "repeated" direction */
	if (get_rep_dir(&dir,FALSE))
	{
		FEAT_IDX feat;

		/* Bash location */
		y = p_ptr->y + ddy[dir];
		x = p_ptr->x + ddx[dir];

		g_ptr = &current_floor_ptr->grid_array[y][x];

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
			take_turn(p_ptr, 100);

			msg_print(_("モンスターが立ちふさがっている！", "There is a monster in the way!"));

			/* Attack */
			py_attack(y, x, 0);
		}

		/* Bash a closed door */
		else
		{
			/* Bash the door */
			more = do_cmd_bash_aux(y, x, dir);
		}
	}

	/* Unless valid action taken, cancel bash */
	if (!more) disturb(FALSE, FALSE);
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
 * This command must always take a current_world_ptr->game_turn, to prevent free detection
 * of invisible monsters.
 * </pre>
 */
void do_cmd_alter(void)
{
	POSITION y, x;
	DIRECTION dir;
	grid_type *g_ptr;
	bool more = FALSE;

	if (p_ptr->special_defense & KATA_MUSOU)
	{
		set_action(ACTION_NONE);
	}

	/* Allow repeated command */
	if (command_arg)
	{
		/* Set repeat count */
		command_rep = command_arg - 1;
		p_ptr->redraw |= (PR_STATE);

		/* Cancel the arg */
		command_arg = 0;
	}

	/* Get a direction */
	if (get_rep_dir(&dir,TRUE))
	{
		FEAT_IDX feat;
		feature_type *f_ptr;

		y = p_ptr->y + ddy[dir];
		x = p_ptr->x + ddx[dir];

		g_ptr = &current_floor_ptr->grid_array[y][x];

		/* Feature code (applying "mimic" field) */
		feat = get_feat_mimic(g_ptr);
		f_ptr = &f_info[feat];

		take_turn(p_ptr, 100);

		if (g_ptr->m_idx)
		{
			py_attack(y, x, 0);
		}

		/* Locked doors */
		else if (have_flag(f_ptr->flags, FF_OPEN))
		{
			more = do_cmd_open_aux(y, x);
		}

		/* Bash jammed doors */
		else if (have_flag(f_ptr->flags, FF_BASH))
		{
			more = do_cmd_bash_aux(y, x, dir);
		}

		/* Tunnel through walls */
		else if (have_flag(f_ptr->flags, FF_TUNNEL))
		{
			more = do_cmd_tunnel_aux(y, x);
		}

		/* Close open doors */
		else if (have_flag(f_ptr->flags, FF_CLOSE))
		{
			more = do_cmd_close_aux(y, x);
		}

		/* Disarm traps */
		else if (have_flag(f_ptr->flags, FF_DISARM))
		{
			more = do_cmd_disarm_aux(y, x, dir);
		}

		else
		{
			msg_print(_("何もない空中を攻撃した。", "You attack the empty air."));
		}
	}

	/* Cancel repetition unless we can continue */
	if (!more) disturb(FALSE, FALSE);
}



/*!
 * @brief 「くさびを打つ」ために必要なオブジェクトがあるかどうかの判定を返す /
 * Find the index of some "spikes", if possible.
 * @param ip くさびとして打てるオブジェクトのID
 * @return オブジェクトがある場合TRUEを返す
 * @details
 * <pre>
 * Let user choose a pile of spikes, perhaps?
 * </pre>
 */
static bool get_spike(INVENTORY_IDX *ip)
{
	INVENTORY_IDX i;

	/* Check every item in the pack */
	for (i = 0; i < INVEN_PACK; i++)
	{
		object_type *o_ptr = &p_ptr->inventory_list[i];
		if (!o_ptr->k_idx) continue;

		/* Check the "tval" code */
		if (o_ptr->tval == TV_SPIKE)
		{
			/* Save the spike index */
			(*ip) = i;

			/* Success */
			return (TRUE);
		}
	}

	return (FALSE);
}


/*!
 * @brief 「くさびを打つ」動作コマンドのメインルーチン /
 * Jam a closed door with a spike
 * @return なし
 * @details
 * <pre>
 * This command may NOT be repeated
 * </pre>
 */
void do_cmd_spike(void)
{
	DIRECTION dir;

	if (p_ptr->wild_mode) return;

	if (p_ptr->special_defense & KATA_MUSOU)
	{
		set_action(ACTION_NONE);
	}

	/* Get a "repeated" direction */
	if (get_rep_dir(&dir, FALSE))
	{
		POSITION y, x;
		INVENTORY_IDX item;
		grid_type *g_ptr;
		FEAT_IDX feat;

		y = p_ptr->y + ddy[dir];
		x = p_ptr->x + ddx[dir];
		g_ptr = &current_floor_ptr->grid_array[y][x];

		/* Feature code (applying "mimic" field) */
		feat = get_feat_mimic(g_ptr);

		/* Require closed door */
		if (!have_flag(f_info[feat].flags, FF_SPIKE))
		{
			msg_print(_("そこにはくさびを打てるものが見当たらない。", "You see nothing there to spike."));
		}

		/* Get a spike */
		else if (!get_spike(&item))
		{
			msg_print(_("くさびを持っていない！", "You have no spikes!"));
		}

		/* Is a monster in the way? */
		else if (g_ptr->m_idx)
		{
			take_turn(p_ptr, 100);

			msg_print(_("モンスターが立ちふさがっている！", "There is a monster in the way!"));

			/* Attack */
			py_attack(y, x, 0);
		}

		/* Go for it */
		else
		{
			take_turn(p_ptr, 100);

			/* Successful jamming */
			msg_format(_("%sにくさびを打ち込んだ。", "You jam the %s with a spike."), f_name + f_info[feat].name);
			cave_alter_feat(y, x, FF_SPIKE);

			/* Use up, and describe, a single spike, from the bottom */
			inven_item_increase(item, -1);
			inven_item_describe(item);
			inven_item_optimize(item);
		}
	}
}



/*!
 * @brief 「歩く」動作コマンドのメインルーチン /
 * Support code for the "Walk" and "Jump" commands
 * @param pickup アイテムの自動拾いを行うならTRUE
 * @return なし
 */
void do_cmd_walk(bool pickup)
{
	DIRECTION dir;

	bool more = FALSE;


	/* Allow repeated command */
	if (command_arg)
	{
		/* Set repeat count */
		command_rep = command_arg - 1;
		p_ptr->redraw |= (PR_STATE);

		/* Cancel the arg */
		command_arg = 0;
	}

	/* Get a "repeated" direction */
	if (get_rep_dir(&dir, FALSE))
	{
		take_turn(p_ptr, 100);

		if ((dir != 5) && (p_ptr->special_defense & KATA_MUSOU))
		{
			set_action(ACTION_NONE);
		}

		/* Hack -- In small scale wilderness it takes MUCH more time to move */
		if (p_ptr->wild_mode) p_ptr->energy_use *= ((MAX_HGT + MAX_WID) / 2);
		if (p_ptr->action == ACTION_HAYAGAKE) p_ptr->energy_use = p_ptr->energy_use * (45-(p_ptr->lev/2)) / 100;

		/* Actually move the character */
		move_player(dir, pickup, FALSE);

		/* Allow more walking */
		more = TRUE;
	}

	/* Hack again -- Is there a special encounter ??? */
	if (p_ptr->wild_mode && !cave_have_flag_bold(p_ptr->y, p_ptr->x, FF_TOWN))
	{
		int tmp = 120 + p_ptr->lev*10 - wilderness[p_ptr->y][p_ptr->x].level + 5;
		if (tmp < 1) 
			tmp = 1;
		if (((wilderness[p_ptr->y][p_ptr->x].level + 5) > (p_ptr->lev / 2)) && randint0(tmp) < (21-p_ptr->skill_stl))
		{
			/* Inform the player of his horrible fate :=) */
			msg_print(_("襲撃だ！", "You are ambushed !"));

			/* Go into large wilderness view */
			p_ptr->oldpy = randint1(MAX_HGT-2);
			p_ptr->oldpx = randint1(MAX_WID-2);
			change_wild_mode(TRUE);

			/* Give first move to monsters */
			take_turn(p_ptr, 100);

		}
	}

	/* Cancel repeat unless we may continue */
	if (!more) disturb(FALSE, FALSE);
}


/*!
 * @brief 「走る」動作コマンドのメインルーチン /
 * Start running.
 * @return なし
 */
void do_cmd_run(void)
{
	DIRECTION dir;
	if (cmd_limit_confused(p_ptr)) return;

	if (p_ptr->special_defense & KATA_MUSOU)
	{
		set_action(ACTION_NONE);
	}

	/* Get a "repeated" direction */
	if (get_rep_dir(&dir,FALSE))
	{
		/* Hack -- Set the run counter */
		p_ptr->running = (command_arg ? command_arg : 1000);

		/* First step */
		run_step(dir);
	}
}


/*!
 * @brief 「留まる」動作コマンドのメインルーチン /
 * Stay still.  Search.  Enter stores.
 * Pick up treasure if "pickup" is true.
 * @param pickup アイテムの自動拾いを行うならTRUE
 * @return なし
 */
void do_cmd_stay(bool pickup)
{
	u32b mpe_mode = MPE_STAYING | MPE_ENERGY_USE;

	/* Allow repeated command */
	if (command_arg)
	{
		/* Set repeat count */
		command_rep = command_arg - 1;
		p_ptr->redraw |= (PR_STATE);

		/* Cancel the arg */
		command_arg = 0;
	}

	take_turn(p_ptr, 100);

	if (pickup) mpe_mode |= MPE_DO_PICKUP;
	(void)move_player_effect(p_ptr->y, p_ptr->x, mpe_mode);
}


/*!
 * @brief 「休む」動作コマンドのメインルーチン /
 * Resting allows a player to safely restore his hp	-RAK-
 * @return なし
 */
void do_cmd_rest(void)
{

	set_action(ACTION_NONE);

	if ((p_ptr->pclass == CLASS_BARD) && (SINGING_SONG_EFFECT(p_ptr) || INTERUPTING_SONG_EFFECT(p_ptr)))
	{
		stop_singing(p_ptr);
	}

	/* Hex */
	if (hex_spelling_any()) stop_hex_spell_all();

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

	if (p_ptr->special_defense & NINJA_S_STEALTH) set_superstealth(FALSE);

	/* Take a current_world_ptr->game_turn (?) */
	take_turn(p_ptr, 100);

	/* The sin of sloth */
	if (command_arg > 100) chg_virtue(V_DILIGENCE, -1);
	
	/* Why are you sleeping when there's no need?  WAKE UP!*/
	if ((p_ptr->chp == p_ptr->mhp) &&
	    (p_ptr->csp == p_ptr->msp) &&
	    !p_ptr->blind && !p_ptr->confused &&
	    !p_ptr->poisoned && !p_ptr->afraid &&
	    !p_ptr->stun && !p_ptr->cut &&
	    !p_ptr->slow && !p_ptr->paralyzed &&
	    !p_ptr->image && !p_ptr->word_recall &&
	    !p_ptr->alter_reality)
			chg_virtue(V_DILIGENCE, -1);

	/* Save the rest code */
	p_ptr->resting = command_arg;
	p_ptr->action = ACTION_REST;
	p_ptr->update |= (PU_BONUS);
	update_creature(p_ptr);

	p_ptr->redraw |= (PR_STATE);
	update_output();

	Term_fresh();
}



/*!
 * @brief 射撃処理のメインルーチン
 * @return なし
 */
void do_cmd_fire(SPELL_IDX snipe_type)
{
	OBJECT_IDX item;
	object_type *j_ptr, *ammo_ptr;
	concptr q, s;

	if(p_ptr->wild_mode) return;

	p_ptr->is_fired = FALSE;	/* not fired yet */

	/* Get the "bow" (if any) */
	j_ptr = &p_ptr->inventory_list[INVEN_BOW];

	/* Require a launcher */
	if (!j_ptr->tval)
	{
		msg_print(_("射撃用の武器を持っていない。", "You have nothing to fire with."));
		flush();
		return;
	}

	if (j_ptr->sval == SV_CRIMSON)
	{
		msg_print(_("この武器は発動して使うもののようだ。", "Do activate."));
		flush();
		return;
	}

	if (j_ptr->sval == SV_HARP)
	{
		msg_print(_("この武器で射撃はできない。", "It's not for firing."));
		flush();
		return;
	}


	if (p_ptr->special_defense & KATA_MUSOU)
	{
		set_action(ACTION_NONE);
	}

	q = _("どれを撃ちますか? ", "Fire which item? ");
	s = _("発射されるアイテムがありません。", "You have nothing to fire.");

	ammo_ptr = choose_object(&item, q, s, (USE_INVEN | USE_FLOOR), p_ptr->tval_ammo);
	if (!ammo_ptr)
	{
		flush();
		return;
	}

	/* Fire the item */
	exe_fire(item, j_ptr, snipe_type);

	if (!p_ptr->is_fired || p_ptr->pclass != CLASS_SNIPER) return;

	/* Sniper actions after some shootings */
	if (snipe_type == SP_AWAY)
	{
		teleport_player(10 + (p_ptr->concent * 2), 0L);
	}
	if (snipe_type == SP_FINAL)
	{
		msg_print(_("射撃の反動が体を襲った。", "A reactionary of shooting attacked you. "));
		(void)set_slow(p_ptr->slow + randint0(7) + 7, FALSE);
		(void)set_stun(p_ptr->stun + randint1(25));
	}
}


/*!
 * @brief 投射処理メインルーチン /
 * Throw an object from the pack or floor.
 * @param mult 威力の倍率
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
bool do_cmd_throw(int mult, bool boomerang, OBJECT_IDX shuriken)
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

	if (p_ptr->wild_mode) return FALSE;

	if (p_ptr->special_defense & KATA_MUSOU)
	{
		set_action(ACTION_NONE);
	}

	if (shuriken >= 0)
	{
		item = shuriken;
		o_ptr = &p_ptr->inventory_list[item];
	}
	else if (boomerang)
	{
		if (has_melee_weapon(INVEN_RARM) && has_melee_weapon(INVEN_LARM))
		{
			item_tester_hook = item_tester_hook_boomerang;
			q = _("どの武器を投げますか? ", "Throw which item? ");
			s = _("投げる武器がない。", "You have nothing to throw.");
			o_ptr = choose_object(&item, q, s, (USE_EQUIP), 0);
			if (!o_ptr)
			{
				flush();
				return FALSE;
			}
		}
		else if (has_melee_weapon(INVEN_LARM))
		{
			item = INVEN_LARM;
			o_ptr = &p_ptr->inventory_list[item];
		}
		else
		{
			item = INVEN_RARM;
			o_ptr = &p_ptr->inventory_list[item];
		}
	}
	else
	{
		q = _("どのアイテムを投げますか? ", "Throw which item? ");
		s = _("投げるアイテムがない。", "You have nothing to throw.");
		o_ptr = choose_object(&item, q, s, (USE_INVEN | USE_FLOOR | USE_EQUIP), 0);
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

	if (p_ptr->inside_arena && !boomerang)
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

	object_desc(o_name, q_ptr, OD_OMIT_PREFIX);

	if (p_ptr->mighty_throw) mult += 3;

	/* Extract a "distance multiplier" */
	/* Changed for 'launcher' mutation */
	mul = 10 + 2 * (mult - 1);

	/* Enforce a minimum "weight" of one pound */
	div = ((q_ptr->weight > 10) ? q_ptr->weight : 10);
	if ((have_flag(flgs, TR_THROW)) || boomerang) div /= 2;

	/* Hack -- Distance -- Reward strength, penalize weight */
	tdis = (adj_str_blow[p_ptr->stat_ind[A_STR]] + 20) * mul / div;

	/* Max distance of 10-18 */
	if (tdis > mul) tdis = mul;

	if (shuriken >= 0)
	{
		ty = randint0(101) - 50 + p_ptr->y;
		tx = randint0(101) - 50 + p_ptr->x;
	}
	else
	{
		project_length = tdis + 1;

		/* Get a direction (or cancel) */
		if (!get_aim_dir(&dir)) return FALSE;

		/* Predict the "target" location */
		tx = p_ptr->x + 99 * ddx[dir];
		ty = p_ptr->y + 99 * ddy[dir];

		/* Check for "target request" */
		if ((dir == 5) && target_okay())
		{
			tx = target_col;
			ty = target_row;
		}

		project_length = 0;  /* reset to default */
	}

	if ((q_ptr->name1 == ART_MJOLLNIR) ||
	    (q_ptr->name1 == ART_AEGISFANG) || boomerang)
		return_when_thrown = TRUE;

	/* Reduce and describe p_ptr->inventory_list */
	if (item >= 0)
	{
		inven_item_increase(item, -1);
		if (!return_when_thrown)
			inven_item_describe(item);
		inven_item_optimize(item);
	}

	/* Reduce and describe floor item */
	else
	{
		floor_item_increase(0 - item, -1);
		floor_item_optimize(0 - item);
	}
	if (item >= INVEN_RARM)
	{
		equiped_item = TRUE;
		p_ptr->redraw |= (PR_EQUIPPY);
	}

	take_turn(p_ptr, 100);

	/* Rogue and Ninja gets bonus */
	if ((p_ptr->pclass == CLASS_ROGUE) || (p_ptr->pclass == CLASS_NINJA))
		p_ptr->energy_use -= p_ptr->lev;

	/* Start at the player */
	y = p_ptr->y;
	x = p_ptr->x;

	handle_stuff();

	if ((p_ptr->pclass == CLASS_NINJA) && ((q_ptr->tval == TV_SPIKE) || ((have_flag(flgs, TR_THROW)) && (q_ptr->tval == TV_SWORD)))) shuriken = TRUE;
	else shuriken = FALSE;

	/* Chance of hitting */
	if (have_flag(flgs, TR_THROW)) chance = ((p_ptr->skill_tht) +
		((p_ptr->to_h_b + q_ptr->to_h) * BTH_PLUS_ADJ));
	else chance = (p_ptr->skill_tht + (p_ptr->to_h_b * BTH_PLUS_ADJ));

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
		mmove2(&ny[cur_dis], &nx[cur_dis], p_ptr->y, p_ptr->x, ty, tx);

		/* Stopped by walls/doors */
		if (!cave_have_flag_bold(ny[cur_dis], nx[cur_dis], FF_PROJECT))
		{
			hit_wall = TRUE;
			if ((q_ptr->tval == TV_FIGURINE) || object_is_potion(q_ptr) || !current_floor_ptr->grid_array[ny[cur_dis]][nx[cur_dis]].m_idx) break;
		}

		/* The player can see the (on screen) missile */
		if (panel_contains(ny[cur_dis], nx[cur_dis]) && player_can_see_bold(ny[cur_dis], nx[cur_dis]))
		{
			SYMBOL_CODE c = object_char(q_ptr);
			TERM_COLOR a = object_attr(q_ptr);

			/* Draw, Hilite, Fresh, Pause, Erase */
			print_rel(c, a, ny[cur_dis], nx[cur_dis]);
			move_cursor_relative(ny[cur_dis], nx[cur_dis]);
			Term_fresh();
			Term_xtra(TERM_XTRA_DELAY, msec);
			lite_spot(ny[cur_dis], nx[cur_dis]);
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
		if (current_floor_ptr->grid_array[y][x].m_idx)
		{
			grid_type *g_ptr = &current_floor_ptr->grid_array[y][x];
			monster_type *m_ptr = &current_floor_ptr->m_list[g_ptr->m_idx];
			GAME_TEXT m_name[MAX_NLEN];
			monster_name(g_ptr->m_idx, m_name);

			/* Check the visibility */
			visible = m_ptr->ml;

			/* Note the collision */
			hit_body = TRUE;

			/* Did we hit it (penalize range) */
			if (test_hit_fire(chance - cur_dis, m_ptr, m_ptr->ml, o_name))
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
						if (!p_ptr->image) monster_race_track(m_ptr->ap_r_idx);
						health_track(g_ptr->m_idx);
					}
				}

				/* Hack -- Base damage from thrown object */
				dd = q_ptr->dd;
				ds = q_ptr->ds;
				torch_dice(q_ptr, &dd, &ds); /* throwing a torch */
				tdam = damroll(dd, ds);
				/* Apply special damage */
				tdam = tot_dam_aux(q_ptr, tdam, m_ptr, 0, TRUE);
				tdam = critical_shot(q_ptr->weight, q_ptr->to_h, 0, tdam);
				if (q_ptr->to_d > 0)
					tdam += q_ptr->to_d;
				else
					tdam += -q_ptr->to_d;

				if (boomerang)
				{
					tdam *= (mult+p_ptr->num_blow[item - INVEN_RARM]);
					tdam += p_ptr->to_d_m;
				}
				else if (have_flag(flgs, TR_THROW))
				{
					tdam *= (3+mult);
					tdam += p_ptr->to_d_m;
				}
				else
				{
					tdam *= mult;
				}
				if (shuriken)
				{
					tdam += ((p_ptr->lev+30)*(p_ptr->lev+30)-900)/55;
				}

				/* No negative damage */
				if (tdam < 0) tdam = 0;

				/* Modify the damage */
				tdam = mon_damage_mod(m_ptr, tdam, FALSE);

				msg_format_wizard(CHEAT_MONSTER, _("%dのダメージを与えた。(残りHP %d/%d(%d))", "You do %d damage. (left HP %d/%d(%d))"),
					tdam, m_ptr->hp - tdam, m_ptr->maxhp, m_ptr->max_maxhp);

				/* Hit the monster, check for death */
				if (mon_take_hit(g_ptr->m_idx, tdam, &fear, extract_note_dies(real_r_idx(m_ptr))))
				{
					/* Dead monster */
				}

				/* No death */
				else
				{
					message_pain(g_ptr->m_idx, tdam);

					/* Anger the monster */
					if ((tdam > 0) && !object_is_potion(q_ptr))
						anger_monster(m_ptr);

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
	j = (hit_body ? breakage_chance(q_ptr, 0) : 0);

	/* Figurines transform */
	if ((q_ptr->tval == TV_FIGURINE) && !(p_ptr->inside_arena))
	{
		j = 100;

		if (!(summon_named_creature(0, y, x, q_ptr->pval, !(object_is_cursed(q_ptr)) ? PM_FORCE_PET : 0L)))
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

			if (potion_smash_effect(0, y, x, q_ptr->k_idx))
			{
				monster_type *m_ptr = &current_floor_ptr->m_list[current_floor_ptr->grid_array[y][x].m_idx];
				if (current_floor_ptr->grid_array[y][x].m_idx && is_friendly(m_ptr) && !MON_INVULNER(m_ptr))
				{
					GAME_TEXT m_name[MAX_NLEN];
					monster_desc(m_name, m_ptr, 0);
					msg_format(_("%sは怒った！", "%^s gets angry!"), m_name);
					set_hostile(&current_floor_ptr->m_list[current_floor_ptr->grid_array[y][x].m_idx]);
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
		int back_chance = randint1(30)+20+((int)(adj_dex_th[p_ptr->stat_ind[A_DEX]]) - 128);
		char o2_name[MAX_NLEN];
		bool super_boomerang = (((q_ptr->name1 == ART_MJOLLNIR) || (q_ptr->name1 == ART_AEGISFANG)) && boomerang);

		j = -1;
		if (boomerang) back_chance += 4+randint1(5);
		if (super_boomerang) back_chance += 100;
		object_desc(o2_name, q_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));

		if((back_chance > 30) && (!one_in_(100) || super_boomerang))
		{
			for (i = cur_dis - 1; i > 0; i--)
			{
				if (panel_contains(ny[i], nx[i]) && player_can_see_bold(ny[i], nx[i]))
				{
					char c = object_char(q_ptr);
					byte a = object_attr(q_ptr);

					/* Draw, Hilite, Fresh, Pause, Erase */
					print_rel(c, a, ny[i], nx[i]);
					move_cursor_relative(ny[i], nx[i]);
					Term_fresh();
					Term_xtra(TERM_XTRA_DELAY, msec);
					lite_spot(ny[i], nx[i]);
					Term_fresh();
				}
				else
				{
					/* Pause anyway, for consistancy */
					Term_xtra(TERM_XTRA_DELAY, msec);
				}
			}
			if((back_chance > 37) && !p_ptr->blind && (item >= 0))
			{
				msg_format(_("%sが手元に返ってきた。", "%s comes back to you."), o2_name);
				come_back = TRUE;
			}
			else
			{
				if (item >= 0)
				{
					msg_format(_("%sを受け損ねた！", "%s backs, but you can't catch!"), o2_name);
				}
				else
				{
					msg_format(_("%sが返ってきた。", "%s comes back."), o2_name);
				}
				y = p_ptr->y;
				x = p_ptr->x;
			}
		}
		else
		{
			msg_format(_("%sが返ってこなかった！", "%s doesn't back!"), o2_name);
		}
	}

	if (come_back)
	{
		if (item == INVEN_RARM || item == INVEN_LARM)
		{
			/* Access the wield slot */
			o_ptr = &p_ptr->inventory_list[item];

			/* Wear the new stuff */
			object_copy(o_ptr, q_ptr);

			p_ptr->total_weight += q_ptr->weight;

			/* Increment the equip counter by hand */
			p_ptr->equip_cnt++;

			p_ptr->update |= (PU_BONUS | PU_TORCH | PU_MANA);
			p_ptr->window |= (PW_EQUIP);
		}
		else
		{
			inven_carry(q_ptr);
		}
		do_drop = FALSE;
	}
	else if (equiped_item)
	{
		kamaenaoshi(item);
		calc_android_exp();
	}

	if (do_drop)
	{
		if (cave_have_flag_bold(y, x, FF_PROJECT))
		{
			(void)drop_near(q_ptr, j, y, x);
		}
		else
		{
			(void)drop_near(q_ptr, j, prev_y, prev_x);
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
void do_cmd_suicide(void)
{
	int i;

	/* Flush input */
	flush();

	/* Verify Retirement */
	if (p_ptr->total_winner)
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


	if (!p_ptr->noscore)
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
	if (p_ptr->last_message) string_free(p_ptr->last_message);
	p_ptr->last_message = NULL;

	/* Hack -- Note *winning* message */
	if (p_ptr->total_winner && last_words)
	{
		char buf[1024] = "";
		play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_WINNER);
		do
		{
			while (!get_string(_("*勝利*メッセージ: ", "*Winning* message: "), buf, sizeof buf));
		} while (!get_check_strict(_("よろしいですか？", "Are you sure? "), CHECK_NO_HISTORY));

		if (buf[0])
		{
			p_ptr->last_message = string_make(buf);
			msg_print(p_ptr->last_message);
		}
	}

	/* Stop playing */
	p_ptr->playing = FALSE;

	/* Kill the player */
	p_ptr->is_dead = TRUE;
	p_ptr->leaving = TRUE;

	if (!p_ptr->total_winner)
	{
		do_cmd_write_nikki(NIKKI_BUNSHOU, 0, _("ダンジョンの探索に絶望して自殺した。", "give up all hope to commit suicide."));
		do_cmd_write_nikki(NIKKI_GAMESTART, 1, _("-------- ゲームオーバー --------", "--------   Game  Over   --------"));
		do_cmd_write_nikki(NIKKI_BUNSHOU, 1, "\n\n\n\n");
	}

	/* Cause of death */
	(void)strcpy(p_ptr->died_from, _("途中終了", "Quitting"));
}
