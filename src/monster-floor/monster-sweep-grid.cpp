/*!
 * @brief モンスターの移動方向を走査する処理
 * @date 2020/03/08
 * @author Hourier
 */

#include "monster-floor/monster-sweep-grid.h"
#include "floor/cave.h"
#include "floor/line-of-sight.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-ability1.h"
#include "monster-race/race-flags-ability2.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags4.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster-floor/monster-safety-hiding.h"
#include "monster/monster-status.h"
#include "mspell/mspell-mask-definitions.h"
#include "system/floor-type-definition.h"
#include "target/projection-path-calculator.h"
#include "player/player-status-flags.h"

 /*!
  * @brief モンスターがプレイヤーから逃走するかどうかを返す /
  * Returns whether a given monster will try to run from the player.
  * @param m_idx 逃走するモンスターの参照ID
  * @return モンスターがプレイヤーから逃走するならばTRUEを返す。
  * @details
  * Monsters will attempt to avoid very powerful players.  See below.\n
  *\n
  * Because this function is called so often, little details are important\n
  * for efficiency.  Like not using "mod" or "div" when possible.  And\n
  * attempting to check the conditions in an optimal order.  Note that\n
  * "(x << 2) == (x * 4)" if "x" has enough bits to hold the result.\n
  *\n
  * Note that this function is responsible for about one to five percent\n
  * of the processor use in normal conditions...\n
  */
static bool mon_will_run(player_type *target_ptr, MONSTER_IDX m_idx)
{
	monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	if (is_pet(m_ptr))
	{
		return ((target_ptr->pet_follow_distance < 0) &&
			(m_ptr->cdis <= (0 - target_ptr->pet_follow_distance)));
	}

	if (m_ptr->cdis > MAX_SIGHT + 5) return FALSE;
	if (monster_fear_remaining(m_ptr)) return TRUE;
	if (m_ptr->cdis <= 5) return FALSE;

	PLAYER_LEVEL p_lev = target_ptr->lev;
	DEPTH m_lev = r_ptr->level + (m_idx & 0x08) + 25;
	if (m_lev > p_lev + 4) return FALSE;
	if (m_lev + 4 <= p_lev) return TRUE;

	HIT_POINT p_chp = target_ptr->chp;
	HIT_POINT p_mhp = target_ptr->mhp;
	HIT_POINT m_chp = m_ptr->hp;
	HIT_POINT m_mhp = m_ptr->maxhp;
	u32b p_val = (p_lev * p_mhp) + (p_chp << 2);
	u32b m_val = (m_lev * m_mhp) + (m_chp << 2);
	if (p_val * m_mhp > m_val * p_mhp) return TRUE;

	return FALSE;
}


/*!
 * @brief モンスターがプレイヤーに向けて遠距離攻撃を行うことが可能なマスを走査する /
 * Search spell castable grid
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx モンスターの参照ID
 * @param yp 適したマスのY座標を返す参照ポインタ
 * @param xp 適したマスのX座標を返す参照ポインタ
 * @return 有効なマスがあった場合TRUEを返す
 */
