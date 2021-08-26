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
 * @brief 武器匠の「武器」鑑定対象になるかを判定する。/ Hook to specify "weapon"
 * @param o_ptr オブジェクトの構造体の参照ポインタ。
 * @return 対象になるならTRUEを返す。
 */
bool object_is_orthodox_melee_weapons(const object_type *o_ptr)
{
    switch (o_ptr->tval) {
    case TV_HAFTED:
    case TV_POLEARM:
    case TV_DIGGING: {
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
 * @brief 修復対象となる壊れた武器かを判定する。 / Hook to specify "broken weapon"
 * @param o_ptr オブジェクトの構造体の参照ポインタ。
 * @return 修復対象になるならTRUEを返す。
 */
bool object_is_broken_weapon(const object_type *o_ptr)
{
    if (o_ptr->tval != TV_SWORD)
        return false;

    switch (o_ptr->sval) {
    case SV_BROKEN_DAGGER:
    case SV_BROKEN_SWORD:
        return true;
    }

    return false;
}

/*!
 * @brief オブジェクトが投射可能な武器かどうかを返す。
 * @param o_ptr 判定するオブジェクトの構造体参照ポインタ
 * @return 投射可能な武器ならばTRUE
 */
bool object_is_boomerang(const object_type *o_ptr)
{
    if ((o_ptr->tval == TV_DIGGING) || (o_ptr->tval == TV_SWORD) || (o_ptr->tval == TV_POLEARM) || (o_ptr->tval == TV_HAFTED))
        return true;

    return false;
}

/*!
 * @brief オブジェクトがどちらの手にも装備できる武器かどうかの判定
 * @param o_ptr 判定するオブジェクトの構造体参照ポインタ
 * @return 左右両方の手で装備できるならばTRUEを返す。
 */
bool object_is_mochikae(const object_type *o_ptr)
{
    /* Check for a usable slot */
    if (((o_ptr->tval >= TV_DIGGING) && (o_ptr->tval <= TV_SWORD)) || (o_ptr->tval == TV_SHIELD) || (o_ptr->tval == TV_CAPTURE) || (o_ptr->tval == TV_CARD))
        return true;

    return false;
}

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
 * @brief オブジェクトが武器として装備できるかどうかを返す / Check if an object is weapon (including bows and ammo)
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 武器として使えるならばTRUEを返す
 */
bool object_is_weapon(const object_type *o_ptr)
{
    if (TV_WEAPON_BEGIN <= o_ptr->tval && o_ptr->tval <= TV_WEAPON_END)
        return true;

    return false;
}

/*!
 * @brief オブジェクトが武器や矢弾として使用できるかを返す / Check if an object is weapon (including bows and ammo)
 * Rare weapons/aromors including Blade of Chaos, Dragon armors, etc.
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 武器や矢弾として使えるならばTRUEを返す
 */
bool object_is_weapon_ammo(const object_type *o_ptr)
{
    if (TV_MISSILE_BEGIN <= o_ptr->tval && o_ptr->tval <= TV_WEAPON_END)
        return true;

    return false;
}

/*!
 * @brief オブジェクトが武器、防具、矢弾として使用できるかを返す / Check if an object is weapon, armour or ammo
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 武器、防具、矢弾として使えるならばTRUEを返す
 */
bool object_is_weapon_armour_ammo(const object_type *o_ptr)
{
    if (object_is_weapon_ammo(o_ptr) || object_is_armour(o_ptr))
        return true;

    return false;
}

/*!
 * @brief オブジェクトが近接武器として装備できるかを返す / Melee weapons
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 近接武器として使えるならばTRUEを返す
 */
bool object_is_melee_weapon(const object_type *o_ptr)
{
    if (TV_DIGGING <= o_ptr->tval && o_ptr->tval <= TV_SWORD)
        return true;

    return false;
}

/*!
 * @brief オブジェクトが装備可能であるかを返す / Wearable including all weapon, all armour, bow, light source, amulet, and ring
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 装備可能ならばTRUEを返す
 */
bool object_is_wearable(const object_type *o_ptr)
{
    if (TV_WEARABLE_BEGIN <= o_ptr->tval && o_ptr->tval <= TV_WEARABLE_END)
        return true;

    return false;
}

/*!
 * @brief オブジェクトが装備品であるかを返す(object_is_wearableに矢弾を含む) / Equipment including all wearable objects and ammo
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 装備品ならばTRUEを返す
 */
bool object_is_equipment(const object_type *o_ptr)
{
    if (TV_EQUIP_BEGIN <= o_ptr->tval && o_ptr->tval <= TV_EQUIP_END)
        return true;

    return false;
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
    if (object_is_weapon_ammo(o_ptr) && !object_refuse_enchant_weapon(o_ptr))
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
    if (object_is_melee_weapon(o_ptr) && !object_refuse_enchant_weapon(o_ptr))
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
    if (object_is_melee_weapon(o_ptr) && ((o_ptr->weight > 99) || (o_ptr->tval == TV_POLEARM)))
        return true;

    return false;
}
