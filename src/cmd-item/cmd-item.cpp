/*!
 *  @brief プレイヤーのアイテムに関するコマンドの実装1 / Inventory and equipment commands
 *  @date 2014/01/02
 *  @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */

#include "cmd-item/cmd-item.h"
#include "action/action-limited.h"
#include "action/activation-execution.h"
#include "action/weapon-shield.h"
#include "cmd-action/cmd-open-close.h"
#include "cmd-action/cmd-pet.h"
#include "cmd-item/cmd-eat.h"
#include "cmd-item/cmd-quaff.h"
#include "cmd-item/cmd-read.h"
#include "cmd-item/cmd-zaprod.h"
#include "cmd-item/cmd-zapwand.h"
#include "combat/shoot.h"
#include "core/asking-player.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "game-option/input-options.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "locale/japanese.h"
#include "mind/snipe-types.h"
#include "object-activation/activation-switcher.h"
#include "object-hook/hook-magic.h"
#include "object-use/quaff/quaff-execution.h"
#include "object-use/read/read-execution.h"
#include "object-use/use-execution.h"
#include "object-use/zaprod-execution.h"
#include "object-use/zapwand-execution.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "perception/identification.h"
#include "perception/object-perception.h"
#include "player-base/player-class.h"
#include "player-info/class-info.h"
#include "player-info/race-types.h"
#include "player-info/samurai-data-type.h"
#include "player-info/self-info.h"
#include "player-status/player-energy.h"
#include "player/attack-defense-types.h"
#include "player/player-personality-types.h"
#include "player/player-status.h"
#include "player/special-defense-types.h"
#include "racial/racial-android.h"
#include "realm/realm-hex-numbers.h"
#include "realm/realm-types.h"
#include "status/action-setter.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/screen-processor.h"
#include "term/z-form.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include "view/display-inventory.h"
#include "view/display-messages.h"

/*!
 * @brief 持ち物一覧を表示するコマンドのメインルーチン / Display inventory_list
 */
void do_cmd_inven(PlayerType *player_ptr)
{
    command_wrk = false;
    if (easy_floor) {
        command_wrk = USE_INVEN;
    }

    screen_save();
    (void)show_inventory(player_ptr, 0, USE_FULL, AllMatchItemTester());
    WEIGHT weight = calc_inventory_weight(player_ptr);
    WEIGHT weight_lim = calc_weight_limit(player_ptr);
    std::string out_val;
#ifdef JP
    out_val = format("持ち物： 合計 %3d.%1d kg (限界の%ld%%) コマンド: ", lb_to_kg_integer(weight), lb_to_kg_fraction(weight),
#else
    out_val = format("Inventory: carrying %d.%d pounds (%ld%% of capacity). Command: ", weight / 10, weight % 10,
#endif
        (long int)(weight * 100) / weight_lim);

    prt(out_val, 0, 0);
    command_new = inkey();
    screen_load();
    if (command_new != ESCAPE) {
        command_see = true;
        return;
    }

    const auto &[wid, hgt] = term_get_size();
    command_new = 0;
    command_gap = wid - 30;
}

/*!
 * @brief アイテムを落とすコマンドのメインルーチン / Drop an item
 */
void do_cmd_drop(PlayerType *player_ptr)
{
    int amt = 1;
    PlayerClass(player_ptr).break_samurai_stance({ SamuraiStanceType::MUSOU });

    constexpr auto q = _("どのアイテムを落としますか? ", "Drop which item? ");
    constexpr auto s = _("落とせるアイテムを持っていない。", "You have nothing to drop.");
    short i_idx;
    auto *o_ptr = choose_object(player_ptr, &i_idx, q, s, (USE_EQUIP | USE_INVEN | IGNORE_BOTHHAND_SLOT));
    if (!o_ptr) {
        return;
    }

    if ((i_idx >= INVEN_MAIN_HAND) && o_ptr->is_cursed()) {
        msg_print(_("ふーむ、どうやら呪われているようだ。", "Hmmm, it seems to be cursed."));
        return;
    }

    if (o_ptr->number > 1) {
        amt = input_quantity(o_ptr->number);
        if (amt <= 0) {
            return;
        }
    }

    PlayerEnergy(player_ptr).set_player_turn_energy(50);
    drop_from_inventory(player_ptr, i_idx, amt);
    if (i_idx >= INVEN_MAIN_HAND) {
        verify_equip_slot(player_ptr, i_idx);
        calc_android_exp(player_ptr);
    }

    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::EQUIPPY);
}

/*!
 * @brief アイテムを調査するコマンドのメインルーチン / Observe an item which has been *identify*-ed
 */
void do_cmd_observe(PlayerType *player_ptr)
{
    constexpr auto q = _("どのアイテムを調べますか? ", "Examine which item? ");
    constexpr auto s = _("調べられるアイテムがない。", "You have nothing to examine.");
    short i_idx;
    auto *o_ptr = choose_object(player_ptr, &i_idx, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT));
    if (!o_ptr) {
        return;
    }

    if (!o_ptr->is_fully_known()) {
        msg_print(_("このアイテムについて特に知っていることはない。", "You have no special knowledge about that item."));
        return;
    }

    const auto item_name = describe_flavor(player_ptr, o_ptr, 0);
    msg_format(_("%sを調べている...", "Examining %s..."), item_name.data());
    if (!screen_object(player_ptr, o_ptr, SCROBJ_FORCE_DETAIL)) {
        msg_print(_("特に変わったところはないようだ。", "You see nothing special."));
    }
}

/*!
 * @brief アイテムの銘を消すコマンドのメインルーチン
 * Remove the inscription from an object XXX Mention item (when done)?
 */
