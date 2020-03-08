/*!
 * @brief モンスターの移動に関する処理
 * @date 2020/03/08
 * @author Hourier
 */

#include "monster/monster-move.h"
#include "cmd/cmd-pet.h"
#include "monster-status.h"
#include "player-move.h"

static bool check_hp_for_feat_destruction(feature_type *f_ptr, monster_type *m_ptr)
{
	return !have_flag(f_ptr->flags, FF_GLASS) ||
		(r_info[m_ptr->r_idx].flags2 & RF2_STUPID) ||
		(m_ptr->hp >= MAX(m_ptr->maxhp / 3, 200));
}


/*!
  * @brief モンスターによる壁の透過・破壊を行う
  * @param target_ptr プレーヤーへの参照ポインタ
  * @param m_ptr モンスターへの参照ポインタ
  * @param ny モンスターのY座標
  * @param nx モンスターのX座標
  * @param can_cross モンスターが地形を踏破できるならばTRUE
  * @return 透過も破壊もしなかった場合はFALSE、それ以外はTRUE
  */
bool process_wall(player_type *target_ptr, turn_flags *turn_flags_ptr, monster_type *m_ptr, POSITION ny, POSITION nx, bool can_cross)
{
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	grid_type *g_ptr;
	g_ptr = &target_ptr->current_floor_ptr->grid_array[ny][nx];
	feature_type *f_ptr;
	f_ptr = &f_info[g_ptr->feat];
	if (player_bold(target_ptr, ny, nx))
	{
		turn_flags_ptr->do_move = TRUE;
		return TRUE;
	}

	if (g_ptr->m_idx > 0)
	{
		turn_flags_ptr->do_move = TRUE;
		return TRUE;
	}

	if (((r_ptr->flags2 & RF2_KILL_WALL) != 0) &&
		(can_cross ? !have_flag(f_ptr->flags, FF_LOS) : !turn_flags_ptr->is_riding_mon) &&
		have_flag(f_ptr->flags, FF_HURT_DISI) && !have_flag(f_ptr->flags, FF_PERMANENT) &&
		check_hp_for_feat_destruction(f_ptr, m_ptr))
	{
		turn_flags_ptr->do_move = TRUE;
		if (!can_cross) turn_flags_ptr->must_alter_to_move = TRUE;

		turn_flags_ptr->did_kill_wall = TRUE;
		return TRUE;
	}

	if (!can_cross) return FALSE;

	turn_flags_ptr->do_move = TRUE;
	if (((r_ptr->flags2 & RF2_PASS_WALL) != 0) && (!turn_flags_ptr->is_riding_mon || target_ptr->pass_wall) &&
		have_flag(f_ptr->flags, FF_CAN_PASS))
	{
		turn_flags_ptr->did_pass_wall = TRUE;
	}

	return TRUE;
}


/*!
 * @brief モンスターが普通のドアを開ける処理
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_ptr モンスターへの参照ポインタ
 * @param ny モンスターのY座標
 * @param nx モンスターのX座標
 * @return ここではドアを開けず、ガラスのドアを開ける可能性があるならTRUE
 */
static bool bash_normal_door(player_type *target_ptr, turn_flags *turn_flags_ptr, monster_type *m_ptr, POSITION ny, POSITION nx)
{
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	grid_type *g_ptr;
	g_ptr = &target_ptr->current_floor_ptr->grid_array[ny][nx];
	feature_type *f_ptr;
	f_ptr = &f_info[g_ptr->feat];
	turn_flags_ptr->do_move = FALSE;
	if (((r_ptr->flags2 & RF2_OPEN_DOOR) == 0) || !have_flag(f_ptr->flags, FF_OPEN) ||
		(is_pet(m_ptr) && ((target_ptr->pet_extra_flags & PF_OPEN_DOORS) == 0)))
		return TRUE;

	if (f_ptr->power == 0)
	{
		turn_flags_ptr->did_open_door = TRUE;
		turn_flags_ptr->do_turn = TRUE;
		return FALSE;
	}

	if (randint0(m_ptr->hp / 10) > f_ptr->power)
	{
		cave_alter_feat(target_ptr, ny, nx, FF_DISARM);
		turn_flags_ptr->do_turn = TRUE;
		return FALSE;
	}

	return TRUE;
}


/*!
 * @brief モンスターがガラスのドアを開ける処理
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_ptr モンスターへの参照ポインタ
 * @param g_ptr グリッドへの参照ポインタ
 * @param f_ptr 地形への参照ポインタ
 * @return なし
 */
