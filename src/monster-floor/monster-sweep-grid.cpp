/*!
 * @brief モンスターの移動方向を走査する処理
 * @date 2020/03/08
 * @author Hourier
 */

#include "monster-floor/monster-sweep-grid.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "floor/line-of-sight.h"
#include "grid/grid.h"
#include "monster-floor/monster-safety-hiding.h"
#include "monster-race/race-ability-mask.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-processor-util.h"
#include "monster/monster-status.h"
#include "player/player-status-flags.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"
#include "util/bit-flags-calculator.h"

/*
 * @brief コンストラクタ
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 移動するモンスターの参照ID
 * @param mm 移動方向を返す方向IDの参照ポインタ
 */
MonsterSweepGrid::MonsterSweepGrid(PlayerType *player_ptr, MONSTER_IDX m_idx, DIRECTION *mm)
    : player_ptr(player_ptr)
    , m_idx(m_idx)
    , mm(mm)
{
}

/*!
 * @brief モンスターの移動方向を返す
 * @return 有効方向があった場合TRUEを返す
 * @todo 分割したいが条件が多すぎて適切な関数名と詳細処理を追いきれない……
 */
bool MonsterSweepGrid::get_movable_grid()
{
    const auto &floor = *this->player_ptr->current_floor_ptr;
    const auto &monster_from = floor.m_list[this->m_idx];
    auto &monrace = monster_from.get_monrace();
    Pos2DVec vec(0, 0);
    auto pos_target = this->player_ptr->get_position();
    this->will_run = this->mon_will_run();
    const auto pos_monster_from = monster_from.get_position();
    const auto no_flow = monster_from.mflag2.has(MonsterConstantFlagType::NOFLOW) && (floor.get_grid(pos_monster_from).get_cost(monrace.get_grid_flow_type()) > 2);
    this->can_pass_wall = monrace.feature_flags.has(MonsterFeatureType::PASS_WALL) && (!monster_from.is_riding() || has_pass_wall(this->player_ptr));
    if (!this->will_run && monster_from.target_y) {
        const auto pos_to = monster_from.get_target_position();
        int t_m_idx = floor.get_grid(pos_to).m_idx;
        if (t_m_idx > 0) {
            const auto is_enemies = monster_from.is_hostile_to_melee(floor.m_list[t_m_idx]);
            const Pos2D pos_from = monster_from.get_position();
            const auto is_los = los(floor, pos_from, pos_to);
            const auto is_projectable = projectable(this->player_ptr, pos_from, pos_to);
            if (is_enemies && is_los && is_projectable) {
                vec = pos_from - pos_to;
                this->done = true;
            }
        }
    }

    const auto &[vec_hiding, pos] = this->check_hiding_grid(vec, pos_target);
    vec = vec_hiding;
    pos_target = pos;
    if (!this->done) {
        const auto pos_movable = this->sweep_movable_grid(pos_target, no_flow);
        vec = pos_monster_from - pos_movable;
    }

    const auto vec_pet = this->search_pet_runnable_grid(vec, no_flow);
    if (vec_pet == Pos2DVec(0, 0)) {
        return false;
    }

    store_moves_val(this->mm, vec_pet);
    return true;
}

