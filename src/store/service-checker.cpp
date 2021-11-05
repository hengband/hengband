﻿#include "store/service-checker.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags3.h"
#include "object-enchant/tr-types.h"
#include "object/object-flags.h"
#include "object/object-value.h"
#include "store/store-util.h"
#include "sv-definition/sv-potion-types.h"
#include "sv-definition/sv-rod-types.h"
#include "sv-definition/sv-scroll-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/monster-race-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"

/*!
 * @brief オブジェクトが祝福されているかの判定を返す /
 * @param o_ptr 判定したいオブジェクト構造体の参照ポインタ
 * @return アイテムが祝福されたアイテムならばTRUEを返す
 */
static bool is_blessed_item(const object_type *o_ptr)
{
    auto flgs = object_flags(o_ptr);
    return flgs.has(TR_BLESSED);
}

static bool check_store_general(const object_type *o_ptr)
{
    switch (o_ptr->tval) {
    case ItemKindType::ROD:
        return (o_ptr->sval == SV_ROD_PESTICIDE);
    case ItemKindType::POTION:
        return (o_ptr->sval == SV_POTION_WATER);
    case ItemKindType::WHISTLE:
    case ItemKindType::FOOD:
    case ItemKindType::LITE:
    case ItemKindType::FLASK:
    case ItemKindType::SPIKE:
    case ItemKindType::SHOT:
    case ItemKindType::ARROW:
    case ItemKindType::BOLT:
    case ItemKindType::DIGGING:
    case ItemKindType::CLOAK:
    case ItemKindType::BOTTLE:
    case ItemKindType::FIGURINE:
    case ItemKindType::STATUE:
    case ItemKindType::CAPTURE:
    case ItemKindType::CARD:
        return true;
    default:
        return false;
    }
}

static bool check_store_armoury(const object_type *o_ptr)
{
    switch (o_ptr->tval) {
    case ItemKindType::BOOTS:
    case ItemKindType::GLOVES:
    case ItemKindType::CROWN:
    case ItemKindType::HELM:
    case ItemKindType::SHIELD:
    case ItemKindType::CLOAK:
    case ItemKindType::SOFT_ARMOR:
    case ItemKindType::HARD_ARMOR:
    case ItemKindType::DRAG_ARMOR:
        return true;
    default:
        return false;
    }
}

static bool check_store_weapon(const object_type *o_ptr)
{
    switch (o_ptr->tval) {
    case ItemKindType::SHOT:
    case ItemKindType::BOLT:
    case ItemKindType::ARROW:
    case ItemKindType::BOW:
    case ItemKindType::DIGGING:
    case ItemKindType::POLEARM:
    case ItemKindType::SWORD:
    case ItemKindType::HISSATSU_BOOK:
        return true;
    case ItemKindType::HAFTED:
        return o_ptr->sval != SV_WIZSTAFF;
    default:
        return false;
    }
}

static bool check_store_temple(const object_type *o_ptr)
{
    switch (o_ptr->tval) {
    case ItemKindType::LIFE_BOOK:
    case ItemKindType::CRUSADE_BOOK:
    case ItemKindType::SCROLL:
    case ItemKindType::POTION:
    case ItemKindType::HAFTED:
        return true;
    case ItemKindType::FIGURINE:
    case ItemKindType::STATUE: {
        monster_race *r_ptr = &r_info[o_ptr->pval];
        if (!(r_ptr->flags3 & RF3_EVIL))
            if (((r_ptr->flags3 & RF3_GOOD) != 0) || ((r_ptr->flags3 & RF3_ANIMAL) != 0) || (angband_strchr("?!", r_ptr->d_char) != nullptr))
                return true;
    }
        /* Fall through */
    case ItemKindType::POLEARM:
    case ItemKindType::SWORD:
        if (is_blessed_item(o_ptr))
            return true;

        /* Fall through */
    default:
        return false;
    }
}

static bool check_store_alchemist(const object_type *o_ptr)
{
    switch (o_ptr->tval) {
    case ItemKindType::SCROLL:
    case ItemKindType::POTION:
        return true;
    default:
        return false;
    }
}

