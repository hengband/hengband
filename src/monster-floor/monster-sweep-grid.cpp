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
#include <span>

namespace {
/*!
 * @brief モンスターが逃走しようとするかを判定する
 *
 * @param m_idx モンスターの参照ID
 * @return 逃走しようとするならtrue、そうでなければfalse
 */
bool mon_will_run(PlayerType *player_ptr, MONSTER_IDX m_idx)
{
    const auto &monster = player_ptr->current_floor_ptr->m_list[m_idx];
    const auto &monrace = monster.get_monrace();
    if (monster.is_pet()) {
        return (player_ptr->pet_follow_distance < 0) && (monster.cdis <= (0 - player_ptr->pet_follow_distance));
    }

    if (monster.cdis > MAX_PLAYER_SIGHT + 5) {
        return false;
    }

    if (monster.is_fearful()) {
        return true;
    }

    if (monster.cdis <= 5) {
        return false;
    }

    const auto p_lev = player_ptr->lev;
    const auto m_lev = monrace.level + (m_idx & 0x08) + 25;
    if (m_lev > p_lev + 4) {
        return false;
    }

    if (m_lev + 4 <= p_lev) {
        return true;
    }

    const auto p_chp = player_ptr->chp;
    const auto p_mhp = player_ptr->mhp;
    const auto m_chp = monster.hp;
    const auto m_mhp = monster.maxhp;
    const uint32_t p_val = (p_lev * p_mhp) + (p_chp << 2);
    const uint32_t m_val = (m_lev * m_mhp) + (m_chp << 2);
    return p_val * m_mhp > m_val * p_mhp;
}
}

/*!
 * @brief モンスターの移動先を決定するクラスの基底クラス
 */
class MonsterMoveGridDecider {
public:
    MonsterMoveGridDecider() = default;
    virtual ~MonsterMoveGridDecider() = default;
    virtual std::optional<Pos2D> decide_move_grid() const = 0;

    /*!
     * @brief モンスターの移動先を決定する
     *
     * @param deciders 移動先を決定するクラスオブジェクトのリスト
     * @param default_pos すべてのクラスオブジェクトを実行しても移動先が決定されなかった場合の移動先
     * @return 移動先を決定するクラスオブジェクトのリストを先頭から順に実行し、最初に移動先が決定した時点でその座標を返す。
     * すべてのクラスオブジェクトを実行しても移動先が決定されなかった場合はdefault_posを返す。
     */
    static Pos2D evalute_deciders(std::span<const std::unique_ptr<const MonsterMoveGridDecider>> deciders, const Pos2D &default_pos)
    {
        for (const auto &decider : deciders) {
            const auto pos = decider->decide_move_grid();
            if (pos) {
                return *pos;
            }
        }

        return default_pos;
    }
};

/*!
 * @brief 特定のターゲットが設定されている場合の移動先を決定するクラス
 */
class SpecificTargetMoveGridDecider : public MonsterMoveGridDecider {
public:
    SpecificTargetMoveGridDecider(PlayerType *player_ptr, MONSTER_IDX m_idx)
        : player_ptr(player_ptr)
        , m_idx(m_idx)
    {
    }

    std::optional<Pos2D> decide_move_grid() const override
    {
        const auto &floor = *this->player_ptr->current_floor_ptr;
        const auto &monster = floor.m_list[this->m_idx];
        const auto pos_target = monster.get_target_position();
        const auto t_m_idx = floor.get_grid(pos_target).m_idx;
        if (t_m_idx <= 0) {
            return std::nullopt;
        }

        const auto is_enemies = monster.is_hostile_to_melee(floor.m_list[t_m_idx]);
        const auto m_pos = monster.get_position();
        const auto is_los = los(floor, m_pos, pos_target);
        const auto is_projectable = projectable(this->player_ptr, m_pos, pos_target);
        if (is_enemies && is_los && is_projectable) {
            return pos_target;
        }

        return std::nullopt;
    }

private:
    PlayerType *player_ptr;
    MONSTER_IDX m_idx;
};

/*!
 * @brief 隠れて待ち伏せし取り囲む事を狙うように移動先を決定するクラス
 */
class HidingMoveGridDecider : public MonsterMoveGridDecider {
public:
    HidingMoveGridDecider(PlayerType *player_ptr, MONSTER_IDX m_idx)
        : player_ptr(player_ptr)
        , m_idx(m_idx)
    {
    }

