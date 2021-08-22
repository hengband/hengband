#include "cmd-action/cmd-travel.h"
#include "action/travel-execution.h"
#include "core/asking-player.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "player/player-move.h"
#include "player/player-status-flags.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "target/grid-selector.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

#define TRAVEL_UNABLE 9999

/*!
 * @brief トラベル処理中に地形に応じた移動コスト基準を返す
 * @param creature_ptr	プレーヤーへの参照ポインタ
 * @param y 該当地点のY座標
 * @param x 該当地点のX座標
 * @return コスト値
 */
static int travel_flow_cost(player_type *creature_ptr, POSITION y, POSITION x)
{
    int cost = 1;
    auto *g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];
    feature_type *f_ptr = &f_info[g_ptr->feat];
    if (f_ptr->flags.has(FF::AVOID_RUN))
        cost += 1;

    if (f_ptr->flags.has_all_of({FF::WATER, FF::DEEP}) && !creature_ptr->levitation)
        cost += 5;

    if (f_ptr->flags.has(FF::LAVA)) {
        int lava = 2;
        if (!has_resist_fire(creature_ptr))
            lava *= 2;

        if (!creature_ptr->levitation)
            lava *= 2;

        if (f_ptr->flags.has(FF::DEEP))
            lava *= 2;

        cost += lava;
    }

    if (g_ptr->is_mark()) {
        if (f_ptr->flags.has(FF::DOOR))
            cost += 1;

        if (f_ptr->flags.has(FF::TRAP))
            cost += 10;
    }

    return cost;
}

/*!
 * @brief トラベル処理の到達地点までの行程を得る処理のサブルーチン
 * @param creature_ptr	プレーヤーへの参照ポインタ
 * @param y 目標地点のY座標
 * @param x 目標地点のX座標
 * @param n 現在のコスト
 * @param wall プレイヤーが壁の中にいるならばTRUE
 */
static void travel_flow_aux(player_type *creature_ptr, POSITION y, POSITION x, int n, bool wall)
{
    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    grid_type *g_ptr = &floor_ptr->grid_array[y][x];
    feature_type *f_ptr = &f_info[g_ptr->feat];
    if (!in_bounds(floor_ptr, y, x))
        return;

    if (floor_ptr->dun_level > 0 && !(g_ptr->info & CAVE_KNOWN))
        return;

    int add_cost = 1;
    int from_wall = (n / TRAVEL_UNABLE);
    if (f_ptr->flags.has(FF::WALL) || f_ptr->flags.has(FF::CAN_DIG) || (f_ptr->flags.has(FF::DOOR) && floor_ptr->grid_array[y][x].mimic)
        || (f_ptr->flags.has_not(FF::MOVE) && f_ptr->flags.has(FF::CAN_FLY) && !creature_ptr->levitation)) {
        if (!wall || !from_wall)
            return;

        add_cost += TRAVEL_UNABLE;
    } else
        add_cost = travel_flow_cost(creature_ptr, y, x);

    int base_cost = (n % TRAVEL_UNABLE);
    int cost = base_cost + add_cost;
    if (travel.cost[y][x] <= cost)
        return;

    travel.cost[y][x] = cost;
    int old_head = flow_head;
    temp2_y[flow_head] = y;
    temp2_x[flow_head] = x;
    if (++flow_head == MAX_SHORT)
        flow_head = 0;

    if (flow_head == flow_tail)
        flow_head = old_head;
}

/*!
 * @brief トラベル処理の到達地点までの行程を得る処理のメインルーチン
 * @param creature_ptr	プレーヤーへの参照ポインタ
 * @param ty 目標地点のY座標
 * @param tx 目標地点のX座標
 */
static void travel_flow(player_type *creature_ptr, POSITION ty, POSITION tx)
{
    flow_head = flow_tail = 0;
    bool wall = false;
    feature_type *f_ptr = &f_info[creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x].feat];
    if (f_ptr->flags.has_not(FF::MOVE))
        wall = true;

    travel_flow_aux(creature_ptr, ty, tx, 0, wall);
    POSITION x, y;
    while (flow_head != flow_tail) {
        y = temp2_y[flow_tail];
        x = temp2_x[flow_tail];
        if (++flow_tail == MAX_SHORT)
            flow_tail = 0;

        for (DIRECTION d = 0; d < 8; d++)
            travel_flow_aux(creature_ptr, y + ddy_ddd[d], x + ddx_ddd[d], travel.cost[y][x], wall);
    }

    flow_head = flow_tail = 0;
}

/*!
 * @brief トラベル処理のメインルーチン
 */
void do_cmd_travel(player_type *creature_ptr)
{
    POSITION x, y;
    if ((travel.x != 0) && (travel.y != 0) && (travel.x != creature_ptr->x) && (travel.y != creature_ptr->y)
        && get_check(_("トラベルを継続しますか？", "Do you continue to travel? "))) {
        y = travel.y;
        x = travel.x;
    } else if (!tgt_pt(creature_ptr, &x, &y))
        return;

    if ((x == creature_ptr->x) && (y == creature_ptr->y)) {
        msg_print(_("すでにそこにいます！", "You are already there!!"));
        return;
    }

    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    feature_type *f_ptr;
    f_ptr = &f_info[floor_ptr->grid_array[y][x].feat];
    if ((floor_ptr->grid_array[y][x].info & CAVE_MARK)
        && (f_ptr->flags.has(FF::WALL) || f_ptr->flags.has(FF::CAN_DIG) || (f_ptr->flags.has(FF::DOOR) && floor_ptr->grid_array[y][x].mimic))) {
        msg_print(_("そこには行くことができません！", "You cannot travel there!"));
        return;
    }

    forget_travel_flow(creature_ptr->current_floor_ptr);
    travel_flow(creature_ptr, y, x);
    travel.x = x;
    travel.y = y;
    travel.run = 255;
    travel.dir = 0;
    POSITION dx = abs(creature_ptr->x - x);
    POSITION dy = abs(creature_ptr->y - y);
    POSITION sx = ((x == creature_ptr->x) || (dx < dy)) ? 0 : ((x > creature_ptr->x) ? 1 : -1);
    POSITION sy = ((y == creature_ptr->y) || (dy < dx)) ? 0 : ((y > creature_ptr->y) ? 1 : -1);
    for (int i = 1; i <= 9; i++)
        if ((sx == ddx[i]) && (sy == ddy[i]))
            travel.dir = i;
}
