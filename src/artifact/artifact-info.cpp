/*!
 * @file artifact-info.cpp
 * @brief アーティファクトの発動効果取得関数定義
 */

#include "artifact/artifact-info.h"
#include "artifact/random-art-effects.h"
#include "object-enchant/activation-info-table.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "object/object-kind.h"
#include "smith/object-smith.h"
#include "system/artifact-type-definition.h"
#include "system/object-type-definition.h"
#include "util/enum-converter.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief オブジェクトから能力発動IDを取得する。
 * @details いくつかのケースで定義されている発動効果から、
 * 鍛冶師による付与＞固定アーティファクト＞エゴ＞ランダムアーティファクト＞ベースアイテムの優先順位で走査していく。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 発動効果のIDを返す
 */
RandomArtActType activation_index(const ObjectType *o_ptr)
{
    if (auto act_idx = Smith::object_activation(o_ptr); act_idx.has_value()) {
        return act_idx.value();
    }

    if (o_ptr->is_fixed_artifact() && a_info[o_ptr->name1].flags.has(TR_ACTIVATE))
        return a_info[o_ptr->name1].act_idx;

    if (o_ptr->is_ego() && e_info[enum2i<EgoType>(o_ptr->name2)].flags.has(TR_ACTIVATE))
        return e_info[enum2i<EgoType>(o_ptr->name2)].act_idx;

    if (!o_ptr->is_random_artifact() && k_info[o_ptr->k_idx].flags.has(TR_ACTIVATE))
        return k_info[o_ptr->k_idx].act_idx;

    return o_ptr->activation_id;
}

/*!
 * @brief オブジェクトから発動効果構造体のポインタを取得する。
 * @details activation_index() 関数の結果から参照する。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 発動効果構造体のポインタを返す
 */
std::optional<const activation_type *> find_activation_info(const ObjectType *o_ptr)
{
    const auto index = activation_index(o_ptr);
    for (const auto &p : activation_info) {
        if (p.index == index) {
            return &p;
        }
    }

    return std::nullopt;
}
