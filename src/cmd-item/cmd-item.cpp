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
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
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
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"
#include "util/quarks.h"
#include "util/string-processor.h"
#include "view/display-inventory.h"
#include "view/display-messages.h"

/*!
 * @brief 持ち物一覧を表示するコマンドのメインルーチン / Display inventory_list
 */
void do_cmd_inven(PlayerType *player_ptr)
{
    char out_val[160];
    command_wrk = false;
    if (easy_floor) {
        command_wrk = USE_INVEN;
    }

    screen_save();
    (void)show_inventory(player_ptr, 0, USE_FULL, AllMatchItemTester());
    WEIGHT weight = calc_inventory_weight(player_ptr);
    WEIGHT weight_lim = calc_weight_limit(player_ptr);
#ifdef JP
    sprintf(out_val, "持ち物： 合計 %3d.%1d kg (限界の%ld%%) コマンド: ", lb_to_kg_integer(weight), lb_to_kg_fraction(weight),
#else
    sprintf(out_val, "Inventory: carrying %d.%d pounds (%ld%% of capacity). Command: ", weight / 10, weight % 10,
#endif
        (long int)(weight * 100) / weight_lim);

    prt(out_val, 0, 0);
    command_new = inkey();
    screen_load();
    if (command_new != ESCAPE) {
        command_see = true;
        return;
    }

    TERM_LEN wid, hgt;
    term_get_size(&wid, &hgt);
    command_new = 0;
    command_gap = wid - 30;
}

/*!
 * @brief アイテムを落とすコマンドのメインルーチン / Drop an item
 */
void do_cmd_drop(PlayerType *player_ptr)
{
    OBJECT_IDX item;
    int amt = 1;
    ItemEntity *o_ptr;
    PlayerClass(player_ptr).break_samurai_stance({ SamuraiStanceType::MUSOU });

    concptr q = _("どのアイテムを落としますか? ", "Drop which item? ");
    concptr s = _("落とせるアイテムを持っていない。", "You have nothing to drop.");
    o_ptr = choose_object(player_ptr, &item, q, s, (USE_EQUIP | USE_INVEN | IGNORE_BOTHHAND_SLOT));
    if (!o_ptr) {
        return;
    }

    if ((item >= INVEN_MAIN_HAND) && o_ptr->is_cursed()) {
        msg_print(_("ふーむ、どうやら呪われているようだ。", "Hmmm, it seems to be cursed."));
        return;
    }

    if (o_ptr->number > 1) {
        amt = get_quantity(nullptr, o_ptr->number);
        if (amt <= 0) {
            return;
        }
    }

    PlayerEnergy(player_ptr).set_player_turn_energy(50);
    drop_from_inventory(player_ptr, item, amt);
    if (item >= INVEN_MAIN_HAND) {
        verify_equip_slot(player_ptr, item);
        calc_android_exp(player_ptr);
    }

    player_ptr->redraw |= (PR_EQUIPPY);
}

/*!
 * @brief アイテムを調査するコマンドのメインルーチン / Observe an item which has been *identify*-ed
 */
void do_cmd_observe(PlayerType *player_ptr)
{
    OBJECT_IDX item;
    ItemEntity *o_ptr;
    GAME_TEXT o_name[MAX_NLEN];
    concptr q = _("どのアイテムを調べますか? ", "Examine which item? ");
    concptr s = _("調べられるアイテムがない。", "You have nothing to examine.");
    o_ptr = choose_object(player_ptr, &item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT));
    if (!o_ptr) {
        return;
    }

    if (!o_ptr->is_fully_known()) {
        msg_print(_("このアイテムについて特に知っていることはない。", "You have no special knowledge about that item."));
        return;
    }

    describe_flavor(player_ptr, o_name, o_ptr, 0);
    msg_format(_("%sを調べている...", "Examining %s..."), o_name);
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
    OBJECT_IDX item;
    ItemEntity *o_ptr;
    concptr q = _("どのアイテムの銘を消しますか? ", "Un-inscribe which item? ");
    concptr s = _("銘を消せるアイテムがない。", "You have nothing to un-inscribe.");
    o_ptr = choose_object(player_ptr, &item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT));
    if (!o_ptr) {
        return;
    }

    if (!o_ptr->inscription) {
        msg_print(_("このアイテムには消すべき銘がない。", "That item had no inscription to remove."));
        return;
    }

    msg_print(_("銘を消した。", "Inscription removed."));
    o_ptr->inscription = 0;
    set_bits(player_ptr->update, PU_COMBINE);
    set_bits(player_ptr->window_flags, PW_INVEN | PW_EQUIP | PW_FLOOR_ITEM_LIST | PW_FOUND_ITEM_LIST);
    set_bits(player_ptr->update, PU_BONUS);
}

