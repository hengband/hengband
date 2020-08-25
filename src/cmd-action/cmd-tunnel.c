#include "cmd-action/cmd-tunnel.h"
#include "action/tunnel-execution.h"
#include "cmd-action/cmd-attack.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "floor/geometry.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "io/input-key-requester.h"
#include "player/attack-defense-types.h"
#include "player/special-defense-types.h"
#include "status/action-setter.h"
#include "system/floor-type-definition.h"
#include "target/target-getter.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief 「掘る」動作コマンドのメインルーチン /
 * Tunnels through "walls" (including rubble and closed doors)
 * @return なし
 * @details
 * <pre>
 * Note that you must tunnel in order to hit invisible monsters
 * in walls, though moving into walls still takes a turn anyway.
 *
 * Digging is very difficult without a "digger" weapon, but can be
 * accomplished by strong players using heavy weapons.
 * </pre>
 */
void do_cmd_tunnel(player_type *creature_ptr)
{
    bool more = FALSE;
    if (creature_ptr->special_defense & KATA_MUSOU)
        set_action(creature_ptr, ACTION_NONE);

    if (command_arg) {
        command_rep = command_arg - 1;
        creature_ptr->redraw |= PR_STATE;
        command_arg = 0;
    }

    DIRECTION dir;
    if (!get_rep_dir(creature_ptr, &dir, FALSE)) {
        if (!more)
            disturb(creature_ptr, FALSE, FALSE);

        return;
    }

    POSITION y = creature_ptr->y + ddy[dir];
    POSITION x = creature_ptr->x + ddx[dir];
    grid_type *g_ptr;
    g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];
    FEAT_IDX feat = get_feat_mimic(g_ptr);
    if (has_flag(f_info[feat].flags, FF_DOOR))
        msg_print(_("ドアは掘れない。", "You cannot tunnel through doors."));
    else if (!has_flag(f_info[feat].flags, FF_TUNNEL))
        msg_print(_("そこは掘れない。", "You can't tunnel through that."));
    else if (g_ptr->m_idx) {
        take_turn(creature_ptr, 100);
        msg_print(_("モンスターが立ちふさがっている！", "There is a monster in the way!"));
        do_cmd_attack(creature_ptr, y, x, 0);
    } else
        more = exe_tunnel(creature_ptr, y, x);

    if (!more)
        disturb(creature_ptr, FALSE, FALSE);
}
