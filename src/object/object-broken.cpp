/*!
 * @brief アイテム破壊処理
 * @date 2019/03/06
 * @author deskull
 */
#include "object/object-broken.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "mind/snipe-types.h"
#include "object-enchant/tr-types.h"
#include "object/object-flags.h"
#include "object/tval-types.h"
#include "sv-definition/sv-potion-types.h"
#include "system/baseitem-info.h"
#include "system/item-entity.h"
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
bool BreakerAcid::hates(ItemEntity *o_ptr) const
{
    /* Analyze the type */
    switch (o_ptr->tval) {
        /* Wearable items */
    case ItemKindType::ARROW:
    case ItemKindType::BOLT:
    case ItemKindType::BOW:
    case ItemKindType::SWORD:
    case ItemKindType::HAFTED:
    case ItemKindType::POLEARM:
    case ItemKindType::HELM:
    case ItemKindType::CROWN:
    case ItemKindType::SHIELD:
    case ItemKindType::BOOTS:
    case ItemKindType::GLOVES:
    case ItemKindType::CLOAK:
    case ItemKindType::SOFT_ARMOR:
    case ItemKindType::HARD_ARMOR:
    case ItemKindType::DRAG_ARMOR: {
        return true;
    }

    /* Staffs/Scrolls are wood/paper */
    case ItemKindType::STAFF:
    case ItemKindType::SCROLL: {
        return true;
    }

    /* Ouch */
    case ItemKindType::CHEST: {
        return true;
    }

    /* Junk is useless */
    case ItemKindType::SKELETON:
    case ItemKindType::BOTTLE:
    case ItemKindType::JUNK: {
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
bool BreakerElec::hates(ItemEntity *o_ptr) const
{
    switch (o_ptr->tval) {
    case ItemKindType::RING:
    case ItemKindType::WAND: {
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
bool BreakerFire::hates(ItemEntity *o_ptr) const
{
    /* Analyze the type */
    switch (o_ptr->tval) {
        /* Wearable */
    case ItemKindType::LITE:
    case ItemKindType::ARROW:
    case ItemKindType::BOW:
    case ItemKindType::HAFTED:
    case ItemKindType::POLEARM:
    case ItemKindType::BOOTS:
    case ItemKindType::GLOVES:
    case ItemKindType::CLOAK:
    case ItemKindType::SOFT_ARMOR: {
        return true;
    }

    /* Books */
    case ItemKindType::LIFE_BOOK:
    case ItemKindType::SORCERY_BOOK:
    case ItemKindType::NATURE_BOOK:
    case ItemKindType::CHAOS_BOOK:
    case ItemKindType::DEATH_BOOK:
    case ItemKindType::TRUMP_BOOK:
    case ItemKindType::ARCANE_BOOK:
    case ItemKindType::CRAFT_BOOK:
    case ItemKindType::DEMON_BOOK:
    case ItemKindType::CRUSADE_BOOK:
    case ItemKindType::MUSIC_BOOK:
    case ItemKindType::HISSATSU_BOOK:
    case ItemKindType::HEX_BOOK: {
        return true;
    }

    /* Chests */
    case ItemKindType::CHEST: {
        return true;
    }

    /* Staffs/Scrolls burn */
    case ItemKindType::STAFF:
    case ItemKindType::SCROLL: {
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
bool BreakerCold::hates(ItemEntity *o_ptr) const
{
    switch (o_ptr->tval) {
    case ItemKindType::POTION:
    case ItemKindType::FLASK:
    case ItemKindType::BOTTLE: {
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
bool ObjectBreaker::can_destroy(ItemEntity *o_ptr) const
{
    if (!this->hates(o_ptr)) {
        return false;
    }
    auto flgs = object_flags(o_ptr);
    if (flgs.has(this->ignore_flg)) {
        return false;
    }
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
bool potion_smash_effect(PlayerType *player_ptr, MONSTER_IDX who, POSITION y, POSITION x, KIND_OBJECT_IDX k_idx)
{
    int radius = 2;
    AttributeType dt = AttributeType::NONE;
    int dam = 0;
    bool angry = false;
    const auto &k_ref = baseitems_info[k_idx];
    switch (k_ref.bi_key.sval().value()) {
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
        dt = AttributeType::OLD_SLOW;
        dam = 5;
        angry = true;
        break;
    case SV_POTION_POISON:
        dt = AttributeType::POIS;
        dam = 3;
        angry = true;
        break;
    case SV_POTION_BLINDNESS:
        dt = AttributeType::DARK;
        angry = true;
        break;
    case SV_POTION_BOOZE:
        dt = AttributeType::OLD_CONF;
        angry = true;
        break;
    case SV_POTION_SLEEP:
        dt = AttributeType::OLD_SLEEP;
        angry = true;
        break;
    case SV_POTION_RUINATION:
    case SV_POTION_DETONATIONS:
        dt = AttributeType::SHARDS;
        dam = damroll(25, 25);
        angry = true;
        break;
    case SV_POTION_DEATH:
        dt = AttributeType::DEATH_RAY;
        dam = k_ref.level * 10;
        angry = true;
        radius = 1;
        break;
    case SV_POTION_SPEED:
        dt = AttributeType::OLD_SPEED;
        break;
    case SV_POTION_CURE_LIGHT:
        dt = AttributeType::OLD_HEAL;
        dam = damroll(2, 3);
        break;
    case SV_POTION_CURE_SERIOUS:
        dt = AttributeType::OLD_HEAL;
        dam = damroll(4, 3);
        break;
    case SV_POTION_CURE_CRITICAL:
    case SV_POTION_CURING:
        dt = AttributeType::OLD_HEAL;
        dam = damroll(6, 3);
        break;
    case SV_POTION_HEALING:
        dt = AttributeType::OLD_HEAL;
        dam = damroll(10, 10);
        break;
    case SV_POTION_RESTORE_EXP:
        dt = AttributeType::STAR_HEAL;
        dam = 0;
        radius = 1;
        break;
    case SV_POTION_LIFE:
        dt = AttributeType::STAR_HEAL;
        dam = damroll(50, 50);
        radius = 1;
        break;
    case SV_POTION_STAR_HEALING:
        dt = AttributeType::OLD_HEAL;
        dam = damroll(50, 50);
        radius = 1;
        break;
    case SV_POTION_RESTORE_MANA:
        dt = AttributeType::MANA;
        dam = damroll(10, 10);
        radius = 1;
        break;
    case SV_POTION_POLY_SELF:
        dt = AttributeType::NEXUS;
        dam = damroll(20, 20);
        radius = 1;
        break;
    default:
        break;
    }

    (void)project(player_ptr, who, radius, y, x, dam, dt, (PROJECT_JUMP | PROJECT_ITEM | PROJECT_KILL));
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
PERCENTAGE breakage_chance(PlayerType *player_ptr, ItemEntity *o_ptr, bool has_archer_bonus, SPELL_IDX snipe_type)
{
    /* Examine the snipe type */
    if (snipe_type) {
        if (snipe_type == SP_KILL_WALL) {
            return 100;
        }
        if (snipe_type == SP_EXPLODE) {
            return 100;
        }
        if (snipe_type == SP_PIERCE) {
            return 100;
        }
        if (snipe_type == SP_FINAL) {
            return 100;
        }
        if (snipe_type == SP_NEEDLE) {
            return 100;
        }
        if (snipe_type == SP_EVILNESS) {
            return 40;
        }
        if (snipe_type == SP_HOLYNESS) {
            return 40;
        }
    }

    /* Examine the item type */
    PERCENTAGE archer_bonus = (has_archer_bonus ? (PERCENTAGE)(player_ptr->lev - 1) / 7 + 4 : 0);
    switch (o_ptr->tval) {
        /* Always break */
    case ItemKindType::FLASK:
    case ItemKindType::POTION:
    case ItemKindType::BOTTLE:
    case ItemKindType::FOOD:
    case ItemKindType::JUNK:
        return 100;

        /* Often break */
    case ItemKindType::LITE:
    case ItemKindType::SCROLL:
    case ItemKindType::SKELETON:
        return 50;

        /* Sometimes break */
    case ItemKindType::WAND:
    case ItemKindType::SPIKE:
        return 25;
    case ItemKindType::ARROW:
        return 20 - archer_bonus * 2;

        /* Rarely break */
    case ItemKindType::SHOT:
    case ItemKindType::BOLT:
        return 10 - archer_bonus;
    default:
        return 10;
    }
}
