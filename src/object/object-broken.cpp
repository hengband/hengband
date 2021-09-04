/*!
 * @brief アイテム破壊処理
 * @date 2019/03/06
 * @author deskull
 */
#include "object/object-broken.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "mind/snipe-types.h"
#include "object-enchant/tr-types.h"
#include "object/object-flags.h"
#include "object/object-kind.h"
#include "spell/spell-types.h"
#include "sv-definition/sv-potion-types.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

ObjectBreaker::ObjectBreaker(tr_type ignore_flg)
    : ignore_flg(ignore_flg)
{
}

BreakerAcid::BreakerAcid()
    : ObjectBreaker(TR_IGNORE_ACID)
{
}

BreakerElec::BreakerElec()
    : ObjectBreaker(TR_IGNORE_ELEC)
{
}

BreakerFire::BreakerFire()
    : ObjectBreaker(TR_IGNORE_FIRE)
{
}

BreakerCold::BreakerCold()
    : ObjectBreaker(TR_IGNORE_COLD)
{
}

/*!
 * @brief アイテムが酸で破損するかどうかを判定する
 * @param o_ptr アイテムの情報参照ポインタ
 * @return 破損するならばTRUEを返す
 * Note that amulets, rods, and high-level spell books are immune
 * to "inventory damage" of any kind.  Also sling ammo and shovels.
 * Does a given class of objects (usually) hate acid?
 * Note that acid can either melt or corrode something.
 */
bool BreakerAcid::hates(object_type *o_ptr) const
{
    /* Analyze the type */
    switch (o_ptr->tval) {
        /* Wearable items */
    case TV_ARROW:
    case TV_BOLT:
    case TV_BOW:
    case TV_SWORD:
    case TV_HAFTED:
    case TV_POLEARM:
    case TV_HELM:
    case TV_CROWN:
    case TV_SHIELD:
    case TV_BOOTS:
    case TV_GLOVES:
    case TV_CLOAK:
    case TV_SOFT_ARMOR:
    case TV_HARD_ARMOR:
    case TV_DRAG_ARMOR: {
        return true;
    }

    /* Staffs/Scrolls are wood/paper */
    case TV_STAFF:
    case TV_SCROLL: {
        return true;
    }

    /* Ouch */
    case TV_CHEST: {
        return true;
    }

    /* Junk is useless */
    case TV_SKELETON:
    case TV_BOTTLE:
    case TV_JUNK: {
        return true;
    }

    default:
        break;
    }

    return false;
}

/*!
 * @brief アイテムが電撃で破損するかどうかを判定する /
 * Does a given object (usually) hate electricity?
 * @param o_ptr アイテムの情報参照ポインタ
 * @return 破損するならばTRUEを返す
 */
bool BreakerElec::hates(object_type *o_ptr) const
{
    switch (o_ptr->tval) {
    case TV_RING:
    case TV_WAND: {
        return true;
    }

    default:
        break;
    }

    return false;
}

/*!
 * @brief アイテムが火炎で破損するかどうかを判定する /
 * Does a given object (usually) hate fire?
 * @param o_ptr アイテムの情報参照ポインタ
 * @return 破損するならばTRUEを返す
 * @details
 * Hafted/Polearm weapons have wooden shafts.
 * Arrows/Bows are mostly wooden.
 */
bool BreakerFire::hates(object_type *o_ptr) const
{
    /* Analyze the type */
    switch (o_ptr->tval) {
        /* Wearable */
    case TV_LITE:
    case TV_ARROW:
    case TV_BOW:
    case TV_HAFTED:
    case TV_POLEARM:
    case TV_BOOTS:
    case TV_GLOVES:
    case TV_CLOAK:
    case TV_SOFT_ARMOR: {
        return true;
    }

    /* Books */
    case TV_LIFE_BOOK:
    case TV_SORCERY_BOOK:
    case TV_NATURE_BOOK:
    case TV_CHAOS_BOOK:
    case TV_DEATH_BOOK:
    case TV_TRUMP_BOOK:
    case TV_ARCANE_BOOK:
    case TV_CRAFT_BOOK:
    case TV_DEMON_BOOK:
    case TV_CRUSADE_BOOK:
    case TV_MUSIC_BOOK:
    case TV_HISSATSU_BOOK:
    case TV_HEX_BOOK: {
        return true;
    }

    /* Chests */
    case TV_CHEST: {
        return true;
    }

    /* Staffs/Scrolls burn */
    case TV_STAFF:
    case TV_SCROLL: {
        return true;
    }

    default:
        break;
    }

    return false;
}

