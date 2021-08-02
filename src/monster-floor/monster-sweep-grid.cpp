/*!
 * @brief モンスターの移動方向を走査する処理
 * @date 2020/03/08
 * @author Hourier
 */

#include "monster-floor/monster-sweep-grid.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "floor/line-of-sight.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "monster-floor/monster-safety-hiding.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-ability-mask.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-processor-util.h"
#include "monster/monster-status.h"
#include "player/player-status-flags.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"
#include "util/bit-flags-calculator.h"

/*
 * @brief コンストラクタ
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx 逃走するモンスターの参照ID
 * @param mm 移動方向を返す方向IDの参照ポインタ
 */
MonsterSweepGrid::MonsterSweepGrid(player_type *target_ptr, MONSTER_IDX m_idx, DIRECTION *mm)
    : target_ptr(this->target_ptr)
    , m_idx(m_idx)
    , mm(mm)
{
}

/*!
 * @brief モンスターの移動方向を返す /
 * Choose "logical" directions for monster movement
 * @return 有効方向があった場合TRUEを返す
 * @todo 分割したいが条件が多すぎて適切な関数名と詳細処理を追いきれない……
 */
bool MonsterSweepGrid::get_movable_grid()
{
    auto *floor_ptr = this->target_ptr->current_floor_ptr;
    auto *m_ptr = &floor_ptr->m_list[this->m_idx];
    auto *r_ptr = &r_info[m_ptr->r_idx];
    POSITION y = 0;
    POSITION x = 0;
    auto y2 = this->target_ptr->y;
    auto x2 = this->target_ptr->x;
    this->will_run = this->mon_will_run();
    auto no_flow = m_ptr->mflag2.has(MFLAG2::NOFLOW) && grid_cost(&floor_ptr->grid_array[m_ptr->fy][m_ptr->fx], r_ptr) > 2;
    auto can_pass_wall = any_bits(r_ptr->flags2, RF2_PASS_WALL) && ((this->m_idx != this->target_ptr->riding) || has_pass_wall(this->target_ptr));
    if (!this->will_run && m_ptr->target_y) {
        int t_m_idx = floor_ptr->grid_array[m_ptr->target_y][m_ptr->target_x].m_idx;
        if ((t_m_idx > 0) && are_enemies(this->target_ptr, m_ptr, &floor_ptr->m_list[t_m_idx])
            && los(this->target_ptr, m_ptr->fy, m_ptr->fx, m_ptr->target_y, m_ptr->target_x)
            && projectable(this->target_ptr, m_ptr->fy, m_ptr->fx, m_ptr->target_y, m_ptr->target_x)) {
            y = m_ptr->fy - m_ptr->target_y;
            x = m_ptr->fx - m_ptr->target_x;
            this->done = true;
        }
    }

    this->check_hiding_grid(&y, &x, &y2, &x2);
    if (!this->done) {
        this->sweep_movable_grid(&y2, &x2, no_flow);
        y = m_ptr->fy - y2;
        x = m_ptr->fx - x2;
    }

    if (is_pet(m_ptr) && this->will_run) {
        y = (-y), x = (-x);
    } else {
        if (!this->done && this->will_run) {
            int tmp_x = (-x);
            int tmp_y = (-y);
            if (find_safety(this->target_ptr, this->m_idx, &y, &x) && !no_flow) {
                if (this->sweep_runnable_away_grid(&y, &x)) {
                    this->done = true;
                }
            }

            if (!this->done) {
                y = tmp_y;
                x = tmp_x;
            }
        }
    }

    if (!x && !y) {
        return false;
    }

    store_moves_val(this->mm, y, x);
    return true;
}

/*!
 * @brief モンスターがプレイヤーから逃走するかどうかを返す /
 * Returns whether a given monster will try to run from the player.
 * @return モンスターがプレイヤーから逃走するならばTRUEを返す。
 */
