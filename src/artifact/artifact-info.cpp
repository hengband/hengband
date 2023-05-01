/*!
 * @file artifact-info.cpp
 * @brief アーティファクトの発動効果取得関数定義
 */

#include "artifact/artifact-info.h"
#include "artifact/random-art-effects.h"
#include "object-enchant/activation-info-table.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "smith/object-smith.h"
#include "system/artifact-type-definition.h"
#include "system/baseitem-info.h"
#include "system/item-entity.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"

/*!
 * @brief オブジェクトから能力発動IDを取得する。
 * @details いくつかのケースで定義されている発動効果から、
 * 鍛冶師による付与＞固定アーティファクト＞エゴ＞ランダムアーティファクト＞ベースアイテムの優先順位で走査していく。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 発動効果のIDを返す
 */
RandomArtActType activation_index(const ItemEntity *o_ptr)
{
    if (auto act_idx = Smith::object_activation(o_ptr); act_idx.has_value()) {
        return act_idx.value();
    }

    if (o_ptr->is_fixed_artifact()) {
        const auto &artifact = o_ptr->get_fixed_artifact();
        if (artifact.flags.has(TR_ACTIVATE)) {
            return artifact.act_idx;
        }
    }

    if (o_ptr->is_ego()) {
        const auto &ego = o_ptr->get_ego();
        if (ego.flags.has(TR_ACTIVATE)) {
            return ego.act_idx;
        }
    }

    if (!o_ptr->is_random_artifact()) {
        const auto &baseitem = o_ptr->get_baseitem();
        if (baseitem.flags.has(TR_ACTIVATE)) {
            return baseitem.act_idx;
        }
    }

    return o_ptr->activation_id;
}

/*!
 * @brief オブジェクトから発動効果構造体のポインタを取得する。
 * @details activation_index() 関数の結果から参照する。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 発動効果構造体のポインタを返す
 */
std::optional<const activation_type *> find_activation_info(const ItemEntity *o_ptr)
{
    const auto index = activation_index(o_ptr);
    for (const auto &p : activation_info) {
        if (p.index == index) {
            return &p;
        }
    }

    return std::nullopt;
}
