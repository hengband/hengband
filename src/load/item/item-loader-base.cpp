#include "load/item/item-loader-base.h"
#include "artifact/fixed-art-types.h"
#include "load/angband-version-comparer.h"
#include "load/load-util.h"
#include "system/artifact-type-definition.h"
#include "system/baseitem-info-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"

/*!
 * @brief アイテムオブジェクトの鑑定情報をロードする.
 */
void ItemLoaderBase::load_item(void)
{
    auto loading_max_k_idx = rd_u16b();
    BaseItemInfo dummy;
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
    ArtifactType dummy;
    auto loading_max_a_idx = rd_u16b();
    for (auto i = 0U; i < loading_max_a_idx; i++) {
        const auto a_idx = i2enum<FixedArtifactId>(i);
        const auto it = a_info.find(a_idx);
        auto &artifact = it != a_info.end() ? it->second : dummy;
        artifact.is_generated = rd_bool();
        if (h_older_than(1, 5, 0, 0)) {
            artifact.floor_id = 0;
            strip_bytes(3);
        } else {
            artifact.floor_id = rd_s16b();
        }
    }

    load_note(_("伝説のアイテムをロードしました", "Loaded Artifacts"));
}
