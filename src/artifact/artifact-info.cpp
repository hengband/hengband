/*!
 * @file artifact-info.cpp 
 * @brief アーティファクトの発動効果取得関数定義
 */

#include "artifact/artifact-info.h"
#include "artifact/random-art-effects.h"
#include "cmd-item/cmd-smith.h"
#include "mind/mind-weaponsmith.h"
#include "object-enchant/activation-info-table.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "object/object-kind.h"
#include "system/artifact-type-definition.h"
#include "system/object-type-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief オブジェクトから能力発動IDを取得する。
 * @details いくつかのケースで定義されている発動効果から、
 * 鍛冶師による付与＞固定アーティファクト＞エゴ＞ランダムアーティファクト＞ベースアイテムの優先順位で走査していく。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 発動効果のIDを返す
 */
int activation_index(const object_type *o_ptr)
{
    if (o_ptr->is_smith()) {
        switch (o_ptr->xtra3 - 1) {
        case ESSENCE_TMP_RES_ACID:
            return ACT_RESIST_ACID;
        case ESSENCE_TMP_RES_ELEC:
            return ACT_RESIST_ELEC;
        case ESSENCE_TMP_RES_FIRE:
            return ACT_RESIST_FIRE;
        case ESSENCE_TMP_RES_COLD:
            return ACT_RESIST_COLD;
        case TR_EARTHQUAKE:
            return ACT_QUAKE;
        }
    }

    if (o_ptr->is_fixed_artifact() && a_info[o_ptr->name1].flags.has(TR_ACTIVATE))
        return a_info[o_ptr->name1].act_idx;

    if (o_ptr->is_ego() && e_info[o_ptr->name2].flags.has(TR_ACTIVATE))
        return e_info[o_ptr->name2].act_idx;

    if (!o_ptr->is_random_artifact() && k_info[o_ptr->k_idx].flags.has(TR_ACTIVATE))
        return k_info[o_ptr->k_idx].act_idx;

    return o_ptr->xtra2;
}

/*!
 * @brief オブジェクトから発動効果構造体のポインタを取得する。
 * @details activation_index() 関数の結果から参照する。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 発動効果構造体のポインタを返す
 */
const activation_type *find_activation_info(const object_type *o_ptr)
{
    const int index = activation_index(o_ptr);
    const activation_type *p;
    for (p = activation_info; p->flag != nullptr; ++p)
        if (p->index == index)
            return p;

    return nullptr;
}
