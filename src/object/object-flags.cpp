#include "object/object-flags.h"
#include "mind/mind-weaponsmith.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "perception/object-perception.h"
#include "smith/object-smith.h"
#include "system/artifact-type-definition.h"
#include "system/baseitem-info.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief 光源用のフラグを付与する
 * @param o_ptr フラグ取得元のオブジェクト構造体ポインタ
 * @param flags フラグ情報を受け取る配列
 */
static void object_flags_lite(const ItemEntity *o_ptr, TrFlags &flags)
{
    if (!o_ptr->is_ego()) {
        return;
    }

    const auto &ego = egos_info[o_ptr->ego_idx];
    flags.set(ego.flags);

    const auto is_out_of_fuel = o_ptr->fuel == 0;
    if ((o_ptr->ego_idx == EgoType::AURA_FIRE) && is_out_of_fuel && o_ptr->is_lite_requiring_fuel()) {
        flags.reset(TR_SH_FIRE);
        return;
    }

    if ((o_ptr->ego_idx == EgoType::LITE_INFRA) && is_out_of_fuel && o_ptr->is_lite_requiring_fuel()) {
        flags.reset(TR_INFRA);
        return;
    }

    if ((o_ptr->ego_idx == EgoType::LITE_EYE) && is_out_of_fuel && o_ptr->is_lite_requiring_fuel()) {
        flags.reset(TR_RES_BLIND);
        flags.reset(TR_SEE_INVIS);
    }
}

/*!
 * @brief オブジェクトのフラグ類を配列に与える
 * @param o_ptr フラグ取得元のオブジェクト構造体ポインタ
 * @param flags フラグ情報を受け取る配列
 */
TrFlags object_flags(const ItemEntity *o_ptr)
{
    const auto &baseitem = baseitems_info[o_ptr->bi_id];
    auto flags = baseitem.flags;

    if (o_ptr->is_fixed_artifact()) {
        flags = artifacts_info.at(o_ptr->fixed_artifact_idx).flags;
    }

    object_flags_lite(o_ptr, flags);
    flags.set(o_ptr->art_flags);
    if (auto effect = Smith::object_effect(o_ptr); effect.has_value()) {
        auto tr_flags = Smith::get_effect_tr_flags(effect.value());
        flags.set(tr_flags);
    }

    if (Smith::object_activation(o_ptr).has_value()) {
        flags.set(TR_ACTIVATE);
    }

    return flags;
}

/*!
 * @brief オブジェクトの明示されているフラグ類を取得する
 * Obtain the "flags" for an item which are known to the player
 * @param o_ptr フラグ取得元のオブジェクト構造体ポインタ
 * @param flags フラグ情報を受け取る配列
 */
TrFlags object_flags_known(const ItemEntity *o_ptr)
{
    TrFlags flags{};
    if (!o_ptr->is_aware()) {
        return flags;
    }

    const auto &baseitem = baseitems_info[o_ptr->bi_id];
    flags = baseitem.flags;
    if (!o_ptr->is_known()) {
        return flags;
    }

    object_flags_lite(o_ptr, flags);
    if (o_ptr->is_fully_known()) {
        if (o_ptr->is_fixed_artifact()) {
            flags = artifacts_info.at(o_ptr->fixed_artifact_idx).flags;
        }

        flags.set(o_ptr->art_flags);
    }

    if (auto effect = Smith::object_effect(o_ptr); effect.has_value()) {
        auto tr_flags = Smith::get_effect_tr_flags(effect.value());
        flags.set(tr_flags);
    }

    if (Smith::object_activation(o_ptr).has_value()) {
        flags.set(TR_ACTIVATE);
    }

    return flags;
}