/*!
 * @brief モンスターがプレイヤーから逃走するかどうかを返す
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

std::pair<Pos2DVec, Pos2D> MonsterSweepGrid::check_hiding_grid(const Pos2DVec &vec_initial, const Pos2D &p_pos)
{
    const auto &floor = *this->player_ptr->current_floor_ptr;
    const auto &monster = floor.m_list[this->m_idx];
    const auto &monrace = monster.get_monrace();
    if (this->done || this->will_run || !monster.is_hostile() || monrace.misc_flags.has_not(MonsterMiscType::HAS_FRIENDS)) {
        return std::pair<Pos2DVec, Pos2D>(vec_initial, p_pos);
    }

    const auto m_pos = monster.get_position();
    const auto gf = monrace.get_grid_flow_type();
    const auto distance = floor.get_grid(m_pos).get_distance(gf);
    if (!los(floor, m_pos, p_pos) || !projectable(this->player_ptr, m_pos, p_pos)) {
        if (distance >= MAX_PLAYER_SIGHT / 2) {
            return std::pair<Pos2DVec, Pos2D>(vec_initial, p_pos);
        }
    }

    const auto vec = this->search_room_to_run(vec_initial);
    if (this->done || (distance >= 3)) {
        return std::pair<Pos2DVec, Pos2D>(vec, p_pos);
    }

    Pos2D pos = p_pos;
    for (auto i = 0; i < 8; i++) {
        const auto dir = (this->m_idx + i) & 7;
        pos = p_pos + Pos2DVec(ddy_ddd[dir], ddx_ddd[dir]);
        if (m_pos == pos) {
            pos = p_pos;
            break;
        }

        if (!in_bounds2(&floor, pos.y, pos.x) || !monster_can_enter(this->player_ptr, pos.y, pos.x, &monrace, 0)) {
            continue;
        }

        break;
    }

    this->done = true;
    return std::pair<Pos2DVec, Pos2D>(m_pos - pos, pos);
}

Pos2DVec MonsterSweepGrid::search_room_to_run(const Pos2DVec &vec_initial)
{
    const auto &floor = *this->player_ptr->current_floor_ptr;
    const auto &monrace = floor.m_list[this->m_idx].get_monrace();
    if (monrace.kind_flags.has_not(MonsterKindType::ANIMAL) || this->can_pass_wall || monrace.feature_flags.has(MonsterFeatureType::KILL_WALL)) {
        return vec_initial;
    }

    auto room = 0;
    for (auto i = 0; i < 8; i++) {
        auto xx = this->player_ptr->x + ddx_ddd[i];
        auto yy = this->player_ptr->y + ddy_ddd[i];
        if (!in_bounds2(&floor, yy, xx)) {
            continue;
        }

        auto *g_ptr = &floor.grid_array[yy][xx];
        if (monster_can_cross_terrain(this->player_ptr, g_ptr->feat, &monrace, 0)) {
            room++;
        }
    }

    if (floor.grid_array[this->player_ptr->y][this->player_ptr->x].is_room()) {
        room -= 2;
    }

    if (monrace.ability_flags.none()) {
        room -= 2;
    }

    if (room >= (8 * (this->player_ptr->chp + this->player_ptr->csp)) / (this->player_ptr->mhp + this->player_ptr->msp)) {
        return vec_initial;
    }

    const auto vec = find_hiding(this->player_ptr, this->m_idx);
    if (!vec) {
        return vec_initial;
    }

    this->done = true;
    return *vec;
}

Pos2DVec MonsterSweepGrid::search_pet_runnable_grid(const Pos2DVec &vec_initial, bool no_flow)
{
    const auto &floor = *this->player_ptr->current_floor_ptr;
    const auto &monster = floor.m_list[this->m_idx];
    const auto vec_inverted = vec_initial.inverted();
    if (monster.is_pet() && this->will_run) {
        return vec_inverted;
    }

    if (this->done || !this->will_run) {
        return vec_initial;
    }

    const auto vec_safety = find_safety(this->player_ptr, this->m_idx);
    if (!vec_safety || no_flow) {
        return vec_inverted;
    }

    const auto vec_runaway = this->sweep_runnable_away_grid(*vec_safety);
    if (!vec_runaway) {
        return vec_inverted;
    }

    this->done = true;
    return *vec_runaway;
}

/*!
 * @brief モンスターがプレイヤーに向けて接近することが可能なマスを走査する
 * @param p_pos プレイヤーの座標
 * @param no_flow モンスターにFLOWフラグが経っていない状態でTRUE
 * @return 接近可能な座標
 */
