#include "object-hook/hook-armor.h"
#include "inventory/inventory-slot-types.h"
#include "object/object-info.h"
#include "player/player-sex.h"
#include "sv-definition/sv-armor-types.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"

/*!
 * @brief オブジェクトを防具として装備できるかの判定 / The "wearable" tester
 * @param o_ptr 判定するオブジェクトの構造体参照ポインタ
 * @return オブジェクトが防具として装備できるならTRUEを返す。
 */
bool item_tester_hook_wear(PlayerType *player_ptr, const ObjectType *o_ptr)
{
    if ((o_ptr->tval == ItemKindType::SOFT_ARMOR) && (o_ptr->sval == SV_ABUNAI_MIZUGI))
        if (player_ptr->psex == SEX_MALE)
            return false;

    /* Check for a usable slot */
    if (wield_slot(player_ptr, o_ptr) >= INVEN_MAIN_HAND)
        return true;

    return false;
}
