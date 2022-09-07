#include "load/item/item-loader-base.h"
#include "load/angband-version-comparer.h"
#include "load/load-util.h"
#include "object/object-kind.h"
#include "system/artifact-type-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief アイテムオブジェクトの鑑定情報をロードする.
 */
void ItemLoaderBase::load_item(void)
{
    auto loading_max_k_idx = rd_u16b();
    object_kind dummy;
    for (auto i = 0U; i < loading_max_k_idx; i++) {
        auto *k_ptr = i < k_info.size() ? &k_info[i] : &dummy;
        auto tmp8u = rd_byte();
        k_ptr->aware = any_bits(tmp8u, 0x01);
        k_ptr->tried = any_bits(tmp8u, 0x02);
    }

    load_note(_("アイテムの記録をロードしました", "Loaded Object Memory"));
}

/*!
 * @brief 固定アーティファクトの出現情報をロードする.
 */
void ItemLoaderBase::load_artifact(void)
{
    auto loading_max_a_idx = rd_u16b();
    artifact_type dummy;
    for (auto i = 0U; i < loading_max_a_idx; i++) {
        auto *a_ptr = i < a_info.size() ? &a_info[i] : &dummy;
        a_ptr->is_generated = rd_bool();
        if (h_older_than(1, 5, 0, 0)) {
            a_ptr->floor_id = 0;
            strip_bytes(3);
        } else {
            a_ptr->floor_id = rd_s16b();
        }
    }

    load_note(_("伝説のアイテムをロードしました", "Loaded Artifacts"));
}
