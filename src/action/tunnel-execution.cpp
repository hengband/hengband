﻿#include "action/tunnel-execution.h"
#include "core/player-update-types.h"
#include "floor/cave.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "player-info/avatar.h"
#include "player/player-move.h"
#include "system/floor-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief 「掘る」コマンドを該当のマスに行えるかの判定と結果メッセージの表示 /
 * Determine if a given grid may be "tunneled"
 * @param y 対象を行うマスのY座標
 * @param x 対象を行うマスのX座標
 * @return
 */
static bool do_cmd_tunnel_test(floor_type *floor_ptr, POSITION y, POSITION x)
{
    grid_type *g_ptr = &floor_ptr->grid_array[y][x];
    if (!(g_ptr->info & CAVE_MARK)) {
        msg_print(_("そこには何も見当たらない。", "You see nothing there."));
        return FALSE;
    }

    if (!cave_has_flag_grid(g_ptr, FF_TUNNEL)) {
        msg_print(_("そこには掘るものが見当たらない。", "You see nothing there to tunnel."));
        return FALSE;
    }

    return TRUE;
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
bool exe_tunnel(player_type *creature_ptr, POSITION y, POSITION x)
{
    grid_type *g_ptr;
    feature_type *f_ptr, *mimic_f_ptr;
    int power;
    concptr name;
    bool more = FALSE;
    if (!do_cmd_tunnel_test(creature_ptr->current_floor_ptr, y, x))
        return FALSE;

    take_turn(creature_ptr, 100);
    g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];
    f_ptr = &f_info[g_ptr->feat];
    power = f_ptr->power;
    mimic_f_ptr = &f_info[get_feat_mimic(g_ptr)];
    name = f_name + mimic_f_ptr->name;
    sound(SOUND_DIG);
    if (has_flag(f_ptr->flags, FF_PERMANENT)) {
        if (has_flag(mimic_f_ptr->flags, FF_PERMANENT))
            msg_print(_("この岩は硬すぎて掘れないようだ。", "This seems to be permanent rock."));
        else
            msg_print(_("そこは掘れない!", "You can't tunnel through that!"));
    } else if (has_flag(f_ptr->flags, FF_CAN_DIG)) {
        if (creature_ptr->skill_dig > randint0(20 * power)) {
            msg_format(_("%sをくずした。", "You have removed the %s."), name);
            cave_alter_feat(creature_ptr, y, x, FF_TUNNEL);
            creature_ptr->update |= PU_FLOW;
        } else {
            msg_format(_("%sをくずしている。", "You dig into the %s."), name);
            more = TRUE;
        }
    } else {
        bool tree = has_flag(mimic_f_ptr->flags, FF_TREE);
        if (creature_ptr->skill_dig > power + randint0(40 * power)) {
            if (tree)
                msg_format(_("%sを切り払った。", "You have cleared away the %s."), name);
            else {
                msg_print(_("穴を掘り終えた。", "You have finished the tunnel."));
                creature_ptr->update |= (PU_FLOW);
            }

            if (has_flag(f_ptr->flags, FF_GLASS))
                sound(SOUND_GLASS);

            cave_alter_feat(creature_ptr, y, x, FF_TUNNEL);
            chg_virtue(creature_ptr, V_DILIGENCE, 1);
            chg_virtue(creature_ptr, V_NATURE, -1);
        } else {
            if (tree) {
                msg_format(_("%sを切っている。", "You chop away at the %s."), name);
                if (randint0(100) < 25)
                    search(creature_ptr);
            } else {
                msg_format(_("%sに穴を掘っている。", "You tunnel into the %s."), name);
            }

            more = TRUE;
        }
    }

    if (is_hidden_door(creature_ptr, g_ptr) && (randint0(100) < 25))
        search(creature_ptr);

    return more;
}
