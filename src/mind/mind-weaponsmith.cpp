#include "mind/mind-weaponsmith.h"
#include "object-hook/hook-weapon.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/object-type-definition.h"

/*!
 * @brief オブジェクトが鍛冶師のエッセンス付加済みかを返す /
 * Check if an object is made by a smith's special ability
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return エッセンス付加済みならばTRUEを返す
 */
bool object_is_smith(const object_type *o_ptr)
{
    if (o_ptr->is_weapon_armour_ammo() && o_ptr->xtra3)
        return true;

    return false;
}