/*!
 * @brief アイテムが冷気で破損するかどうかを判定する /
 * Does a given object (usually) hate cold?
 * @param o_ptr アイテムの情報参照ポインタ
 * @return 破損するならばTRUEを返す
 */
bool BreakerCold::hates(object_type *o_ptr) const
{
    switch (o_ptr->tval) {
    case TV_POTION:
    case TV_FLASK:
    case TV_BOTTLE: {
        return true;
    }

    default:
        break;
    }

    return false;
}

/*!
 * @brief アイテムが属性で破損するかどうかを判定する(メインルーチン) /
 * Destroy things
 * @param o_ptr アイテムの情報参照ポインタ
 * @return 破損するならばTRUEを返す
 * @todo 統合を検討
 */
bool ObjectBreaker::can_destroy(object_type *o_ptr) const
{
    if (!this->hates(o_ptr))
        return false;
    auto flgs = object_flags(o_ptr);
    if (has_flag(flgs, this->ignore_flg))
        return false;
    return true;
}

/*!
 * @brief 薬の破損効果処理 /
 * Potions "smash open" and cause an area effect when
 * @param who 薬破損の主体ID(プレイヤー所持アイテムが壊れた場合0、床上のアイテムの場合モンスターID)
 * @param y 破壊時のY座標
 * @param x 破壊時のX座標
 * @param k_idx 破損した薬のアイテムID
 * @return 薬を浴びたモンスターが起こるならばTRUEを返す
 * @details
 * <pre>
 * (1) they are shattered while in the player's p_ptr->inventory_list,
 * due to cold (etc) attacks;
 * (2) they are thrown at a monster, or obstacle;
 * (3) they are shattered by a "cold ball" or other such spell
 * while lying on the floor.
 *
 * Arguments:
 *    who   ---  who caused the potion to shatter (0=player)
 *          potions that smash on the floor are assumed to
 *          be caused by no-one (who = 1), as are those that
 *          shatter inside the player inventory.
 *          (Not anymore -- I changed this; TY)
 *    y, x  --- coordinates of the potion (or player if
 *          the potion was in her inventory);
 *    o_ptr --- pointer to the potion object.
 * </pre>
 */