static bool check_store_magic(const object_type *o_ptr)
{
    switch (o_ptr->tval) {
    case ItemKindType::SORCERY_BOOK:
    case ItemKindType::NATURE_BOOK:
    case ItemKindType::CHAOS_BOOK:
    case ItemKindType::DEATH_BOOK:
    case ItemKindType::TRUMP_BOOK:
    case ItemKindType::ARCANE_BOOK:
    case ItemKindType::CRAFT_BOOK:
    case ItemKindType::DEMON_BOOK:
    case ItemKindType::MUSIC_BOOK:
    case ItemKindType::HEX_BOOK:
    case ItemKindType::AMULET:
    case ItemKindType::RING:
    case ItemKindType::STAFF:
    case ItemKindType::WAND:
    case ItemKindType::ROD:
    case ItemKindType::SCROLL:
    case ItemKindType::POTION:
    case ItemKindType::FIGURINE:
        return true;
    case ItemKindType::HAFTED:
        return o_ptr->sval == SV_WIZSTAFF;
    default:
        return false;
    }
}

static bool check_store_book(const object_type *o_ptr)
{
    switch (o_ptr->tval) {
    case ItemKindType::SORCERY_BOOK:
    case ItemKindType::NATURE_BOOK:
    case ItemKindType::CHAOS_BOOK:
    case ItemKindType::DEATH_BOOK:
    case ItemKindType::LIFE_BOOK:
    case ItemKindType::TRUMP_BOOK:
    case ItemKindType::ARCANE_BOOK:
    case ItemKindType::CRAFT_BOOK:
    case ItemKindType::DEMON_BOOK:
    case ItemKindType::CRUSADE_BOOK:
    case ItemKindType::MUSIC_BOOK:
    case ItemKindType::HEX_BOOK:
        return true;
    default:
        return false;
    }
}

static bool switch_store_check(const object_type *o_ptr)
{
    switch (cur_store_num) {
    case StoreSaleType::GENERAL:
        return check_store_general(o_ptr);
    case StoreSaleType::ARMOURY:
        return check_store_armoury(o_ptr);
    case StoreSaleType::WEAPON:
        return check_store_weapon(o_ptr);
    case StoreSaleType::TEMPLE:
        return check_store_temple(o_ptr);
    case StoreSaleType::ALCHEMIST:
        return check_store_alchemist(o_ptr);
    case StoreSaleType::MAGIC:
        return check_store_magic(o_ptr);
    case StoreSaleType::BOOK:
        return check_store_book(o_ptr);
    default:
        return true;
    }
}

/*!
 * @brief オブジェクトが所定の店舗で引き取れるかどうかを返す /
 * Determine if the current store will purchase the given item
 * @param o_ptr 判定したいオブジェクト構造体の参照ポインタ
 * @return アイテムが買い取れるならばTRUEを返す
 * @note
 * Note that a shop-keeper must refuse to buy "worthless" items
 */
bool store_will_buy(PlayerType *, const object_type *o_ptr)
{
    if ((cur_store_num == StoreSaleType::HOME) || (cur_store_num == StoreSaleType::MUSEUM))
        return true;

    if (!switch_store_check(o_ptr))
        return false;

    return object_value(o_ptr) > 0;
}

static int mass_lite_produce(const PRICE cost)
{
    int size = 1;
    if (cost <= 5L)
        size += damroll(3, 5);

    if (cost <= 20L)
        size += damroll(3, 5);

    if (cost <= 50L)
        size += damroll(2, 2);

    return size;
}

static int mass_scroll_produce(object_type *o_ptr, const PRICE cost)
{
    int size = 1;
    if (cost <= 60L)
        size += damroll(3, 5);

    if (cost <= 240L)
        size += damroll(1, 5);

    if (o_ptr->sval == SV_SCROLL_STAR_IDENTIFY)
        size += damroll(3, 5);

    if (o_ptr->sval == SV_SCROLL_STAR_REMOVE_CURSE)
        size += damroll(1, 4);

    return size;
}

static int mass_book_produce(const PRICE cost)
{
    int size = 1;
    if (cost <= 50L)
        size += damroll(2, 3);

    if (cost <= 500L)
        size += damroll(1, 3);

    return size;
}

