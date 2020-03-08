/*!
 * @brief モンスター情報のアップデート処理
 * @date 2020/03/08
 * @author Hourier
 */

#include "monster/monster-update.h"
#include "player-move.h"

/*!
 * @brief 騎乗中のモンスター情報を更新する
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_idx モンスターID
 * @param oy 移動前の、モンスターのY座標
 * @param ox 移動前の、モンスターのX座標
 * @param ny 移動後の、モンスターのY座標
 * @param ox 移動後の、モンスターのX座標
 * @return アイテム等に影響を及ぼしたらTRUE
 */
bool update_riding_monster(player_type *target_ptr, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx, POSITION oy, POSITION ox, POSITION ny, POSITION nx)
{
	monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
	grid_type *g_ptr;
	g_ptr = &target_ptr->current_floor_ptr->grid_array[ny][nx];
	monster_type *y_ptr;
	y_ptr = &target_ptr->current_floor_ptr->m_list[g_ptr->m_idx];
	if (turn_flags_ptr->is_riding_mon)
		return move_player_effect(target_ptr, ny, nx, MPE_DONT_PICKUP);

	target_ptr->current_floor_ptr->grid_array[oy][ox].m_idx = g_ptr->m_idx;
	if (g_ptr->m_idx)
	{
		y_ptr->fy = oy;
		y_ptr->fx = ox;
		update_monster(target_ptr, g_ptr->m_idx, TRUE);
	}

	g_ptr->m_idx = m_idx;
	m_ptr->fy = ny;
	m_ptr->fx = nx;
	update_monster(target_ptr, m_idx, TRUE);

	lite_spot(target_ptr, oy, ox);
	lite_spot(target_ptr, ny, nx);
	return TRUE;
}


/*!
 * @brief updateフィールドを更新する
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @return なし
 */
void update_player_type(player_type *target_ptr, turn_flags *turn_flags_ptr, monster_race *r_ptr)
{
	if (turn_flags_ptr->do_view)
	{
		target_ptr->update |= (PU_FLOW);
		target_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
	}

	if (turn_flags_ptr->do_move && ((r_ptr->flags7 & (RF7_SELF_LD_MASK | RF7_HAS_DARK_1 | RF7_HAS_DARK_2))
		|| ((r_ptr->flags7 & (RF7_HAS_LITE_1 | RF7_HAS_LITE_2)) && !target_ptr->phase_out)))
	{
		target_ptr->update |= (PU_MON_LITE);
	}
}


/*!
 * @brief モンスターのフラグを更新する
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_ptr モンスターへの参照ポインタ
 * @return なし
 */
void update_monster_race_flags(player_type *target_ptr, turn_flags *turn_flags_ptr, monster_type *m_ptr)
{
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	if (!is_original_ap_and_seen(target_ptr, m_ptr)) return;

	if (turn_flags_ptr->did_open_door) r_ptr->r_flags2 |= (RF2_OPEN_DOOR);
	if (turn_flags_ptr->did_bash_door) r_ptr->r_flags2 |= (RF2_BASH_DOOR);
	if (turn_flags_ptr->did_take_item) r_ptr->r_flags2 |= (RF2_TAKE_ITEM);
	if (turn_flags_ptr->did_kill_item) r_ptr->r_flags2 |= (RF2_KILL_ITEM);
	if (turn_flags_ptr->did_move_body) r_ptr->r_flags2 |= (RF2_MOVE_BODY);
	if (turn_flags_ptr->did_pass_wall) r_ptr->r_flags2 |= (RF2_PASS_WALL);
	if (turn_flags_ptr->did_kill_wall) r_ptr->r_flags2 |= (RF2_KILL_WALL);
}


/*!
 * @brief モンスターフラグの更新に基づき、モンスター表示を更新する
 * @param monster_race_idx モンスターID
 * @param window ウィンドウフラグ
 * @param old_race_flags_ptr モンスターフラグへの参照ポインタ
 * @return なし
 */
void update_player_window(player_type *target_ptr, old_race_flags *old_race_flags_ptr)
{
	monster_race *r_ptr;
	r_ptr = &r_info[target_ptr->monster_race_idx];
	if ((old_race_flags_ptr->old_r_flags1 != r_ptr->r_flags1) ||
		(old_race_flags_ptr->old_r_flags2 != r_ptr->r_flags2) ||
		(old_race_flags_ptr->old_r_flags3 != r_ptr->r_flags3) ||
		(old_race_flags_ptr->old_r_flags4 != r_ptr->r_flags4) ||
		(old_race_flags_ptr->old_r_flags5 != r_ptr->r_flags5) ||
		(old_race_flags_ptr->old_r_flags6 != r_ptr->r_flags6) ||
		(old_race_flags_ptr->old_r_flagsr != r_ptr->r_flagsr) ||
		(old_race_flags_ptr->old_r_blows0 != r_ptr->r_blows[0]) ||
		(old_race_flags_ptr->old_r_blows1 != r_ptr->r_blows[1]) ||
		(old_race_flags_ptr->old_r_blows2 != r_ptr->r_blows[2]) ||
		(old_race_flags_ptr->old_r_blows3 != r_ptr->r_blows[3]) ||
		(old_race_flags_ptr->old_r_cast_spell != r_ptr->r_cast_spell))
	{
		target_ptr->window |= (PW_MONSTER);
	}
}
