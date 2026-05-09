#include "mind/mind-priest.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object-hook/hook-weapon.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "racial/racial-android.h"
#include "system/item/item-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief 武器の祝福処理 /
 * Bless a weapon
 * @return ターン消費を要する処理を行ったならばTRUEを返す
 */
bool bless_weapon(PlayerType *player_ptr)
{
    constexpr auto q = _("どのアイテムを祝福しますか？", "Bless which weapon? ");
    constexpr auto s = _("祝福できる武器がありません。", "You have no weapon to bless.");
    constexpr BIT_FLAGS options = USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT;
    const auto &[item, i_idx] = choose_item(player_ptr, q, s, options, FuncItemTester(&ItemEntity::is_weapon));
    if (!item) {
        return false;
    }

    const auto item_name = describe_flavor(player_ptr, *item, OD_OMIT_PREFIX | OD_NAME_ONLY);
    const auto item_flags = item->get_flags();
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    if (item->is_cursed()) {
        auto can_disturb_blessing = item->curse_flags.has(CurseTraitType::HEAVY_CURSE) && (randint1(100) < 33);
        can_disturb_blessing |= item_flags.has(TR_ADD_L_CURSE);
        can_disturb_blessing |= item_flags.has(TR_ADD_H_CURSE);
        can_disturb_blessing |= item->curse_flags.has(CurseTraitType::PERSISTENT_CURSE);
        can_disturb_blessing |= item->curse_flags.has(CurseTraitType::PERMA_CURSE);
        if (can_disturb_blessing) {
#ifdef JP
            msg_format("%sを覆う黒いオーラは祝福を跳ね返した！", item_name.data());
#else
            msg_format("The black aura on %s %s disrupts the blessing!", ((i_idx >= 0) ? "your" : "the"), item_name.data());
#endif

            return true;
        }

#ifdef JP
        msg_format("%s から邪悪なオーラが消えた。", item_name.data());
#else
        msg_format("A malignant aura leaves %s %s.", ((i_idx >= 0) ? "your" : "the"), item_name.data());
#endif
        item->curse_flags.clear();
        set_bits(item->ident, IDENT_SENSE);
        item->feeling = FEEL_NONE;
        rfu.set_flag(StatusRecalculatingFlag::BONUS);
        static constexpr auto flags = {
            SubWindowRedrawingFlag::EQUIPMENT,
            SubWindowRedrawingFlag::FLOOR_ITEMS,
            SubWindowRedrawingFlag::FOUND_ITEMS,
        };
        rfu.set_flags(flags);
    }

    /*
     * Next, we try to bless it. Artifacts have a 1/3 chance of
     * being blessed, otherwise, the operation simply disenchants
     * them, godly power negating the magic. Ok, the explanation
     * is silly, but otherwise priests would always bless every
     * artifact weapon they find. Ego weapons and normal weapons
     * can be blessed automatically.
     */
    if (item_flags.has(TR_BLESSED)) {
#ifdef JP
        msg_format("%s は既に祝福されている。", item_name.data());
#else
        msg_format("%s %s %s blessed already.", ((i_idx >= 0) ? "Your" : "The"), item_name.data(), ((item->number > 1) ? "were" : "was"));
#endif
        return true;
    }

    if (!(item->is_fixed_or_random_artifact() || item->is_ego()) || one_in_(3)) {
#ifdef JP
        msg_format("%sは輝いた！", item_name.data());
#else
        msg_format("%s %s shine%s!", ((i_idx >= 0) ? "Your" : "The"), item_name.data(), ((item->number > 1) ? "" : "s"));
#endif
        item->art_flags.set(TR_BLESSED);
        item->discount = 99;
    } else {
        bool dis_happened = false;
        msg_print(_("その武器は祝福を嫌っている！", "The weapon resists your blessing!"));

        /* Disenchant tohit */
        if (item->to_h > 0) {
            item->to_h--;
            dis_happened = true;
        }

        if ((item->to_h > 5) && evaluate_percent(33)) {
            item->to_h--;
        }

        /* Disenchant todam */
        if (item->to_d > 0) {
            item->to_d--;
            dis_happened = true;
        }

        if ((item->to_d > 5) && evaluate_percent(33)) {
            item->to_d--;
        }

        /* Disenchant toac */
        if (item->to_a > 0) {
            item->to_a--;
            dis_happened = true;
        }

        if ((item->to_a > 5) && evaluate_percent(33)) {
            item->to_a--;
        }

        if (dis_happened) {
            msg_print(_("周囲が凡庸な雰囲気で満ちた...", "There is a static feeling in the air..."));

#ifdef JP
            msg_format("%s は劣化した！", item_name.data());
#else
            msg_format("%s %s %s disenchanted!", ((i_idx >= 0) ? "Your" : "The"), item_name.data(), ((item->number > 1) ? "were" : "was"));
#endif
        }
    }

    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::EQUIPMENT,
        SubWindowRedrawingFlag::PLAYER,
        SubWindowRedrawingFlag::FLOOR_ITEMS,
        SubWindowRedrawingFlag::FOUND_ITEMS,
    };
    rfu.set_flags(flags_swrf);
    calc_android_exp(player_ptr);
    return true;
}
