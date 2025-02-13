#include "cmd-action/cmd-travel.h"
#include "action/travel-execution.h"
#include "core/asking-player.h"
#include "floor/geometry.h"
#include "grid/grid.h"
#include "player/player-move.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "target/grid-selector.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

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
    travel.set_goal(player_ptr->get_position(), *pos);
    travel.update_flow(player_ptr);
}
