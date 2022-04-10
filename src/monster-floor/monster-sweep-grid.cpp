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
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"
#include "util/bit-flags-calculator.h"

/*
 * @brief コンストラクタ
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 逃走するモンスターの参照ID
 * @param mm 移動方向を返す方向IDの参照ポインタ
 */
MonsterSweepGrid::MonsterSweepGrid(PlayerType *player_ptr, MONSTER_IDX m_idx, DIRECTION *mm)
    : player_ptr(player_ptr)
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
    auto *floor_ptr = this->player_ptr->current_floor_ptr;
    auto *m_ptr = &floor_ptr->m_list[this->m_idx];
    auto *r_ptr = &r_info[m_ptr->r_idx];
    POSITION y = 0;
    POSITION x = 0;
    auto y2 = this->player_ptr->y;
    auto x2 = this->player_ptr->x;
    this->will_run = this->mon_will_run();
    auto no_flow = m_ptr->mflag2.has(MonsterConstantFlagType::NOFLOW) && floor_ptr->grid_array[m_ptr->fy][m_ptr->fx].get_cost(r_ptr) > 2;
    this->can_pass_wall = r_ptr->feature_flags.has(MonsterFeatureType::PASS_WALL) && ((this->m_idx != this->player_ptr->riding) || has_pass_wall(this->player_ptr));
    if (!this->will_run && m_ptr->target_y) {
        int t_m_idx = floor_ptr->grid_array[m_ptr->target_y][m_ptr->target_x].m_idx;
        if ((t_m_idx > 0) && are_enemies(this->player_ptr, m_ptr, &floor_ptr->m_list[t_m_idx]) && los(this->player_ptr, m_ptr->fy, m_ptr->fx, m_ptr->target_y, m_ptr->target_x) && projectable(this->player_ptr, m_ptr->fy, m_ptr->fx, m_ptr->target_y, m_ptr->target_x)) {
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

    this->search_pet_runnable_grid(&y, &x, no_flow);
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
    auto *m_ptr = &this->player_ptr->current_floor_ptr->m_list[this->m_idx];
    auto *r_ptr = &r_info[m_ptr->r_idx];
    if (is_pet(m_ptr)) {
        return (this->player_ptr->pet_follow_distance < 0) && (m_ptr->cdis <= (0 - this->player_ptr->pet_follow_distance));
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

    auto p_lev = this->player_ptr->lev;
    auto m_lev = r_ptr->level + (this->m_idx & 0x08) + 25;
    if (m_lev > p_lev + 4) {
        return false;
    }

    if (m_lev + 4 <= p_lev) {
        return true;
    }

    auto p_chp = this->player_ptr->chp;
    auto p_mhp = this->player_ptr->mhp;
    auto m_chp = m_ptr->hp;
    auto m_mhp = m_ptr->maxhp;
    uint32_t p_val = (p_lev * p_mhp) + (p_chp << 2);
    uint32_t m_val = (m_lev * m_mhp) + (m_chp << 2);
    return p_val * m_mhp > m_val * p_mhp;
}

void MonsterSweepGrid::check_hiding_grid(POSITION *y, POSITION *x, POSITION *y2, POSITION *x2)
{
    auto *floor_ptr = this->player_ptr->current_floor_ptr;
    auto *m_ptr = &floor_ptr->m_list[this->m_idx];
    auto *r_ptr = &r_info[m_ptr->r_idx];
    if (this->done || this->will_run || !is_hostile(m_ptr) || none_bits(r_ptr->flags1, RF1_FRIENDS)) {
        return;
    }

    if ((!los(this->player_ptr, m_ptr->fy, m_ptr->fx, this->player_ptr->y, this->player_ptr->x) || !projectable(this->player_ptr, m_ptr->fy, m_ptr->fx, this->player_ptr->y, this->player_ptr->x))) {
        if (floor_ptr->grid_array[m_ptr->fy][m_ptr->fx].get_distance(r_ptr) >= MAX_SIGHT / 2) {
            return;
        }
    }

    this->search_room_to_run(y, x);
    if (this->done || (floor_ptr->grid_array[m_ptr->fy][m_ptr->fx].get_distance(r_ptr) >= 3)) {
        return;
    }

    for (auto i = 0; i < 8; i++) {
        *y2 = this->player_ptr->y + ddy_ddd[(this->m_idx + i) & 7];
        *x2 = this->player_ptr->x + ddx_ddd[(this->m_idx + i) & 7];
        if ((m_ptr->fy == *y2) && (m_ptr->fx == *x2)) {
            *y2 = this->player_ptr->y;
            *x2 = this->player_ptr->x;
            break;
        }

        if (!in_bounds2(floor_ptr, *y2, *x2) || !monster_can_enter(this->player_ptr, *y2, *x2, r_ptr, 0)) {
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
    auto *floor_ptr = this->player_ptr->current_floor_ptr;
    auto *r_ptr = &r_info[floor_ptr->m_list[this->m_idx].r_idx];
    if (r_ptr->kind_flags.has_not(MonsterKindType::ANIMAL) || this->can_pass_wall || r_ptr->feature_flags.has(MonsterFeatureType::KILL_WALL)) {
        return;
    }

    auto room = 0;
    for (auto i = 0; i < 8; i++) {
        auto xx = this->player_ptr->x + ddx_ddd[i];
        auto yy = this->player_ptr->y + ddy_ddd[i];
        if (!in_bounds2(floor_ptr, yy, xx)) {
            continue;
        }

        auto *g_ptr = &floor_ptr->grid_array[yy][xx];
        if (monster_can_cross_terrain(this->player_ptr, g_ptr->feat, r_ptr, 0)) {
            room++;
        }
    }

    if (floor_ptr->grid_array[this->player_ptr->y][this->player_ptr->x].is_room()) {
        room -= 2;
    }

    if (r_ptr->ability_flags.none()) {
        room -= 2;
    }

    if (room >= (8 * (this->player_ptr->chp + this->player_ptr->csp)) / (this->player_ptr->mhp + this->player_ptr->msp)) {
        return;
    }

    if (find_hiding(this->player_ptr, this->m_idx, y, x)) {
        this->done = true;
    }
}

void MonsterSweepGrid::search_pet_runnable_grid(POSITION *y, POSITION *x, bool no_flow)
{
    auto *floor_ptr = this->player_ptr->current_floor_ptr;
    auto *m_ptr = &floor_ptr->m_list[this->m_idx];
    if (is_pet(m_ptr) && this->will_run) {
        *y = -(*y);
        *x = -(*x);
        return;
    }

    if (this->done || !this->will_run) {
        return;
    }

    auto tmp_x = -(*x);
    auto tmp_y = -(*y);
    if (find_safety(this->player_ptr, this->m_idx, y, x) && !no_flow && this->sweep_runnable_away_grid(y, x)) {
        this->done = true;
    }

    if (this->done) {
        return;
    }

    *y = tmp_y;
    *x = tmp_x;
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
    auto *floor_ptr = this->player_ptr->current_floor_ptr;
    auto *m_ptr = &floor_ptr->m_list[this->m_idx];
    auto *r_ptr = &r_info[m_ptr->r_idx];
    if (!this->check_movable_grid(yp, xp, no_flow)) {
        return;
    }

    auto y1 = m_ptr->fy;
    auto x1 = m_ptr->fx;
    auto *g_ptr = &floor_ptr->grid_array[y1][x1];
    if (player_has_los_bold(this->player_ptr, y1, x1) && projectable(this->player_ptr, this->player_ptr->y, this->player_ptr->x, y1, x1)) {
        if ((distance(y1, x1, this->player_ptr->y, this->player_ptr->x) == 1) || (r_ptr->freq_spell > 0) || (g_ptr->get_cost(r_ptr) > 5)) {
            return;
        }
    }

    auto use_scent = false;
    if (g_ptr->get_cost(r_ptr)) {
        this->best = 999;
    } else if (g_ptr->when) {
        if (floor_ptr->grid_array[this->player_ptr->y][this->player_ptr->x].when - g_ptr->when > 127) {
            return;
        }

        use_scent = true;
        this->best = 0;
    } else {
        return;
    }

    this->determine_when_cost(yp, xp, y1, x1, use_scent);
}

bool MonsterSweepGrid::check_movable_grid(POSITION *yp, POSITION *xp, const bool no_flow)
{
    auto *r_ptr = &r_info[this->player_ptr->current_floor_ptr->m_list[this->m_idx].r_idx];
    if ((r_ptr->ability_flags.has_any_of(RF_ABILITY_ATTACK_MASK)) && (sweep_ranged_attack_grid(yp, xp))) {
        return false;
    }

    if (no_flow) {
        return false;
    }

    if (r_ptr->feature_flags.has(MonsterFeatureType::PASS_WALL) && ((this->m_idx != this->player_ptr->riding) || has_pass_wall(this->player_ptr))) {
        return false;
    }

    if (r_ptr->feature_flags.has(MonsterFeatureType::KILL_WALL) && (this->m_idx != this->player_ptr->riding)) {
        return false;
    }

    return true;
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
    auto *floor_ptr = this->player_ptr->current_floor_ptr;
    auto *m_ptr = &floor_ptr->m_list[this->m_idx];
    auto *r_ptr = &r_info[m_ptr->r_idx];
    auto y1 = m_ptr->fy;
    auto x1 = m_ptr->fx;
    if (projectable(this->player_ptr, y1, x1, this->player_ptr->y, this->player_ptr->x)) {
        return false;
    }

    auto now_cost = (int)floor_ptr->grid_array[y1][x1].get_cost(r_ptr);
    if (now_cost == 0) {
        now_cost = 999;
    }

    if (r_ptr->behavior_flags.has_any_of({ MonsterBehaviorType::BASH_DOOR, MonsterBehaviorType::OPEN_DOOR })) {
        this->can_open_door = true;
    }

    for (auto i = 7; i >= 0; i--) {
        auto y = y1 + ddy_ddd[i];
        auto x = x1 + ddx_ddd[i];
        if (!in_bounds2(floor_ptr, y, x)) {
            continue;
        }

        if (player_bold(this->player_ptr, y, x)) {
            return false;
        }

        auto *g_ptr = &floor_ptr->grid_array[y][x];
        this->cost = (int)g_ptr->get_cost(r_ptr);
        if (!this->is_best_cost(y, x, now_cost)) {
            continue;
        }

        this->best = this->cost;
        *yp = y1 + ddy_ddd[i];
        *xp = x1 + ddx_ddd[i];
    }

    return this->best != 999;
}

bool MonsterSweepGrid::is_best_cost(const POSITION y, const POSITION x, const int now_cost)
{
    auto *floor_ptr = this->player_ptr->current_floor_ptr;
    auto *r_ptr = &r_info[floor_ptr->m_list[this->m_idx].r_idx];
    auto is_riding = this->m_idx == this->player_ptr->riding;
    if ((r_ptr->feature_flags.has_not(MonsterFeatureType::PASS_WALL) || (is_riding && !has_pass_wall(this->player_ptr))) && (r_ptr->feature_flags.has_not(MonsterFeatureType::KILL_WALL) || is_riding)) {
        if (this->cost == 0) {
            return false;
        }

        auto *g_ptr = &floor_ptr->grid_array[y][x];
        if (!this->can_open_door && is_closed_door(this->player_ptr, g_ptr->feat)) {
            return false;
        }
    }

    if (this->cost == 0) {
        this->cost = 998;
    }

    if (now_cost < this->cost) {
        return false;
    }

    if (!projectable(this->player_ptr, y, x, this->player_ptr->y, this->player_ptr->x)) {
        return false;
    }

    if (this->best < this->cost) {
        return false;
    }

    return true;
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
    auto *floor_ptr = this->player_ptr->current_floor_ptr;
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
        auto s = 5000 / (dis + 3) - 500 / (floor_ptr->grid_array[y][x].get_distance(r_ptr) + 1);
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

void MonsterSweepGrid::determine_when_cost(POSITION *yp, POSITION *xp, POSITION y1, POSITION x1, const bool use_scent)
{
    auto *floor_ptr = this->player_ptr->current_floor_ptr;
    for (auto i = 7; i >= 0; i--) {
        auto y = y1 + ddy_ddd[i];
        auto x = x1 + ddx_ddd[i];
        if (!in_bounds2(floor_ptr, y, x)) {
            continue;
        }

        auto *g_ptr = &floor_ptr->grid_array[y][x];
        if (use_scent) {
            int when = g_ptr->when;
            if (this->best > when) {
                continue;
            }

            this->best = when;
        } else {
            auto *r_ptr = &r_info[floor_ptr->m_list[this->m_idx].r_idx];
            this->cost = r_ptr->behavior_flags.has_any_of({ MonsterBehaviorType::BASH_DOOR, MonsterBehaviorType::OPEN_DOOR }) ? g_ptr->get_distance(r_ptr) : g_ptr->get_cost(r_ptr);
            if ((this->cost == 0) || (this->best < this->cost)) {
                continue;
            }

            this->best = this->cost;
        }

        *yp = this->player_ptr->y + 16 * ddy_ddd[i];
        *xp = this->player_ptr->x + 16 * ddx_ddd[i];
    }
}
