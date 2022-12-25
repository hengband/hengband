#include "market/building-craft-fix.h"
#include "artifact/artifact-info.h"
#include "artifact/fixed-art-types.h"
#include "artifact/random-art-effects.h"
#include "core/asking-player.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "inventory/inventory-object.h"
#include "market/building-util.h"
#include "object-enchant/object-boost.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-types.h"
#include "object-hook/hook-weapon.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-flags.h"
#include "object/object-kind-hook.h"
#include "object/object-value.h"
#include "racial/racial-android.h"
#include "spell-realm/spells-hex.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/baseitem-info.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief 修復材料のオブジェクトから修復対象に特性を移植する。
 * @param to_ptr 修復対象オブジェクトの構造体の参照ポインタ。
 * @param from_ptr 修復材料オブジェクトの構造体の参照ポインタ。
 * @return 修復対象になるならTRUEを返す。
 */
static void give_one_ability_of_object(ItemEntity *to_ptr, ItemEntity *from_ptr)
{
    auto to_flgs = object_flags(to_ptr);
    auto from_flgs = object_flags(from_ptr);

    int n = 0;
    tr_type cand[TR_FLAG_MAX];
    for (int i = 0; i < TR_FLAG_MAX; i++) {
        switch (i) {
        case TR_IGNORE_ACID:
        case TR_IGNORE_ELEC:
        case TR_IGNORE_FIRE:
        case TR_IGNORE_COLD:
        case TR_ACTIVATE:
        case TR_RIDING:
        case TR_THROW:
        case TR_SHOW_MODS:
        case TR_HIDE_TYPE:
        case TR_XXX_93:
        case TR_XXX_94:
        case TR_FULL_NAME:
        case TR_FIXED_FLAVOR:
            break;
        default:
            auto tr_flag = i2enum<tr_type>(i);
            if (from_flgs.has(tr_flag) && to_flgs.has_not(tr_flag)) {
                if (!(TR_PVAL_FLAG_MASK.has(tr_flag) && (from_ptr->pval < 1))) {
                    cand[n++] = tr_flag;
                }
            }
        }
    }

    if (n <= 0) {
        return;
    }

    auto tr_idx = cand[randint0(n)];
    to_ptr->art_flags.set(tr_idx);
    if (TR_PVAL_FLAG_MASK.has(tr_idx)) {
        to_ptr->pval = std::max<short>(to_ptr->pval, 1);
    }
    auto bmax = std::min<short>(3, std::max<short>(1, 40 / (to_ptr->dd * to_ptr->ds)));
    if (tr_idx == TR_BLOWS) {
        to_ptr->pval = std::min<short>(to_ptr->pval, bmax);
    }
    if (tr_idx == TR_SPEED) {
        to_ptr->pval = std::min<short>(to_ptr->pval, 4);
    }
}

/*!
 * @brief アイテム修復処理のメインルーチン / Repair broken weapon
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param bcost 基本修復費用
 * @return 実際にかかった費用
 */
