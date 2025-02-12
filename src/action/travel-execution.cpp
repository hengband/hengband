/*!
 * @file travel-execution.cpp
 * @brief トラベル移動処理実装
 */

#include "action/travel-execution.h"
#include "action/movement-execution.h"
#include "action/run-execution.h"
#include "core/disturbance.h"
#include "floor/geometry.h"
#include "game-option/disturbance-options.h"
#include "game-option/input-options.h"
#include "game-option/special-options.h"
#include "grid/grid.h"
#include "player-status/player-energy.h"
#include "player/player-move.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "timed-effect/timed-effects.h"
#include "view/display-messages.h"

Travel travel{};

const std::optional<Pos2D> &Travel::get_goal() const
{
    return this->pos_goal;
}

void Travel::set_goal(const Pos2D &pos)
{
    this->pos_goal = pos;
    this->run = 255;
}

void Travel::reset_goal()
{
    this->pos_goal.reset();
    this->run = 0;
}

bool Travel::is_started() const
{
    return this->run < 255;
}

bool Travel::is_ongoing() const
{
    return this->run > 0;
}

void Travel::stop()
{
    this->run = 0;
}

/*!
 * @brief トラベル機能の判定処理 /
 * Test for traveling
 * @param player_ptr	プレイヤーへの参照ポインタ
 * @param prev_dir 前回移動を行った元の方角ID
 * @return 次の方向
 */
static DIRECTION travel_test(PlayerType *player_ptr, DIRECTION prev_dir)
{
    const auto &blindness = player_ptr->effects()->blindness();
    if (blindness.is_blind() || no_lite(player_ptr)) {
        msg_print(_("目が見えない！", "You cannot see!"));
        return 0;
    }

    auto &floor = *player_ptr->current_floor_ptr;
    if ((disturb_trap_detect || alert_trap_detect) && player_ptr->dtrap && !(floor.grid_array[player_ptr->y][player_ptr->x].info & CAVE_IN_DETECT)) {
        player_ptr->dtrap = false;
        if (!(floor.grid_array[player_ptr->y][player_ptr->x].info & CAVE_UNSAFE)) {
            if (alert_trap_detect) {
                msg_print(_("* 注意:この先はトラップの感知範囲外です！ *", "*Leaving trap detect region!*"));
            }

            if (disturb_trap_detect) {
                return 0;
            }
        }
    }

    int max = (prev_dir & 0x01) + 1;
    for (int i = -max; i <= max; i++) {
        DIRECTION dir = cycle[chome[prev_dir] + i];
        const auto pos = player_ptr->get_neighbor(dir);
        const auto &grid = floor.get_grid(pos);
        if (grid.has_monster()) {
            const auto &monster = floor.m_list[grid.m_idx];
            if (monster.ml) {
                return 0;
            }
        }
    }

    int cost = travel.cost[player_ptr->y][player_ptr->x];
    DIRECTION new_dir = 0;
    for (const auto &d : Direction::directions_8()) {
        const auto pos_neighbor = player_ptr->get_neighbor(d);
        int dir_cost = travel.cost[pos_neighbor.y][pos_neighbor.x];
        if (dir_cost < cost) {
            new_dir = d.dir();
            cost = dir_cost;
        }
    }

    if (!new_dir) {
        return 0;
    }

    const auto pos_new = player_ptr->get_neighbor(new_dir);
    if (!easy_open && floor.has_closed_door_at(pos_new)) {
        return 0;
    }

    const auto &grid = floor.get_grid(pos_new);
    if (!grid.mimic && !trap_can_be_ignored(player_ptr, grid.feat)) {
        return 0;
    }

    return new_dir;
}

/*!
 * @brief トラベル機能の実装 /
 * Travel command
 * @param player_ptr	プレイヤーへの参照ポインタ
 */
void Travel::step(PlayerType *player_ptr)
{
    this->dir = travel_test(player_ptr, this->dir);
    if (!this->dir) {
        if (!this->is_started()) {
            msg_print(_("道筋が見つかりません！", "No route is found!"));
            this->reset_goal();
        }

        disturb(player_ptr, false, true);
        return;
    }

    PlayerEnergy(player_ptr).set_player_turn_energy(100);
    exe_movement(player_ptr, Direction(this->dir), always_pickup, false);
    if (player_ptr->get_position() == this->get_goal()) {
        this->reset_goal();
    } else if (this->run > 0) {
        this->run--;
    }
}

/*!
 * @brief トラベル処理の記憶配列を初期化する Hack: forget the "flow" information
 * @param player_ptr	プレイヤーへの参照ポインタ
 */
void forget_travel_flow(const FloorType &floor)
{
    for (POSITION y = 0; y < floor.height; y++) {
        for (POSITION x = 0; x < floor.width; x++) {
            travel.cost[y][x] = MAX_SHORT;
        }
    }

    travel.reset_goal();
}
