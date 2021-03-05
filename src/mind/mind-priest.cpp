﻿#include "mind/mind-priest.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/trc-types.h"
#include "object-enchant/tr-types.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-enchant.h"
#include "object-hook/hook-weapon.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-flags.h"
#include "racial/racial-android.h"
#include "system/object-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief 武器の祝福処理 /
 * Bless a weapon
 * @return ターン消費を要する処理を行ったならばTRUEを返す
 */
bool bless_weapon(player_type *caster_ptr)
{
    item_tester_hook = object_is_weapon;

    concptr q = _("どのアイテムを祝福しますか？", "Bless which weapon? ");
    concptr s = _("祝福できる武器がありません。", "You have weapon to bless.");

    OBJECT_IDX item;
    object_type *o_ptr = choose_object(caster_ptr, &item, q, s, USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT, TV_NONE);
    if (!o_ptr)
        return FALSE;

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(caster_ptr, o_name, o_ptr, OD_OMIT_PREFIX | OD_NAME_ONLY);
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    object_flags(caster_ptr, o_ptr, flgs);

    if (object_is_cursed(o_ptr)) {
        if (((o_ptr->curse_flags & TRC_HEAVY_CURSE) && (randint1(100) < 33)) || has_flag(flgs, TR_ADD_L_CURSE) || has_flag(flgs, TR_ADD_H_CURSE)
            || (o_ptr->curse_flags & TRC_PERMA_CURSE)) {
#ifdef JP
            msg_format("%sを覆う黒いオーラは祝福を跳ね返した！", o_name);
#else
            msg_format("The black aura on %s %s disrupts the blessing!", ((item >= 0) ? "your" : "the"), o_name);
#endif

            return TRUE;
        }

#ifdef JP
        msg_format("%s から邪悪なオーラが消えた。", o_name);
#else
        msg_format("A malignant aura leaves %s %s.", ((item >= 0) ? "your" : "the"), o_name);
#endif
        o_ptr->curse_flags = 0L;
        o_ptr->ident |= IDENT_SENSE;
        o_ptr->feeling = FEEL_NONE;
        caster_ptr->update |= PU_BONUS;
        caster_ptr->window_flags |= PW_EQUIP;
    }

    /*
     * Next, we try to bless it. Artifacts have a 1/3 chance of
     * being blessed, otherwise, the operation simply disenchants
     * them, godly power negating the magic. Ok, the explanation
     * is silly, but otherwise priests would always bless every
     * artifact weapon they find. Ego weapons and normal weapons
     * can be blessed automatically.
     */
    if (has_flag(flgs, TR_BLESSED)) {
#ifdef JP
        msg_format("%s は既に祝福されている。", o_name);
#else
        msg_format("%s %s %s blessed already.", ((item >= 0) ? "Your" : "The"), o_name, ((o_ptr->number > 1) ? "were" : "was"));
#endif
        return TRUE;
    }

    if (!(object_is_artifact(o_ptr) || object_is_ego(o_ptr)) || one_in_(3)) {
#ifdef JP
        msg_format("%sは輝いた！", o_name);
#else
        msg_format("%s %s shine%s!", ((item >= 0) ? "Your" : "The"), o_name, ((o_ptr->number > 1) ? "" : "s"));
#endif
        add_flag(o_ptr->art_flags, TR_BLESSED);
        o_ptr->discount = 99;
    } else {
        bool dis_happened = FALSE;
        msg_print(_("その武器は祝福を嫌っている！", "The weapon resists your blessing!"));

        /* Disenchant tohit */
        if (o_ptr->to_h > 0) {
            o_ptr->to_h--;
            dis_happened = TRUE;
        }

        if ((o_ptr->to_h > 5) && (randint0(100) < 33))
            o_ptr->to_h--;

        /* Disenchant todam */
        if (o_ptr->to_d > 0) {
            o_ptr->to_d--;
            dis_happened = TRUE;
        }

        if ((o_ptr->to_d > 5) && (randint0(100) < 33))
            o_ptr->to_d--;

        /* Disenchant toac */
        if (o_ptr->to_a > 0) {
            o_ptr->to_a--;
            dis_happened = TRUE;
        }

        if ((o_ptr->to_a > 5) && (randint0(100) < 33))
            o_ptr->to_a--;

        if (dis_happened) {
            msg_print(_("周囲が凡庸な雰囲気で満ちた...", "There is a static feeling in the air..."));

#ifdef JP
            msg_format("%s は劣化した！", o_name);
#else
            msg_format("%s %s %s disenchanted!", ((item >= 0) ? "Your" : "The"), o_name, ((o_ptr->number > 1) ? "were" : "was"));
#endif
        }
    }

    caster_ptr->update |= PU_BONUS;
    caster_ptr->window_flags |= PW_EQUIP | PW_PLAYER;
    calc_android_exp(caster_ptr);
    return TRUE;
}
