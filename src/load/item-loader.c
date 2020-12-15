#include "load/item-loader.h"
#include "artifact/fixed-art-types.h"
#include "game-option/runtime-arguments.h"
#include "load/angband-version-comparer.h"
#include "load/load-util.h"
#include "load/load-v1-5-0.h"
#include "load/savedata-flag-types.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "object-hook/hook-enchant.h"
#include "object/object-flags.h"
#include "object/object-kind.h"
#include "sv-definition/sv-lite-types.h"
#include "system/artifact-type-definition.h"
#include "system/object-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/quarks.h"

/*!
 * @brief アイテムオブジェクトを読み込む(現版) / Read an object (New method)
 * @param o_ptr アイテムオブジェクト保存先ポインタ
 * @return なし
 */
void rd_item(player_type *player_ptr, object_type *o_ptr)
{
    if (h_older_than(1, 5, 0, 0)) {
        rd_item_old(player_ptr, o_ptr);
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

    if (flags & SAVE_ITEM_PVAL)
        rd_s16b(&o_ptr->pval);
    else
        o_ptr->pval = 0;

    if (flags & SAVE_ITEM_DISCOUNT)
        rd_byte(&o_ptr->discount);
    else
        o_ptr->discount = 0;
    if (flags & SAVE_ITEM_NUMBER) {
        rd_byte(&tmp8u);
        o_ptr->number = tmp8u;
    } else
        o_ptr->number = 1;

    s16b tmp16s;
    rd_s16b(&tmp16s);
    o_ptr->weight = tmp16s;

    if (flags & SAVE_ITEM_NAME1) {
        rd_byte(&tmp8u);
        o_ptr->name1 = tmp8u;
    } else
        o_ptr->name1 = 0;

    if (flags & SAVE_ITEM_NAME2) {
        rd_byte(&tmp8u);
        o_ptr->name2 = tmp8u;
    } else
        o_ptr->name2 = 0;

    if (flags & SAVE_ITEM_TIMEOUT)
        rd_s16b(&o_ptr->timeout);
    else
        o_ptr->timeout = 0;

    if (flags & SAVE_ITEM_TO_H)
        rd_s16b(&o_ptr->to_h);
    else
        o_ptr->to_h = 0;

    if (flags & SAVE_ITEM_TO_D) {
        rd_s16b(&tmp16s);
        o_ptr->to_d = tmp16s;
    } else
        o_ptr->to_d = 0;

    if (flags & SAVE_ITEM_TO_A)
        rd_s16b(&o_ptr->to_a);
    else
        o_ptr->to_a = 0;

    if (flags & SAVE_ITEM_AC)
        rd_s16b(&o_ptr->ac);
    else
        o_ptr->ac = 0;

    if (flags & SAVE_ITEM_DD) {
        rd_byte(&tmp8u);
        o_ptr->dd = tmp8u;
    } else
        o_ptr->dd = 0;

    if (flags & SAVE_ITEM_DS) {
        rd_byte(&tmp8u);
        o_ptr->ds = tmp8u;
    } else
        o_ptr->ds = 0;

    if (flags & SAVE_ITEM_IDENT)
        rd_byte(&o_ptr->ident);
    else
        o_ptr->ident = 0;

    if (flags & SAVE_ITEM_MARKED)
        rd_byte(&o_ptr->marked);
    else
        o_ptr->marked = 0;

    /* Object flags */
    if (flags & SAVE_ITEM_ART_FLAGS0)
        rd_u32b(&o_ptr->art_flags[0]);
    else
        o_ptr->art_flags[0] = 0;

    if (flags & SAVE_ITEM_ART_FLAGS1)
        rd_u32b(&o_ptr->art_flags[1]);
    else
        o_ptr->art_flags[1] = 0;

    if (flags & SAVE_ITEM_ART_FLAGS2)
        rd_u32b(&o_ptr->art_flags[2]);
    else
        o_ptr->art_flags[2] = 0;

    if (flags & SAVE_ITEM_ART_FLAGS3)
        rd_u32b(&o_ptr->art_flags[3]);
    else
        o_ptr->art_flags[3] = 0;

    if (flags & SAVE_ITEM_ART_FLAGS4)
        rd_u32b(&o_ptr->art_flags[4]);
    else
        o_ptr->art_flags[4] = 0;

    if (flags & SAVE_ITEM_CURSE_FLAGS)
        rd_u32b(&o_ptr->curse_flags);
    else
        o_ptr->curse_flags = 0;

    /* Monster holding object */
    if (flags & SAVE_ITEM_HELD_M_IDX)
        rd_s16b(&o_ptr->held_m_idx);
    else
        o_ptr->held_m_idx = 0;

    /* Special powers */
    if (flags & SAVE_ITEM_XTRA1)
        rd_byte(&o_ptr->xtra1);
    else
        o_ptr->xtra1 = 0;

    if (flags & SAVE_ITEM_XTRA2)
        rd_byte(&o_ptr->xtra2);
    else
        o_ptr->xtra2 = 0;

    if (flags & SAVE_ITEM_XTRA3)
        rd_byte(&o_ptr->xtra3);
    else
        o_ptr->xtra3 = 0;

    if (flags & SAVE_ITEM_XTRA4)
        rd_s16b(&o_ptr->xtra4);
    else
        o_ptr->xtra4 = 0;

    if (flags & SAVE_ITEM_XTRA5)
        rd_s16b(&o_ptr->xtra5);
    else
        o_ptr->xtra5 = 0;

    if (flags & SAVE_ITEM_FEELING)
        rd_byte(&o_ptr->feeling);
    else
        o_ptr->feeling = 0;

    if (flags & SAVE_ITEM_INSCRIPTION) {
        char buf[128];
        rd_string(buf, sizeof(buf));
        o_ptr->inscription = quark_add(buf);
    } else
        o_ptr->inscription = 0;

    if (flags & SAVE_ITEM_ART_NAME) {
        char buf[128];
        rd_string(buf, sizeof(buf));
        o_ptr->art_name = quark_add(buf);
    } else {
        o_ptr->art_name = 0;
    }

    if (!h_older_than(2, 1, 2, 4))
        return;

    BIT_FLAGS flgs[TR_FLAG_SIZE];
    object_flags(player_ptr, o_ptr, flgs);

    if ((o_ptr->name2 == EGO_DARK) || (o_ptr->name2 == EGO_ANCIENT_CURSE) || (o_ptr->name1 == ART_NIGHT)) {
        add_flag(o_ptr->art_flags, TR_LITE_M1);
        remove_flag(o_ptr->art_flags, TR_LITE_1);
        remove_flag(o_ptr->art_flags, TR_LITE_2);
        remove_flag(o_ptr->art_flags, TR_LITE_3);
        return;
    }

    if (o_ptr->name2 == EGO_LITE_DARKNESS) {
        if (o_ptr->tval != TV_LITE) {
            add_flag(o_ptr->art_flags, TR_LITE_M1);
            return;
        }

        if (o_ptr->sval == SV_LITE_TORCH) {
            add_flag(o_ptr->art_flags, TR_LITE_M1);
        } else if (o_ptr->sval == SV_LITE_LANTERN) {
            add_flag(o_ptr->art_flags, TR_LITE_M2);
        } else if (o_ptr->sval == SV_LITE_FEANOR) {
            add_flag(o_ptr->art_flags, TR_LITE_M3);
        }

        return;
    }

    if (o_ptr->tval == TV_LITE) {
        if (object_is_fixed_artifact(o_ptr)) {
            add_flag(o_ptr->art_flags, TR_LITE_3);
            return;
        }

        if (o_ptr->sval == SV_LITE_TORCH) {
            add_flag(o_ptr->art_flags, TR_LITE_1);
            add_flag(o_ptr->art_flags, TR_LITE_FUEL);
            return;
        }

        if (o_ptr->sval == SV_LITE_LANTERN) {
            add_flag(o_ptr->art_flags, TR_LITE_2);
            add_flag(o_ptr->art_flags, TR_LITE_FUEL);
            return;
        }

        if (o_ptr->sval == SV_LITE_FEANOR) {
            add_flag(o_ptr->art_flags, TR_LITE_2);
            return;
        }
    }
}

errr load_item(void)
{
    u16b tmp16u;
    rd_u16b(&tmp16u);
    if (tmp16u > max_k_idx) {
        load_note(format(_("アイテムの種類が多すぎる(%u)！", "Too many (%u) object kinds!"), tmp16u));
        return 22;
    }

    byte tmp8u;
    for (int i = 0; i < tmp16u; i++) {
        object_kind *k_ptr = &k_info[i];
        rd_byte(&tmp8u);
        k_ptr->aware = (tmp8u & 0x01) ? TRUE : FALSE;
        k_ptr->tried = (tmp8u & 0x02) ? TRUE : FALSE;
    }

    if (arg_fiddle)
        load_note(_("アイテムの記録をロードしました", "Loaded Object Memory"));

    return 0;
}

errr load_artifact(void)
{
    u16b tmp16u;
    rd_u16b(&tmp16u);
    if (tmp16u > max_a_idx) {
        load_note(format(_("伝説のアイテムが多すぎる(%u)！", "Too many (%u) artifacts!"), tmp16u));
        return 24;
    }

    byte tmp8u;
    for (int i = 0; i < tmp16u; i++) {
        artifact_type *a_ptr = &a_info[i];
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

    if (arg_fiddle)
        load_note(_("伝説のアイテムをロードしました", "Loaded Artifacts"));

    return 0;
}