    std::optional<Pos2D> decide_move_grid() const override
    {
        const auto &floor = *this->player_ptr->current_floor_ptr;
        const auto &monrace = floor.m_list[this->m_idx].get_monrace();
        const auto p_pos = this->player_ptr->get_position();

        auto room = 0;
        for (const auto &d : Direction::directions_8()) {
            const auto pos_p_neighbor = p_pos + d.vec();
            if (!in_bounds2(floor, pos_p_neighbor.y, pos_p_neighbor.x)) {
                continue;
            }

            const auto &grid = floor.get_grid(pos_p_neighbor);
            if (monster_can_cross_terrain(this->player_ptr, grid.feat, &monrace, 0)) {
                room++;
            }
        }

        if (floor.get_grid(p_pos).is_room()) {
            room -= 2;
        }

        if (monrace.ability_flags.none()) {
            room -= 2;
        }

        if (room >= (8 * (this->player_ptr->chp + this->player_ptr->csp)) / (this->player_ptr->mhp + this->player_ptr->msp)) {
            return std::nullopt;
        }

        return find_hiding(this->player_ptr, this->m_idx);
    }

private:
    PlayerType *player_ptr;
    MONSTER_IDX m_idx;
};

/*!
 * @brief プレイヤーの周囲を取り囲むように移動先を決定するクラス
 */
class SurroundingMoveGridDecider : public MonsterMoveGridDecider {
public:
    SurroundingMoveGridDecider(PlayerType *player_ptr, MONSTER_IDX m_idx)
        : player_ptr(player_ptr)
        , m_idx(m_idx)
    {
    }

    std::optional<Pos2D> decide_move_grid() const override
    {
        const auto &floor = *this->player_ptr->current_floor_ptr;
        const auto &monster = floor.m_list[this->m_idx];
        const auto &monrace = monster.get_monrace();
        const auto p_pos = this->player_ptr->get_position();
        const auto m_pos = monster.get_position();

        /// @todo std::views::enumerate
        constexpr auto directions = Direction::directions_8();
        for (auto i = 0; i < 8; i++) {
            const auto d = (this->m_idx + i) & 7;
            const auto pos_move = p_pos + directions[d].vec();
            if (m_pos == pos_move) {
                // プレイヤーを攻撃する
                return p_pos;
            }

            if (!in_bounds2(floor, pos_move.y, pos_move.x) || !monster_can_enter(this->player_ptr, pos_move.y, pos_move.x, &monrace, 0)) {
                continue;
            }

            return pos_move;
        }

        // プレイヤーの周囲に空きが無い場合はプレイヤーに向かって移動する
        return p_pos;
    }

private:
    PlayerType *player_ptr;
    MONSTER_IDX m_idx;
};

/*!
 * @brief 遠隔攻撃を行えるマスに移動するように移動先を決定するクラス
 */
class RangedAttackMoveGridDecider : public MonsterMoveGridDecider {
public:
    RangedAttackMoveGridDecider(PlayerType *player_ptr, MONSTER_IDX m_idx)
        : player_ptr(player_ptr)
        , m_idx(m_idx)
    {
    }

    std::optional<Pos2D> decide_move_grid() const override
    {
        const auto &floor = *this->player_ptr->current_floor_ptr;
        const auto &monster = floor.m_list[this->m_idx];
        const auto &monrace = monster.get_monrace();
        const auto p_pos = this->player_ptr->get_position();
        const auto m_pos = monster.get_position();

        if (projectable(this->player_ptr, m_pos, p_pos)) {
            return std::nullopt;
        }

        const auto gf = monrace.get_grid_flow_type();
        int now_cost = floor.get_grid(m_pos).get_cost(gf);
        if (now_cost == 0) {
            now_cost = 999;
        }

        const auto can_open_door = monrace.behavior_flags.has_any_of({ MonsterBehaviorType::BASH_DOOR, MonsterBehaviorType::OPEN_DOOR });
        const auto can_pass_wall = monrace.feature_flags.has(MonsterFeatureType::PASS_WALL) && (!monster.is_riding() || has_pass_wall(player_ptr));
        const auto can_kill_wall = monrace.feature_flags.has(MonsterFeatureType::KILL_WALL) && !monster.is_riding();

        auto best = 999;
        std::optional<Pos2D> pos_move;
        for (const auto &d : Direction::directions_8_reverse()) {
            const auto pos_neighbor = m_pos + d.vec();
            if (!in_bounds2(floor, pos_neighbor.y, pos_neighbor.x)) {
                continue;
            }

            // プレイヤーに隣接している場合最初のprojectableで除外されるため
            // ここで判定する必要はないはずだが、元のコードで判定しているので一応残しておく
            if (p_pos == pos_neighbor) {
                return std::nullopt;
            }

            const auto &grid = floor.get_grid(pos_neighbor);
            int cost = grid.get_cost(gf);

            if (!can_pass_wall && !can_kill_wall) {
                if (cost == 0) {
                    continue;
                }

                if (!can_open_door && floor.has_closed_door_at(pos_neighbor)) {
                    continue;
                }
            }

            if (cost == 0) {
                cost = 998;
            }

            if (now_cost < cost || best < cost) {
                continue;
            }

            if (!projectable(this->player_ptr, pos_neighbor, p_pos)) {
                continue;
            }

            best = cost;
            pos_move = pos_neighbor;
        }

        return pos_move;
    }

private:
    PlayerType *player_ptr;
    MONSTER_IDX m_idx;
};

