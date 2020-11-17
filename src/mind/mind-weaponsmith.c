#include "mind/mind-weaponsmith.h"
#include "object-hook/hook-weapon.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/object-type-definition.h"

/*!
 * @brief エッセンスの付加可能な武器や矢弾かを返す
 * @param o_ptr チェックしたいオブジェクトの構造体参照ポインタ
 * @return エッセンスの付加可能な武器か矢弾ならばTRUEを返す。
 */
bool item_tester_hook_melee_ammo(player_type *player_ptr, object_type *o_ptr)
{
    /* Unused */
    (void)player_ptr;

    switch (o_ptr->tval) {
    case TV_HAFTED:
    case TV_POLEARM:
    case TV_DIGGING:
    case TV_BOLT:
    case TV_ARROW:
    case TV_SHOT: {
        return TRUE;
    }
    case TV_SWORD: {
        if (o_ptr->sval != SV_POISON_NEEDLE)
            return TRUE;
    }
    }

    return FALSE;
}

/*!
 * @brief オブジェクトが鍛冶師のエッセンス付加済みかを返す /
 * Check if an object is made by a smith's special ability
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return エッセンス付加済みならばTRUEを返す
 */
bool object_is_smith(player_type *player_ptr, object_type *o_ptr)
{
    /* Unused */
    (void)player_ptr;

    if (object_is_weapon_armour_ammo(player_ptr, o_ptr) && o_ptr->xtra3)
        return TRUE;

    return FALSE;
}