Pos2D MonsterSweepGrid::sweep_movable_grid(const Pos2D &p_pos, bool no_flow)
{
    const auto &floor = *this->player_ptr->current_floor_ptr;
    const auto &monster = floor.m_list[this->m_idx];
    const auto &monrace = monster.get_monrace();
    const auto &[pos_movable, check] = this->check_movable_grid(p_pos, no_flow);
    if (!check) {
        return pos_movable;
    }

    const auto m_pos = monster.get_position();
    const auto &grid = floor.get_grid(m_pos);
    const auto gf = monrace.get_grid_flow_type();
    if (grid.has_los() && projectable(this->player_ptr, p_pos, m_pos)) {
        if ((Grid::calc_distance(m_pos, p_pos) == 1) || (monrace.freq_spell > 0) || (grid.get_cost(gf) > 5)) {
            return pos_movable;
        }
    }

    auto use_scent = false;
    if (grid.get_cost(gf)) {
        this->best = 999;
    } else if (grid.when) {
        if (floor.get_grid(p_pos).when - grid.when > 127) {
            return pos_movable;
        }

        use_scent = true;
        this->best = 0;
    } else {
        return pos_movable;
    }

    const auto pos = this->determine_when_cost(pos_movable, m_pos, use_scent);
    return pos;
}

std::pair<Pos2D, bool> MonsterSweepGrid::check_movable_grid(const Pos2D &pos_initial, bool no_flow)
{
    const auto &monster = this->player_ptr->current_floor_ptr->m_list[this->m_idx];
    const auto &monrace = monster.get_monrace();
    if (monrace.ability_flags.has_any_of(RF_ABILITY_ATTACK_MASK)) {
        const auto &pos = this->sweep_ranged_attack_grid(pos_initial);
        if (pos) {
            return std::pair<Pos2D, bool>(*pos, false);
        }
    }

    if (no_flow) {
        return std::pair<Pos2D, bool>(pos_initial, false);
    }

    if (monrace.feature_flags.has(MonsterFeatureType::PASS_WALL) && (!monster.is_riding() || has_pass_wall(this->player_ptr))) {
        return std::pair<Pos2D, bool>(pos_initial, false);
    }

    if (monrace.feature_flags.has(MonsterFeatureType::KILL_WALL) && !monster.is_riding()) {
        return std::pair<Pos2D, bool>(pos_initial, false);
    }

    return std::pair<Pos2D, bool>(pos_initial, true);
}

/*!
 * @brief モンスターがプレイヤーに向けて遠距離攻撃を行うことが可能なマスを走査する
 * @return 有効なグリッドがあった場合その座標、なかったらnullopt
 */
std::optional<Pos2D> MonsterSweepGrid::sweep_ranged_attack_grid(const Pos2D &pos_initial)
{
    const auto &floor = *this->player_ptr->current_floor_ptr;
    const auto &monster = floor.m_list[this->m_idx];
    const auto &monrace = monster.get_monrace();
    const auto m_pos = monster.get_position();
    if (projectable(this->player_ptr, m_pos, this->player_ptr->get_position())) {
        return std::nullopt;
    }

    const auto gf = monrace.get_grid_flow_type();
    int now_cost = floor.grid_array[m_pos.y][m_pos.x].get_cost(gf);
    if (now_cost == 0) {
        now_cost = 999;
    }

    if (monrace.behavior_flags.has_any_of({ MonsterBehaviorType::BASH_DOOR, MonsterBehaviorType::OPEN_DOOR })) {
        this->can_open_door = true;
    }

    Pos2D pos = pos_initial;
    for (auto i = 7; i >= 0; i--) {
        const Pos2D pos_neighbor(m_pos.y + ddy_ddd[i], m_pos.x + ddx_ddd[i]);
        if (!in_bounds2(&floor, pos_neighbor.y, pos_neighbor.x)) {
            continue;
        }

        if (this->player_ptr->is_located_at(pos_neighbor)) {
            return std::nullopt;
        }

        const auto &grid = floor.get_grid(pos_neighbor);
        this->cost = grid.get_cost(gf);
        if (!this->is_best_cost(pos_neighbor, now_cost)) {
            continue;
        }

        this->best = this->cost;
        pos = m_pos + Pos2DVec(ddy_ddd[i], ddx_ddd[i]);
    }

    if (this->best >= 999) {
        return std::nullopt;
    }

    return pos;
}