static bool sweep_ranged_attack_grid(player_type *target_ptr, MONSTER_IDX m_idx, POSITION *yp, POSITION *xp)
{
	floor_type *floor_ptr = target_ptr->current_floor_ptr;
	monster_type *m_ptr = &floor_ptr->m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	POSITION y1 = m_ptr->fy;
	POSITION x1 = m_ptr->fx;

	if (projectable(target_ptr, y1, x1, target_ptr->y, target_ptr->x)) return FALSE;

	int now_cost = floor_ptr->grid_array[y1][x1].cost;
	if (now_cost == 0) now_cost = 999;

	bool can_open_door = FALSE;
	if (r_ptr->flags2 & (RF2_BASH_DOOR | RF2_OPEN_DOOR))
	{
		can_open_door = TRUE;
	}

	int best = 999;
	for (int i = 7; i >= 0; i--)
	{
		POSITION y = y1 + ddy_ddd[i];
		POSITION x = x1 + ddx_ddd[i];
		if (!in_bounds2(floor_ptr, y, x)) continue;
		if (player_bold(target_ptr, y, x)) return FALSE;

		grid_type *g_ptr;
		g_ptr = &floor_ptr->grid_array[y][x];
		int cost = g_ptr->cost;
		if (!(((r_ptr->flags2 & RF2_PASS_WALL) && ((m_idx != target_ptr->riding) || has_pass_wall(target_ptr))) || ((r_ptr->flags2 & RF2_KILL_WALL) && (m_idx != target_ptr->riding))))
		{
			if (cost == 0) continue;
			if (!can_open_door && is_closed_door(target_ptr, g_ptr->feat)) continue;
		}

		if (cost == 0) cost = 998;

		if (now_cost < cost) continue;
		if (!projectable(target_ptr, y, x, target_ptr->y, target_ptr->x)) continue;
		if (best < cost) continue;

		best = cost;
		*yp = y1 + ddy_ddd[i];
		*xp = x1 + ddx_ddd[i];
	}

	if (best == 999) return FALSE;

	return TRUE;
}


/*!
 * @brief モンスターがプレイヤーに向けて接近することが可能なマスを走査する /
 * Choose the "best" direction for "flowing"
 * @param m_idx モンスターの参照ID
 * @param yp 移動先のマスのY座標を返す参照ポインタ
 * @param xp 移動先のマスのX座標を返す参照ポインタ
 * @param no_flow モンスターにFLOWフラグが経っていない状態でTRUE
 * @return なし
 * @details
 * Note that ghosts and rock-eaters are never allowed to "flow",\n
 * since they should move directly towards the player.\n
 *\n
 * Prefer "non-diagonal" directions, but twiddle them a little\n
 * to angle slightly towards the player's actual location.\n
 *\n
 * Allow very perceptive monsters to track old "spoor" left by\n
 * previous locations occupied by the player.  This will tend\n
 * to have monsters end up either near the player or on a grid\n
 * recently occupied by the player (and left via "teleport").\n
 *\n
 * Note that if "smell" is turned on, all monsters get vicious.\n
 *\n
 * Also note that teleporting away from a location will cause\n
 * the monsters who were chasing you to converge on that location\n
 * as long as you are still near enough to "annoy" them without\n
 * being close enough to chase directly.  I have no idea what will\n
 * happen if you combine "smell" with low "aaf" values.\n
 */
static void sweep_movable_grid(player_type *target_ptr, MONSTER_IDX m_idx, POSITION *yp, POSITION *xp, bool no_flow)
{
	grid_type *g_ptr;
	floor_type *floor_ptr = target_ptr->current_floor_ptr;
	monster_type *m_ptr = &floor_ptr->m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	if (r_ptr->flags4 & (RF4_ATTACK_MASK) ||
		r_ptr->a_ability_flags1 & (RF5_ATTACK_MASK) ||
		r_ptr->a_ability_flags2 & (RF6_ATTACK_MASK))
	{
		if (sweep_ranged_attack_grid(target_ptr, m_idx, yp, xp)) return;
	}

	if (no_flow) return;
	if ((r_ptr->flags2 & RF2_PASS_WALL) && ((m_idx != target_ptr->riding) || has_pass_wall(target_ptr))) return;
	if ((r_ptr->flags2 & RF2_KILL_WALL) && (m_idx != target_ptr->riding)) return;

	POSITION y1 = m_ptr->fy;
	POSITION x1 = m_ptr->fx;
	if (player_has_los_bold(target_ptr, y1, x1) && projectable(target_ptr, target_ptr->y, target_ptr->x, y1, x1)) return;

	g_ptr = &floor_ptr->grid_array[y1][x1];

	int best;
	bool use_scent = FALSE;
	if (g_ptr->cost)
	{
		best = 999;
	}
	else if (g_ptr->when)
	{
		if (floor_ptr->grid_array[target_ptr->y][target_ptr->x].when - g_ptr->when > 127) return;

		use_scent = TRUE;
		best = 0;
	}
	else
	{
		return;
	}

	for (int i = 7; i >= 0; i--)
	{
		POSITION y = y1 + ddy_ddd[i];
		POSITION x = x1 + ddx_ddd[i];

		if (!in_bounds2(floor_ptr, y, x)) continue;

		g_ptr = &floor_ptr->grid_array[y][x];
		if (use_scent)
		{
			int when = g_ptr->when;
			if (best > when) continue;

			best = when;
		}
		else
		{
			int cost;
			if (r_ptr->flags2 & (RF2_BASH_DOOR | RF2_OPEN_DOOR))
			{
				cost = g_ptr->dist;
			}
			else
			{
				cost = g_ptr->cost;
			}

			if ((cost == 0) || (best < cost)) continue;

			best = cost;
		}

		*yp = target_ptr->y + 16 * ddy_ddd[i];
		*xp = target_ptr->x + 16 * ddx_ddd[i];
	}
}


