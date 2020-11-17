/*!
 * @brief モンスターの逃走・隠匿に関する処理
 * @date 2020/03/08
 * @author Hourier
 */

#include "monster-floor/monster-safety-hiding.h"
#include "floor/cave.h"
#include "grid/grid.h"
#include "monster-floor/monster-dist-offsets.h"
#include "monster-race/monster-race.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "mspell/mspell-checker.h"
#include "system/floor-type-definition.h"
#include "target/projection-path-calculator.h"

 /*!
  * @brief モンスターが逃げ込める地点を走査する
  * @param target_ptr プレーヤーへの参照ポインタ
  * @param m_idx モンスターID
  * @param y_offsets
  * @param x_offsets
  * @param d モンスターがいる地点からの距離
  * @return 逃げ込める地点の候補地
  */
static coordinate_candidate sweep_safe_coordinate(player_type *target_ptr, MONSTER_IDX m_idx, const POSITION *y_offsets, const POSITION *x_offsets, int d)
{
	coordinate_candidate candidate = init_coordinate_candidate();
	floor_type *floor_ptr = target_ptr->current_floor_ptr;
	monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
	for (POSITION i = 0, dx = x_offsets[0], dy = y_offsets[0];
		dx != 0 || dy != 0;
		i++, dx = x_offsets[i], dy = y_offsets[i])
	{
		POSITION y = m_ptr->fy + dy;
		POSITION x = m_ptr->fx + dx;
		if (!in_bounds(floor_ptr, y, x)) continue;

		grid_type *g_ptr;
		g_ptr = &floor_ptr->grid_array[y][x];

		BIT_FLAGS16 riding_mode = (m_idx == target_ptr->riding) ? CEM_RIDING : 0;
		if (!monster_can_cross_terrain(target_ptr, g_ptr->feat, &r_info[m_ptr->r_idx], riding_mode))
			continue;

		if (!(m_ptr->mflag2 & MFLAG2_NOFLOW))
		{
			if (g_ptr->dist == 0) continue;
			if (g_ptr->dist > floor_ptr->grid_array[m_ptr->fy][m_ptr->fx].dist + 2 * d) continue;
		}

		if (projectable(target_ptr, target_ptr->y, target_ptr->x, y, x)) continue;

		POSITION dis = distance(y, x, target_ptr->y, target_ptr->x);
		if (dis <= candidate.gdis) continue;

		candidate.gy = y;
		candidate.gx = x;
		candidate.gdis = dis;
	}

	return candidate;
}


/*!
 * @brief モンスターが逃げ込める安全な地点を返す /
 * Choose a "safe" location near a monster for it to run toward.
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx モンスターの参照ID
 * @param yp 移動先のマスのY座標を返す参照ポインタ
 * @param xp 移動先のマスのX座標を返す参照ポインタ
 * @return 有効なマスがあった場合TRUEを返す
 * @details
 * A location is "safe" if it can be reached quickly and the player\n
 * is not able to fire into it (it isn't a "clean shot").  So, this will\n
 * cause monsters to "duck" behind walls.  Hopefully, monsters will also\n
 * try to run towards corridor openings if they are in a room.\n
 *\n
 * This function may take lots of CPU time if lots of monsters are\n
 * fleeing.\n
 *\n
 * Return TRUE if a safe location is available.\n
 */
bool find_safety(player_type *target_ptr, MONSTER_IDX m_idx, POSITION *yp, POSITION *xp)
{
	monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
	for (POSITION d = 1; d < 10; d++)
	{
		const POSITION *y_offsets;
		y_offsets = dist_offsets_y[d];

		const POSITION *x_offsets;
		x_offsets = dist_offsets_x[d];

		coordinate_candidate candidate = sweep_safe_coordinate(target_ptr, m_idx, y_offsets, x_offsets, d);

		if (candidate.gdis <= 0) continue;

		*yp = m_ptr->fy - candidate.gy;
		*xp = m_ptr->fx - candidate.gx;

		return TRUE;
	}

	return FALSE;
}


/*!
 * @brief モンスターが隠れられる地点を走査する
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @param y_offsets
 * @param x_offsets
 * @param candidate 隠れられる地点の候補地
 * @return なし
 */
static void sweep_hiding_candidate(player_type *target_ptr, monster_type *m_ptr, const POSITION *y_offsets, const POSITION *x_offsets, coordinate_candidate *candidate)
{
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	for (POSITION i = 0, dx = x_offsets[0], dy = y_offsets[0];
		dx != 0 || dy != 0;
		i++, dx = x_offsets[i], dy = y_offsets[i])
	{
		POSITION y = m_ptr->fy + dy;
		POSITION x = m_ptr->fx + dx;
		if (!in_bounds(target_ptr->current_floor_ptr, y, x)) continue;
		if (!monster_can_enter(target_ptr, y, x, r_ptr, 0)) continue;
		if (projectable(target_ptr, target_ptr->y, target_ptr->x, y, x) && clean_shot(target_ptr, m_ptr->fy, m_ptr->fx, y, x, FALSE))
			continue;

		POSITION dis = distance(y, x, target_ptr->y, target_ptr->x);
		if (dis < candidate->gdis && dis >= 2)
		{
			candidate->gy = y;
			candidate->gx = x;
			candidate->gdis = dis;
		}
	}
}


/*!
 * @brief モンスターが隠れ潜める地点を返す /
 * Choose a good hiding place near a monster for it to run toward.
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx モンスターの参照ID
 * @param yp 移動先のマスのY座標を返す参照ポインタ
 * @param xp 移動先のマスのX座標を返す参照ポインタ
 * @return 有効なマスがあった場合TRUEを返す
 * @details
 * Pack monsters will use this to "ambush" the player and lure him out\n
 * of corridors into open space so they can swarm him.\n
 *\n
 * Return TRUE if a good location is available.\n
 */
bool find_hiding(player_type *target_ptr, MONSTER_IDX m_idx, POSITION *yp, POSITION *xp)
{
	monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
	coordinate_candidate candidate = init_coordinate_candidate();
	candidate.gdis = 999;

	for (POSITION d = 1; d < 10; d++)
	{
		const POSITION *y_offsets;
		y_offsets = dist_offsets_y[d];

		const POSITION *x_offsets;
		x_offsets = dist_offsets_x[d];

		sweep_hiding_candidate(target_ptr, m_ptr, y_offsets, x_offsets, &candidate);
		if (candidate.gdis >= 999) continue;

		*yp = m_ptr->fy - candidate.gy;
		*xp = m_ptr->fx - candidate.gx;
		return TRUE;
	}

	return FALSE;
}