void do_cmd_uninscribe(PlayerType *player_ptr)
{
    constexpr auto q = _("どのアイテムの銘を消しますか? ", "Un-inscribe which item? ");
    constexpr auto s = _("銘を消せるアイテムがない。", "You have nothing to un-inscribe.");
    short i_idx;
    auto *o_ptr = choose_object(player_ptr, &i_idx, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT));
    if (!o_ptr) {
        return;
    }

    if (!o_ptr->is_inscribed()) {
        msg_print(_("このアイテムには消すべき銘がない。", "That item had no inscription to remove."));
        return;
    }

    msg_print(_("銘を消した。", "Inscription removed."));
    o_ptr->inscription.reset();
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::COMBINATION,
        StatusRecalculatingFlag::BONUS,
    };
    rfu.set_flags(flags_srf);
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::INVENTORY,
        SubWindowRedrawingFlag::EQUIPMENT,
        SubWindowRedrawingFlag::FLOOR_ITEMS,
        SubWindowRedrawingFlag::FOUND_ITEMS,
    };
    rfu.set_flags(flags_swrf);
}

/*!
 * @brief アイテムの銘を刻むコマンドのメインルーチン
 * Inscribe an object with a comment
 */
void do_cmd_inscribe(PlayerType *player_ptr)
{
    constexpr auto q = _("どのアイテムに銘を刻みますか? ", "Inscribe which item? ");
    constexpr auto s = _("銘を刻めるアイテムがない。", "You have nothing to inscribe.");
    short i_idx;
    auto *o_ptr = choose_object(player_ptr, &i_idx, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT));
    if (!o_ptr) {
        return;
    }

    const auto item_name = describe_flavor(player_ptr, o_ptr, OD_OMIT_INSCRIPTION);
    msg_format(_("%sに銘を刻む。", "Inscribing %s."), item_name.data());
    msg_print(nullptr);
    const auto initial_inscription = o_ptr->is_inscribed() ? o_ptr->inscription.value() : "";
    const auto input_inscription = input_string(_("銘: ", "Inscription: "), MAX_INSCRIPTION, initial_inscription);
    if (!input_inscription.has_value()) {
        return;
    }

    o_ptr->inscription.emplace(input_inscription.value());
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::COMBINATION,
        StatusRecalculatingFlag::BONUS,
    };
    rfu.set_flags(flags_srf);
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::INVENTORY,
        SubWindowRedrawingFlag::EQUIPMENT,
        SubWindowRedrawingFlag::FLOOR_ITEMS,
        SubWindowRedrawingFlag::FOUND_ITEMS,
    };
    rfu.set_flags(flags_swrf);
}

/*!
 * @brief アイテムを汎用的に「使う」コマンドのメインルーチン /
 * Use an item
 * @details
 * XXX - Add actions for other item types
 */
void do_cmd_use(PlayerType *player_ptr)
{
    if (player_ptr->wild_mode || cmd_limit_arena(player_ptr)) {
        return;
    }

    PlayerClass(player_ptr).break_samurai_stance({ SamuraiStanceType::MUSOU, SamuraiStanceType::KOUKIJIN });
    constexpr auto q = _("どれを使いますか？", "Use which item? ");
    constexpr auto s = _("使えるものがありません。", "You have nothing to use.");
    const auto options = USE_INVEN | USE_EQUIP | USE_FLOOR | IGNORE_BOTHHAND_SLOT;
    short i_idx;
    const auto *o_ptr = choose_object(player_ptr, &i_idx, q, s, options, FuncItemTester(item_tester_hook_use, player_ptr));
    if (o_ptr == nullptr) {
        return;
    }

    switch (o_ptr->bi_key.tval()) {
    case ItemKindType::SPIKE:
        do_cmd_spike(player_ptr);
        break;
    case ItemKindType::FOOD:
        exe_eat_food(player_ptr, i_idx);
        break;
    case ItemKindType::WAND:
        ObjectZapWandEntity(player_ptr).execute(i_idx);
        break;
    case ItemKindType::STAFF:
        ObjectUseEntity(player_ptr, i_idx).execute();
        break;
    case ItemKindType::ROD:
        ObjectZapRodEntity(player_ptr).execute(i_idx);
        break;
    case ItemKindType::POTION:
        ObjectQuaffEntity(player_ptr).execute(i_idx);
        break;
    case ItemKindType::SCROLL:
        if (cmd_limit_blind(player_ptr) || cmd_limit_confused(player_ptr)) {
            return;
        }

        ObjectReadEntity(player_ptr, i_idx).execute(true);
        break;
    case ItemKindType::SHOT:
    case ItemKindType::ARROW:
    case ItemKindType::BOLT:
        exe_fire(player_ptr, i_idx, &player_ptr->inventory_list[INVEN_BOW], SP_NONE);
        break;
    default:
        exe_activate(player_ptr, i_idx);
        break;
    }
}

/*!
 * @brief 装備を発動するコマンドのメインルーチン /
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void do_cmd_activate(PlayerType *player_ptr)
{
    if (player_ptr->wild_mode || cmd_limit_arena(player_ptr)) {
        return;
    }

    PlayerClass(player_ptr).break_samurai_stance({ SamuraiStanceType::MUSOU, SamuraiStanceType::KOUKIJIN });
    constexpr auto q = _("どのアイテムを始動させますか? ", "Activate which item? ");
    constexpr auto s = _("始動できるアイテムを装備していない。", "You have nothing to activate.");
    short i_idx;
    if (!choose_object(player_ptr, &i_idx, q, s, (USE_EQUIP | IGNORE_BOTHHAND_SLOT), FuncItemTester(&ItemEntity::is_activatable))) {
        return;
    }

    exe_activate(player_ptr, i_idx);
}
