#include "object-hook/hook-weapon.h"
#include "object-enchant/tr-types.h"
#include "object-hook/hook-armor.h"
#include "object/object-flags.h"
#include "player/player-skill.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief オブジェクトがプレイヤーの職業に応じた適正武器か否かを返す / Favorite weapons
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return オブジェクトが適正武器ならばTRUEを返す
 */
bool object_is_favorite(player_type *player_ptr, const object_type *o_ptr)
{
    /* Only melee weapons match */
    if (!(o_ptr->tval == TV_POLEARM || o_ptr->tval == TV_SWORD || o_ptr->tval == TV_DIGGING || o_ptr->tval == TV_HAFTED)) {
        return false;
    }

    /* Favorite weapons are varied depend on the class */
    switch (player_ptr->pclass) {
    case CLASS_PRIEST: {
        TrFlags flgs;
        object_flags_known(o_ptr, flgs);

        if (!has_flag(flgs, TR_BLESSED) && !(o_ptr->tval == TV_HAFTED))
            return false;
        break;
    }

    case CLASS_MONK:
    case CLASS_FORCETRAINER:
        /* Icky to wield? */
        if (!(s_info[player_ptr->pclass].w_max[o_ptr->tval - TV_WEAPON_BEGIN][o_ptr->sval]))
            return false;
        break;

    case CLASS_BEASTMASTER:
    case CLASS_CAVALRY: {
        TrFlags flgs;
        object_flags_known(o_ptr, flgs);

        /* Is it known to be suitable to using while riding? */
        if (!(has_flag(flgs, TR_RIDING)))
            return false;

        break;
    }

    case CLASS_SORCERER:
        if (s_info[player_ptr->pclass].w_max[o_ptr->tval - TV_WEAPON_BEGIN][o_ptr->sval] < WEAPON_EXP_MASTER)
            return false;
        break;

    case CLASS_NINJA:
        /* Icky to wield? */
        if (s_info[player_ptr->pclass].w_max[o_ptr->tval - TV_WEAPON_BEGIN][o_ptr->sval] <= WEAPON_EXP_BEGINNER)
            return false;
        break;

    default:
        /* All weapons are okay for non-special classes */
        return true;
    }

    return true;
}

/*!
 * @brief オブジェクトが強化不能武器であるかを返す / Poison needle can not be enchanted
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 強化不能ならばTRUEを返す
 */
bool object_refuse_enchant_weapon(const object_type *o_ptr)
{
    if (o_ptr->tval == TV_SWORD && o_ptr->sval == SV_POISON_NEEDLE)
        return true;

    return false;
}

/*!
 * @brief オブジェクトが強化可能武器であるかを返す /
 * Check if an object is weapon (including bows and ammo) and allows enchantment
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 強化可能ならばTRUEを返す
 */
bool object_allow_enchant_weapon(const object_type *o_ptr)
{
    if (o_ptr->is_weapon_ammo() && !object_refuse_enchant_weapon(o_ptr))
        return true;

    return false;
}

/*!
 * @brief オブジェクトが強化可能な近接武器であるかを返す /
 * Check if an object is melee weapon and allows enchantment
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 強化可能な近接武器ならばTRUEを返す
 */
bool object_allow_enchant_melee_weapon(const object_type *o_ptr)
{
    if (o_ptr->is_melee_weapon() && !object_refuse_enchant_weapon(o_ptr))
        return true;

    return false;
}

/*!
 * @brief オブジェクトが両手持ち可能な武器かを返す /
 * Check if an object is melee weapon and allows wielding with two-hands
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 両手持ち可能ならばTRUEを返す
 */
bool object_allow_two_hands_wielding(const object_type *o_ptr)
{
    if (o_ptr->is_melee_weapon() && ((o_ptr->weight > 99) || (o_ptr->tval == TV_POLEARM)))
        return true;

    return false;
}
