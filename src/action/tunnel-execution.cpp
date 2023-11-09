/*!
 * @file tunnel-execution.cpp
 * @brief 掘削処理実装
 */
#include "action/tunnel-execution.h"
#include "avatar/avatar.h"
#include "floor/cave.h"
#include "grid/grid.h"
#include "io/input-key-requester.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "player-status/player-energy.h"
#include "player/player-move.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief 「掘る」コマンドを該当のマスに行えるかの判定と結果メッセージの表示 /
 * Determine if a given grid may be "tunneled"
 * @param y 対象を行うマスのY座標
 * @param x 対象を行うマスのX座標
 * @return
 */
static bool do_cmd_tunnel_test(const Grid &grid)
{
    if (!grid.is_mark()) {
        msg_print(_("そこには何も見当たらない。", "You see nothing there."));
        return false;
    }

    if (!grid.cave_has_flag(TerrainCharacteristics::TUNNEL)) {
        msg_print(_("そこには掘るものが見当たらない。", "You see nothing there to tunnel."));
        return false;
    }

    return true;
}

/*!
 * @brief 「掘る」動作コマンドのサブルーチン /
 * Perform the basic "tunnel" command
 * @param y 対象を行うマスのY座標
 * @param x 対象を行うマスのX座標
 * @return 実際に処理が行われた場合TRUEを返す。
 * @details
 * Assumes that no monster is blocking the destination
 * Do not use twall anymore
 * Returns TRUE if repeated commands may continue
 */
bool exe_tunnel(PlayerType *player_ptr, POSITION y, POSITION x)
{
    auto more = false;
    const Pos2D pos(y, x);
    const auto &grid = player_ptr->current_floor_ptr->get_grid(pos);
    if (!do_cmd_tunnel_test(grid)) {
        return false;
    }

    PlayerEnergy(player_ptr).set_player_turn_energy(100);
    const auto &terrain = grid.get_terrain();
    const auto power = terrain.power;
    const auto &terrain_mimic = grid.get_terrain_mimic();
    const auto &name = terrain_mimic.name;
    if (command_rep == 0) {
        sound(SOUND_DIG);
    }

    if (terrain.flags.has(TerrainCharacteristics::PERMANENT)) {
        if (terrain_mimic.flags.has(TerrainCharacteristics::PERMANENT)) {
            msg_print(_("この岩は硬すぎて掘れないようだ。", "This seems to be permanent rock."));
        } else {
            msg_print(_("そこは掘れない!", "You can't tunnel through that!"));
        }
    } else if (terrain.flags.has(TerrainCharacteristics::CAN_DIG)) {
        if (player_ptr->skill_dig > randint0(20 * power)) {
            sound(SOUND_DIG_THROUGH);
            msg_format(_("%sをくずした。", "You have removed the %s."), name.data());
            cave_alter_feat(player_ptr, y, x, TerrainCharacteristics::TUNNEL);
            RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::FLOW);
        } else {
            msg_format(_("%sをくずしている。", "You dig into the %s."), name.data());
            more = true;
        }
    } else {
        bool tree = terrain_mimic.flags.has(TerrainCharacteristics::TREE);
        if (player_ptr->skill_dig > power + randint0(40 * power)) {
            sound(SOUND_DIG_THROUGH);
            if (tree) {
                msg_format(_("%sを切り払った。", "You have cleared away the %s."), name.data());
            } else {
                msg_print(_("穴を掘り終えた。", "You have finished the tunnel."));
                RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::FLOW);
            }

            if (terrain.flags.has(TerrainCharacteristics::GLASS)) {
                sound(SOUND_GLASS);
            }

            cave_alter_feat(player_ptr, y, x, TerrainCharacteristics::TUNNEL);
            chg_virtue(player_ptr, Virtue::DILIGENCE, 1);
            chg_virtue(player_ptr, Virtue::NATURE, -1);
        } else {
            if (tree) {
                msg_format(_("%sを切っている。", "You chop away at the %s."), name.data());
                if (randint0(100) < 25) {
                    search(player_ptr);
                }
            } else {
                msg_format(_("%sに穴を掘っている。", "You tunnel into the %s."), name.data());
            }

            more = true;
        }
    }

    if (is_hidden_door(player_ptr, grid) && (randint0(100) < 25)) {
        search(player_ptr);
    }

    return more;
}
