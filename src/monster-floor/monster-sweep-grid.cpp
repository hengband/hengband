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
#include "monster-race/race-ability-mask.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-processor-util.h"
#include "monster/monster-status.h"
#include "player/player-status-flags.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
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
    auto &floor = *this->player_ptr->current_floor_ptr;
    const auto &monster_from = floor.m_list[this->m_idx];
    auto &monrace = monster_from.get_monrace();
    auto y = 0;
    auto x = 0;
    auto y2 = this->player_ptr->y;
    auto x2 = this->player_ptr->x;
    this->will_run = this->mon_will_run();
    Pos2D pos_monster_from(monster_from.fy, monster_from.fx);
    const auto no_flow = monster_from.mflag2.has(MonsterConstantFlagType::NOFLOW) && (floor.get_grid(pos_monster_from).get_cost(&monrace) > 2);
    this->can_pass_wall = monrace.feature_flags.has(MonsterFeatureType::PASS_WALL) && (!monster_from.is_riding() || has_pass_wall(this->player_ptr));
    if (!this->will_run && monster_from.target_y) {
        Pos2D pos_target(monster_from.target_y, monster_from.target_x);
        int t_m_idx = floor.get_grid(pos_target).m_idx;
        if (t_m_idx > 0) {
            const auto is_enemies = monster_from.is_hostile_to_melee(floor.m_list[t_m_idx]);
            const auto is_los = los(monster_from.fy, monster_from.fx, monster_from.target_y, monster_from.target_x);
            const auto is_projectable = projectable(this->player_ptr, monster_from.fy, monster_from.fx, monster_from.target_y, monster_from.target_x);
            if (is_enemies && is_los && is_projectable) {
                y = monster_from.fy - monster_from.target_y;
                x = monster_from.fx - monster_from.target_x;
                this->done = true;
            }
        }
    }

    this->check_hiding_grid(&y, &x, &y2, &x2);
    if (!this->done) {
        this->sweep_movable_grid(&y2, &x2, no_flow);
        y = monster_from.fy - y2;
        x = monster_from.fx - x2;
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
    auto *r_ptr = &m_ptr->get_monrace();
    if (m_ptr->is_pet()) {
        return (this->player_ptr->pet_follow_distance < 0) && (m_ptr->cdis <= (0 - this->player_ptr->pet_follow_distance));
    }

    if (m_ptr->cdis > MAX_PLAYER_SIGHT + 5) {
        return false;
    }

    if (m_ptr->is_fearful()) {
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
    auto *r_ptr = &m_ptr->get_monrace();
    if (this->done || this->will_run || !m_ptr->is_hostile() || r_ptr->misc_flags.has_not(MonsterMiscType::HAS_FRIENDS)) {
        return;
    }

    if ((!los(m_ptr->fy, m_ptr->fx, this->player_ptr->y, this->player_ptr->x) || !projectable(this->player_ptr, m_ptr->fy, m_ptr->fx, this->player_ptr->y, this->player_ptr->x))) {
        if (floor_ptr->grid_array[m_ptr->fy][m_ptr->fx].get_distance(r_ptr) >= MAX_PLAYER_SIGHT / 2) {
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
    const auto &monrace = floor_ptr->m_list[this->m_idx].get_monrace();
    if (monrace.kind_flags.has_not(MonsterKindType::ANIMAL) || this->can_pass_wall || monrace.feature_flags.has(MonsterFeatureType::KILL_WALL)) {
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
        if (monster_can_cross_terrain(this->player_ptr, g_ptr->feat, &monrace, 0)) {
            room++;
        }
    }

    if (floor_ptr->grid_array[this->player_ptr->y][this->player_ptr->x].is_room()) {
        room -= 2;
    }

    if (monrace.ability_flags.none()) {
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
    if (m_ptr->is_pet() && this->will_run) {
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
    auto &floor = *this->player_ptr->current_floor_ptr;
    auto &monster = floor.m_list[this->m_idx];
    auto &monrace = monster.get_monrace();
    if (!this->check_movable_grid(yp, xp, no_flow)) {
        return;
    }

    auto y1 = monster.fy;
    auto x1 = monster.fx;
    const Pos2D pos(y1, x1);
    const auto &grid = floor.get_grid(pos);
    if (grid.has_los() && projectable(this->player_ptr, this->player_ptr->y, this->player_ptr->x, y1, x1)) {
        if ((distance(y1, x1, this->player_ptr->y, this->player_ptr->x) == 1) || (monrace.freq_spell > 0) || (grid.get_cost(&monrace) > 5)) {
            return;
        }
    }

    auto use_scent = false;
    if (grid.get_cost(&monrace)) {
        this->best = 999;
    } else if (grid.when) {
        const auto p_pos = this->player_ptr->get_position();
        if (floor.get_grid(p_pos).when - grid.when > 127) {
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
    const auto &monster = this->player_ptr->current_floor_ptr->m_list[this->m_idx];
    const auto &monrace = monster.get_monrace();
    if ((monrace.ability_flags.has_any_of(RF_ABILITY_ATTACK_MASK)) && (sweep_ranged_attack_grid(yp, xp))) {
        return false;
    }

    if (no_flow) {
        return false;
    }

    if (monrace.feature_flags.has(MonsterFeatureType::PASS_WALL) && (!monster.is_riding() || has_pass_wall(this->player_ptr))) {
        return false;
    }

    if (monrace.feature_flags.has(MonsterFeatureType::KILL_WALL) && !monster.is_riding()) {
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
    auto *r_ptr = &m_ptr->get_monrace();
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
        const Pos2D pos(y1 + ddy_ddd[i], x1 + ddx_ddd[i]);
        if (!in_bounds2(floor_ptr, pos.y, pos.x)) {
            continue;
        }

        if (this->player_ptr->is_located_at(pos)) {
            return false;
        }

        const auto &grid = floor_ptr->get_grid(pos);
        this->cost = grid.get_cost(r_ptr);
        if (!this->is_best_cost(pos.y, pos.x, now_cost)) {
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
    const auto &monster = floor_ptr->m_list[this->m_idx];
    const auto &monrace = monster.get_monrace();
    auto is_riding = monster.is_riding();
    if ((monrace.feature_flags.has_not(MonsterFeatureType::PASS_WALL) || (is_riding && !has_pass_wall(this->player_ptr))) && (monrace.feature_flags.has_not(MonsterFeatureType::KILL_WALL) || is_riding)) {
        if (this->cost == 0) {
            return false;
        }

        auto *g_ptr = &floor_ptr->grid_array[y][x];
        if (!this->can_open_door && is_closed_door(g_ptr->feat)) {
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
    auto *r_ptr = &m_ptr->get_monrace();
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
            const auto &monrace = floor_ptr->m_list[this->m_idx].get_monrace();
            this->cost = monrace.behavior_flags.has_any_of({ MonsterBehaviorType::BASH_DOOR, MonsterBehaviorType::OPEN_DOOR }) ? g_ptr->get_distance(&monrace) : g_ptr->get_cost(&monrace);
            if ((this->cost == 0) || (this->best < this->cost)) {
                continue;
            }

            this->best = this->cost;
        }

        *yp = this->player_ptr->y + 16 * ddy_ddd[i];
        *xp = this->player_ptr->x + 16 * ddx_ddd[i];
    }
}
