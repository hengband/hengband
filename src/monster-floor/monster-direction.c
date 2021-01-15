/*!
 * @brief モンスターの移動方向を決定する処理
 * @date 2020/03/08
 * @author Hourier
 */

#include "monster-floor/monster-direction.h"
#include "floor/cave.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-floor/monster-sweep-grid.h"
#include "monster/monster-processor-util.h"
#include "monster/monster-status.h"
#include "monster/monster-info.h"
#include "pet/pet-util.h"
#include "spell/range-calc.h"
#include "system/floor-type-definition.h"
#include "target/projection-path-calculator.h"
#include "player/player-status-flags.h"

/*!
 * @brief ペットが敵に接近するための方向を決定する
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_ptr 移動を試みているモンスターへの参照ポインタ
 * @param t_ptr 移動先モンスターへの参照ポインタ
 * @param plus モンスターIDの増減 (1/2 の確率で+1、1/2の確率で-1)
 * @return ペットがモンスターに近づくならばTRUE
 */
static bool decide_pet_approch_direction(player_type *target_ptr, monster_type *m_ptr, monster_type *t_ptr)
{
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	if (!is_pet(m_ptr)) return FALSE;

	if (target_ptr->pet_follow_distance < 0)
	{
		if (t_ptr->cdis <= (0 - target_ptr->pet_follow_distance))
		{
			return TRUE;
		}
	}
	else if ((m_ptr->cdis < t_ptr->cdis) && (t_ptr->cdis > target_ptr->pet_follow_distance))
	{
		return TRUE;
	}

	return (r_ptr->aaf < t_ptr->cdis);
}


/*!
 * @brief モンスターが敵に接近するための方向を決定する
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @param start モンスターIDの開始
 * @param plus モンスターIDの増減 (1/2 の確率で+1、1/2の確率で-1)
 * @param y モンスターの移動方向Y
 * @param x モンスターの移動方向X
 * @return なし
 */
static void decide_enemy_approch_direction(player_type *target_ptr, MONSTER_IDX m_idx, int start, int plus, POSITION *y, POSITION *x)
{
	floor_type *floor_ptr = target_ptr->current_floor_ptr;
	monster_type *m_ptr = &floor_ptr->m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	for (int i = start; ((i < start + floor_ptr->m_max) && (i > start - floor_ptr->m_max)); i += plus)
	{
		MONSTER_IDX dummy = (i % floor_ptr->m_max);
		if (dummy == 0) continue;

		MONSTER_IDX t_idx = dummy;
		monster_type *t_ptr;
		t_ptr = &floor_ptr->m_list[t_idx];
		if (t_ptr == m_ptr) continue;
		if (!monster_is_valid(t_ptr)) continue;
		if (decide_pet_approch_direction(target_ptr, m_ptr, t_ptr)) continue;
		if (!are_enemies(target_ptr, m_ptr, t_ptr)) continue;

		if (((r_ptr->flags2 & RF2_PASS_WALL) && ((m_idx != target_ptr->riding) || has_pass_wall(target_ptr))) ||
			((r_ptr->flags2 & RF2_KILL_WALL) && (m_idx != target_ptr->riding)))
		{
			if (!in_disintegration_range(floor_ptr, m_ptr->fy, m_ptr->fx, t_ptr->fy, t_ptr->fx)) continue;
		}
		else
		{
			if (!projectable(target_ptr, m_ptr->fy, m_ptr->fx, t_ptr->fy, t_ptr->fx)) continue;
		}

		*y = t_ptr->fy;
		*x = t_ptr->fx;
		return;
	}
}


/*!
 * @brief モンスターが敵に接近するための方向を計算するメインルーチン
 * Calculate the direction to the next enemy
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx モンスターの参照ID
 * @param mm 移動するべき方角IDを返す参照ポインタ
 * @return 方向が確定した場合TRUE、接近する敵がそもそもいない場合FALSEを返す
 */
bool get_enemy_dir(player_type *target_ptr, MONSTER_IDX m_idx, int *mm)
{
	floor_type *floor_ptr = target_ptr->current_floor_ptr;
	monster_type *m_ptr = &floor_ptr->m_list[m_idx];

	POSITION x = 0, y = 0;
	if (target_ptr->riding_t_m_idx && player_bold(target_ptr, m_ptr->fy, m_ptr->fx))
	{
		y = floor_ptr->m_list[target_ptr->riding_t_m_idx].fy;
		x = floor_ptr->m_list[target_ptr->riding_t_m_idx].fx;
	}
	else if (is_pet(m_ptr) && target_ptr->pet_t_m_idx)
	{
		y = floor_ptr->m_list[target_ptr->pet_t_m_idx].fy;
		x = floor_ptr->m_list[target_ptr->pet_t_m_idx].fx;
	}
	else
	{
		int start;
		int plus = 1;
		if (target_ptr->phase_out)
		{
			start = randint1(floor_ptr->m_max - 1) + floor_ptr->m_max;
			if (randint0(2)) plus = -1;
		}
		else
		{
			start = floor_ptr->m_max + 1;
		}

		decide_enemy_approch_direction(target_ptr, m_idx, start, plus, &y, &x);

		if ((x == 0) && (y == 0)) return FALSE;
	}

	x -= m_ptr->fx;
	y -= m_ptr->fy;

	store_enemy_approch_direction(mm, y, x);
	return TRUE;
}