bool potion_smash_effect(player_type *owner_ptr, MONSTER_IDX who, POSITION y, POSITION x, KIND_OBJECT_IDX k_idx)
{
    int radius = 2;
    int dt = 0;
    int dam = 0;
    bool angry = false;
    object_kind *k_ptr = &k_info[k_idx];
    switch (k_ptr->sval) {
    case SV_POTION_SALT_WATER:
    case SV_POTION_SLIME_MOLD:
    case SV_POTION_LOSE_MEMORIES:
    case SV_POTION_DEC_STR:
    case SV_POTION_DEC_INT:
    case SV_POTION_DEC_WIS:
    case SV_POTION_DEC_DEX:
    case SV_POTION_DEC_CON:
    case SV_POTION_DEC_CHR:
    case SV_POTION_WATER: /* perhaps a 'water' attack? */
    case SV_POTION_APPLE_JUICE:
        return true;

    case SV_POTION_INFRAVISION:
    case SV_POTION_DETECT_INVIS:
    case SV_POTION_SLOW_POISON:
    case SV_POTION_CURE_POISON:
    case SV_POTION_BOLDNESS:
    case SV_POTION_RESIST_HEAT:
    case SV_POTION_RESIST_COLD:
    case SV_POTION_HEROISM:
    case SV_POTION_BESERK_STRENGTH:
    case SV_POTION_RES_STR:
    case SV_POTION_RES_INT:
    case SV_POTION_RES_WIS:
    case SV_POTION_RES_DEX:
    case SV_POTION_RES_CON:
    case SV_POTION_RES_CHR:
    case SV_POTION_INC_STR:
    case SV_POTION_INC_INT:
    case SV_POTION_INC_WIS:
    case SV_POTION_INC_DEX:
    case SV_POTION_INC_CON:
    case SV_POTION_INC_CHR:
    case SV_POTION_AUGMENTATION:
    case SV_POTION_ENLIGHTENMENT:
    case SV_POTION_STAR_ENLIGHTENMENT:
    case SV_POTION_SELF_KNOWLEDGE:
    case SV_POTION_EXPERIENCE:
    case SV_POTION_RESISTANCE:
    case SV_POTION_INVULNERABILITY:
    case SV_POTION_NEW_LIFE:
        /* All of the above potions have no effect when shattered */
        return false;
    case SV_POTION_SLOWNESS:
        dt = GF_OLD_SLOW;
        dam = 5;
        angry = true;
        break;
    case SV_POTION_POISON:
        dt = GF_POIS;
        dam = 3;
        angry = true;
        break;
    case SV_POTION_BLINDNESS:
        dt = GF_DARK;
        angry = true;
        break;
    case SV_POTION_BOOZE:
        dt = GF_OLD_CONF;
        angry = true;
        break;
    case SV_POTION_SLEEP:
        dt = GF_OLD_SLEEP;
        angry = true;
        break;
    case SV_POTION_RUINATION:
    case SV_POTION_DETONATIONS:
        dt = GF_SHARDS;
        dam = damroll(25, 25);
        angry = true;
        break;
    case SV_POTION_DEATH:
        dt = GF_DEATH_RAY;
        dam = k_ptr->level * 10;
        angry = true;
        radius = 1;
        break;
    case SV_POTION_SPEED:
        dt = GF_OLD_SPEED;
        break;
    case SV_POTION_CURE_LIGHT:
        dt = GF_OLD_HEAL;
        dam = damroll(2, 3);
        break;
    case SV_POTION_CURE_SERIOUS:
        dt = GF_OLD_HEAL;
        dam = damroll(4, 3);
        break;
    case SV_POTION_CURE_CRITICAL:
    case SV_POTION_CURING:
        dt = GF_OLD_HEAL;
        dam = damroll(6, 3);
        break;
    case SV_POTION_HEALING:
        dt = GF_OLD_HEAL;
        dam = damroll(10, 10);
        break;
    case SV_POTION_RESTORE_EXP:
        dt = GF_STAR_HEAL;
        dam = 0;
        radius = 1;
        break;
    case SV_POTION_LIFE:
        dt = GF_STAR_HEAL;
        dam = damroll(50, 50);
        radius = 1;
        break;
    case SV_POTION_STAR_HEALING:
        dt = GF_OLD_HEAL;
        dam = damroll(50, 50);
        radius = 1;
        break;
    case SV_POTION_RESTORE_MANA:
        dt = GF_MANA;
        dam = damroll(10, 10);
        radius = 1;
        break;
    case SV_POTION_POLY_SELF:
        dt = GF_NEXUS;
        dam = damroll(20, 20);
        radius = 1;
        break;
    default:
        break;
    }

    (void)project(owner_ptr, who, radius, y, x, dam, dt, (PROJECT_JUMP | PROJECT_ITEM | PROJECT_KILL));
    return angry;
}

/*!
 * @brief 矢弾を射撃した場合の破損確率を返す /
 * Determines the odds of an object breaking when thrown at a monster
 * @param o_ptr 矢弾のオブジェクト構造体参照ポインタ
 * @return 破損確率(%)
 * @details
 * Note that artifacts never break, see the "drop_near()" function.
 */
PERCENTAGE breakage_chance(player_type *owner_ptr, object_type *o_ptr, bool has_archer_bonus, SPELL_IDX snipe_type)
{
    /* Examine the snipe type */
    if (snipe_type) {
        if (snipe_type == SP_KILL_WALL)
            return 100;
        if (snipe_type == SP_EXPLODE)
            return 100;
        if (snipe_type == SP_PIERCE)
            return 100;
        if (snipe_type == SP_FINAL)
            return 100;
        if (snipe_type == SP_NEEDLE)
            return 100;
        if (snipe_type == SP_EVILNESS)
            return 40;
        if (snipe_type == SP_HOLYNESS)
            return 40;
    }

    /* Examine the item type */
    PERCENTAGE archer_bonus = (has_archer_bonus ? (PERCENTAGE)(owner_ptr->lev - 1) / 7 + 4 : 0);
    switch (o_ptr->tval) {
        /* Always break */
    case TV_FLASK:
    case TV_POTION:
    case TV_BOTTLE:
    case TV_FOOD:
    case TV_JUNK:
        return 100;

        /* Often break */
    case TV_LITE:
    case TV_SCROLL:
    case TV_SKELETON:
        return 50;

        /* Sometimes break */
    case TV_WAND:
    case TV_SPIKE:
        return 25;
    case TV_ARROW:
        return 20 - archer_bonus * 2;

        /* Rarely break */
    case TV_SHOT:
    case TV_BOLT:
        return 10 - archer_bonus;
    default:
        return 10;
    }
}
