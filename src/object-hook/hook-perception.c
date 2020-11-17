#include "object-hook/hook-perception.h"
#include "object-hook/hook-enchant.h"
#include "object-hook/hook-weapon.h"
#include "perception/object-perception.h"
#include "system/object-type-definition.h"

/*!
 * @brief アイテムが並の価値のアイテムかどうか判定する /
 * Check if an object is nameless weapon or armour
 * @param o_ptr 判定するアイテムの情報参照ポインタ
 * @return 並ならばTRUEを返す
 */
bool item_tester_hook_nameless_weapon_armour(player_type *player_ptr, object_type *o_ptr)
{
    /* Unused */
    (void)player_ptr;

    /* Require weapon or armour */
    if (!object_is_weapon_armour_ammo(player_ptr, o_ptr))
        return FALSE;

    /* Require nameless object if the object is well known */
    if (object_is_known(o_ptr) && !object_is_nameless(player_ptr, o_ptr))
        return FALSE;

    return TRUE;
}

/*!
 * @brief アイテムが鑑定済みかを判定する /
 * @param o_ptr 判定するアイテムの情報参照ポインタ
 * @return 実際に鑑定済みならばTRUEを返す
 */
bool item_tester_hook_identify(player_type *player_ptr, object_type *o_ptr)
{
    /* Unused */
    (void)player_ptr;

    return (bool)!object_is_known(o_ptr);
}

/*!
 * @brief アイテムが鑑定済みの武器防具かを判定する /
 * @param o_ptr 判定するアイテムの情報参照ポインタ
 * @return 実際に鑑定済みならばTRUEを返す
 */
bool item_tester_hook_identify_weapon_armour(player_type *player_ptr, object_type *o_ptr)
{
    /* Unused */
    (void)player_ptr;

    if (object_is_known(o_ptr))

        return FALSE;
    return object_is_weapon_armour_ammo(player_ptr, o_ptr);
}

/*!
 * @brief アイテムが*鑑定*済みかを判定する /
 * @param o_ptr 判定するアイテムの情報参照ポインタ
 * @return 実際に鑑定済みならばTRUEを返す
 */
bool item_tester_hook_identify_fully(player_type *player_ptr, object_type *o_ptr)
{
    /* Unused */
    (void)player_ptr;

    return (bool)(!object_is_known(o_ptr) || !object_is_fully_known(o_ptr));
}

/*!
 * @brief アイテムが*鑑定*済みの武器防具かを判定する /
 * @param o_ptr 判定するアイテムの情報参照ポインタ
 * @return 実際に鑑定済みならばTRUEを返す
 */
bool item_tester_hook_identify_fully_weapon_armour(player_type *player_ptr, object_type *o_ptr)
{
    /* Unused */
    (void)player_ptr;

    if (!item_tester_hook_identify_fully(player_ptr, o_ptr))
        return FALSE;

    return object_is_weapon_armour_ammo(player_ptr, o_ptr);
}
