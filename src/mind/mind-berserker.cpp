#include "mind/mind-berserker.h"
#include "action/movement-execution.h"
#include "cmd-action/cmd-attack.h"
#include "floor/geometry.h"
#include "game-option/input-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "mind/mind-numbers.h"
#include "player-attack/player-attack.h"
#include "player/player-move.h"
#include "spell-kind/earthquake.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "spell-kind/spells-detection.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

/*!
 * @brief 怒りの発動 /
 * do_cmd_cast calls this function if the player's class is 'berserker'.
 * @param spell 発動する特殊技能のID
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
 */
bool cast_berserk_spell(player_type *player_ptr, mind_berserker_type spell)
{
    POSITION y, x;
    DIRECTION dir;
    switch (spell) {
    case DETECT_MANACE:
        detect_monsters_mind(player_ptr, DETECT_RAD_DEFAULT);
        break;
    case CHARGE: {
        if (player_ptr->riding) {
            msg_print(_("乗馬中には無理だ。", "You cannot do it when riding."));
            return false;
        }

        if (!get_direction(player_ptr, &dir, false, false) || (dir == 5))
            return false;

        y = player_ptr->y + ddy[dir];
        x = player_ptr->x + ddx[dir];
        if (!player_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
            msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
            return false;
        }

        do_cmd_attack(player_ptr, y, x, HISSATSU_NONE);
        if (!player_can_enter(player_ptr, player_ptr->current_floor_ptr->grid_array[y][x].feat, 0)
            || is_trap(player_ptr, player_ptr->current_floor_ptr->grid_array[y][x].feat))
            break;

        y += ddy[dir];
        x += ddx[dir];
        if (player_can_enter(player_ptr, player_ptr->current_floor_ptr->grid_array[y][x].feat, 0)
            && !is_trap(player_ptr, player_ptr->current_floor_ptr->grid_array[y][x].feat) && !player_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
            msg_print(nullptr);
            (void)move_player_effect(player_ptr, y, x, MPE_FORGET_FLOW | MPE_HANDLE_STUFF | MPE_DONT_PICKUP);
        }

        break;
    }
    case SMASH_TRAP: {
        if (!get_direction(player_ptr, &dir, false, false))
            return false;

        y = player_ptr->y + ddy[dir];
        x = player_ptr->x + ddx[dir];
        exe_movement(player_ptr, dir, easy_disarm, true);
        break;
    }
    case QUAKE:
        earthquake(player_ptr, player_ptr->y, player_ptr->x, 8 + randint0(5), 0);
        break;
    case MASSACRE:
        massacre(player_ptr);
        break;
    default:
        msg_print(_("なに？", "Zap?"));
        break;
    }

    return true;
}
