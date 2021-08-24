#include "object/object-flags.h"
#include "cmd-item/cmd-smith.h" //!< @todo 相互参照している.
#include "mind/mind-weaponsmith.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "object-hook/hook-enchant.h"
#include "object/object-kind.h"
#include "perception/object-perception.h"
#include "sv-definition/sv-lite-types.h"
#include "system/artifact-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief オブジェクトのフラグ類を配列に与える
 * Obtain the "flags" for an item
 * @param o_ptr フラグ取得元のオブジェクト構造体ポインタ
 * @param flgs フラグ情報を受け取る配列
 */
void object_flags(const object_type *o_ptr, TrFlags &flgs)
{
    object_kind *k_ptr = &k_info[o_ptr->k_idx];

    /* Base object */
    for (int i = 0; i < TR_FLAG_SIZE; i++) {
        flgs[i] = k_ptr->flags[i];
    }

    if (object_is_fixed_artifact(o_ptr)) {
        artifact_type *a_ptr = &a_info[o_ptr->name1];
        for (int i = 0; i < TR_FLAG_SIZE; i++) {
            flgs[i] = a_ptr->flags[i];
        }
    }

    if (object_is_ego(o_ptr)) {
        ego_item_type *e_ptr = &e_info[o_ptr->name2];
        for (int i = 0; i < TR_FLAG_SIZE; i++) {
            flgs[i] |= e_ptr->flags[i];
        }

        if ((o_ptr->name2 == EGO_LITE_AURA_FIRE) && !o_ptr->xtra4 && (o_ptr->sval <= SV_LITE_LANTERN)) {
            remove_flag(flgs, TR_SH_FIRE);
        } else if ((o_ptr->name2 == EGO_LITE_INFRA) && !o_ptr->xtra4 && (o_ptr->sval <= SV_LITE_LANTERN)) {
            remove_flag(flgs, TR_INFRA);
        } else if ((o_ptr->name2 == EGO_LITE_EYE) && !o_ptr->xtra4 && (o_ptr->sval <= SV_LITE_LANTERN)) {
            remove_flag(flgs, TR_RES_BLIND);
            remove_flag(flgs, TR_SEE_INVIS);
        }
    }

    /* Random artifact ! */
    for (int i = 0; i < TR_FLAG_SIZE; i++) {
        flgs[i] |= o_ptr->art_flags[i];
    }

    if (object_is_smith(o_ptr)) {
        int add = o_ptr->xtra3 - 1;
        if (add < TR_FLAG_MAX) {
            add_flag(flgs, add);
        } else if (add == ESSENCE_TMP_RES_ACID) {
            add_flag(flgs, TR_RES_ACID);
            add_flag(flgs, TR_ACTIVATE);
        } else if (add == ESSENCE_TMP_RES_ELEC) {
            add_flag(flgs, TR_RES_ELEC);
            add_flag(flgs, TR_ACTIVATE);
        } else if (add == ESSENCE_TMP_RES_FIRE) {
            add_flag(flgs, TR_RES_FIRE);
            add_flag(flgs, TR_ACTIVATE);
        } else if (add == ESSENCE_TMP_RES_COLD) {
            add_flag(flgs, TR_RES_COLD);
            add_flag(flgs, TR_ACTIVATE);
        } else if (add == ESSENCE_SH_FIRE) {
            add_flag(flgs, TR_RES_FIRE);
            add_flag(flgs, TR_SH_FIRE);
        } else if (add == ESSENCE_SH_ELEC) {
            add_flag(flgs, TR_RES_ELEC);
            add_flag(flgs, TR_SH_ELEC);
        } else if (add == ESSENCE_SH_COLD) {
            add_flag(flgs, TR_RES_COLD);
            add_flag(flgs, TR_SH_COLD);
        } else if (add == ESSENCE_RESISTANCE) {
            add_flag(flgs, TR_RES_ACID);
            add_flag(flgs, TR_RES_ELEC);
            add_flag(flgs, TR_RES_FIRE);
            add_flag(flgs, TR_RES_COLD);
        } else if (add == TR_EARTHQUAKE) {
            add_flag(flgs, TR_ACTIVATE);
        }
    }
}

