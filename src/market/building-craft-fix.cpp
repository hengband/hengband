#include "market/building-craft-fix.h"
#include "artifact/fixed-art-types.h"
#include "artifact/random-art-effects.h"
#include "core/asking-player.h"
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
#include "object/object-value.h"
#include "racial/racial-android.h"
#include "spell-realm/spells-hex.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/item/item-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include <utility>
#include <vector>

/*!
 * @brief 修復材料のオブジェクトから修復対象に特性を移植する。
 * @param to_ptr 修復対象オブジェクトの構造体の参照ポインタ。
 * @param from_ptr 修復材料オブジェクトの構造体の参照ポインタ。
 * @return 修復対象になるならTRUEを返す。
 */
static void give_one_ability_of_object(ItemEntity *to_ptr, ItemEntity *from_ptr)
{
    const auto to_flags = to_ptr->get_flags();
    const auto from_flags = from_ptr->get_flags();

    std::vector<tr_type> candidates;
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
            if (from_flags.has(tr_flag) && to_flags.has_not(tr_flag)) {
                if (!(TR_PVAL_FLAG_MASK.has(tr_flag) && (from_ptr->pval < 1))) {
                    candidates.push_back(tr_flag);
                }
            }
        }
    }

    if (candidates.empty()) {
        return;
    }

    const auto tr_idx = rand_choice(candidates);
    to_ptr->art_flags.set(tr_idx);
    if (TR_PVAL_FLAG_MASK.has(tr_idx)) {
        to_ptr->pval = std::max<short>(to_ptr->pval, 1);
    }
    auto bmax = std::min<short>(3, std::max<short>(1, 40 / to_ptr->damage_dice.maxroll()));
    if (tr_idx == TR_BLOWS) {
        to_ptr->pval = std::min<short>(to_ptr->pval, bmax);
    }
    if (tr_idx == TR_SPEED) {
        to_ptr->pval = std::min<short>(to_ptr->pval, 4);
    }
}

static std::pair<short, std::shared_ptr<ItemEntity>> select_repairing_broken_weapon(PlayerType *player_ptr, const int row)
{
    prt(_("修復には材料となるもう1つの武器が必要です。", "Hand one material weapon to repair a broken weapon."), row, 2);
    prt(_("材料に使用した武器はなくなります！", "The material weapon will disappear after repairing!!"), row + 1, 2);
    constexpr auto q = _("どの折れた武器を修復しますか？", "Repair which broken weapon? ");
    constexpr auto s = _("修復できる折れた武器がありません。", "You have no broken weapon to repair.");
    const auto &[item, i_idx] = choose_item(player_ptr, q, s, (USE_INVEN | USE_EQUIP), FuncItemTester(&ItemEntity::is_broken_weapon));
    if (!item) {
        return { i_idx, nullptr };
    }

    if (!item->is_ego() && !item->is_fixed_or_random_artifact()) {
        msg_format(_("それは直してもしょうがないぜ。", "It is worthless to repair."));
        return { i_idx, item };
    }

    if (item->number > 1) {
        msg_format(_("一度に複数を修復することはできません！", "They are too many to repair at once!"));
        return { i_idx, item };
    }

    return { i_idx, item };
}

static void display_reparing_weapon(PlayerType *player_ptr, const ItemEntity &item, const int row)
{
    const auto item_name = describe_flavor(player_ptr, item, OD_NAME_ONLY);
    prt(format(_("修復する武器　： %s", "Repairing: %s"), item_name.data()), row + 3, 2);
}

