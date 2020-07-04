#include "object-hook/hook-armor.h"
#include "inventory/inventory-slot-types.h"
#include "object-hook/hook-checker.h"
#include "object/object-info.h"
#include "sv-definition/sv-armor-types.h"
#include "system/object-type-definition.h"

/*!
 * @brief オブジェクトを防具として装備できるかの判定 / The "wearable" tester
 * @param o_ptr 判定するオブジェクトの構造体参照ポインタ
 * @return オブジェクトが防具として装備できるならTRUEを返す。
 */
bool item_tester_hook_wear(player_type *player_ptr, object_type *o_ptr)
{
    if ((o_ptr->tval == TV_SOFT_ARMOR) && (o_ptr->sval == SV_ABUNAI_MIZUGI))
        if (player_ptr->psex == SEX_MALE)
            return FALSE;

    /* Check for a usable slot */
    if (wield_slot(player_ptr, o_ptr) >= INVEN_RARM)
        return TRUE;

    return FALSE;
}

/*!
 * @brief 呪術領域の各処理に使える呪われた装備かどうかを返す。 / An "item_tester_hook" for offer
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @return 使える装備ならばTRUEを返す
 */
bool item_tester_hook_cursed(player_type *player_ptr, object_type *o_ptr)
{
    /* Unused */
    (void)player_ptr;

    return (bool)(object_is_cursed(o_ptr));
}

/*!
 * @brief オブジェクトが防具として装備できるかどうかを返す / Check if an object is armour
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 矢弾として使えるならばTRUEを返す
 */
bool object_is_armour(player_type *player_ptr, object_type *o_ptr)
{
    /* Unused */
    (void)player_ptr;

    if (TV_ARMOR_BEGIN <= o_ptr->tval && o_ptr->tval <= TV_ARMOR_END)
        return TRUE;

    return FALSE;
}
