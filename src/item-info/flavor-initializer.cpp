/*!
 * @brief アイテムのフレーバー初期化 (未鑑定名のシャッフル処理)
 * @author Hourier
 * @date 2022/12/28
 */

#include "item-info/flavor-initializer.h"
#include "object/tval-types.h"
#include "system/angband-system.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/baseitem/baseitem-record.h"
#include "system/baseitem/baseitem-records.h"
#include "system/baseitem/baseitem-service.h"
#include "util/finalizer.h"

/*!
 * @brief ゲーム開始時に行われるベースアイテムの初期化ルーチン
 */
void initialize_items_flavor()
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
        auto &baseitem = baseitem_records.get_record(bi_id);
        if (!baseitem.is_apparent()) {
            baseitem.mark_awareness(true);
        }

        baseitems.get_baseitem(bi_id).decide_easy_know();
    }
}
