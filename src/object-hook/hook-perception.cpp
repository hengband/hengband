#include "object-hook/hook-perception.h"
#include "object-hook/hook-weapon.h"
#include "perception/object-perception.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"

/*!
 * @brief アイテムが並の価値のアイテムかどうか判定する /
 * Check if an object is nameless weapon or armour
 * @param o_ptr 判定するアイテムの情報参照ポインタ
 * @return 並ならばTRUEを返す
 */
bool object_is_nameless_weapon_armour(const ObjectType *o_ptr)
{
    /* Require weapon or armour */
    if (!o_ptr->is_weapon_armour_ammo())
        return false;

    /* Require nameless object if the object is well known */
    if (o_ptr->is_known() && !o_ptr->is_nameless())
        return false;

    return true;
}

/*!
 * @brief アイテムが未鑑定かを判定する /
 * @param o_ptr 判定するアイテムの情報参照ポインタ
 * @return 実際に未鑑定ならばTRUEを返す
 */
bool object_is_not_identified(const ObjectType *o_ptr)
{
    return !o_ptr->is_known();
}

/*!
 * @brief アイテムが未鑑定の武器防具かを判定する /
 * @param o_ptr 判定するアイテムの情報参照ポインタ
 * @return 実際に未鑑定の武器防具ならばTRUEを返す
 */
bool object_is_not_identified_weapon_armor(const ObjectType *o_ptr)
{
    if (!object_is_not_identified(o_ptr))
        return false;

    return o_ptr->is_weapon_armour_ammo();
}

/*!
 * @brief アイテムが未*鑑定*かを判定する /
 * @param o_ptr 判定するアイテムの情報参照ポインタ
 * @return 実際に未*鑑定*ならばTRUEを返す
 */
bool object_is_not_fully_identified(const ObjectType *o_ptr)
{
    return !o_ptr->is_known() || !o_ptr->is_fully_known();
}

/*!
 * @brief アイテムが未*鑑定*の武器防具かを判定する /
 * @param o_ptr 判定するアイテムの情報参照ポインタ
 * @return 実際に未*鑑定*の武器防具ならばTRUEを返す
 */
bool object_is_not_fully_identified_weapon_armour(const ObjectType *o_ptr)
{
    if (!object_is_not_fully_identified(o_ptr))
        return false;

    return o_ptr->is_weapon_armour_ammo();
}
