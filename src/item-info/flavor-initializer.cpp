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

/*!
 * @brief ゲーム開始時に行われるベースアイテムの初期化ルーチン
 */
void initialize_items_flavor()
{
    auto &system = AngbandSystem::get_instance();
    const Xoshiro128StarStar rng_backup = system.get_rng();
    Xoshiro128StarStar flavor_rng(system.get_seed_flavor());
    system.set_rng(flavor_rng);
    auto &baseitems = BaseitemList::get_instance();
    for (auto &baseitem : baseitems) {
        if (baseitem.flavor_name.empty()) {
            continue;
        }

        baseitem.flavor = baseitem.idx;
    }

    baseitems.shuffle_flavors();
    system.set_rng(rng_backup);
    for (auto &baseitem : baseitems) {
        if (!baseitem.is_valid()) {
            continue;
        }

        if (!baseitem.flavor) {
            baseitem.mark_awareness(true);
        }

        baseitem.decide_easy_know();
    }
}
