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
#include "cmd-item/cmd-usestaff.h"
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
#include "mind/snipe-types.h"
#include "object-activation/activation-switcher.h"
#include "object-hook/hook-magic.h"
#include "object-use/quaff-execution.h"
#include "object-use/read-execution.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "perception/identification.h"
#include "perception/object-perception.h"
#include "player-info/class-info.h"
#include "player-info/race-types.h"
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
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"
#include "util/quarks.h"
#include "view/display-inventory.h"
#include "view/display-messages.h"

/*!
 * @brief 持ち物一覧を表示するコマンドのメインルーチン / Display inventory_list
 */
void do_cmd_inven(player_type *creature_ptr)
{
    char out_val[160];
    command_wrk = false;
    if (easy_floor)
        command_wrk = USE_INVEN;

    screen_save();
    (void)show_inventory(creature_ptr, 0, USE_FULL, AllMatchItemTester());
    WEIGHT weight = calc_inventory_weight(creature_ptr);
    WEIGHT weight_lim = calc_weight_limit(creature_ptr);
#ifdef JP
    sprintf(out_val, "持ち物： 合計 %3d.%1d kg (限界の%ld%%) コマンド: ", (int)lbtokg1(weight), (int)lbtokg2(weight),
#else
    sprintf(out_val, "Inventory: carrying %d.%d pounds (%ld%% of capacity). Command: ", (int)(weight / 10), (int)(weight % 10),
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
void do_cmd_drop(player_type *creature_ptr)
{
    OBJECT_IDX item;
    int amt = 1;
    object_type *o_ptr;
    if (creature_ptr->special_defense & KATA_MUSOU)
        set_action(creature_ptr, ACTION_NONE);

    concptr q = _("どのアイテムを落としますか? ", "Drop which item? ");
    concptr s = _("落とせるアイテムを持っていない。", "You have nothing to drop.");
    o_ptr = choose_object(creature_ptr, &item, q, s, (USE_EQUIP | USE_INVEN | IGNORE_BOTHHAND_SLOT));
    if (!o_ptr)
        return;

    if ((item >= INVEN_MAIN_HAND) && o_ptr->is_cursed()) {
        msg_print(_("ふーむ、どうやら呪われているようだ。", "Hmmm, it seems to be cursed."));
        return;
    }

    if (o_ptr->number > 1) {
        amt = get_quantity(nullptr, o_ptr->number);
        if (amt <= 0)
            return;
    }

    PlayerEnergy(creature_ptr).set_player_turn_energy(50);
    drop_from_inventory(creature_ptr, item, amt);
    if (item >= INVEN_MAIN_HAND) {
        verify_equip_slot(creature_ptr, item);
        calc_android_exp(creature_ptr);
    }

    creature_ptr->redraw |= (PR_EQUIPPY);
}

/*!
 * @brief アイテムを調査するコマンドのメインルーチン / Observe an item which has been *identify*-ed
 */
void do_cmd_observe(player_type *creature_ptr)
{
    OBJECT_IDX item;
    object_type *o_ptr;
    GAME_TEXT o_name[MAX_NLEN];
    concptr q = _("どのアイテムを調べますか? ", "Examine which item? ");
    concptr s = _("調べられるアイテムがない。", "You have nothing to examine.");
    o_ptr = choose_object(creature_ptr, &item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT));
    if (!o_ptr)
        return;

    if (!o_ptr->is_fully_known()) {
        msg_print(_("このアイテムについて特に知っていることはない。", "You have no special knowledge about that item."));
        return;
    }

    describe_flavor(creature_ptr, o_name, o_ptr, 0);
    msg_format(_("%sを調べている...", "Examining %s..."), o_name);
    if (!screen_object(creature_ptr, o_ptr, SCROBJ_FORCE_DETAIL))
        msg_print(_("特に変わったところはないようだ。", "You see nothing special."));
}

/*!
 * @brief アイテムの銘を消すコマンドのメインルーチン
 * Remove the inscription from an object XXX Mention item (when done)?
 */
void do_cmd_uninscribe(player_type *creature_ptr)
{
    OBJECT_IDX item;
    object_type *o_ptr;
    concptr q = _("どのアイテムの銘を消しますか? ", "Un-inscribe which item? ");
    concptr s = _("銘を消せるアイテムがない。", "You have nothing to un-inscribe.");
    o_ptr = choose_object(creature_ptr, &item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT));
    if (!o_ptr)
        return;

    if (!o_ptr->inscription) {
        msg_print(_("このアイテムには消すべき銘がない。", "That item had no inscription to remove."));
        return;
    }

    msg_print(_("銘を消した。", "Inscription removed."));
    o_ptr->inscription = 0;
    set_bits(creature_ptr->update, PU_COMBINE);
    set_bits(creature_ptr->window_flags, PW_INVEN | PW_EQUIP | PW_FLOOR_ITEM_LIST);
    set_bits(creature_ptr->update, PU_BONUS);
}

/*!
 * @brief アイテムの銘を刻むコマンドのメインルーチン
 * Inscribe an object with a comment
 */
void do_cmd_inscribe(player_type *creature_ptr)
{
    OBJECT_IDX item;
    object_type *o_ptr;
    GAME_TEXT o_name[MAX_NLEN];
    char out_val[80];
    concptr q = _("どのアイテムに銘を刻みますか? ", "Inscribe which item? ");
    concptr s = _("銘を刻めるアイテムがない。", "You have nothing to inscribe.");
    o_ptr = choose_object(creature_ptr, &item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT));
    if (!o_ptr)
        return;

    describe_flavor(creature_ptr, o_name, o_ptr, OD_OMIT_INSCRIPTION);
    msg_format(_("%sに銘を刻む。", "Inscribing %s."), o_name);
    msg_print(nullptr);
    strcpy(out_val, "");
    if (o_ptr->inscription)
        strcpy(out_val, quark_str(o_ptr->inscription));

    if (get_string(_("銘: ", "Inscription: "), out_val, 80)) {
        o_ptr->inscription = quark_add(out_val);
        set_bits(creature_ptr->update, PU_COMBINE);
        set_bits(creature_ptr->window_flags, PW_INVEN | PW_EQUIP | PW_FLOOR_ITEM_LIST);
        set_bits(creature_ptr->update, PU_BONUS);
    }
}

