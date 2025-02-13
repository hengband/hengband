#include "cmd-action/cmd-travel.h"
#include "action/travel-execution.h"
#include "core/asking-player.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "grid/grid.h"
#include "player/player-move.h"
#include "player/player-status-flags.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "target/grid-selector.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include <queue>
#include <vector>

constexpr auto TRAVEL_UNABLE = 9999;

/*!
 * @brief トラベル処理中に地形に応じた移動コスト基準を返す
 * @param player_ptr	プレイヤーへの参照ポインタ
 * @param pos 該当地点の座標
 * @return コスト値
 */
static int travel_flow_cost(PlayerType *player_ptr, const Pos2D &pos)
{
    int cost = 1;
    const auto &grid = player_ptr->current_floor_ptr->get_grid(pos);
    const auto &terrain = grid.get_terrain();
    if (terrain.flags.has(TerrainCharacteristics::AVOID_RUN)) {
        cost += 1;
    }

    if (terrain.flags.has_all_of({ TerrainCharacteristics::WATER, TerrainCharacteristics::DEEP }) && !player_ptr->levitation) {
        cost += 5;
    }

    if (terrain.flags.has(TerrainCharacteristics::LAVA)) {
        int lava = 2;
        if (!has_resist_fire(player_ptr)) {
            lava *= 2;
        }

        if (!player_ptr->levitation) {
            lava *= 2;
        }

        if (terrain.flags.has(TerrainCharacteristics::DEEP)) {
            lava *= 2;
        }

        cost += lava;
    }

    if (grid.is_mark()) {
        if (terrain.flags.has(TerrainCharacteristics::DOOR)) {
            cost += 1;
        }

        if (terrain.flags.has(TerrainCharacteristics::TRAP)) {
            cost += 10;
        }
    }

    return cost;
}

/*!
 * @brief 隣接するマスのコストを計算する
 * @param player_ptr	プレイヤーへの参照ポインタ
 * @param pos 隣接するマスの座標
 * @param current_cost 現在のマスのコスト
 * @param wall プレイヤーが壁の中にいるならばTRUE
 * @return 隣接するマスのコスト。移動不可ならばstd::nullopt
 */
static std::optional<int> travel_flow_aux(PlayerType *player_ptr, const Pos2D pos, int current_cost, bool wall)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &grid = floor.get_grid(pos);
    const auto &terrain = grid.get_terrain();
    if (!in_bounds(floor, pos.y, pos.x)) {
        return std::nullopt;
    }

    if (floor.is_underground() && !(grid.info & CAVE_KNOWN)) {
        return std::nullopt;
    }

    const auto from_wall = (current_cost / TRAVEL_UNABLE);
    auto is_wall = terrain.flags.has_any_of({ TerrainCharacteristics::WALL, TerrainCharacteristics::CAN_DIG });
    is_wall |= terrain.flags.has(TerrainCharacteristics::DOOR) && (grid.mimic > 0);
    auto can_move = terrain.flags.has_not(TerrainCharacteristics::MOVE);
    can_move &= terrain.flags.has(TerrainCharacteristics::CAN_FLY);
    can_move &= !player_ptr->levitation;

    auto add_cost = 1;
    if (is_wall || can_move) {
        if (!wall || !from_wall) {
            return std::nullopt;
        }

        add_cost += TRAVEL_UNABLE;
    } else {
        add_cost = travel_flow_cost(player_ptr, pos);
    }

    const auto base_cost = (current_cost % TRAVEL_UNABLE);
    return base_cost + add_cost;
}

/*!
 * @brief トラベル処理の到達地点までの行程を得る処理のメインルーチン
 * @param player_ptr	プレイヤーへの参照ポインタ
 * @param pos_goal 目標地点の座標
 */
static void travel_flow(PlayerType *player_ptr, const Pos2D pos_goal)
{
    const auto &terrain = player_ptr->current_floor_ptr->get_grid(player_ptr->get_position()).get_terrain();
    const auto wall = terrain.flags.has(TerrainCharacteristics::MOVE);

    using CostAndPos = std::pair<int, Pos2D>;
    auto compare = [](const auto &a, const auto &b) { return a.first > b.first; }; // costが小さいものを優先
    std::priority_queue<CostAndPos, std::vector<CostAndPos>, decltype(compare)> pq(compare);

    travel.costs[pos_goal.y][pos_goal.x] = 1;
    pq.emplace(1, pos_goal);

    while (!pq.empty()) {
        // MSVCの警告対策のため構造化束縛を使わない
        const auto cost = pq.top().first;
        const auto pos = pq.top().second;
        pq.pop();

        for (const auto &d : Direction::directions_8()) {
            const auto pos_neighbor = pos + d.vec();
            auto &cost_neighbor = travel.costs[pos_neighbor.y][pos_neighbor.x];
            const auto new_cost = travel_flow_aux(player_ptr, pos_neighbor, cost, wall);
            if (new_cost && *new_cost < cost_neighbor) {
                cost_neighbor = *new_cost;
                pq.emplace(cost_neighbor, pos_neighbor);
            }
        }
    }
}

static std::optional<Pos2D> decide_travel_goal(PlayerType *player_ptr)
{
    const auto &pos_current_goal = travel.get_goal();
    if (pos_current_goal && pos_current_goal != player_ptr->get_position() && input_check(_("トラベルを継続しますか？", "Do you continue to travel? "))) {
        return *pos_current_goal;
    }

    int x, y;
    return tgt_pt(player_ptr, &x, &y) ? std::make_optional<Pos2D>(y, x) : std::nullopt;
}

/*!
 * @brief トラベル処理のメインルーチン
 */
void do_cmd_travel(PlayerType *player_ptr)
{
    const auto pos = decide_travel_goal(player_ptr);
    if (!pos) {
        return;
    }

    if (player_ptr->is_located_at(*pos)) {
        msg_print(_("すでにそこにいます！", "You are already there!!"));
        return;
    }

    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &grid = floor.get_grid(*pos);
    const auto &terrain = grid.get_terrain();
    const auto is_marked = grid.is_mark();
    const auto is_wall = terrain.flags.has_any_of({ TerrainCharacteristics::WALL, TerrainCharacteristics::CAN_DIG });
    const auto is_door = terrain.flags.has(TerrainCharacteristics::DOOR) && (grid.mimic > 0);
    if (is_marked && (is_wall || is_door)) {
        msg_print(_("そこには行くことができません！", "You cannot travel there!"));
        return;
    }

    travel.forget_flow();
    travel_flow(player_ptr, *pos);
    travel.set_goal(player_ptr->get_position(), *pos);
}