/*!
 * @brief オブジェクトの明示されているフラグ類を取得する
 * Obtain the "flags" for an item which are known to the player
 * @param o_ptr フラグ取得元のオブジェクト構造体ポインタ
 * @param flgs フラグ情報を受け取る配列
 */
void object_flags_known(const object_type *o_ptr, TrFlags &flgs)
{
    bool spoil = false;
    object_kind *k_ptr = &k_info[o_ptr->k_idx];
    for (int i = 0; i < TR_FLAG_SIZE; i++) {
        flgs[i] = 0;
    }

    if (!object_is_aware(o_ptr))
        return;

    /* Base object */
    for (int i = 0; i < TR_FLAG_SIZE; i++) {
        flgs[i] = k_ptr->flags[i];
    }

    if (!object_is_known(o_ptr))
        return;

    if (object_is_ego(o_ptr)) {
        ego_item_type *e_ptr = &e_info[o_ptr->name2];
        for (int i = 0; i < TR_FLAG_SIZE; i++) {
            flgs[i] |= e_ptr->flags[i];
        }

        if ((o_ptr->name2 == EGO_LITE_AURA_FIRE) && !o_ptr->xtra4 && (o_ptr->sval <= SV_LITE_LANTERN)) {
            remove_flag(flgs, TR_SH_FIRE);
        } else if ((o_ptr->name2 == EGO_LITE_INFRA) && !o_ptr->xtra4 && (o_ptr->sval <= SV_LITE_LANTERN)) {
            remove_flag(flgs, TR_INFRA);
        } else if ((o_ptr->name2 == EGO_LITE_EYE) && !o_ptr->xtra4 && (o_ptr->sval <= SV_LITE_LANTERN)) {
            remove_flag(flgs, TR_RES_BLIND);
            remove_flag(flgs, TR_SEE_INVIS);
        }
    }

    if (spoil || object_is_fully_known(o_ptr)) {
        if (object_is_fixed_artifact(o_ptr)) {
            artifact_type *a_ptr = &a_info[o_ptr->name1];

            for (int i = 0; i < TR_FLAG_SIZE; i++) {
                flgs[i] = a_ptr->flags[i];
            }
        }

        /* Random artifact ! */
        for (int i = 0; i < TR_FLAG_SIZE; i++) {
            flgs[i] |= o_ptr->art_flags[i];
        }
    }

    if (!object_is_smith(o_ptr))
        return;

    int add = o_ptr->xtra3 - 1;
    if (add < TR_FLAG_MAX) {
        add_flag(flgs, add);
    } else if (add == ESSENCE_TMP_RES_ACID) {
        add_flag(flgs, TR_RES_ACID);
    } else if (add == ESSENCE_TMP_RES_ELEC) {
        add_flag(flgs, TR_RES_ELEC);
    } else if (add == ESSENCE_TMP_RES_FIRE) {
        add_flag(flgs, TR_RES_FIRE);
    } else if (add == ESSENCE_TMP_RES_COLD) {
        add_flag(flgs, TR_RES_COLD);
    } else if (add == ESSENCE_SH_FIRE) {
        add_flag(flgs, TR_RES_FIRE);
        add_flag(flgs, TR_SH_FIRE);
    } else if (add == ESSENCE_SH_ELEC) {
        add_flag(flgs, TR_RES_ELEC);
        add_flag(flgs, TR_SH_ELEC);
    } else if (add == ESSENCE_SH_COLD) {
        add_flag(flgs, TR_RES_COLD);
        add_flag(flgs, TR_SH_COLD);
    } else if (add == ESSENCE_RESISTANCE) {
        add_flag(flgs, TR_RES_ACID);
        add_flag(flgs, TR_RES_ELEC);
        add_flag(flgs, TR_RES_FIRE);
        add_flag(flgs, TR_RES_COLD);
    }
}