static void display_repair_success_message(PlayerType *player_ptr, const ItemEntity &item, const int cost)
{
    const auto item_name = describe_flavor(player_ptr, item, OD_NAME_ONLY);
#ifdef JP
    msg_format("＄%dで%sに修復しました。", cost, item_name.data());
#else
    msg_format("Repaired into %s for %d gold.", item_name.data(), cost);
#endif
    msg_erase();
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
    auto row = 7;
    const auto &[item_idx_broken, item_broken] = select_repairing_broken_weapon(player_ptr, row);
    if (!item_broken) {
        return 0;
    }

    display_reparing_weapon(player_ptr, *item_broken, row);
    constexpr auto q = _("材料となる武器は？", "Which weapon for material? ");
    constexpr auto s = _("材料となる武器がありません。", "You have no material for the repair.");
    const auto &[item_material, material_idx] = choose_item(player_ptr, q, s, (USE_INVEN | USE_EQUIP), FuncItemTester(&ItemEntity::is_orthodox_melee_weapons));
    if (!item_material) {
        return 0;
    }

    if (material_idx == item_idx_broken) {
        msg_print(_("クラインの壷じゃない！", "This is not a Klein bottle!"));
        return 0;
    }

    const auto item_name = describe_flavor(player_ptr, *item_material, OD_NAME_ONLY);
    prt(format(_("材料とする武器： %s", "Material : %s"), item_name.data()), row + 4, 2);
    const auto cost = bcost + object_value_real(item_broken.get()) * 2;
    if (!input_check(format(_("＄%dかかりますがよろしいですか？ ", "Costs %d gold, okay? "), cost))) {
        return 0;
    }

    if (player_ptr->au < cost) {
        msg_format(_("%sを修復するだけのゴールドがありません！", "You do not have the gold to repair %s!"), item_name.data());
        msg_erase();
        return 0;
    }

    short bi_id;
    const auto &baseitems = BaseitemList::get_instance();
    if (item_broken->bi_key.sval() == SV_BROKEN_DAGGER) {
        auto n = 1;
        bi_id = 0;
        for (const auto &baseitem : baseitems) {
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
        auto tval = (one_in_(5) ? item_material->bi_key.tval() : ItemKindType::SWORD);
        while (true) {
            bi_id = baseitems.lookup_baseitem_id({ tval });
            const auto &baseitem = baseitems.get_baseitem(bi_id);
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

    const auto &baseitem_o = item_broken->get_baseitem();
    const auto &baseitem_mo = item_material->get_baseitem();
    auto dd_bonus = item_broken->damage_dice.num - baseitem_o.damage_dice.num;
    auto ds_bonus = item_broken->damage_dice.sides - baseitem_o.damage_dice.sides;
    dd_bonus += item_material->damage_dice.num - baseitem_mo.damage_dice.num;
    ds_bonus += item_material->damage_dice.sides - baseitem_mo.damage_dice.sides;

    const auto &baseitem = baseitems.get_baseitem(bi_id);
    item_broken->bi_id = bi_id;
    item_broken->weight = baseitem.weight;
    item_broken->bi_key = baseitem.bi_key;
    item_broken->damage_dice = baseitem.damage_dice;
    item_broken->art_flags.set(baseitem.flags);
    if (baseitem.pval) {
        item_broken->pval = std::max(item_broken->pval, randnum1<short>(baseitem.pval));
    }

    if (baseitem.flags.has(TR_ACTIVATE)) {
        item_broken->activation_id = baseitem.act_idx;
    }

    auto &dice = item_broken->damage_dice;
    if (dd_bonus > 0) {
        dice.num++;
        for (int i = 1; i < dd_bonus; i++) {
            if (one_in_(dice.num + i)) {
                dice.num++;
            }
        }
    }

    if (ds_bonus > 0) {
        dice.sides++;
        for (int i = 1; i < ds_bonus; i++) {
            if (one_in_(dice.sides + i)) {
                dice.sides++;
            }
        }
    }

    if (baseitem.flags.has(TR_BLOWS)) {
        auto bmax = std::min<short>(3, std::max<short>(1, 40 / item_broken->damage_dice.maxroll()));
        item_broken->pval = std::min<short>(item_broken->pval, bmax);
    }

    give_one_ability_of_object(item_broken.get(), item_material.get());
    item_broken->to_d += std::max(0, (item_material->to_d / 3));
    item_broken->to_h += std::max<short>(0, (item_material->to_h / 3));
    item_broken->to_a += std::max<short>(0, (item_material->to_a));

    const auto is_narsil = item_broken->is_specific_artifact(FixedArtifactId::NARSIL);
    if (is_narsil || (item_broken->is_random_artifact() && one_in_(1)) || (item_broken->is_ego() && one_in_(7))) {
        if (item_broken->is_ego()) {
            item_broken->art_flags.set(TR_IGNORE_FIRE);
            item_broken->art_flags.set(TR_IGNORE_ACID);
        }

        give_one_ability_of_object(item_broken.get(), item_material.get());
        if (!item_broken->has_activation()) {
            one_activation(item_broken.get());
        }

        if (is_narsil) {
            one_high_resistance(item_broken.get());
            one_ability(item_broken.get());
        }

        msg_print(_("これはかなりの業物だったようだ。", "This blade seems to be exceptional."));
    }

    display_repair_success_message(player_ptr, *item_broken, cost);
    item_broken->ident &= ~(IDENT_BROKEN);
    item_broken->discount = 99;

    calc_android_exp(player_ptr);
    inven_item_increase(player_ptr, material_idx, -1);
    inven_item_optimize(player_ptr, material_idx);

    RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
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