/*!
 * @brief アイテムの銘を刻むコマンドのメインルーチン
 * Inscribe an object with a comment
 */
void do_cmd_inscribe(PlayerType *player_ptr)
{
    OBJECT_IDX item;
    ItemEntity *o_ptr;
    GAME_TEXT o_name[MAX_NLEN];
    char out_val[MAX_INSCRIPTION + 1] = "";
    concptr q = _("どのアイテムに銘を刻みますか? ", "Inscribe which item? ");
    concptr s = _("銘を刻めるアイテムがない。", "You have nothing to inscribe.");
    o_ptr = choose_object(player_ptr, &item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT));
    if (!o_ptr) {
        return;
    }

    describe_flavor(player_ptr, o_name, o_ptr, OD_OMIT_INSCRIPTION);
    msg_format(_("%sに銘を刻む。", "Inscribing %s."), o_name);
    msg_print(nullptr);
    strcpy(out_val, "");
    if (o_ptr->inscription) {
        angband_strcpy(out_val, quark_str(o_ptr->inscription), MAX_INSCRIPTION);
    }

    if (get_string(_("銘: ", "Inscription: "), out_val, MAX_INSCRIPTION)) {
        o_ptr->inscription = quark_add(out_val);
        set_bits(player_ptr->update, PU_COMBINE);
        set_bits(player_ptr->window_flags, PW_INVEN | PW_EQUIP | PW_FLOOR_ITEM_LIST | PW_FOUND_ITEM_LIST);
        set_bits(player_ptr->update, PU_BONUS);
    }
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
    const auto q = _("どれを使いますか？", "Use which item? ");
    const auto s = _("使えるものがありません。", "You have nothing to use.");
    const auto options = USE_INVEN | USE_EQUIP | USE_FLOOR | IGNORE_BOTHHAND_SLOT;
    short item;
    const auto *o_ptr = choose_object(player_ptr, &item, q, s, options, FuncItemTester(item_tester_hook_use, player_ptr));
    if (o_ptr == nullptr) {
        return;
    }

    switch (o_ptr->bi_key.tval()) {
    case ItemKindType::SPIKE:
        do_cmd_spike(player_ptr);
        break;
    case ItemKindType::FOOD:
        exe_eat_food(player_ptr, item);
        break;
    case ItemKindType::WAND:
        ObjectZapWandEntity(player_ptr).execute(item);
        break;
    case ItemKindType::STAFF:
        ObjectUseEntity(player_ptr, item).execute();
        break;
    case ItemKindType::ROD:
        ObjectZapRodEntity(player_ptr).execute(item);
        break;
    case ItemKindType::POTION:
        ObjectQuaffEntity(player_ptr).execute(item);
        break;
    case ItemKindType::SCROLL:
        if (cmd_limit_blind(player_ptr) || cmd_limit_confused(player_ptr)) {
            return;
        }

        ObjectReadEntity(player_ptr, item).execute(true);
        break;
    case ItemKindType::SHOT:
    case ItemKindType::ARROW:
    case ItemKindType::BOLT:
        exe_fire(player_ptr, item, &player_ptr->inventory_list[INVEN_BOW], SP_NONE);
        break;
    default:
        exe_activate(player_ptr, item);
        break;
    }
}

/*!
 * @brief 装備を発動するコマンドのメインルーチン /
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void do_cmd_activate(PlayerType *player_ptr)
{
    OBJECT_IDX item;
    if (player_ptr->wild_mode || cmd_limit_arena(player_ptr)) {
        return;
    }

    PlayerClass(player_ptr).break_samurai_stance({ SamuraiStanceType::MUSOU, SamuraiStanceType::KOUKIJIN });

    concptr q = _("どのアイテムを始動させますか? ", "Activate which item? ");
    concptr s = _("始動できるアイテムを装備していない。", "You have nothing to activate.");
    if (!choose_object(player_ptr, &item, q, s, (USE_EQUIP | IGNORE_BOTHHAND_SLOT), FuncItemTester(&ItemEntity::is_activatable))) {
        return;
    }

    exe_activate(player_ptr, item);
}
