#include "object-hook/hook-bow.h"
#include "sv-definition/sv-other-types.h"
#include "system/object-type-definition.h"

/*!
 * @brief 対象のアイテムが矢やクロスボウの矢の材料になるかを返す。/
 * Hook to determine if an object is contertible in an arrow/bolt
 * @param o_ptr オブジェクトの構造体の参照ポインタ。
 * @return 材料にできるならTRUEを返す
 */
bool item_tester_hook_convertible(player_type *player_ptr, object_type *o_ptr)
{
    /* Unused */
    (void)player_ptr;

    if ((o_ptr->tval == TV_JUNK) || (o_ptr->tval == TV_SKELETON))
        return TRUE;
    if ((o_ptr->tval == TV_CORPSE) && (o_ptr->sval == SV_SKELETON))
        return TRUE;
    return FALSE;
}

/*!
 * @brief 武器匠の「矢弾」鑑定対象になるかを判定する。/ Hook to specify "weapon"
 * @param o_ptr オブジェクトの構造体の参照ポインタ。
 * @return 対象になるならTRUEを返す。
 */
bool item_tester_hook_ammo(player_type *player_ptr, object_type *o_ptr)
{
    /* Unused */
    (void)player_ptr;

    switch (o_ptr->tval) {
    case TV_SHOT:
    case TV_ARROW:
    case TV_BOLT: {
        return TRUE;
    }
    }

    return FALSE;
}

/*!
 * @brief オブジェクトが矢弾として使用できるかどうかを返す / Check if an object is ammo
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 矢弾として使えるならばTRUEを返す
 */
bool object_is_ammo(object_type *o_ptr)
{
    if (TV_MISSILE_BEGIN <= o_ptr->tval && o_ptr->tval <= TV_MISSILE_END)
        return TRUE;

    return FALSE;
}