static void bash_glass_door(player_type *target_ptr, turn_flags *turn_flags_ptr, monster_type *m_ptr, feature_type *f_ptr, bool may_bash)
{
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	if (!may_bash || ((r_ptr->flags2 & RF2_BASH_DOOR) == 0) || !have_flag(f_ptr->flags, FF_BASH) ||
		(is_pet(m_ptr) && ((target_ptr->pet_extra_flags & PF_OPEN_DOORS) == 0)))
		return;

	if (!check_hp_for_feat_destruction(f_ptr, m_ptr) || (randint0(m_ptr->hp / 10) <= f_ptr->power))
		return;

	if (have_flag(f_ptr->flags, FF_GLASS))
		msg_print(_("ガラスが砕ける音がした！", "You hear glass breaking!"));
	else
		msg_print(_("ドアを叩き開ける音がした！", "You hear a door burst open!"));

	if (disturb_minor) disturb(target_ptr, FALSE, FALSE);

	turn_flags_ptr->did_bash_door = TRUE;
	turn_flags_ptr->do_move = TRUE;
	turn_flags_ptr->must_alter_to_move = TRUE;
}


/*!
 * @brief モンスターによるドアの開放・破壊を行う
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_ptr モンスターへの参照ポインタ
 * @param ny モンスターのY座標
 * @param nx モンスターのX座標
 * @return モンスターが死亡した場合のみFALSE
 */
bool process_door(player_type *target_ptr, turn_flags *turn_flags_ptr, monster_type *m_ptr, POSITION ny, POSITION nx)
{
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	grid_type *g_ptr;
	g_ptr = &target_ptr->current_floor_ptr->grid_array[ny][nx];
	if (!is_closed_door(target_ptr, g_ptr->feat)) return TRUE;

	feature_type *f_ptr;
	f_ptr = &f_info[g_ptr->feat];
	bool may_bash = bash_normal_door(target_ptr, turn_flags_ptr, m_ptr, ny, nx);
	bash_glass_door(target_ptr, turn_flags_ptr, m_ptr, f_ptr, may_bash);

	if (!turn_flags_ptr->did_open_door && !turn_flags_ptr->did_bash_door) return TRUE;

	if (turn_flags_ptr->did_bash_door &&
		((randint0(100) < 50) || (feat_state(target_ptr, g_ptr->feat, FF_OPEN) == g_ptr->feat) || have_flag(f_ptr->flags, FF_GLASS)))
	{
		cave_alter_feat(target_ptr, ny, nx, FF_BASH);
		if (!monster_is_valid(m_ptr))
		{
			target_ptr->update |= (PU_FLOW);
			target_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
			if (is_original_ap_and_seen(target_ptr, m_ptr)) r_ptr->r_flags2 |= (RF2_BASH_DOOR);

			return FALSE;
		}
	}
	else
	{
		cave_alter_feat(target_ptr, ny, nx, FF_OPEN);
	}

	f_ptr = &f_info[g_ptr->feat];
	turn_flags_ptr->do_view = TRUE;
	return TRUE;
}


/*!
 * @brief モンスターが壁を掘った後続処理を実行する
 * @param target_ptr プレーヤーへの参照ポインタ
 * @turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_ptr モンスターへの参照ポインタ
 * @param ny モンスターのY座標
 * @param nx モンスターのX座標
 * @return モンスターが死亡した場合のみFALSE
 */
bool process_post_dig_wall(player_type *target_ptr, turn_flags *turn_flags_ptr, monster_type *m_ptr, POSITION ny, POSITION nx)
{
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	grid_type *g_ptr;
	g_ptr = &target_ptr->current_floor_ptr->grid_array[ny][nx];
	feature_type *f_ptr;
	f_ptr = &f_info[g_ptr->feat];
	if (!turn_flags_ptr->did_kill_wall || !turn_flags_ptr->do_move) return TRUE;

	if (one_in_(GRINDNOISE))
	{
		if (have_flag(f_ptr->flags, FF_GLASS))
			msg_print(_("何かの砕ける音が聞こえる。", "There is a crashing sound."));
		else
			msg_print(_("ギシギシいう音が聞こえる。", "There is a grinding sound."));
	}

	cave_alter_feat(target_ptr, ny, nx, FF_HURT_DISI);

	if (!monster_is_valid(m_ptr))
	{
		target_ptr->update |= (PU_FLOW);
		target_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
		if (is_original_ap_and_seen(target_ptr, m_ptr)) r_ptr->r_flags2 |= (RF2_KILL_WALL);

		return FALSE;
	}

	f_ptr = &f_info[g_ptr->feat];
	turn_flags_ptr->do_view = TRUE;
	turn_flags_ptr->do_turn = TRUE;
	return TRUE;
}


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
