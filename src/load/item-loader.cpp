#include "load/item-loader.h"
#include "artifact/fixed-art-types.h"
#include "game-option/runtime-arguments.h"
#include "load/angband-version-comparer.h"
#include "load/load-util.h"
#include "load/load-v1-5-0.h"
#include "load/savedata-flag-types.h"
#include "load/savedata-old-flag-types.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "object/object-flags.h"
#include "object/object-kind.h"
#include "sv-definition/sv-lite-types.h"
#include "system/artifact-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"
#include "util/quarks.h"

/*!
 * @brief アイテムオブジェクトを読み込む(現版) / Read an object (New method)
 * @param o_ptr アイテムオブジェクト保存先ポインタ
 */
void rd_item(object_type *o_ptr)
{
    if (h_older_than(1, 5, 0, 0)) {
        rd_item_old(o_ptr);
        return;
    }

    BIT_FLAGS flags;
    rd_u32b(&flags);
    rd_s16b(&o_ptr->k_idx);

    byte tmp8u;
    rd_byte(&tmp8u);
    o_ptr->iy = (POSITION)tmp8u;
    rd_byte(&tmp8u);
    o_ptr->ix = (POSITION)tmp8u;

    object_kind *k_ptr;
    k_ptr = &k_info[o_ptr->k_idx];
    o_ptr->tval = k_ptr->tval;
    o_ptr->sval = k_ptr->sval;

    if (any_bits(flags, SaveDataItemFlagType::PVAL))
        rd_s16b(&o_ptr->pval);
    else
        o_ptr->pval = 0;

    if (any_bits(flags, SaveDataItemFlagType::DISCOUNT))
        rd_byte(&o_ptr->discount);
    else
        o_ptr->discount = 0;
    if (any_bits(flags, SaveDataItemFlagType::NUMBER)) {
        rd_byte(&tmp8u);
        o_ptr->number = tmp8u;
    } else
        o_ptr->number = 1;

    int16_t tmp16s;
    rd_s16b(&tmp16s);
    o_ptr->weight = tmp16s;

    if (any_bits(flags, SaveDataItemFlagType::NAME1)) {
        if (h_older_than(3, 0, 0, 2)) {
            rd_byte(&tmp8u);
            o_ptr->name1 = tmp8u;
        } else {
            rd_s16b(&tmp16s);
            o_ptr->name1 = tmp16s;
        }
    } else
        o_ptr->name1 = 0;

    if (any_bits(flags, SaveDataItemFlagType::NAME2)) {
        rd_byte(&tmp8u);
        o_ptr->name2 = tmp8u;
    } else
        o_ptr->name2 = 0;

    if (any_bits(flags, SaveDataItemFlagType::TIMEOUT))
        rd_s16b(&o_ptr->timeout);
    else
        o_ptr->timeout = 0;

    if (any_bits(flags, SaveDataItemFlagType::TO_H))
        rd_s16b(&o_ptr->to_h);
    else
        o_ptr->to_h = 0;

    if (any_bits(flags, SaveDataItemFlagType::TO_D)) {
        rd_s16b(&tmp16s);
        o_ptr->to_d = tmp16s;
    } else
        o_ptr->to_d = 0;

    if (any_bits(flags, SaveDataItemFlagType::TO_A))
        rd_s16b(&o_ptr->to_a);
    else
        o_ptr->to_a = 0;

    if (any_bits(flags, SaveDataItemFlagType::AC))
        rd_s16b(&o_ptr->ac);
    else
        o_ptr->ac = 0;

    if (any_bits(flags, SaveDataItemFlagType::DD)) {
        rd_byte(&tmp8u);
        o_ptr->dd = tmp8u;
    } else
        o_ptr->dd = 0;

    if (any_bits(flags, SaveDataItemFlagType::DS)) {
        rd_byte(&tmp8u);
        o_ptr->ds = tmp8u;
    } else
        o_ptr->ds = 0;

    if (any_bits(flags, SaveDataItemFlagType::IDENT))
        rd_byte(&o_ptr->ident);
    else
        o_ptr->ident = 0;

    if (any_bits(flags, SaveDataItemFlagType::MARKED))
        rd_byte(&o_ptr->marked);
    else
        o_ptr->marked = 0;

    /* Object flags */
    if (loading_savefile_version_is_older_than(7)) {
        constexpr savedata_item_older_than_7_flag_type old_savefile_art_flags[] = {
            SAVE_ITEM_OLDER_THAN_7_ART_FLAGS0,
            SAVE_ITEM_OLDER_THAN_7_ART_FLAGS1,
            SAVE_ITEM_OLDER_THAN_7_ART_FLAGS2,
            SAVE_ITEM_OLDER_THAN_7_ART_FLAGS3,
            SAVE_ITEM_OLDER_THAN_7_ART_FLAGS4,
        };
        auto start = 0;
        for (auto f : old_savefile_art_flags) {
            if (any_bits(flags, f)) {
                uint32_t tmp32u;
                rd_u32b(&tmp32u);
                migrate_bitflag_to_flaggroup(o_ptr->art_flags, tmp32u, start);
            }
            start += 32;
        }
    } else {
        if (any_bits(flags, SaveDataItemFlagType::ART_FLAGS)) {
            rd_FlagGroup(o_ptr->art_flags, rd_byte);
        } else {
            o_ptr->art_flags.clear();
        }
    }

    if (any_bits(flags, SaveDataItemFlagType::CURSE_FLAGS)) {
        if (loading_savefile_version_is_older_than(5)) {
            uint32_t tmp32u;
            rd_u32b(&tmp32u);
            migrate_bitflag_to_flaggroup(o_ptr->curse_flags, tmp32u);
        } else {
            rd_FlagGroup(o_ptr->curse_flags, rd_byte);
        }
    } else {
        o_ptr->curse_flags.clear();
    }

    /* Monster holding object */
    if (any_bits(flags, SaveDataItemFlagType::HELD_M_IDX))
        rd_s16b(&o_ptr->held_m_idx);
    else
        o_ptr->held_m_idx = 0;

    /* Special powers */
    if (any_bits(flags, SaveDataItemFlagType::XTRA1))
        rd_byte(&o_ptr->xtra1);
    else
        o_ptr->xtra1 = 0;

    if (any_bits(flags, SaveDataItemFlagType::ACTIVATION_ID)) {
        if (h_older_than(3, 0, 0, 2)) {
            rd_byte(&tmp8u);
            o_ptr->activation_id = i2enum<RandomArtActType>(tmp8u);
        } else {
            rd_s16b(&tmp16s);
            o_ptr->activation_id = i2enum<RandomArtActType>(tmp16s);
        }
    } else {
        o_ptr->activation_id = i2enum<RandomArtActType>(0);
    }

    if (any_bits(flags, SaveDataItemFlagType::XTRA3))
        rd_byte(&o_ptr->xtra3);
    else
        o_ptr->xtra3 = 0;

    if (any_bits(flags, SaveDataItemFlagType::XTRA4))
        rd_s16b(&o_ptr->xtra4);
    else
        o_ptr->xtra4 = 0;

    if (any_bits(flags, SaveDataItemFlagType::XTRA5))
        rd_s16b(&o_ptr->xtra5);
    else
        o_ptr->xtra5 = 0;

    if (any_bits(flags, SaveDataItemFlagType::FEELING))
        rd_byte(&o_ptr->feeling);
    else
        o_ptr->feeling = 0;

    if (any_bits(flags, SaveDataItemFlagType::STACK_IDX))
        rd_s16b(&o_ptr->stack_idx);
    else
        o_ptr->stack_idx = 0;

    if (any_bits(flags, SaveDataItemFlagType::SMITH) && !loading_savefile_version_is_older_than(7)) {
        rd_s16b(&tmp16s);
        if (tmp16s > 0) {
            o_ptr->smith_effect = static_cast<SmithEffect>(tmp16s);
        }

        rd_s16b(&tmp16s);
        if (tmp16s > 0) {
            o_ptr->smith_act_idx = static_cast<RandomArtActType>(tmp16s);
        }
    }

    if (any_bits(flags, SaveDataItemFlagType::INSCRIPTION)) {
        char buf[128];
        rd_string(buf, sizeof(buf));
        o_ptr->inscription = quark_add(buf);
    } else
        o_ptr->inscription = 0;

    if (any_bits(flags, SaveDataItemFlagType::ART_NAME)) {
        char buf[128];
        rd_string(buf, sizeof(buf));
        o_ptr->art_name = quark_add(buf);
    } else {
        o_ptr->art_name = 0;
    }

    if (!h_older_than(2, 1, 2, 4))
        return;

    if ((o_ptr->name2 == EGO_DARK) || (o_ptr->name2 == EGO_ANCIENT_CURSE) || (o_ptr->name1 == ART_NIGHT)) {
        o_ptr->art_flags.set(TR_LITE_M1);
        o_ptr->art_flags.reset(TR_LITE_1);
        o_ptr->art_flags.reset(TR_LITE_2);
        o_ptr->art_flags.reset(TR_LITE_3);
        return;
    }

    if (o_ptr->name2 == EGO_LITE_DARKNESS) {
        if (o_ptr->tval != TV_LITE) {
            o_ptr->art_flags.set(TR_LITE_M1);
            return;
        }

        if (o_ptr->sval == SV_LITE_TORCH) {
            o_ptr->art_flags.set(TR_LITE_M1);
        } else if (o_ptr->sval == SV_LITE_LANTERN) {
            o_ptr->art_flags.set(TR_LITE_M2);
        } else if (o_ptr->sval == SV_LITE_FEANOR) {
            o_ptr->art_flags.set(TR_LITE_M3);
        }

        return;
    }

    if (o_ptr->tval == TV_LITE) {
        if (o_ptr->is_fixed_artifact()) {
            o_ptr->art_flags.set(TR_LITE_3);
            return;
        }

        if (o_ptr->sval == SV_LITE_TORCH) {
            o_ptr->art_flags.set(TR_LITE_1);
            o_ptr->art_flags.set(TR_LITE_FUEL);
            return;
        }

        if (o_ptr->sval == SV_LITE_LANTERN) {
            o_ptr->art_flags.set(TR_LITE_2);
            o_ptr->art_flags.set(TR_LITE_FUEL);
            return;
        }

        if (o_ptr->sval == SV_LITE_FEANOR) {
            o_ptr->art_flags.set(TR_LITE_2);
            return;
        }
    }
}