/*!
 * @brief grid.dists もしくは grid.costs を使用してプレイヤーの位置を追跡するように移動先を決定するクラス
 */
class NoiseTrackingMoveGridDecider : public MonsterMoveGridDecider {
public:
    NoiseTrackingMoveGridDecider(PlayerType *player_ptr, MONSTER_IDX m_idx)
        : player_ptr(player_ptr)
        , m_idx(m_idx)
    {
    }

    std::optional<Pos2D> decide_move_grid() const override
    {
        const auto &floor = *this->player_ptr->current_floor_ptr;
        const auto &monster = floor.m_list[this->m_idx];
        const auto &monrace = monster.get_monrace();
        const auto p_pos = this->player_ptr->get_position();
        const auto m_pos = monster.get_position();

        auto best = 999;
        std::optional<Pos2D> pos_move;
        for (const auto &d : Direction::directions_8_reverse()) {
            const auto pos_neighbor = m_pos + d.vec();
            if (!in_bounds2(floor, pos_neighbor.y, pos_neighbor.x)) {
                continue;
            }

            const auto &grid = floor.get_grid(pos_neighbor);
            const auto gf = monrace.get_grid_flow_type();
            const auto cost = monrace.behavior_flags.has_any_of({ MonsterBehaviorType::BASH_DOOR, MonsterBehaviorType::OPEN_DOOR }) ? grid.get_distance(gf) : grid.get_cost(gf);
            if (cost == 0 || best < cost) {
                continue;
            }

            best = cost;
            pos_move = p_pos + d.vec() * 16;
        }

        return pos_move;
    }

private:
    PlayerType *player_ptr;
    MONSTER_IDX m_idx;
};

/*!
 * @brief grid.when を使用してプレイヤーの位置を追跡するように移動先を決定するクラス
 */
class ScentTrackingMoveGridDecider : public MonsterMoveGridDecider {
public:
    ScentTrackingMoveGridDecider(PlayerType *player_ptr, MONSTER_IDX m_idx)
        : player_ptr(player_ptr)
        , m_idx(m_idx)
    {
    }

    std::optional<Pos2D> decide_move_grid() const override
    {
        const auto &floor = *this->player_ptr->current_floor_ptr;
        const auto &monster = floor.m_list[this->m_idx];
        const auto p_pos = this->player_ptr->get_position();
        const auto m_pos = monster.get_position();

        auto best = 0;
        std::optional<Pos2D> pos_move;
        for (const auto &d : Direction::directions_8_reverse()) {
            const auto pos_neighbor = m_pos + d.vec();
            if (!in_bounds2(floor, pos_neighbor.y, pos_neighbor.x)) {
                continue;
            }

            const auto &grid = floor.get_grid(pos_neighbor);
            const auto when = grid.when;
            if (best > when) {
                continue;
            }

            best = when;
            pos_move = p_pos + d.vec() * 16;
        }

        return pos_move;
    }

private:
    PlayerType *player_ptr;
    MONSTER_IDX m_idx;
};

