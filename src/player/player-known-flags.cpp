#include "player/player-known-flags.h"
#include "inventory/inventory-slot-types.h"
#include "player/permanent-resistances.h"
#include "player/race-resistances.h"
#include "player/temporary-resistances.h"
#include "system/item/item-entity.h"
#include "system/player-type-definition.h"

/*!
 * @brief プレイヤーの特性を供給源ごとに集める
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 装備 (鑑定済み)・常時・一時・免疫・弱点・乗馬の特性
 */
PlayerKnownFlags collect_player_known_flags(PlayerType *player_ptr)
{
    PlayerKnownFlags result;
    for (const auto slot : INVEN_WIELDING_SLOTS) {
        result.equipment.set(player_ptr->inventory[slot]->get_flags_known());
    }
    player_flags(player_ptr, result.permanent);
    tim_player_flags(player_ptr, result.temporary);
    player_immunity(player_ptr, result.immunity);
    tim_player_immunity(player_ptr, result.temporary_immunity);
    player_vulnerability_flags(player_ptr, result.vulnerability);
    riding_flags(player_ptr, result.riding, result.riding_negative);
    return result;
}