bool MonsterSweepGrid::mon_will_run()
{
    monster_type *m_ptr = &this->target_ptr->current_floor_ptr->m_list[this->m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];

    if (is_pet(m_ptr)) {
        return ((this->target_ptr->pet_follow_distance < 0) && (m_ptr->cdis <= (0 - this->target_ptr->pet_follow_distance)));
    }

    if (m_ptr->cdis > MAX_SIGHT + 5) {
        return false;
    }

    if (monster_fear_remaining(m_ptr)) {
        return true;
    }

    if (m_ptr->cdis <= 5) {
        return false;
    }

    auto p_lev = this->target_ptr->lev;
    auto m_lev = r_ptr->level + (this->m_idx & 0x08) + 25;
    if (m_lev > p_lev + 4) {
        return false;
    }

    if (m_lev + 4 <= p_lev) {
        return true;
    }

    auto p_chp = this->target_ptr->chp;
    auto p_mhp = this->target_ptr->mhp;
    auto m_chp = m_ptr->hp;
    auto m_mhp = m_ptr->maxhp;
    u32b p_val = (p_lev * p_mhp) + (p_chp << 2);
    u32b m_val = (m_lev * m_mhp) + (m_chp << 2);
    return p_val * m_mhp > m_val * p_mhp;
}

void MonsterSweepGrid::check_hiding_grid(POSITION *y, POSITION *x, POSITION *y2, POSITION *x2)
{
    auto *floor_ptr = this->target_ptr->current_floor_ptr;
    auto *m_ptr = &floor_ptr->m_list[this->m_idx];
    auto *r_ptr = &r_info[m_ptr->r_idx];
    if (this->done || this->will_run || !is_hostile(m_ptr) || none_bits(r_ptr->flags1, RF1_FRIENDS)) {
        return;
    }

    if ((!los(this->target_ptr, m_ptr->fy, m_ptr->fx, this->target_ptr->y, this->target_ptr->x)
            || !projectable(this->target_ptr, m_ptr->fy, m_ptr->fx, this->target_ptr->y, this->target_ptr->x))) {
        if (grid_dist(&floor_ptr->grid_array[m_ptr->fy][m_ptr->fx], r_ptr) >= MAX_SIGHT / 2) {
            return;
        }
    }

    this->search_room_to_run(y, x);
    if (this->done || (grid_dist(&floor_ptr->grid_array[m_ptr->fy][m_ptr->fx], r_ptr) >= 3)) {
        return;
    }

    for (auto i = 0; i < 8; i++) {
        *y2 = this->target_ptr->y + ddy_ddd[(this->m_idx + i) & 7];
        *x2 = this->target_ptr->x + ddx_ddd[(this->m_idx + i) & 7];
        if ((m_ptr->fy == *y2) && (m_ptr->fx == *x2)) {
            *y2 = this->target_ptr->y;
            *x2 = this->target_ptr->x;
            break;
        }

        if (!in_bounds2(floor_ptr, *y2, *x2) || !monster_can_enter(this->target_ptr, *y2, *x2, r_ptr, 0)) {
            continue;
        }

        break;
    }

    *y = m_ptr->fy - *y2;
    *x = m_ptr->fx - *x2;
    this->done = true;
}

void MonsterSweepGrid::search_room_to_run(POSITION *y, POSITION *x)
{
    auto *floor_ptr = this->target_ptr->current_floor_ptr;
    auto *r_ptr = &r_info[floor_ptr->m_list[this->m_idx].r_idx];
    if (none_bits(r_ptr->flags3, RF3_ANIMAL) || this->can_pass_wall || any_bits(r_ptr->flags2, RF2_KILL_WALL)) {
        return;
    }

    auto room = 0;
    for (auto i = 0; i < 8; i++) {
        auto xx = this->target_ptr->x + ddx_ddd[i];
        auto yy = this->target_ptr->y + ddy_ddd[i];
        if (!in_bounds2(floor_ptr, yy, xx)) {
            continue;
        }

        auto *g_ptr = &floor_ptr->grid_array[yy][xx];
        if (monster_can_cross_terrain(this->target_ptr, g_ptr->feat, r_ptr, 0)) {
            room++;
        }
    }

    if (any_bits(floor_ptr->grid_array[this->target_ptr->y][this->target_ptr->x].info, CAVE_ROOM)) {
        room -= 2;
    }

    if (r_ptr->ability_flags.none()) {
        room -= 2;
    }

    if (room >= (8 * (this->target_ptr->chp + this->target_ptr->csp)) / (this->target_ptr->mhp + this->target_ptr->msp)) {
        return;
    }

    if (find_hiding(this->target_ptr, this->m_idx, y, x)) {
        this->done = true;
    }
}