class MonsterMoveGridDecidersFactory {
public:
    static std::vector<std::unique_ptr<const MonsterMoveGridDecider>> create_deciders(PlayerType *player_ptr, MONSTER_IDX m_idx)
    {
        const auto &floor = *player_ptr->current_floor_ptr;
        const auto &monster = floor.m_list[m_idx];
        const auto &monrace = monster.get_monrace();
        const auto will_run = mon_will_run(player_ptr, m_idx);
        const auto p_pos = player_ptr->get_position();
        const auto m_pos = monster.get_position();
        const auto &m_grid = floor.get_grid(m_pos);
        const auto gf = monrace.get_grid_flow_type();
        const auto dist_to_player = m_grid.get_distance(gf); // 経由グリッド数換算(Grid::dists)による距離
        const auto distance_to_player = Grid::calc_distance(m_pos, p_pos); // Grid::calc_distance()による直線距離
        const auto no_flow = monster.mflag2.has(MonsterConstantFlagType::NOFLOW) && (m_grid.get_cost(gf) > 2);
        const auto can_pass_wall = monrace.feature_flags.has(MonsterFeatureType::PASS_WALL) && (!monster.is_riding() || has_pass_wall(player_ptr));
        const auto can_kill_wall = monrace.feature_flags.has(MonsterFeatureType::KILL_WALL) && !monster.is_riding();
        const auto is_visible_from_player = m_grid.has_los() && projectable(player_ptr, p_pos, m_pos);
        const auto can_see_player = los(floor, m_pos, p_pos) && projectable(player_ptr, m_pos, p_pos);

        std::vector<std::unique_ptr<const MonsterMoveGridDecider>> deciders;

        if (!will_run && monster.target_y) {
            deciders.push_back(std::make_unique<SpecificTargetMoveGridDecider>(player_ptr, m_idx));
        }

        if (!will_run && monster.is_hostile() && monrace.misc_flags.has(MonsterMiscType::HAS_FRIENDS) &&
            (can_see_player || (dist_to_player < MAX_PLAYER_SIGHT / 2))) {
            if (monrace.kind_flags.has(MonsterKindType::ANIMAL) && !can_pass_wall && monrace.feature_flags.has_not(MonsterFeatureType::KILL_WALL)) {
                deciders.push_back(std::make_unique<HidingMoveGridDecider>(player_ptr, m_idx));
            }
            if (dist_to_player < 3) {
                deciders.push_back(std::make_unique<SurroundingMoveGridDecider>(player_ptr, m_idx));
            }
        }

        if (!will_run && distance_to_player <= AngbandSystem::get_instance().get_max_range() + 1 && monrace.ability_flags.has_any_of(RF_ABILITY_ATTACK_MASK)) {
            deciders.push_back(std::make_unique<RangedAttackMoveGridDecider>(player_ptr, m_idx));
        }

        const auto should_go_straight = no_flow || can_pass_wall || can_kill_wall;
        const auto try_circumventing = (distance_to_player > 1) && (monrace.freq_spell == 0) && (m_grid.get_cost(gf) <= 5);
        if (!should_go_straight && (!is_visible_from_player || try_circumventing)) {
            if (m_grid.get_cost(gf) > 0) {
                deciders.push_back(std::make_unique<NoiseTrackingMoveGridDecider>(player_ptr, m_idx));
            } else if (m_grid.when > 0) {
                deciders.push_back(std::make_unique<ScentTrackingMoveGridDecider>(player_ptr, m_idx));
            }
        }

        return deciders;
    }
};

/*!
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
    const auto deciders = MonsterMoveGridDecidersFactory::create_deciders(this->player_ptr, this->m_idx);
    const auto pos_move = MonsterMoveGridDecider::evalute_deciders(deciders, this->player_ptr->get_position());

    const auto &floor = *this->player_ptr->current_floor_ptr;
    const auto &monster = floor.m_list[this->m_idx];
    const auto &monrace = monster.get_monrace();

    const auto vec = monster.get_position() - pos_move;

    const auto m_pos = monster.get_position();
    const auto no_flow = monster.mflag2.has(MonsterConstantFlagType::NOFLOW) && (floor.get_grid(m_pos).get_cost(monrace.get_grid_flow_type()) > 2);
    const auto will_run = mon_will_run(this->player_ptr, this->m_idx);
    const auto vec_pet = this->search_pet_runnable_grid(vec, will_run, no_flow);
    if (vec_pet == Pos2DVec(0, 0)) {
        return false;
    }

    store_moves_val(this->mm, vec_pet);
    return true;
}

Pos2DVec MonsterSweepGrid::search_pet_runnable_grid(const Pos2DVec &vec_initial, bool will_run, bool no_flow)
{
    const auto &floor = *this->player_ptr->current_floor_ptr;
    const auto &monster = floor.m_list[this->m_idx];
    const auto vec_inverted = vec_initial.inverted();
    if (monster.is_pet() && will_run) {
        return vec_inverted;
    }

    if (!will_run) {
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

    return *vec_runaway;
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
    for (const auto &d : Direction::directions_8_reverse()) {
        const auto pos = m_pos + d.vec();
        if (!in_bounds2(floor, pos.y, pos.x)) {
            continue;
        }

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
