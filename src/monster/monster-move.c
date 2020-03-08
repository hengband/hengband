/*!
 * @brief モンスターの移動に関する処理
 * @date 2020/03/08
 * @author Hourier
 */

#include "monster/monster-move.h"

/*!
 * @brief 守りのルーンによるモンスターの移動制限を処理する
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_ptr モンスターへの参照ポインタ
 * @param ny モンスターのY座標
 * @param nx モンスターのX座標
 * @return ルーンのある/なし
 */
bool process_protection_rune(player_type *target_ptr, turn_flags *turn_flags_ptr, monster_type *m_ptr, POSITION ny, POSITION nx)
{
	grid_type *g_ptr;
	g_ptr = &target_ptr->current_floor_ptr->grid_array[ny][nx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	if (!turn_flags_ptr->do_move || !is_glyph_grid(g_ptr) ||
		(((r_ptr->flags1 & RF1_NEVER_BLOW) != 0) && player_bold(target_ptr, ny, nx)))
		return FALSE;

	turn_flags_ptr->do_move = FALSE;
	if (is_pet(m_ptr) || (randint1(BREAK_GLYPH) >= r_ptr->level))
		return TRUE;

	if (g_ptr->info & CAVE_MARK)
	{
		msg_print(_("守りのルーンが壊れた！", "The rune of protection is broken!"));
	}

	g_ptr->info &= ~(CAVE_MARK);
	g_ptr->info &= ~(CAVE_OBJECT);
	g_ptr->mimic = 0;
	turn_flags_ptr->do_move = TRUE;
	note_spot(target_ptr, ny, nx);
	return TRUE;
}


/*!
 * @brief 爆発のルーンを処理する
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_ptr モンスターへの参照ポインタ
 * @param ny モンスターのY座標
 * @param nx モンスターのX座標
 * @return モンスターが死亡した場合のみFALSE
 */
bool process_explosive_rune(player_type *target_ptr, turn_flags *turn_flags_ptr, monster_type *m_ptr, POSITION ny, POSITION nx)
{
	grid_type *g_ptr;
	g_ptr = &target_ptr->current_floor_ptr->grid_array[ny][nx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	if (!turn_flags_ptr->do_move || !is_explosive_rune_grid(g_ptr) ||
		(((r_ptr->flags1 & RF1_NEVER_BLOW) != 0) && player_bold(target_ptr, ny, nx)))
		return TRUE;

	turn_flags_ptr->do_move = FALSE;
	if (is_pet(m_ptr)) return TRUE;

	if (randint1(BREAK_MINOR_GLYPH) > r_ptr->level)
	{
		if (g_ptr->info & CAVE_MARK)
		{
			msg_print(_("ルーンが爆発した！", "The rune explodes!"));
			BIT_FLAGS project_flags = PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP | PROJECT_NO_HANGEKI;
			project(target_ptr, 0, 2, ny, nx, 2 * (target_ptr->lev + damroll(7, 7)), GF_MANA, project_flags, -1);
		}
	}
	else
	{
		msg_print(_("爆発のルーンは解除された。", "An explosive rune was disarmed."));
	}

	g_ptr->info &= ~(CAVE_MARK);
	g_ptr->info &= ~(CAVE_OBJECT);
	g_ptr->mimic = 0;

	note_spot(target_ptr, ny, nx);
	lite_spot(target_ptr, ny, nx);

	if (!monster_is_valid(m_ptr)) return FALSE;

	turn_flags_ptr->do_move = TRUE;
	return TRUE;
}