/*!
 * @brief アイテムを汎用的に「使う」コマンドのメインルーチン /
 * Use an item
 * @details
 * XXX - Add actions for other item types
 */
void do_cmd_use(player_type *creature_ptr)
{
    OBJECT_IDX item;
    object_type *o_ptr;
    if (creature_ptr->wild_mode || cmd_limit_arena(creature_ptr))
        return;

    if (creature_ptr->special_defense & (KATA_MUSOU | KATA_KOUKIJIN))
        set_action(creature_ptr, ACTION_NONE);

    concptr q = _("どれを使いますか？", "Use which item? ");
    concptr s = _("使えるものがありません。", "You have nothing to use.");
    o_ptr = choose_object(
        creature_ptr, &item, q, s, (USE_INVEN | USE_EQUIP | USE_FLOOR | IGNORE_BOTHHAND_SLOT), FuncItemTester(item_tester_hook_use, creature_ptr));
    if (!o_ptr)
        return;

    switch (o_ptr->tval) {
    case TV_SPIKE:
        do_cmd_spike(creature_ptr);
        break;
    case TV_FOOD:
        exe_eat_food(creature_ptr, item);
        break;
    case TV_WAND:
        exe_aim_wand(creature_ptr, item);
        break;
    case TV_STAFF:
        exe_use_staff(creature_ptr, item);
        break;
    case TV_ROD:
        exe_zap_rod(creature_ptr, item);
        break;
    case TV_POTION:
        exe_quaff_potion(creature_ptr, item);
        break;
    case TV_SCROLL:
        if (cmd_limit_blind(creature_ptr) || cmd_limit_confused(creature_ptr))
            return;

        exe_read(creature_ptr, item, true);
        break;
    case TV_SHOT:
    case TV_ARROW:
    case TV_BOLT:
        exe_fire(creature_ptr, item, &creature_ptr->inventory_list[INVEN_BOW], SP_NONE);
        break;
    default:
        exe_activate(creature_ptr, item);
        break;
    }
}

/*!
 * @brief 装備を発動するコマンドのメインルーチン /
 * @param user_ptr プレーヤーへの参照ポインタ
 */
void do_cmd_activate(player_type *user_ptr)
{
    OBJECT_IDX item;
    if (user_ptr->wild_mode || cmd_limit_arena(user_ptr))
        return;

    if (user_ptr->special_defense & (KATA_MUSOU | KATA_KOUKIJIN))
        set_action(user_ptr, ACTION_NONE);

    concptr q = _("どのアイテムを始動させますか? ", "Activate which item? ");
    concptr s = _("始動できるアイテムを装備していない。", "You have nothing to activate.");
    if (!choose_object(user_ptr, &item, q, s, (USE_EQUIP | IGNORE_BOTHHAND_SLOT), FuncItemTester(&object_type::is_activatable)))
        return;

    exe_activate(user_ptr, item);
}