/*!
 * @brief モンスターがプレイヤーに向けて接近することが可能なマスを走査する /
 * Choose the "best" direction for "flowing"
 * @param yp 移動先のマスのY座標を返す参照ポインタ
 * @param xp 移動先のマスのX座標を返す参照ポインタ
 * @param no_flow モンスターにFLOWフラグが経っていない状態でTRUE
 */
void MonsterSweepGrid::sweep_movable_grid(POSITION *yp, POSITION *xp, bool no_flow)
{
    auto *floor_ptr = this->target_ptr->current_floor_ptr;
    auto *m_ptr = &floor_ptr->m_list[this->m_idx];
    auto *r_ptr = &r_info[m_ptr->r_idx];
    if ((r_ptr->ability_flags.has_any_of(RF_ABILITY_ATTACK_MASK)) && (sweep_ranged_attack_grid(yp, xp))) {
        return;
    }

    if (no_flow) {
        return;
    }

    if (any_bits(r_ptr->flags2, RF2_PASS_WALL) && ((this->m_idx != this->target_ptr->riding) || has_pass_wall(this->target_ptr))) {
        return;
    }

    if (any_bits(r_ptr->flags2, RF2_KILL_WALL) && (this->m_idx != this->target_ptr->riding)) {
        return;
    }

    auto y1 = m_ptr->fy;
    auto x1 = m_ptr->fx;
    auto *g_ptr = &floor_ptr->grid_array[y1][x1];
    if (player_has_los_bold(this->target_ptr, y1, x1) && projectable(this->target_ptr, this->target_ptr->y, this->target_ptr->x, y1, x1)) {
        if ((distance(y1, x1, this->target_ptr->y, this->target_ptr->x) == 1) || (r_ptr->freq_spell > 0) || (grid_cost(g_ptr, r_ptr) > 5)) {
            return;
        }
    }

    int best;
    auto use_scent = false;
    if (grid_cost(g_ptr, r_ptr)) {
        best = 999;
    } else if (g_ptr->when) {
        if (floor_ptr->grid_array[this->target_ptr->y][this->target_ptr->x].when - g_ptr->when > 127) {
            return;
        }

        use_scent = true;
        best = 0;
    } else {
        return;
    }

    for (auto i = 7; i >= 0; i--) {
        auto y = y1 + ddy_ddd[i];
        auto x = x1 + ddx_ddd[i];
        if (!in_bounds2(floor_ptr, y, x)) {
            continue;
        }

        g_ptr = &floor_ptr->grid_array[y][x];
        if (use_scent) {
            int when = g_ptr->when;
            if (best > when) {
                continue;
            }

            best = when;
        } else {
            auto cost = any_bits(r_ptr->flags2, RF2_BASH_DOOR | RF2_OPEN_DOOR) ? grid_dist(g_ptr, r_ptr) : grid_cost(g_ptr, r_ptr);
            if ((cost == 0) || (best < cost)) {
                continue;
            }

            best = cost;
        }

        *yp = this->target_ptr->y + 16 * ddy_ddd[i];
        *xp = this->target_ptr->x + 16 * ddx_ddd[i];
    }
}

/*!
 * @brief モンスターがプレイヤーに向けて遠距離攻撃を行うことが可能なマスを走査する /
 * Search spell castable grid
 * @param yp 適したマスのY座標を返す参照ポインタ
 * @param xp 適したマスのX座標を返す参照ポインタ
 * @return 有効なマスがあった場合TRUEを返す
 */
