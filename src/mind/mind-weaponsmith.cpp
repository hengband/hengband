#include "mind/mind-weaponsmith.h"
#include "object-hook/hook-weapon.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/object-type-definition.h"

/*!
 * @brief エッセンスの付加可能な武器や矢弾かを返す
 * @param o_ptr チェックしたいオブジェクトの構造体参照ポインタ
 * @return エッセンスの付加可能な武器か矢弾ならばTRUEを返す。
 */
bool object_is_melee_ammo(const object_type *o_ptr)
{
    switch (o_ptr->tval) {
    case TV_HAFTED:
    case TV_POLEARM:
    case TV_DIGGING:
    case TV_BOLT:
    case TV_ARROW:
    case TV_SHOT: {
        return true;
    }
    case TV_SWORD: {
        if (o_ptr->sval != SV_POISON_NEEDLE)
            return true;
    }

    default:
        break;
    }

    return false;
}

/*!
 * @brief オブジェクトが鍛冶師のエッセンス付加済みかを返す /
 * Check if an object is made by a smith's special ability
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return エッセンス付加済みならばTRUEを返す
 */
bool object_is_smith(const object_type *o_ptr)
{
    if (object_is_weapon_armour_ammo(o_ptr) && o_ptr->xtra3)
        return true;

    return false;
}