/*!
 * @brief モンスターがプレイヤーから逃走することが可能なマスを走査する /
 * Provide a location to flee to, but give the player a wide berth.
 * @param m_idx モンスターの参照ID
 * @param yp 移動先のマスのY座標を返す参照ポインタ
 * @param xp 移動先のマスのX座標を返す参照ポインタ
 * @return 有効なマスがあった場合TRUEを返す
 * @details
 * A monster may wish to flee to a location that is behind the player,\n
 * but instead of heading directly for it, the monster should "swerve"\n
 * around the player so that he has a smaller chance of getting hit.\n
 */
static bool sweep_runnable_away_grid(floor_type *floor_ptr, MONSTER_IDX m_idx, POSITION *yp, POSITION *xp)
{
	POSITION gy = 0, gx = 0;

	monster_type *m_ptr = &floor_ptr->m_list[m_idx];
	POSITION fy = m_ptr->fy;
	POSITION fx = m_ptr->fx;

	POSITION y1 = fy - (*yp);
	POSITION x1 = fx - (*xp);

	int score = -1;
	for (int i = 7; i >= 0; i--)
	{
		POSITION y = fy + ddy_ddd[i];
		POSITION x = fx + ddx_ddd[i];
		if (!in_bounds2(floor_ptr, y, x)) continue;

		POSITION dis = distance(y, x, y1, x1);
		POSITION s = 5000 / (dis + 3) - 500 / (floor_ptr->grid_array[y][x].dist + 1);
		if (s < 0) s = 0;

		if (s < score) continue;

		score = s;
		gy = y;
		gx = x;
	}

	if (score == -1) return FALSE;

	(*yp) = fy - gy;
	(*xp) = fx - gx;

	return TRUE;
}


/*!
 * todo 分割したいが条件が多すぎて適切な関数名と詳細処理を追いきれない……
 * @brief モンスターの移動方向を返す /
 * Choose "logical" directions for monster movement
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx モンスターの参照ID
 * @param mm 移動方向を返す方向IDの参照ポインタ
 * @return 有効方向があった場合TRUEを返す
 */
