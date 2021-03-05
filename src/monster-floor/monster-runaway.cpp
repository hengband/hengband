﻿/*!
 * @brief モンスターの逃走に関する処理
 * @date 2020/03/08
 * @author Hourier
 */

#include "monster-floor/monster-runaway.h"
#include "core/disturbance.h"
#include "dungeon/quest-completion-checker.h"
#include "floor/cave.h"
#include "grid/grid.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster-floor/monster-remover.h"
#include "pet/pet-fall-off.h"
#include "system/floor-type-definition.h"
#include "target/projection-path-calculator.h"
#include "view/display-messages.h"

 /*!
  * @brief HPが1/3未満になった有効的なユニークモンスターの逃走処理を行う
  * @param target_ptr プレーヤーへの参照ポインタ
  * @param is_riding_mon 騎乗状態ならばTRUE
  * @param m_ptr モンスターへの参照ポインタ
  * @param m_name モンスター名称
  * @param see_m モンスターが視界内にいたらTRUE
  * @return なし
  */
static void escape_monster(player_type *target_ptr, turn_flags *turn_flags_ptr, monster_type *m_ptr, GAME_TEXT *m_name)
{
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	if (turn_flags_ptr->is_riding_mon)
	{
		msg_format(_("%sはあなたの束縛から脱出した。", "%^s succeeded to escape from your restriction!"), m_name);
		if (process_fall_off_horse(target_ptr, -1, FALSE))
		{
			msg_print(_("地面に落とされた。", "You have fallen from the pet you were riding."));
		}
	}

	if (turn_flags_ptr->see_m)
	{
		if ((r_ptr->flags2 & RF2_CAN_SPEAK) && (m_ptr->r_idx != MON_GRIP) && (m_ptr->r_idx != MON_WOLF) && (m_ptr->r_idx != MON_FANG) &&
			player_has_los_bold(target_ptr, m_ptr->fy, m_ptr->fx) && projectable(target_ptr, m_ptr->fy, m_ptr->fx, target_ptr->y, target_ptr->x))
		{
			msg_format(_("%^s「ピンチだ！退却させてもらう！」", "%^s says 'It is the pinch! I will retreat'."), m_name);
		}

		msg_format(_("%^sがテレポート・レベルの巻物を読んだ。", "%^s reads a scroll of teleport level."), m_name);
		msg_format(_("%^sが消え去った。", "%^s disappears."), m_name);
	}

	if (turn_flags_ptr->is_riding_mon && process_fall_off_horse(target_ptr, -1, FALSE))
	{
		msg_print(_("地面に落とされた。", "You have fallen from the pet you were riding."));
	}
}


/*!
 * @brief ペットや友好的なモンスターがフロアから逃げる処理を行う
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @param is_riding_mon 騎乗状態ならばTRUE
 * @param see_m モンスターが視界内にいたらTRUE
 * @return モンスターがフロアから消えたらTRUE
 */
bool runaway_monster(player_type *target_ptr, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx)
{
	monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	bool can_runaway = is_pet(m_ptr) || is_friendly(m_ptr);
	can_runaway &= ((r_ptr->flags1 & RF1_UNIQUE) != 0) || ((r_ptr->flags7 & RF7_NAZGUL) != 0);
	can_runaway &= !target_ptr->phase_out;
	if (!can_runaway) return FALSE;

	static int riding_pinch = 0;

	if (m_ptr->hp >= m_ptr->maxhp / 3)
	{
		/* Reset the counter */
		if (turn_flags_ptr->is_riding_mon) riding_pinch = 0;

		return FALSE;
	}

	GAME_TEXT m_name[MAX_NLEN];
	monster_desc(target_ptr, m_name, m_ptr, 0);
	if (turn_flags_ptr->is_riding_mon && riding_pinch < 2)
	{
		msg_format(_("%sは傷の痛さの余りあなたの束縛から逃れようとしている。",
			"%^s seems to be in so much pain and tries to escape from your restriction."), m_name);
		riding_pinch++;
		disturb(target_ptr, TRUE, TRUE);
		return FALSE;
	}

	escape_monster(target_ptr, turn_flags_ptr, m_ptr, m_name);
	check_quest_completion(target_ptr, m_ptr);
	delete_monster_idx(target_ptr, m_idx);
	return TRUE;
}
