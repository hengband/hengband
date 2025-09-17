#include "object-hook/hook-armor.h"
#include "inventory/inventory-slot-types.h"
#include "object/object-info.h"
#include "player/player-sex.h"
#include "sv-definition/sv-armor-types.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"

/*!
 * @brief オブジェクトを防具として装備できるかの判定
 * @param item 判定大正アイテムへの参照
 * @return オブジェクトが防具として装備できるならTRUEを返す。
 */
bool item_tester_hook_wear(PlayerType *player_ptr, const ItemEntity &item)
{
    if (item.bi_key == BaseitemKey(ItemKindType::SOFT_ARMOR, SV_ABUNAI_MIZUGI)) {
        if (player_ptr->psex == SEX_MALE) {
            return false;
        }
    }

    /* Check for a usable slot */
    if (wield_slot(player_ptr, &item) >= INVEN_MAIN_HAND) {
        return true;
    }

    return false;
}
