#include "object/object-flags.h"
#include "mind/mind-weaponsmith.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "object/object-kind.h"
#include "perception/object-perception.h"
#include "smith/object-smith.h"
#include "sv-definition/sv-lite-types.h"
#include "system/artifact-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief 光源用のフラグを付与する
 * @param o_ptr フラグ取得元のオブジェクト構造体ポインタ
 * @param flgs フラグ情報を受け取る配列
 */
static void object_flags_lite(const ObjectType *o_ptr, TrFlags &flgs)
{
    if (!o_ptr->is_ego()) {
        return;
    }

    auto *e_ptr = &e_info[o_ptr->ego_idx];
    flgs.set(e_ptr->flags);

    auto is_out_of_fuel = o_ptr->fuel == 0;
    if ((o_ptr->ego_idx == EgoType::AURA_FIRE) && is_out_of_fuel && (o_ptr->sval <= SV_LITE_LANTERN)) {
        flgs.reset(TR_SH_FIRE);
        return;
    }

    if ((o_ptr->ego_idx == EgoType::LITE_INFRA) && is_out_of_fuel && (o_ptr->sval <= SV_LITE_LANTERN)) {
        flgs.reset(TR_INFRA);
        return;
    }

    if ((o_ptr->ego_idx == EgoType::LITE_EYE) && is_out_of_fuel && (o_ptr->sval <= SV_LITE_LANTERN)) {
        flgs.reset(TR_RES_BLIND);
        flgs.reset(TR_SEE_INVIS);
    }
}

/*!
 * @brief オブジェクトのフラグ類を配列に与える
 * @param o_ptr フラグ取得元のオブジェクト構造体ポインタ
 * @param flgs フラグ情報を受け取る配列
 */
TrFlags object_flags(const ObjectType *o_ptr)
{
    auto *k_ptr = &k_info[o_ptr->k_idx];

    /* Base object */
    auto flgs = k_ptr->flags;

    if (o_ptr->is_fixed_artifact()) {
        flgs = a_info[o_ptr->fixed_artifact_idx].flags;
    }

    object_flags_lite(o_ptr, flgs);

    /* Random artifact ! */
    flgs.set(o_ptr->art_flags);

    if (auto effect = Smith::object_effect(o_ptr); effect.has_value()) {
        auto tr_flags = Smith::get_effect_tr_flags(effect.value());
        flgs.set(tr_flags);
    }
    if (Smith::object_activation(o_ptr).has_value()) {
        flgs.set(TR_ACTIVATE);
    }

    return flgs;
}

/*!
 * @brief オブジェクトの明示されているフラグ類を取得する
 * Obtain the "flags" for an item which are known to the player
 * @param o_ptr フラグ取得元のオブジェクト構造体ポインタ
 * @param flgs フラグ情報を受け取る配列
 */
TrFlags object_flags_known(const ObjectType *o_ptr)
{
    bool spoil = false;
    auto *k_ptr = &k_info[o_ptr->k_idx];
    TrFlags flgs{};

    if (!o_ptr->is_aware())
        return flgs;

    /* Base object */
    flgs = k_ptr->flags;

    if (!o_ptr->is_known())
        return flgs;

    object_flags_lite(o_ptr, flgs);
    if (spoil || o_ptr->is_fully_known()) {
        if (o_ptr->is_fixed_artifact()) {
            flgs = a_info[o_ptr->fixed_artifact_idx].flags;
        }

        /* Random artifact ! */
        flgs.set(o_ptr->art_flags);
    }

    if (auto effect = Smith::object_effect(o_ptr); effect.has_value()) {
        auto tr_flags = Smith::get_effect_tr_flags(effect.value());
        flgs.set(tr_flags);
    }
    if (Smith::object_activation(o_ptr).has_value()) {
        flgs.set(TR_ACTIVATE);
    }

    return flgs;
}
