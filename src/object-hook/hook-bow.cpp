#include "object-hook/hook-bow.h"
#include "sv-definition/sv-other-types.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"

/*!
 * @brief 対象のアイテムが矢やクロスボウの矢の材料になるかを返す。/
 * Hook to determine if an object is contertible in an arrow/bolt
 * @param o_ptr オブジェクトの構造体の参照ポインタ。
 * @return 材料にできるならTRUEを返す
 */
bool object_is_convertible(const object_type *o_ptr)
{
    if ((o_ptr->tval == TV_JUNK) || (o_ptr->tval == TV_SKELETON))
        return true;
    if ((o_ptr->tval == TV_CORPSE) && (o_ptr->sval == SV_SKELETON))
        return true;
    return false;
}

/*!
 * @brief オブジェクトが矢弾として使用できるかどうかを返す / Check if an object is ammo
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 矢弾として使えるならばTRUEを返す
 */
bool object_is_ammo(const object_type *o_ptr)
{
    if (TV_MISSILE_BEGIN <= o_ptr->tval && o_ptr->tval <= TV_MISSILE_END)
        return true;

    return false;
}
