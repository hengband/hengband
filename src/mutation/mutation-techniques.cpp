/*!
 * @brief 突然変異でのみ得ることができる特殊能力処理
 * @date 2020/07/04
 * @author Hourier
 */

#include "mutation/mutation-techniques.h"
#include "cmd-action/cmd-attack.h"
#include "floor/geometry.h"
#include "grid/grid.h"
#include "monster/monster-info.h"
#include "player/digestion-processor.h"
#include "player/player-move.h"
#include "player/player-status.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/terrain-type-definition.h"
#include "target/target-getter.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * 岩石食い
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return コマンドの入力方向に地形があればTRUE
 */
bool eat_rock(PlayerType *player_ptr)
{
    DIRECTION dir;
    if (!get_direction(player_ptr, &dir)) {
        return false;
    }

    const Pos2D pos(player_ptr->y + ddy[dir], player_ptr->x + ddx[dir]);
    const auto &grid = player_ptr->current_floor_ptr->get_grid(pos);
    const auto &terrain = terrains_info[grid.feat];
    const auto &terrain_mimic = terrains_info[grid.get_feat_mimic()];

    stop_mouth(player_ptr);
    if (terrain_mimic.flags.has_not(TerrainCharacteristics::HURT_ROCK)) {
        msg_print(_("この地形は食べられない。", "You cannot eat this feature."));
    } else if (terrain.flags.has(TerrainCharacteristics::PERMANENT)) {
        msg_format(_("いてっ！この%sはあなたの歯より硬い！", "Ouch!  This %s is harder than your teeth!"), terrain_mimic.name.data());
    } else if (grid.m_idx) {
        const auto &monster = player_ptr->current_floor_ptr->m_list[grid.m_idx];
        msg_print(_("何かが邪魔しています！", "There's something in the way!"));
        if (!monster.ml || !monster.is_pet()) {
            do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_NONE);
        }
    } else if (terrain.flags.has(TerrainCharacteristics::TREE)) {
        msg_print(_("木の味は好きじゃない！", "You don't like the woody taste!"));
    } else if (terrain.flags.has(TerrainCharacteristics::GLASS)) {
        msg_print(_("ガラスの味は好きじゃない！", "You don't like the glassy taste!"));
    } else if (terrain.flags.has(TerrainCharacteristics::DOOR) || terrain.flags.has(TerrainCharacteristics::CAN_DIG)) {
        (void)set_food(player_ptr, player_ptr->food + 3000);
    } else if (terrain.flags.has(TerrainCharacteristics::MAY_HAVE_GOLD) || terrain.flags.has(TerrainCharacteristics::HAS_GOLD)) {
        (void)set_food(player_ptr, player_ptr->food + 5000);
    } else {
        msg_format(_("この%sはとてもおいしい！", "This %s is very filling!"), terrain_mimic.name.data());
        (void)set_food(player_ptr, player_ptr->food + 10000);
    }

    cave_alter_feat(player_ptr, pos.y, pos.x, TerrainCharacteristics::HURT_ROCK);
    (void)move_player_effect(player_ptr, pos.y, pos.x, MPE_DONT_PICKUP);
    return true;
}
