#include "save/item-writer.h"
#include "artifact/random-art-effects.h"
#include "load/old/item-flag-types-savefile50.h"
#include "object/object-kind.h"
#include "save/save-util.h"
#include "system/object-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"
#include "util/quarks.h"

static void write_item_flags(object_type *o_ptr, BIT_FLAGS *flags)
{
    if (o_ptr->pval)
        set_bits(*flags, SaveDataItemFlagType::PVAL);

    if (o_ptr->discount)
        set_bits(*flags, SaveDataItemFlagType::DISCOUNT);

    if (o_ptr->number != 1)
        set_bits(*flags, SaveDataItemFlagType::NUMBER);

    if (o_ptr->name1)
        set_bits(*flags, SaveDataItemFlagType::NAME1);

    if (o_ptr->name2)
        set_bits(*flags, SaveDataItemFlagType::NAME2);

    if (o_ptr->timeout)
        set_bits(*flags, SaveDataItemFlagType::TIMEOUT);

    if (o_ptr->to_h)
        set_bits(*flags, SaveDataItemFlagType::TO_H);

    if (o_ptr->to_d)
        set_bits(*flags, SaveDataItemFlagType::TO_D);

    if (o_ptr->to_a)
        set_bits(*flags, SaveDataItemFlagType::TO_A);

    if (o_ptr->ac)
        set_bits(*flags, SaveDataItemFlagType::AC);

    if (o_ptr->dd)
        set_bits(*flags, SaveDataItemFlagType::DD);

    if (o_ptr->ds)
        set_bits(*flags, SaveDataItemFlagType::DS);

    if (o_ptr->ident)
        set_bits(*flags, SaveDataItemFlagType::IDENT);

    if (o_ptr->marked)
        set_bits(*flags, SaveDataItemFlagType::MARKED);

    if (o_ptr->art_flags.any())
        set_bits(*flags, SaveDataItemFlagType::ART_FLAGS);

    if (o_ptr->curse_flags.any())
        set_bits(*flags, SaveDataItemFlagType::CURSE_FLAGS);

    if (o_ptr->held_m_idx)
        set_bits(*flags, SaveDataItemFlagType::HELD_M_IDX);

    if (o_ptr->xtra1)
        set_bits(*flags, SaveDataItemFlagType::XTRA1);

    if (o_ptr->activation_id > RandomArtActType::NONE)
        set_bits(*flags, SaveDataItemFlagType::ACTIVATION_ID);

    if (o_ptr->xtra3)
        set_bits(*flags, SaveDataItemFlagType::XTRA3);

    if (o_ptr->xtra4)
        set_bits(*flags, SaveDataItemFlagType::XTRA4);

    if (o_ptr->xtra5)
        set_bits(*flags, SaveDataItemFlagType::XTRA5);

    if (o_ptr->feeling)
        set_bits(*flags, SaveDataItemFlagType::FEELING);

    if (o_ptr->inscription)
        set_bits(*flags, SaveDataItemFlagType::INSCRIPTION);

    if (o_ptr->art_name)
        set_bits(*flags, SaveDataItemFlagType::ART_NAME);

    if (o_ptr->stack_idx)
        set_bits(*flags, SaveDataItemFlagType::STACK_IDX);

    if (o_ptr->is_smith()) {
        set_bits(*flags, SaveDataItemFlagType::SMITH);
    }

    wr_u32b(*flags);
}

static void write_item_info(object_type *o_ptr, const BIT_FLAGS flags)
{
    wr_s16b((int16_t)o_ptr->weight);
    if (any_bits(flags, SaveDataItemFlagType::NAME1))
        wr_s16b(o_ptr->name1);

    if (any_bits(flags, SaveDataItemFlagType::NAME2))
        wr_byte((byte)o_ptr->name2);

    if (any_bits(flags, SaveDataItemFlagType::TIMEOUT))
        wr_s16b(o_ptr->timeout);

    if (any_bits(flags, SaveDataItemFlagType::TO_H))
        wr_s16b(o_ptr->to_h);

    if (any_bits(flags, SaveDataItemFlagType::TO_D))
        wr_s16b((int16_t)o_ptr->to_d);

    if (any_bits(flags, SaveDataItemFlagType::TO_A))
        wr_s16b(o_ptr->to_a);

    if (any_bits(flags, SaveDataItemFlagType::AC))
        wr_s16b(o_ptr->ac);

    if (any_bits(flags, SaveDataItemFlagType::DD))
        wr_byte((byte)o_ptr->dd);

    if (any_bits(flags, SaveDataItemFlagType::DS))
        wr_byte((byte)o_ptr->ds);

    if (any_bits(flags, SaveDataItemFlagType::IDENT))
        wr_byte(o_ptr->ident);

    if (any_bits(flags, SaveDataItemFlagType::MARKED))
        wr_byte(o_ptr->marked);

    if (any_bits(flags, SaveDataItemFlagType::ART_FLAGS))
        wr_FlagGroup(o_ptr->art_flags, wr_byte);

    if (any_bits(flags, SaveDataItemFlagType::CURSE_FLAGS))
        wr_FlagGroup(o_ptr->curse_flags, wr_byte);

    if (any_bits(flags, SaveDataItemFlagType::HELD_M_IDX))
        wr_s16b(o_ptr->held_m_idx);

    if (any_bits(flags, SaveDataItemFlagType::XTRA1))
        wr_byte(o_ptr->xtra1);

    if (any_bits(flags, SaveDataItemFlagType::ACTIVATION_ID))
        wr_s16b(enum2i(o_ptr->activation_id));

    if (any_bits(flags, SaveDataItemFlagType::XTRA3))
        wr_byte(o_ptr->xtra3);

    if (any_bits(flags, SaveDataItemFlagType::XTRA4))
        wr_s16b(o_ptr->xtra4);

    if (any_bits(flags, SaveDataItemFlagType::XTRA5))
        wr_s16b(o_ptr->xtra5);

    if (any_bits(flags, SaveDataItemFlagType::FEELING))
        wr_byte(o_ptr->feeling);

    if (any_bits(flags, SaveDataItemFlagType::STACK_IDX))
        wr_s16b(o_ptr->stack_idx);

    if (any_bits(flags, SaveDataItemFlagType::SMITH)) {
        if (o_ptr->smith_effect.has_value()) {
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
    if (any_bits(flags, SaveDataItemFlagType::PVAL))
        wr_s16b(o_ptr->pval);

    if (any_bits(flags, SaveDataItemFlagType::DISCOUNT))
        wr_byte(o_ptr->discount);

    if (any_bits(flags, SaveDataItemFlagType::NUMBER))
        wr_byte((byte)o_ptr->number);

    write_item_info(o_ptr, flags);
    if (any_bits(flags, SaveDataItemFlagType::INSCRIPTION))
        wr_string(quark_str(o_ptr->inscription));

    if (any_bits(flags, SaveDataItemFlagType::ART_NAME))
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
