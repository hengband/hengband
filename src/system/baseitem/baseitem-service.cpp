#include "system/baseitem/baseitem-service.h"
#include "object/tval-types.h"
#include "sv-definition/sv-potion-types.h"
#include "sv-definition/sv-staff-types.h"
#include "system/baseitem/baseitem-config.h"
#include "system/baseitem/baseitem-configs.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/baseitem/baseitem-record.h"
#include "system/baseitem/baseitem-records.h"
#include "term/z-rand.h"
#include "util/finalizer.h"
#include "view/display-symbol.h"
#include <functional>
#include <range/v3/algorithm/for_each.hpp>
#include <range/v3/view.hpp>

void BaseitemService::initialize_baseitem_records()
{
    const auto &baseitems = BaseitemList::get_instance();
    BaseitemRecords::get_instance().initialize(baseitems.size());
}

void BaseitemService::initialize_baseitem_configs()
{
    const auto &baseitems = BaseitemList::get_instance();
    auto &baseitem_configs = BaseitemConfigs::get_instance();
    const auto range = ranges::views::iota(0) | ranges::views::take(baseitems.size());
    ranges::for_each(range, [&baseitems, &baseitem_configs](short bi_id) {
        const auto &baseitem = baseitems.get_baseitem(bi_id);
        baseitem_configs.emplace_back(baseitem.get_symbol());
    });
}

void BaseitemService::reset_all_visuals()
{
    const auto &baseitems = BaseitemList::get_instance();
    auto &baseitem_configs = BaseitemConfigs::get_instance();
    const auto range = ranges::views::iota(0) | ranges::views::take(baseitems.size());
    ranges::for_each(range, [&baseitems, &baseitem_configs](short bi_id) {
        const auto &baseitem = baseitems.get_baseitem(bi_id);
        baseitem_configs.set_config(bi_id, baseitem.get_symbol());
    });
}

const BaseitemConfig &BaseitemService::pick_one_at_random()
{
    const auto &baseitems = BaseitemList::get_instance();
    const auto &baseitem_configs = BaseitemConfigs::get_instance();
    while (true) {
        const auto bi_id = randnum1<short>(baseitems.size() - 1); // 0は無効値なので最初から選ばない
        if (baseitems.is_valid(bi_id)) {
            return baseitem_configs.get_config(bi_id);
        }
    }
}

const DisplaySymbol &BaseitemService::get_dummy_symbol()
{
    static const auto ds = BaseitemConfigs::get_instance().get_config(0).get_symbol();
    return ds;
}

void BaseitemService::shuffle_flavors()
{
    shuffle_flavors(ItemKindType::RING);
    shuffle_flavors(ItemKindType::AMULET);
    shuffle_flavors(ItemKindType::STAFF);
    shuffle_flavors(ItemKindType::WAND);
    shuffle_flavors(ItemKindType::ROD);
    shuffle_flavors(ItemKindType::FOOD);
    shuffle_flavors(ItemKindType::POTION);
    shuffle_flavors(ItemKindType::SCROLL);
}

/*!
 * @brief 未鑑定アイテム種別の内、ゲーム開始時から鑑定済とするアイテムの鑑定済フラグをONにする
 * @todo 食料用の杖は該当種族 (ゴーレム/骸骨/ゾンビ/幽霊)では鑑定済だが、本来はこのメソッドで鑑定済にすべき.
 */
void BaseitemService::mark_common_items_as_aware()
{
    std::vector<BaseitemKey> bi_keys;
    bi_keys.emplace_back(ItemKindType::POTION, SV_POTION_WATER);
    bi_keys.emplace_back(ItemKindType::STAFF, SV_STAFF_NOTHING);
    const auto &baseitems = BaseitemList::get_instance();
    auto &baseitem_records = BaseitemRecords::get_instance();
    for (const auto &bi_key : bi_keys) {
        const auto bi_id = baseitems.lookup_baseitem_id(bi_key);
        baseitem_records.get_record(bi_id).mark_awareness(true);
    }
}

/*!
 * @brief ゲーム開始時に行われるベースアイテムの初期化ルーチン
 */
void BaseitemService::initialize_items_flavor()
{
    auto &system = AngbandSystem::get_instance();
    auto &baseitems = BaseitemList::get_instance();
    auto &baseitem_records = BaseitemRecords::get_instance();
    for (short bi_id : baseitems.collect_valid_bi_ids()) {
        const auto &baseitem = baseitems.get_baseitem(bi_id);
        if (baseitem.flavor_name.empty()) {
            continue;
        }

        auto &baseitem_record = baseitem_records.get_record(bi_id);
        baseitem_record.set_appearance_id(bi_id);
    }

    {
        const auto restore_rng = util::make_finalizer([&system, rng_backup = system.get_rng()]() { system.set_rng(rng_backup); });
        xso::rng32 flavor_rng(system.get_seed_flavor());
        system.set_rng(flavor_rng);
        BaseitemService::shuffle_flavors();
    }

    for (short bi_id : baseitems.collect_valid_bi_ids()) {
        auto &baseitem_record = baseitem_records.get_record(bi_id);
        if (!baseitem_record.is_apparent()) {
            baseitem_record.mark_awareness(true);
        }

        baseitems.get_baseitem(bi_id).decide_easy_know();
    }
}

/*!
 * @brief ベースアイテムの未確定名を共通tval間でシャッフルする
 * @param tval シャッフルしたいtval
 * @details 巻物、各種魔道具などに利用される。
 */
void BaseitemService::shuffle_flavors(ItemKindType tval)
{
    const auto &baseitems = BaseitemList::get_instance();
    auto &baseitem_records = BaseitemRecords::get_instance();
    std::vector<std::reference_wrapper<short>> flavors;
    for (auto bi_id : baseitems.collect_valid_bi_ids()) {
        const auto &baseitem = baseitems.get_baseitem(bi_id);
        auto &baseitem_record = baseitem_records.get_record(bi_id);
        if (baseitem.bi_key.tval() != tval) {
            continue;
        }

        if (!baseitem_record.is_apparent()) {
            continue;
        }

        if (baseitem.flags.has(TR_FIXED_FLAVOR)) {
            continue;
        }

        flavors.push_back(baseitem_record.get_appearance_id_ref());
    }

    rand_shuffle(flavors.begin(), flavors.end());
}