bool MonsterSweepGrid::sweep_ranged_attack_grid(POSITION *yp, POSITION *xp)
{
    auto *floor_ptr = this->target_ptr->current_floor_ptr;
    auto *m_ptr = &floor_ptr->m_list[this->m_idx];
    auto *r_ptr = &r_info[m_ptr->r_idx];
    auto y1 = m_ptr->fy;
    auto x1 = m_ptr->fx;
    if (projectable(this->target_ptr, y1, x1, this->target_ptr->y, this->target_ptr->x)) {
        return false;
    }

    auto now_cost = (int)grid_cost(&floor_ptr->grid_array[y1][x1], r_ptr);
    if (now_cost == 0) {
        now_cost = 999;
    }

    auto can_open_door = false;
    if (r_ptr->flags2 & (RF2_BASH_DOOR | RF2_OPEN_DOOR)) {
        can_open_door = true;
    }

    auto best = 999;
    for (auto i = 7; i >= 0; i--) {
        auto y = y1 + ddy_ddd[i];
        auto x = x1 + ddx_ddd[i];
        if (!in_bounds2(floor_ptr, y, x)) {
            continue;
        }

        if (player_bold(this->target_ptr, y, x)) {
            return false;
        }

        auto *g_ptr = &floor_ptr->grid_array[y][x];
        auto cost = (int)grid_cost(g_ptr, r_ptr);
        if (!((any_bits(r_ptr->flags2, RF2_PASS_WALL) && ((this->m_idx != this->target_ptr->riding) || has_pass_wall(this->target_ptr)))
                || (any_bits(r_ptr->flags2, RF2_KILL_WALL) && (this->m_idx != this->target_ptr->riding)))) {
            if (cost == 0) {
                continue;
            }

            if (!can_open_door && is_closed_door(this->target_ptr, g_ptr->feat))
                continue;
        }

        if (cost == 0) {
            cost = 998;
        }

        if (now_cost < cost) {
            continue;
        }

        if (!projectable(this->target_ptr, y, x, this->target_ptr->y, this->target_ptr->x)) {
            continue;
        }

        if (best < cost) {
            continue;
        }

        best = cost;
        *yp = y1 + ddy_ddd[i];
        *xp = x1 + ddx_ddd[i];
    }

    return best != 999;
}

/*!
 * @brief モンスターがプレイヤーから逃走することが可能なマスを走査する /
 * Provide a location to flee to, but give the player a wide berth.
 * @param yp 移動先のマスのY座標を返す参照ポインタ
 * @param xp 移動先のマスのX座標を返す参照ポインタ
 * @return 有効なマスがあった場合TRUEを返す
 */
bool MonsterSweepGrid::sweep_runnable_away_grid(POSITION *yp, POSITION *xp)
{
    auto gy = 0;
    auto gx = 0;
    auto *floor_ptr = this->target_ptr->current_floor_ptr;
    auto *m_ptr = &floor_ptr->m_list[this->m_idx];
    auto *r_ptr = &r_info[m_ptr->r_idx];
    auto fy = m_ptr->fy;
    auto fx = m_ptr->fx;
    auto y1 = fy - *yp;
    auto x1 = fx - *xp;
    auto score = -1;
    for (auto i = 7; i >= 0; i--) {
        auto y = fy + ddy_ddd[i];
        auto x = fx + ddx_ddd[i];
        if (!in_bounds2(floor_ptr, y, x)) {
            continue;
        }

        auto dis = distance(y, x, y1, x1);
        auto s = 5000 / (dis + 3) - 500 / (grid_dist(&floor_ptr->grid_array[y][x], r_ptr) + 1);
        if (s < 0) {
            s = 0;
        }

        if (s < score) {
            continue;
        }

        score = s;
        gy = y;
        gx = x;
    }

    if (score == -1) {
        return false;
    }

    *yp = fy - gy;
    *xp = fx - gx;
    return true;
}
