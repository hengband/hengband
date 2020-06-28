#include "object-hook/hook-weapon.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/object-type-definition.h"

/*!
 * @brief 武器匠の「武器」鑑定対象になるかを判定する。/ Hook to specify "weapon"
 * @param o_ptr オブジェクトの構造体の参照ポインタ。
 * @return 対象になるならTRUEを返す。
 */
bool item_tester_hook_orthodox_melee_weapons(player_type *player_ptr, object_type *o_ptr)
{
    /* Unused */
    (void)player_ptr;

    switch (o_ptr->tval) {
    case TV_HAFTED:
    case TV_POLEARM:
    case TV_DIGGING: {
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
 * @brief オブジェクトが右手か左手に装備できる武器かどうかの判定
 * @param o_ptr 判定するオブジェクトの構造体参照ポインタ
 * @return 右手か左手の武器として装備できるならばTRUEを返す。
 */
bool item_tester_hook_melee_weapon(player_type *player_ptr, object_type *o_ptr)
{
    /* Unused */
    (void)player_ptr;

    /* Check for a usable slot */
    if ((o_ptr->tval >= TV_DIGGING) && (o_ptr->tval <= TV_SWORD))
        return TRUE;

    return FALSE;
}

/*!
 * @brief 修復対象となる壊れた武器かを判定する。 / Hook to specify "broken weapon"
 * @param o_ptr オブジェクトの構造体の参照ポインタ。
 * @return 修復対象になるならTRUEを返す。
 */
bool item_tester_hook_broken_weapon(player_type *player_ptr, object_type *o_ptr)
{
    /* Unused */
    (void)player_ptr;

    if (o_ptr->tval != TV_SWORD)
        return FALSE;

    switch (o_ptr->sval) {
    case SV_BROKEN_DAGGER:
    case SV_BROKEN_SWORD:
        return TRUE;
    }

    return FALSE;
}

/*!
 * @brief オブジェクトが投射可能な武器かどうかを返す。
 * @param o_ptr 判定するオブジェクトの構造体参照ポインタ
 * @return 投射可能な武器ならばTRUE
 */
bool item_tester_hook_boomerang(player_type *player_ptr, object_type *o_ptr)
{
    /* Unused */
    (void)player_ptr;

    if ((o_ptr->tval == TV_DIGGING) || (o_ptr->tval == TV_SWORD) || (o_ptr->tval == TV_POLEARM) || (o_ptr->tval == TV_HAFTED))
        return TRUE;

    return FALSE;
}

/*!
 * @brief オブジェクトがどちらの手にも装備できる武器かどうかの判定
 * @param o_ptr 判定するオブジェクトの構造体参照ポインタ
 * @return 左右両方の手で装備できるならばTRUEを返す。
 */
bool item_tester_hook_mochikae(player_type *player_ptr, object_type *o_ptr)
{
    /* Unused */
    (void)player_ptr;

    /* Check for a usable slot */
    if (((o_ptr->tval >= TV_DIGGING) && (o_ptr->tval <= TV_SWORD)) || (o_ptr->tval == TV_SHIELD) || (o_ptr->tval == TV_CAPTURE) || (o_ptr->tval == TV_CARD))
        return TRUE;

    return FALSE;
}
