#include "mind/mind-berserker.h"
#include "action/movement-execution.h"
#include "cmd-action/cmd-attack.h"
#include "floor/geometry.h"
#include "game-option/input-options.h"
#include "grid/grid.h"
#include "mind/mind-numbers.h"
#include "player-attack/player-attack.h"
#include "player/player-move.h"
#include "spell-kind/earthquake.h"
#include "spell-kind/spells-detection.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

/*!
 * @brief 怒りの発動 /
 * do_cmd_cast calls this function if the player's class is 'berserker'.
 * @param spell 発動する特殊技能のID
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
 */
bool cast_berserk_spell(PlayerType *player_ptr, MindBerserkerType spell)
{
    switch (spell) {
    case MindBerserkerType::DETECT_MANACE:
        detect_monsters_mind(player_ptr, DETECT_RAD_DEFAULT);
        return true;
    case MindBerserkerType::CHARGE: {
        if (player_ptr->riding) {
            msg_print(_("乗馬中には無理だ。", "You cannot do it when riding."));
            return false;
        }

        const auto dir = get_direction(player_ptr);
        if (!dir || (dir == 5)) {
            return false;
        }

        const auto pos = player_ptr->get_neighbor(*dir);
        const auto &floor = *player_ptr->current_floor_ptr;
        const auto &grid = floor.get_grid(pos);
        if (!grid.has_monster()) {
            msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
            return false;
        }

        do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_NONE);
        if (!player_can_enter(player_ptr, grid.feat, 0) || floor.has_trap_at(pos)) {
            return true;
        }

        const auto pos_new = pos + Direction(*dir).vec();
        const auto &grid_new = floor.get_grid(pos_new);
        if (player_can_enter(player_ptr, grid_new.feat, 0) && !floor.has_trap_at(pos_new) && !grid_new.has_monster()) {
            msg_print(nullptr);
            (void)move_player_effect(player_ptr, pos_new.y, pos_new.x, MPE_FORGET_FLOW | MPE_HANDLE_STUFF | MPE_DONT_PICKUP);
        }

        return true;
    }
    case MindBerserkerType::SMASH_TRAP: {
        const auto dir = get_direction(player_ptr);
        if (!dir) {
            return false;
        }

        exe_movement(player_ptr, *dir, easy_disarm, true);
        return true;
    }
    case MindBerserkerType::QUAKE:
        earthquake(player_ptr, player_ptr->y, player_ptr->x, 8 + randint0(5), 0);
        return true;
    case MindBerserkerType::MASSACRE:
        massacre(player_ptr);
        return true;
    default:
        msg_print(_("なに？", "Zap?"));
        return true;
    }
}