/*!
 * @brief アイテムオブジェクトの鑑定情報をロードする
 * @return 成功なら0
 */
errr load_item(void)
{
    uint16_t loading_max_k_idx;
    rd_u16b(&loading_max_k_idx);

    object_kind *k_ptr;
    object_kind dummy;
    byte tmp8u;
    for (auto i = 0U; i < loading_max_k_idx; i++) {
        if (i < k_info.size())
            k_ptr = &k_info[i];
        else
            k_ptr = &dummy;

        rd_byte(&tmp8u);
        k_ptr->aware = any_bits(tmp8u, 0x01);
        k_ptr->tried = any_bits(tmp8u, 0x02);
    }

    load_note(_("アイテムの記録をロードしました", "Loaded Object Memory"));
    return 0;
}

/*!
 * @brief 固定アーティファクトの出現情報をロードする
 * @return 成功なら0
 */
errr load_artifact(void)
{
    uint16_t loading_max_a_idx;
    rd_u16b(&loading_max_a_idx);

    artifact_type *a_ptr;
    artifact_type dummy;
    byte tmp8u;
    for (auto i = 0U; i < loading_max_a_idx; i++) {
        if (i < a_info.size())
            a_ptr = &a_info[i];
        else
            a_ptr = &dummy;

        rd_byte(&tmp8u);
        a_ptr->cur_num = tmp8u;
        if (h_older_than(1, 5, 0, 0)) {
            a_ptr->floor_id = 0;
            rd_byte(&tmp8u);
            rd_byte(&tmp8u);
            rd_byte(&tmp8u);
        } else {
            rd_s16b(&a_ptr->floor_id);
        }
    }

    load_note(_("伝説のアイテムをロードしました", "Loaded Artifacts"));
    return 0;
}