static PRICE repair_broken_weapon_aux(PlayerType *player_ptr, PRICE bcost)
{
    clear_bldg(0, 22);
    int row = 7;
    prt(_("修復には材料となるもう1つの武器が必要です。", "Hand one material weapon to repair a broken weapon."), row, 2);
    prt(_("材料に使用した武器はなくなります！", "The material weapon will disappear after repairing!!"), row + 1, 2);

    concptr q = _("どの折れた武器を修復しますか？", "Repair which broken weapon? ");
    concptr s = _("修復できる折れた武器がありません。", "You have no broken weapon to repair.");

    OBJECT_IDX item;
    ItemEntity *o_ptr;
    o_ptr = choose_object(player_ptr, &item, q, s, (USE_INVEN | USE_EQUIP), FuncItemTester(&ItemEntity::is_broken_weapon));
    if (!o_ptr) {
        return 0;
    }

    if (!o_ptr->is_ego() && !o_ptr->is_artifact()) {
        msg_format(_("それは直してもしょうがないぜ。", "It is worthless to repair."));
        return 0;
    }

    if (o_ptr->number > 1) {
        msg_format(_("一度に複数を修復することはできません！", "They are too many to repair at once!"));
        return 0;
    }

    char basenm[MAX_NLEN];
    describe_flavor(player_ptr, basenm, o_ptr, OD_NAME_ONLY);
    prt(format(_("修復する武器　： %s", "Repairing: %s"), basenm), row + 3, 2);

    q = _("材料となる武器は？", "Which weapon for material? ");
    s = _("材料となる武器がありません。", "You have no material for the repair.");

    OBJECT_IDX mater;
    ItemEntity *mo_ptr;
    mo_ptr = choose_object(player_ptr, &mater, q, s, (USE_INVEN | USE_EQUIP), FuncItemTester(&ItemEntity::is_orthodox_melee_weapons));
    if (!mo_ptr) {
        return 0;
    }
    if (mater == item) {
        msg_print(_("クラインの壷じゃない！", "This is not a Klein bottle!"));
        return 0;
    }

    describe_flavor(player_ptr, basenm, mo_ptr, OD_NAME_ONLY);
    prt(format(_("材料とする武器： %s", "Material : %s"), basenm), row + 4, 2);
    PRICE cost = bcost + object_value_real(o_ptr) * 2;
    if (!get_check(format(_("＄%dかかりますがよろしいですか？ ", "Costs %d gold, okay? "), cost))) {
        return 0;
    }

    if (player_ptr->au < cost) {
        describe_flavor(player_ptr, basenm, o_ptr, OD_NAME_ONLY);
        msg_format(_("%sを修復するだけのゴールドがありません！", "You do not have the gold to repair %s!"), basenm);
        msg_print(nullptr);
        return 0;
    }

    short bi_id;
    if (o_ptr->bi_key.sval() == SV_BROKEN_DAGGER) {
        auto n = 1;
        bi_id = 0;
        for (const auto &baseitem : baseitems_info) {
            if (baseitem.bi_key.tval() != ItemKindType::SWORD) {
                continue;
            }

            const auto sval = baseitem.bi_key.sval();
            if ((sval == SV_BROKEN_DAGGER) || (sval == SV_BROKEN_SWORD) || (sval == SV_POISON_NEEDLE)) {
                continue;
            }

            if (baseitem.weight > 99) {
                continue;
            }

            if (one_in_(n)) {
                bi_id = baseitem.idx;
                n++;
            }
        }
    } else {
        auto tval = (one_in_(5) ? mo_ptr->bi_key.tval() : ItemKindType::SWORD);
        while (true) {
            bi_id = lookup_baseitem_id({ tval });
            const auto &baseitem = baseitems_info[bi_id];
            const auto sval = baseitem.bi_key.sval();
            if (tval == ItemKindType::SWORD) {
                if ((sval == SV_BROKEN_DAGGER) || (sval == SV_BROKEN_SWORD) || (sval == SV_DIAMOND_EDGE) || (sval == SV_POISON_NEEDLE)) {
                    continue;
                }
            }

            if (tval == ItemKindType::POLEARM) {
                if ((sval == SV_DEATH_SCYTHE) || (sval == SV_TSURIZAO)) {
                    continue;
                }
            }

            if (tval == ItemKindType::HAFTED) {
                if ((sval == SV_GROND) || (sval == SV_WIZSTAFF) || (sval == SV_NAMAKE_HAMMER)) {
                    continue;
                }
            }

            break;
        }
    }

    auto dd_bonus = o_ptr->dd - baseitems_info[o_ptr->bi_id].dd;
    auto ds_bonus = o_ptr->ds - baseitems_info[o_ptr->bi_id].ds;
    dd_bonus += mo_ptr->dd - baseitems_info[mo_ptr->bi_id].dd;
    ds_bonus += mo_ptr->ds - baseitems_info[mo_ptr->bi_id].ds;

    const auto &baseitem = baseitems_info[bi_id];
    o_ptr->bi_id = bi_id;
    o_ptr->weight = baseitem.weight;
    o_ptr->bi_key = baseitem.bi_key;
    o_ptr->dd = baseitem.dd;
    o_ptr->ds = baseitem.ds;
    o_ptr->art_flags.set(baseitem.flags);
    if (baseitem.pval) {
        o_ptr->pval = std::max<short>(o_ptr->pval, randint1(baseitem.pval));
    }

    if (baseitem.flags.has(TR_ACTIVATE)) {
        o_ptr->activation_id = baseitem.act_idx;
    }

    if (dd_bonus > 0) {
        o_ptr->dd++;
        for (int i = 1; i < dd_bonus; i++) {
            if (one_in_(o_ptr->dd + i)) {
                o_ptr->dd++;
            }
        }
    }

    if (ds_bonus > 0) {
        o_ptr->ds++;
        for (int i = 1; i < ds_bonus; i++) {
            if (one_in_(o_ptr->ds + i)) {
                o_ptr->ds++;
            }
        }
    }

    if (baseitem.flags.has(TR_BLOWS)) {
        auto bmax = std::min<short>(3, std::max<short>(1, 40 / (o_ptr->dd * o_ptr->ds)));
        o_ptr->pval = std::min<short>(o_ptr->pval, bmax);
    }

    give_one_ability_of_object(o_ptr, mo_ptr);
    o_ptr->to_d += std::max(0, (mo_ptr->to_d / 3));
    o_ptr->to_h += std::max<short>(0, (mo_ptr->to_h / 3));
    o_ptr->to_a += std::max<short>(0, (mo_ptr->to_a));

    const auto is_narsil = o_ptr->is_specific_artifact(FixedArtifactId::NARSIL);
    if (is_narsil || (o_ptr->is_random_artifact() && one_in_(1)) || (o_ptr->is_ego() && one_in_(7))) {
        if (o_ptr->is_ego()) {
            o_ptr->art_flags.set(TR_IGNORE_FIRE);
            o_ptr->art_flags.set(TR_IGNORE_ACID);
        }

        give_one_ability_of_object(o_ptr, mo_ptr);
        if (activation_index(o_ptr) == RandomArtActType::NONE) {
            one_activation(o_ptr);
        }

        if (is_narsil) {
            one_high_resistance(o_ptr);
            one_ability(o_ptr);
        }

        msg_print(_("これはかなりの業物だったようだ。", "This blade seems to be exceptional."));
    }

    describe_flavor(player_ptr, basenm, o_ptr, OD_NAME_ONLY);
#ifdef JP
    msg_format("＄%dで%sに修復しました。", cost, basenm);
#else
    msg_format("Repaired into %s for %d gold.", basenm, cost);
#endif
    msg_print(nullptr);
    o_ptr->ident &= ~(IDENT_BROKEN);
    o_ptr->discount = 99;

    calc_android_exp(player_ptr);
    inven_item_increase(player_ptr, mater, -1);
    inven_item_optimize(player_ptr, mater);

    player_ptr->update |= PU_BONUS;
    handle_stuff(player_ptr);
    return cost;
}

/*!
 * @brief アイテム修復処理の過渡ルーチン / Repair broken weapon
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param bcost 基本鑑定費用
 * @return 実際にかかった費用
 */
int repair_broken_weapon(PlayerType *player_ptr, PRICE bcost)
{
    PRICE cost;
    screen_save();
    cost = repair_broken_weapon_aux(player_ptr, bcost);
    screen_load();
    return cost;
}
