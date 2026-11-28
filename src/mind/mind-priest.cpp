#include "mind/mind-priest.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object-hook/hook-weapon.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "perception/simple-perception.h"
#include "player/player-realm.h"
#include "racial/racial-android.h"
#include "system/deferred-update-requests.h"
#include "system/item/item-entity.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief プリーストにふさわしい祝福の特性フラグを返す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 第一領域が善の領域 (生命・破邪) なら善の神の祝福、そうでなければ (暗黒・悪魔) 邪の神の祝福
 * @details プリーストは、この祝福を持つ剣・長柄武器なら罰を受けずに持てる。
 */
tr_type get_priest_blessing(PlayerType *player_ptr)
{
    return PlayerRealm(player_ptr).realm1().is_good_attribute() ? TR_BLESSED : TR_BLESSED_EVIL;
}

/*!
 * @brief 武器に祝福を与える (判定と、武器への適用)
 * @param item 祝福を与える武器
 * @return 結果と、その途中で武器に起きたこと
 * @details メッセージの表示やプレイヤーの状態の更新は行わない (呼び出し側の bless_weapon() が行う)。
 * - 呪われた武器は、強い呪いなどに邪魔されると祝福できない。邪魔されなければ呪いを解く。
 * - 既に祝福されていれば何もしない。
 * - アーティファクトとエゴは 3 分の 1 でだけ祝福でき、それ以外は抵抗されて修正値が下がる。
 *   (こうしないと、プリーストは見つけたアーティファクトの武器をすべて祝福してしまう)
 */
BlessWeaponOutcome apply_blessing_to_weapon(ItemEntity &item)
{
    BlessWeaponOutcome outcome{ BlessWeaponResult::BLESSED, false, false };
    const auto item_flags = item.get_flags();
    if (item.is_cursed()) {
        auto can_disturb_blessing = item.curse_flags.has(CurseTraitType::HEAVY_CURSE) && (randint1(100) < 33);
        can_disturb_blessing |= item_flags.has(TR_ADD_L_CURSE);
        can_disturb_blessing |= item_flags.has(TR_ADD_H_CURSE);
        can_disturb_blessing |= item.curse_flags.has(CurseTraitType::PERSISTENT_CURSE);
        can_disturb_blessing |= item.curse_flags.has(CurseTraitType::PERMA_CURSE);
        if (can_disturb_blessing) {
            outcome.result = BlessWeaponResult::DISTURBED;
            return outcome;
        }

        item.remove_curses();
        outcome.is_curse_removed = true;
    }

    if (item.is_blessed()) {
        outcome.result = BlessWeaponResult::ALREADY_BLESSED;
        return outcome;
    }

    if (!(item.is_fixed_or_random_artifact() || item.is_ego()) || one_in_(3)) {
        item.art_flags.set(TR_BLESSED);
        item.discount = 99;
        return outcome;
    }

    outcome.result = BlessWeaponResult::RESISTED;

    /* Disenchant tohit */
    if (item.to_h > 0) {
        item.to_h--;
        outcome.is_disenchanted = true;
    }

    if ((item.to_h > 5) && evaluate_percent(33)) {
        item.to_h--;
    }

    /* Disenchant todam */
    if (item.to_d > 0) {
        item.to_d--;
        outcome.is_disenchanted = true;
    }

    if ((item.to_d > 5) && evaluate_percent(33)) {
        item.to_d--;
    }

    /* Disenchant toac */
    if (item.to_a > 0) {
        item.to_a--;
        outcome.is_disenchanted = true;
    }

    if ((item.to_a > 5) && evaluate_percent(33)) {
        item.to_a--;
    }

    return outcome;
}

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
    const auto outcome = apply_blessing_to_weapon(*item);
    if (outcome.result == BlessWeaponResult::DISTURBED) {
#ifdef JP
        msg_print("{}を覆う黒いオーラは祝福を跳ね返した！", item_name);
#else
        msg_print("The black aura on {} {} disrupts the blessing!", ((i_idx >= 0) ? "your" : "the"), item_name);
#endif
        sense_curse(player_ptr, i_idx);
        return true;
    }

    auto &dur = DeferredUpdateRequests::get_instance();
    if (outcome.is_curse_removed) {
#ifdef JP
        msg_print("{} から邪悪なオーラが消えた。", item_name);
#else
        msg_print("A malignant aura leaves {} {}.", ((i_idx >= 0) ? "your" : "the"), item_name);
#endif
        dur.set_flag(StatusRecalculatingFlag::BONUS);
        dur.request_item_lists_redrawing();
    }

    if (outcome.result == BlessWeaponResult::ALREADY_BLESSED) {
#ifdef JP
        msg_print("{} は既に祝福されている。", item_name);
#else
        msg_print("{} {} {} blessed already.", ((i_idx >= 0) ? "Your" : "The"), item_name, ((item->number > 1) ? "were" : "was"));
#endif
        return true;
    }

    if (outcome.result == BlessWeaponResult::BLESSED) {
#ifdef JP
        msg_print("{}は輝いた！", item_name);
#else
        msg_print("{} {} shine{}!", ((i_idx >= 0) ? "Your" : "The"), item_name, ((item->number > 1) ? "" : "s"));
#endif
    } else {
        msg_print(_("その武器は祝福を嫌っている！", "The weapon resists your blessing!"));
        if (outcome.is_disenchanted) {
            msg_print(_("周囲が凡庸な雰囲気で満ちた...", "There is a static feeling in the air..."));

#ifdef JP
            msg_print("{} は劣化した！", item_name);
#else
            msg_print("{} {} {} disenchanted!", ((i_idx >= 0) ? "Your" : "The"), item_name, ((item->number > 1) ? "were" : "was"));
#endif
        }
    }

    dur.set_flag(StatusRecalculatingFlag::BONUS);
    dur.request_item_lists_redrawing();
    calc_android_exp(player_ptr);
    return true;
}