bool MonsterSweepGrid::is_best_cost(const Pos2D &pos, const int now_cost)
{
    const auto &floor = *this->player_ptr->current_floor_ptr;
    const auto &monster = floor.m_list[this->m_idx];
    const auto &monrace = monster.get_monrace();
    auto is_riding = monster.is_riding();
    if ((monrace.feature_flags.has_not(MonsterFeatureType::PASS_WALL) || (is_riding && !has_pass_wall(this->player_ptr))) && (monrace.feature_flags.has_not(MonsterFeatureType::KILL_WALL) || is_riding)) {
        if (this->cost == 0) {
            return false;
        }

        if (!this->can_open_door && floor.has_closed_door_at(pos)) {
            return false;
        }
    }

    if (this->cost == 0) {
        this->cost = 998;
    }

    if (now_cost < this->cost) {
        return false;
    }

    if (!projectable(this->player_ptr, pos, this->player_ptr->get_position())) {
        return false;
    }

    if (this->best < this->cost) {
        return false;
    }

    return true;
}

/*!
 * @brief モンスターがプレイヤーから逃走することが可能なマスを走査する
 * @param vec_initial 移動先から移動元へ向かうベクトル
 * @return 有効なマスがあった場合はその座標、なかったらnullopt
 */
std::optional<Pos2DVec> MonsterSweepGrid::sweep_runnable_away_grid(const Pos2DVec &vec_initial) const
{
    Pos2D pos_run(0, 0);
    const auto &floor = *this->player_ptr->current_floor_ptr;
    const auto &monster = floor.m_list[this->m_idx];
    const auto &monrace = monster.get_monrace();
    const auto m_pos = monster.get_position();
    auto pos1 = m_pos + vec_initial.inverted();
    auto score = -1;
    for (auto i = 7; i >= 0; i--) {
        auto y = m_pos.y + ddy_ddd[i];
        auto x = m_pos.x + ddx_ddd[i];
        if (!in_bounds2(&floor, y, x)) {
            continue;
        }

        const Pos2D pos(y, x);
        const auto dis = Grid::calc_distance(pos, pos1);
        auto s = 5000 / (dis + 3) - 500 / (floor.get_grid(pos).get_distance(monrace.get_grid_flow_type()) + 1);
        if (s < 0) {
            s = 0;
        }

        if (s < score) {
            continue;
        }

        score = s;
        pos_run = pos;
    }

    if (score == -1) {
        return vec_initial;
    }

    return m_pos - pos_run;
}

Pos2D MonsterSweepGrid::determine_when_cost(const Pos2D &pos_initial, const Pos2D &m_pos, const bool use_scent)
{
    Pos2D pos = pos_initial;
    const auto &floor = *this->player_ptr->current_floor_ptr;
    for (auto i = 7; i >= 0; i--) {
        auto y = m_pos.y + ddy_ddd[i];
        auto x = m_pos.x + ddx_ddd[i];
        if (!in_bounds2(&floor, y, x)) {
            continue;
        }

        const auto &grid = floor.grid_array[y][x];
        if (use_scent) {
            int when = grid.when;
            if (this->best > when) {
                continue;
            }

            this->best = when;
        } else {
            const auto &monrace = floor.m_list[this->m_idx].get_monrace();
            const auto gf = monrace.get_grid_flow_type();
            this->cost = monrace.behavior_flags.has_any_of({ MonsterBehaviorType::BASH_DOOR, MonsterBehaviorType::OPEN_DOOR }) ? grid.get_distance(gf) : grid.get_cost(gf);
            if ((this->cost == 0) || (this->best < this->cost)) {
                continue;
            }

            this->best = this->cost;
        }

        pos = this->player_ptr->get_position() + Pos2DVec(ddy_ddd[i], ddx_ddd[i]) * 16;
    }

    return pos;
}