static int mass_equipment_produce(object_type *o_ptr, const PRICE cost)
{
    int size = 1;
    if (o_ptr->is_artifact() || o_ptr->is_ego())
        return size;

    if (cost <= 10L)
        size += damroll(3, 5);

    if (cost <= 100L)
        size += damroll(3, 5);

    return size;
}

static int mass_arrow_produce(const PRICE cost)
{
    int size = 1;
    if (cost <= 5L)
        size += damroll(5, 5);

    if (cost <= 50L)
        size += damroll(5, 5);

    if (cost <= 500L)
        size += damroll(5, 5);

    return size;
}

static int mass_figurine_produce(const PRICE cost)
{
    int size = 1;
    if (cost <= 100L)
        size += damroll(2, 2);

    if (cost <= 1000L)
        size += damroll(2, 2);

    return size;
}

static int mass_magic_produce(const PRICE cost)
{
    int size = 1;
    if ((cur_store_num != StoreSaleType::BLACK) || !one_in_(3))
        return size;

    if (cost < 1601L)
        size += damroll(1, 5);
    else if (cost < 3201L)
        size += damroll(1, 3);

    return size;
}

static int switch_mass_production(object_type *o_ptr, const PRICE cost)
{
    switch (o_ptr->tval) {
    case ItemKindType::FOOD:
    case ItemKindType::FLASK:
    case ItemKindType::LITE:
        return mass_lite_produce(cost);
    case ItemKindType::POTION:
    case ItemKindType::SCROLL:
        return mass_scroll_produce(o_ptr, cost);
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
    case ItemKindType::HEX_BOOK:
        return mass_book_produce(cost);
    case ItemKindType::SOFT_ARMOR:
    case ItemKindType::HARD_ARMOR:
    case ItemKindType::SHIELD:
    case ItemKindType::GLOVES:
    case ItemKindType::BOOTS:
    case ItemKindType::CLOAK:
    case ItemKindType::HELM:
    case ItemKindType::CROWN:
    case ItemKindType::SWORD:
    case ItemKindType::POLEARM:
    case ItemKindType::HAFTED:
    case ItemKindType::DIGGING:
    case ItemKindType::BOW:
        return mass_equipment_produce(o_ptr, cost);
    case ItemKindType::SPIKE:
    case ItemKindType::SHOT:
    case ItemKindType::ARROW:
    case ItemKindType::BOLT:
        return mass_arrow_produce(cost);
    case ItemKindType::FIGURINE:
        return mass_figurine_produce(cost);
    case ItemKindType::CAPTURE:
    case ItemKindType::STATUE:
    case ItemKindType::CARD:
        return 1;
    case ItemKindType::ROD:
    case ItemKindType::WAND:
    case ItemKindType::STAFF:
        return mass_magic_produce(cost);
    default:
        return 1;
    }
}

static byte decide_discount_rate(const PRICE cost)
{
    if (cost < 5)
        return 0;
    
    if (one_in_(25))
        return 25;
    
    if (one_in_(150))
        return 50;
    
    if (one_in_(300))
        return 75;
    
    if (one_in_(500))
        return 90;

    return 0;
}

/*!
 * @brief 安価な消耗品の販売数を増やし、低確率で割引にする /
 * Certain "cheap" objects should be created in "piles"
 * @param o_ptr 店舗に並べるオブジェクト構造体の参照ポインタ
 * @details
 * <pre>
 * Some objects can be sold at a "discount" (in small piles)
 * </pre>
 */
void mass_produce(PlayerType *, object_type *o_ptr)
{
    const PRICE cost = object_value(o_ptr);
    int size = switch_mass_production(o_ptr, cost);
    auto discount = decide_discount_rate(cost);
    if (o_ptr->art_name)
        discount = 0;

    o_ptr->discount = discount;
    o_ptr->number = size - (size * discount / 100);
    if ((o_ptr->tval == ItemKindType::ROD) || (o_ptr->tval == ItemKindType::WAND))
        o_ptr->pval *= (PARAMETER_VALUE)o_ptr->number;
}
