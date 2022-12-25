#include "store/service-checker.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags3.h"
#include "object-enchant/tr-types.h"
#include "object/object-flags.h"
#include "object/object-value.h"
#include "object/tval-types.h"
#include "store/store-util.h"
#include "sv-definition/sv-potion-types.h"
#include "sv-definition/sv-rod-types.h"
#include "sv-definition/sv-scroll-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/item-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"

/*!
 * @brief オブジェクトが祝福されているかの判定を返す /
 * @param item_ptr 判定したいオブジェクト構造体の参照ポインタ
 * @return アイテムが祝福されたアイテムならばTRUEを返す
 */
static bool is_blessed_item(const ItemEntity *item_ptr)
{
    auto flgs = object_flags(item_ptr);
    return flgs.has(TR_BLESSED);
}

static bool check_store_general(const ItemEntity &item)
{
    const auto &bi_key = item.bi_key;
    switch (bi_key.tval()) {
    case ItemKindType::ROD:
        return (bi_key.sval() == SV_ROD_PESTICIDE);
    case ItemKindType::POTION:
        return (bi_key.sval() == SV_POTION_WATER);
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

static bool check_store_armoury(const ItemEntity &item)
{
    switch (item.bi_key.tval()) {
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

static bool check_store_weapon(const ItemEntity &item)
{
    switch (item.bi_key.tval()) {
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
        return item.bi_key.sval() != SV_WIZSTAFF;
    default:
        return false;
    }
}

static bool check_store_temple(const ItemEntity &item)
{
    switch (item.bi_key.tval()) {
    case ItemKindType::LIFE_BOOK:
    case ItemKindType::CRUSADE_BOOK:
    case ItemKindType::SCROLL:
    case ItemKindType::POTION:
    case ItemKindType::HAFTED:
        return true;
    case ItemKindType::FIGURINE:
    case ItemKindType::STATUE: {
        auto *r_ptr = &monraces_info[i2enum<MonsterRaceId>(item.pval)];
        if (r_ptr->kind_flags.has_not(MonsterKindType::EVIL)) {
            if ((r_ptr->kind_flags.has(MonsterKindType::GOOD)) || (r_ptr->kind_flags.has(MonsterKindType::ANIMAL)) || (angband_strchr("?!", r_ptr->d_char) != nullptr)) {
                return true;
            }
        }
    }
        [[fallthrough]];
    case ItemKindType::POLEARM:
    case ItemKindType::SWORD:
        if (is_blessed_item(&item)) {
            return true;
        }

        [[fallthrough]];
    default:
        return false;
    }
}

static bool check_store_alchemist(const ItemEntity &item)
{
    switch (item.bi_key.tval()) {
    case ItemKindType::SCROLL:
    case ItemKindType::POTION:
        return true;
    default:
        return false;
    }
}

static bool check_store_magic(const ItemEntity &item)
{
    switch (item.bi_key.tval()) {
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
        return item.bi_key.sval() == SV_WIZSTAFF;
    default:
        return false;
    }
}

static bool check_store_book(const ItemEntity &item)
{
    switch (item.bi_key.tval()) {
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

static bool switch_store_check(const ItemEntity &item, StoreSaleType store_num)
{
    switch (store_num) {
    case StoreSaleType::GENERAL:
        return check_store_general(item);
    case StoreSaleType::ARMOURY:
        return check_store_armoury(item);
    case StoreSaleType::WEAPON:
        return check_store_weapon(item);
    case StoreSaleType::TEMPLE:
        return check_store_temple(item);
    case StoreSaleType::ALCHEMIST:
        return check_store_alchemist(item);
    case StoreSaleType::MAGIC:
        return check_store_magic(item);
    case StoreSaleType::BOOK:
        return check_store_book(item);
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

bool store_will_buy(PlayerType *, const ItemEntity *o_ptr, StoreSaleType store_num)
{
    if ((store_num == StoreSaleType::HOME) || (store_num == StoreSaleType::MUSEUM)) {
        return true;
    }

    if (!switch_store_check(*o_ptr, store_num)) {
        return false;
    }

    return o_ptr->get_price() > 0;
}

static int mass_lite_produce(const PRICE cost)
{
    int size = 1;
    if (cost <= 5L) {
        size += damroll(3, 5);
    }

    if (cost <= 20L) {
        size += damroll(3, 5);
    }

    if (cost <= 50L) {
        size += damroll(2, 2);
    }

    return size;
}

static int mass_scroll_produce(const ItemEntity &item, const PRICE cost)
{
    int size = 1;
    if (cost <= 60L) {
        size += damroll(3, 5);
    }

    if (cost <= 240L) {
        size += damroll(1, 5);
    }

    const auto sval = item.bi_key.sval();
    if (sval == SV_SCROLL_STAR_IDENTIFY) {
        size += damroll(3, 5);
    }

    if (sval == SV_SCROLL_STAR_REMOVE_CURSE) {
        size += damroll(1, 4);
    }

    return size;
}

static int mass_book_produce(const PRICE cost)
{
    int size = 1;
    if (cost <= 50L) {
        size += damroll(2, 3);
    }

    if (cost <= 500L) {
        size += damroll(1, 3);
    }

    return size;
}

static int mass_equipment_produce(const ItemEntity &item, const PRICE cost)
{
    int size = 1;
    if (item.is_artifact() || item.is_ego()) {
        return size;
    }

    if (cost <= 10L) {
        size += damroll(3, 5);
    }

    if (cost <= 100L) {
        size += damroll(3, 5);
    }

    return size;
}

static int mass_arrow_produce(const PRICE cost)
{
    int size = 1;
    if (cost <= 5L) {
        size += damroll(5, 5);
    }

    if (cost <= 50L) {
        size += damroll(5, 5);
    }

    if (cost <= 500L) {
        size += damroll(5, 5);
    }

    return size;
}

static int mass_figurine_produce(const PRICE cost)
{
    int size = 1;
    if (cost <= 100L) {
        size += damroll(2, 2);
    }

    if (cost <= 1000L) {
        size += damroll(2, 2);
    }

    return size;
}

static int mass_magic_produce(const PRICE cost, StoreSaleType store_num)
{
    int size = 1;
    if ((store_num != StoreSaleType::BLACK) || !one_in_(3)) {
        return size;
    }

    if (cost < 1601L) {
        size += damroll(1, 5);
    } else if (cost < 3201L) {
        size += damroll(1, 3);
    }

    return size;
}

static int switch_mass_production(const ItemEntity &item, const PRICE cost, StoreSaleType store_num)
{
    switch (item.bi_key.tval()) {
    case ItemKindType::FOOD:
    case ItemKindType::FLASK:
    case ItemKindType::LITE:
        return mass_lite_produce(cost);
    case ItemKindType::POTION:
    case ItemKindType::SCROLL:
        return mass_scroll_produce(item, cost);
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
        return mass_equipment_produce(item, cost);
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
        return mass_magic_produce(cost, store_num);
    default:
        return 1;
    }
}

static byte decide_discount_rate(const PRICE cost)
{
    if (cost < 5) {
        return 0;
    }

    if (one_in_(25)) {
        return 25;
    }

    if (one_in_(150)) {
        return 50;
    }

    if (one_in_(300)) {
        return 75;
    }

    if (one_in_(500)) {
        return 90;
    }

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
void mass_produce(ItemEntity *o_ptr, StoreSaleType store_num)
{
    const auto cost = o_ptr->get_price();
    int size = switch_mass_production(*o_ptr, cost, store_num);
    auto discount = decide_discount_rate(cost);
    if (o_ptr->is_random_artifact()) {
        discount = 0;
    }

    o_ptr->discount = discount;
    o_ptr->number = size - (size * discount / 100);
    if (o_ptr->is_wand_rod()) {
        o_ptr->pval *= (PARAMETER_VALUE)o_ptr->number;
    }
}
