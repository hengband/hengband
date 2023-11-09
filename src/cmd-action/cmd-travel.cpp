#include "cmd-action/cmd-travel.h"
#include "action/travel-execution.h"
#include "core/asking-player.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "grid/grid.h"
#include "player/player-move.h"
#include "player/player-status-flags.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "system/terrain-type-definition.h"
#include "target/grid-selector.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

#define TRAVEL_UNABLE 9999

/*!
 * @brief トラベル処理中に地形に応じた移動コスト基準を返す
 * @param player_ptr	プレイヤーへの参照ポインタ
 * @param y 該当地点のY座標
 * @param x 該当地点のX座標
 * @return コスト値
 */
static int travel_flow_cost(PlayerType *player_ptr, POSITION y, POSITION x)
{
    int cost = 1;
    const Pos2D pos(y, x);
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
 * @brief トラベル処理の到達地点までの行程を得る処理のサブルーチン
 * @param player_ptr	プレイヤーへの参照ポインタ
 * @param y 目標地点のY座標
 * @param x 目標地点のX座標
 * @param n 現在のコスト
 * @param wall プレイヤーが壁の中にいるならばTRUE
 */
static void travel_flow_aux(PlayerType *player_ptr, const Pos2D pos, int n, bool wall)
{
    auto &floor = *player_ptr->current_floor_ptr;
    auto &grid = floor.get_grid(pos);
    auto &terrain = grid.get_terrain();
    if (!in_bounds(&floor, pos.y, pos.x)) {
        return;
    }

    if (floor.dun_level > 0 && !(grid.info & CAVE_KNOWN)) {
        return;
    }

    auto add_cost = 1;
    auto from_wall = (n / TRAVEL_UNABLE);
    auto is_wall = terrain.flags.has_any_of({ TerrainCharacteristics::WALL, TerrainCharacteristics::CAN_DIG });
    is_wall |= terrain.flags.has(TerrainCharacteristics::DOOR) && (grid.mimic > 0);
    auto can_move = terrain.flags.has_not(TerrainCharacteristics::MOVE);
    can_move &= terrain.flags.has(TerrainCharacteristics::CAN_FLY);
    can_move &= !player_ptr->levitation;
    if (is_wall || can_move) {
        if (!wall || !from_wall) {
            return;
        }

        add_cost += TRAVEL_UNABLE;
    } else {
        add_cost = travel_flow_cost(player_ptr, pos.y, pos.x);
    }

    auto base_cost = (n % TRAVEL_UNABLE);
    auto cost = base_cost + add_cost;
    auto &travel_cost = travel.cost[pos.y][pos.x];
    if (travel_cost <= cost) {
        return;
    }

    travel_cost = cost;
    auto old_head = flow_head;
    temp2_y[flow_head] = pos.y;
    temp2_x[flow_head] = pos.x;
    if (++flow_head == MAX_SHORT) {
        flow_head = 0;
    }

    if (flow_head == flow_tail) {
        flow_head = old_head;
    }
}

/*!
 * @brief トラベル処理の到達地点までの行程を得る処理のメインルーチン
 * @param player_ptr	プレイヤーへの参照ポインタ
 * @param ty 目標地点のY座標
 * @param tx 目標地点のX座標
 */
static void travel_flow(PlayerType *player_ptr, const Pos2D pos)
{
    flow_head = flow_tail = 0;
    const auto &terrain = player_ptr->current_floor_ptr->get_grid(player_ptr->get_position()).get_terrain();
    auto wall = terrain.flags.has(TerrainCharacteristics::MOVE);

    travel_flow_aux(player_ptr, pos, 0, wall);
    while (flow_head != flow_tail) {
        const Pos2D pos_flow(temp2_y[flow_tail], temp2_x[flow_tail]);
        if (++flow_tail == MAX_SHORT) {
            flow_tail = 0;
        }

        for (auto d = 0; d < 8; d++) {
            const Pos2D pos_neighbor(pos_flow.y + ddy_ddd[d], pos_flow.x + ddx_ddd[d]);
            travel_flow_aux(player_ptr, pos_neighbor, travel.cost[pos_flow.y][pos_flow.x], wall);
        }
    }

    flow_head = flow_tail = 0;
}

/*!
 * @brief トラベル処理のメインルーチン
 */
void do_cmd_travel(PlayerType *player_ptr)
{
    int x, y;
    if ((travel.x != 0) && (travel.y != 0) && (travel.x != player_ptr->x) && (travel.y != player_ptr->y) && input_check(_("トラベルを継続しますか？", "Do you continue to travel? "))) {
        y = travel.y;
        x = travel.x;
    } else if (!tgt_pt(player_ptr, &x, &y)) {
        return;
    }

    const Pos2D pos(y, x);
    if (player_ptr->is_located_at(pos)) {
        msg_print(_("すでにそこにいます！", "You are already there!!"));
        return;
    }

    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &grid = floor.get_grid(pos);
    const auto &terrain = grid.get_terrain();
    const auto is_marked = any_bits(grid.info, CAVE_MARK);
    const auto is_wall = terrain.flags.has_any_of({ TerrainCharacteristics::WALL, TerrainCharacteristics::CAN_DIG });
    const auto is_door = terrain.flags.has(TerrainCharacteristics::DOOR) && (grid.mimic > 0);
    if (is_marked && (is_wall || is_door)) {
        msg_print(_("そこには行くことができません！", "You cannot travel there!"));
        return;
    }

    forget_travel_flow(player_ptr->current_floor_ptr);
    travel_flow(player_ptr, pos);
    travel.x = x;
    travel.y = y;
    travel.run = 255;
    travel.dir = 0;
    auto dx = std::abs(player_ptr->x - x);
    auto dy = std::abs(player_ptr->y - y);
    auto sx = ((x == player_ptr->x) || (dx < dy)) ? 0 : ((x > player_ptr->x) ? 1 : -1);
    auto sy = ((y == player_ptr->y) || (dy < dx)) ? 0 : ((y > player_ptr->y) ? 1 : -1);
    for (auto i = 1; i <= 9; i++) {
        if ((sx == ddx[i]) && (sy == ddy[i])) {
            travel.dir = i;
        }
    }
}