bool get_movable_grid(player_type *target_ptr, MONSTER_IDX m_idx, DIRECTION *mm)
{
	floor_type *floor_ptr = target_ptr->current_floor_ptr;
	monster_type *m_ptr = &floor_ptr->m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	POSITION y = 0, x = 0;
	POSITION y2 = target_ptr->y;
	POSITION x2 = target_ptr->x;
	bool done = FALSE;
	bool will_run = mon_will_run(target_ptr, m_idx);
	grid_type *g_ptr;
	bool no_flow = ((m_ptr->mflag2 & MFLAG2_NOFLOW) != 0) && (floor_ptr->grid_array[m_ptr->fy][m_ptr->fx].cost > 2);
	bool can_pass_wall = ((r_ptr->flags2 & RF2_PASS_WALL) != 0) && ((m_idx != target_ptr->riding) || has_pass_wall(target_ptr));

	if (!will_run && m_ptr->target_y)
	{
		int t_m_idx = floor_ptr->grid_array[m_ptr->target_y][m_ptr->target_x].m_idx;
		if ((t_m_idx > 0) &&
			are_enemies(target_ptr, m_ptr, &floor_ptr->m_list[t_m_idx]) &&
			los(target_ptr, m_ptr->fy, m_ptr->fx, m_ptr->target_y, m_ptr->target_x) &&
			projectable(target_ptr, m_ptr->fy, m_ptr->fx, m_ptr->target_y, m_ptr->target_x))
		{
			y = m_ptr->fy - m_ptr->target_y;
			x = m_ptr->fx - m_ptr->target_x;
			done = TRUE;
		}
	}

	if (!done && !will_run && is_hostile(m_ptr) &&
		(r_ptr->flags1 & RF1_FRIENDS) &&
		((los(target_ptr, m_ptr->fy, m_ptr->fx, target_ptr->y, target_ptr->x) && projectable(target_ptr, m_ptr->fy, m_ptr->fx, target_ptr->y, target_ptr->x)) ||
		(floor_ptr->grid_array[m_ptr->fy][m_ptr->fx].dist < MAX_SIGHT / 2)))
	{
		if ((r_ptr->flags3 & RF3_ANIMAL) && !can_pass_wall &&
			!(r_ptr->flags2 & RF2_KILL_WALL))
		{
			int room = 0;
			for (int i = 0; i < 8; i++)
			{
				int xx = target_ptr->x + ddx_ddd[i];
				int yy = target_ptr->y + ddy_ddd[i];

				if (!in_bounds2(floor_ptr, yy, xx)) continue;

				g_ptr = &floor_ptr->grid_array[yy][xx];
				if (monster_can_cross_terrain(target_ptr, g_ptr->feat, r_ptr, 0))
				{
					room++;
				}
			}

			if (floor_ptr->grid_array[target_ptr->y][target_ptr->x].info & CAVE_ROOM) room -= 2;
			if (!r_ptr->flags4 && !r_ptr->a_ability_flags1 && !r_ptr->a_ability_flags2) room -= 2;

			if (room < (8 * (target_ptr->chp + target_ptr->csp)) /
				(target_ptr->mhp + target_ptr->msp))
			{
				if (find_hiding(target_ptr, m_idx, &y, &x)) done = TRUE;
			}
		}

		if (!done && (floor_ptr->grid_array[m_ptr->fy][m_ptr->fx].dist < 3))
		{
			for (int i = 0; i < 8; i++)
			{
				y2 = target_ptr->y + ddy_ddd[(m_idx + i) & 7];
				x2 = target_ptr->x + ddx_ddd[(m_idx + i) & 7];
				if ((m_ptr->fy == y2) && (m_ptr->fx == x2))
				{
					y2 = target_ptr->y;
					x2 = target_ptr->x;
					break;
				}

				if (!in_bounds2(floor_ptr, y2, x2)) continue;
				if (!monster_can_enter(target_ptr, y2, x2, r_ptr, 0)) continue;

				break;
			}

			y = m_ptr->fy - y2;
			x = m_ptr->fx - x2;
			done = TRUE;
		}
	}

	if (!done)
	{
		sweep_movable_grid(target_ptr, m_idx, &y2, &x2, no_flow);
		y = m_ptr->fy - y2;
		x = m_ptr->fx - x2;
	}

	if (is_pet(m_ptr) && will_run)
	{
		y = (-y), x = (-x);
	}
	else
	{
		if (!done && will_run)
		{
			int tmp_x = (-x);
			int tmp_y = (-y);
			if (find_safety(target_ptr, m_idx, &y, &x) && !no_flow)
			{
				if (sweep_runnable_away_grid(target_ptr->current_floor_ptr, m_idx, &y, &x))
					done = TRUE;
			}

			if (!done)
			{
				y = tmp_y;
				x = tmp_x;
			}
		}
	}

	if (!x && !y) return FALSE;

	store_moves_val(mm, y, x);
	return TRUE;
}