/*!
 * todo ↓のように書いたが、"5"とはもしかして「その場に留まる」という意味か？
 * @brief 不規則歩行フラグを持つモンスターの移動方向をその確率に基づいて決定する
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param mm 移動方向
 * @param m_ptr モンスターへの参照ポインタ
 * @return 不規則な方向へ歩くことになったらTRUE
 */
static bool random_walk(player_type *target_ptr, DIRECTION *mm, monster_type *m_ptr)
{
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	if (((r_ptr->flags1 & (RF1_RAND_50 | RF1_RAND_25)) == (RF1_RAND_50 | RF1_RAND_25)) && (randint0(100) < 75))
	{
		if (is_original_ap_and_seen(target_ptr, m_ptr)) r_ptr->r_flags1 |= (RF1_RAND_50 | RF1_RAND_25);

		mm[0] = mm[1] = mm[2] = mm[3] = 5;
		return TRUE;
	}

	if ((r_ptr->flags1 & RF1_RAND_50) && (randint0(100) < 50))
	{
		if (is_original_ap_and_seen(target_ptr, m_ptr)) r_ptr->r_flags1 |= RF1_RAND_50;

		mm[0] = mm[1] = mm[2] = mm[3] = 5;
		return TRUE;
	}

	if ((r_ptr->flags1 & RF1_RAND_25) && (randint0(100) < 25))
	{
		if (is_original_ap_and_seen(target_ptr, m_ptr)) r_ptr->r_flags1 |= RF1_RAND_25;

		mm[0] = mm[1] = mm[2] = mm[3] = 5;
		return TRUE;
	}

	return FALSE;
}


/*!
 * @brief ペットや友好的なモンスターがフロアから逃げる処理を行う
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param mm 移動方向
 * @param m_idx モンスターID
 * @return モンスターがペットであればTRUE
 */
static bool decide_pet_movement_direction(player_type *target_ptr, DIRECTION *mm, MONSTER_IDX m_idx)
{
	monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
	if (!is_pet(m_ptr)) return FALSE;

	bool avoid = ((target_ptr->pet_follow_distance < 0) && (m_ptr->cdis <= (0 - target_ptr->pet_follow_distance)));
	bool lonely = (!avoid && (m_ptr->cdis > target_ptr->pet_follow_distance));
	bool distant = (m_ptr->cdis > PET_SEEK_DIST);
	mm[0] = mm[1] = mm[2] = mm[3] = 5;
	if (get_enemy_dir(target_ptr, m_idx, mm)) return TRUE;
	if (!avoid && !lonely && !distant) return TRUE;

	POSITION dis = target_ptr->pet_follow_distance;
	if (target_ptr->pet_follow_distance > PET_SEEK_DIST)
	{
		target_ptr->pet_follow_distance = PET_SEEK_DIST;
	}

	(void)get_movable_grid(target_ptr, m_idx, mm);
	target_ptr->pet_follow_distance = (s16b)dis;
	return TRUE;
}


/*!
 * @brief モンスターの移動パターンを決定する
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param mm 移動方向
 * @param m_idx モンスターID
 * @param aware モンスターがプレーヤーに気付いているならばTRUE、超隠密状態ならばFALSE
 * @return 移動先が存在すればTRUE
 */
bool decide_monster_movement_direction(player_type *target_ptr, DIRECTION *mm, MONSTER_IDX m_idx, bool aware)
{
	monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	if (monster_confused_remaining(m_ptr) || !aware)
	{
		mm[0] = mm[1] = mm[2] = mm[3] = 5;
		return TRUE;
	}

	if (random_walk(target_ptr, mm, m_ptr)) return TRUE;

	if ((r_ptr->flags1 & RF1_NEVER_MOVE) && (m_ptr->cdis > 1))
	{
		mm[0] = mm[1] = mm[2] = mm[3] = 5;
		return TRUE;
	}

	if (decide_pet_movement_direction(target_ptr, mm, m_idx)) return TRUE;

	if (!is_hostile(m_ptr))
	{
		mm[0] = mm[1] = mm[2] = mm[3] = 5;
		get_enemy_dir(target_ptr, m_idx, mm);
		return TRUE;
	}

	if (!get_movable_grid(target_ptr, m_idx, mm)) return FALSE;

	return TRUE;
}
