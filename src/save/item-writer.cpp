#include "save/item-writer.h"
#include "load/savedata-flag-types.h"
#include "object/object-kind.h"
#include "save/save-util.h"
#include "system/object-type-definition.h"
#include "util/enum-converter.h"
#include "util/quarks.h"

static void write_item_flags(object_type *o_ptr, BIT_FLAGS *flags)
{
    if (o_ptr->pval)
        *flags |= SAVE_ITEM_PVAL;

    if (o_ptr->discount)
        *flags |= SAVE_ITEM_DISCOUNT;

    if (o_ptr->number != 1)
        *flags |= SAVE_ITEM_NUMBER;

    if (o_ptr->name1)
        *flags |= SAVE_ITEM_NAME1;

    if (o_ptr->name2)
        *flags |= SAVE_ITEM_NAME2;

    if (o_ptr->timeout)
        *flags |= SAVE_ITEM_TIMEOUT;

    if (o_ptr->to_h)
        *flags |= SAVE_ITEM_TO_H;

    if (o_ptr->to_d)
        *flags |= SAVE_ITEM_TO_D;

    if (o_ptr->to_a)
        *flags |= SAVE_ITEM_TO_A;

    if (o_ptr->ac)
        *flags |= SAVE_ITEM_AC;

    if (o_ptr->dd)
        *flags |= SAVE_ITEM_DD;

    if (o_ptr->ds)
        *flags |= SAVE_ITEM_DS;

    if (o_ptr->ident)
        *flags |= SAVE_ITEM_IDENT;

    if (o_ptr->marked)
        *flags |= SAVE_ITEM_MARKED;

    if (o_ptr->art_flags.any())
        *flags |= SAVE_ITEM_ART_FLAGS;

    if (o_ptr->curse_flags.any())
        *flags |= SAVE_ITEM_CURSE_FLAGS;

    if (o_ptr->held_m_idx)
        *flags |= SAVE_ITEM_HELD_M_IDX;

    if (o_ptr->xtra1)
        *flags |= SAVE_ITEM_XTRA1;

    if (o_ptr->xtra2)
        *flags |= SAVE_ITEM_XTRA2;

    if (o_ptr->xtra3)
        *flags |= SAVE_ITEM_XTRA3;

    if (o_ptr->xtra4)
        *flags |= SAVE_ITEM_XTRA4;

    if (o_ptr->xtra5)
        *flags |= SAVE_ITEM_XTRA5;

    if (o_ptr->feeling)
        *flags |= SAVE_ITEM_FEELING;

    if (o_ptr->inscription)
        *flags |= SAVE_ITEM_INSCRIPTION;

    if (o_ptr->art_name)
        *flags |= SAVE_ITEM_ART_NAME;

    if (o_ptr->stack_idx)
        *flags |= SAVE_ITEM_STACK_IDX;

    if (o_ptr->is_smith()) {
        *flags |= SAVE_ITEM_SMITH;
    }

    wr_u32b(*flags);
}

static void write_item_info(object_type *o_ptr, const BIT_FLAGS flags)
{
    wr_s16b((int16_t)o_ptr->weight);
    if (flags & SAVE_ITEM_NAME1)
        wr_s16b(o_ptr->name1);

    if (flags & SAVE_ITEM_NAME2)
        wr_byte((byte)o_ptr->name2);

    if (flags & SAVE_ITEM_TIMEOUT)
        wr_s16b(o_ptr->timeout);

    if (flags & SAVE_ITEM_TO_H)
        wr_s16b(o_ptr->to_h);

    if (flags & SAVE_ITEM_TO_D)
        wr_s16b((int16_t)o_ptr->to_d);

    if (flags & SAVE_ITEM_TO_A)
        wr_s16b(o_ptr->to_a);

    if (flags & SAVE_ITEM_AC)
        wr_s16b(o_ptr->ac);

    if (flags & SAVE_ITEM_DD)
        wr_byte((byte)o_ptr->dd);

    if (flags & SAVE_ITEM_DS)
        wr_byte((byte)o_ptr->ds);

    if (flags & SAVE_ITEM_IDENT)
        wr_byte(o_ptr->ident);

    if (flags & SAVE_ITEM_MARKED)
        wr_byte(o_ptr->marked);

    if (flags & SAVE_ITEM_ART_FLAGS)
        wr_FlagGroup(o_ptr->art_flags, wr_byte);

    if (flags & SAVE_ITEM_CURSE_FLAGS)
        wr_FlagGroup(o_ptr->curse_flags, wr_byte);

    if (flags & SAVE_ITEM_HELD_M_IDX)
        wr_s16b(o_ptr->held_m_idx);

    if (flags & SAVE_ITEM_XTRA1)
        wr_byte(o_ptr->xtra1);

    if (flags & SAVE_ITEM_XTRA2)
        wr_s16b(o_ptr->xtra2);

    if (flags & SAVE_ITEM_XTRA3)
        wr_byte(o_ptr->xtra3);

    if (flags & SAVE_ITEM_XTRA4)
        wr_s16b(o_ptr->xtra4);

    if (flags & SAVE_ITEM_XTRA5)
        wr_s16b(o_ptr->xtra5);

    if (flags & SAVE_ITEM_FEELING)
        wr_byte(o_ptr->feeling);

    if (flags & SAVE_ITEM_STACK_IDX)
        wr_s16b(o_ptr->stack_idx);

    if (flags & SAVE_ITEM_SMITH) {
        if (o_ptr->smith_effect.has_value()){
            wr_s16b(enum2i(o_ptr->smith_effect.value()));
        } else {
            wr_s16b(0);
        }
        if (o_ptr->smith_act_idx.has_value()) {
            wr_s16b(enum2i(o_ptr->smith_act_idx.value()));
        } else {
            wr_s16b(0);
        }
    }
}

/*!
 * @brief アイテムオブジェクトを書き込む / Write an "item" record
 * @param o_ptr アイテムオブジェクト保存元ポインタ
 */
void wr_item(object_type *o_ptr)
{
    BIT_FLAGS flags = 0x00000000;
    write_item_flags(o_ptr, &flags);

    wr_s16b(o_ptr->k_idx);
    wr_byte((byte)o_ptr->iy);
    wr_byte((byte)o_ptr->ix);
    if (flags & SAVE_ITEM_PVAL)
        wr_s16b(o_ptr->pval);

    if (flags & SAVE_ITEM_DISCOUNT)
        wr_byte(o_ptr->discount);

    if (flags & SAVE_ITEM_NUMBER)
        wr_byte((byte)o_ptr->number);

    write_item_info(o_ptr, flags);
    if (flags & SAVE_ITEM_INSCRIPTION)
        wr_string(quark_str(o_ptr->inscription));

    if (flags & SAVE_ITEM_ART_NAME)
        wr_string(quark_str(o_ptr->art_name));
}

/*!
 * @brief セーブデータにアイテムの鑑定情報を書き込む / Write an "perception" record
 * @param k_idx ベースアイテムのID
 */
void wr_perception(KIND_OBJECT_IDX k_idx)
{
    byte tmp8u = 0;
    object_kind *k_ptr = &k_info[k_idx];
    if (k_ptr->aware)
        tmp8u |= 0x01;

    if (k_ptr->tried)
        tmp8u |= 0x02;

    wr_byte(tmp8u);
}
