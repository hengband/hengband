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
#include "autopick/autopick-registry.h"
#include "autopick/autopick.h"
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
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "game-option/input-options.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/snipe-types.h"
#include "object-activation/activation-switcher.h"
#include "object-enchant/object-ego.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-expendable.h"
#include "object-hook/hook-magic.h"
#include "object-use/quaff-execution.h"
#include "object-use/read-execution.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-generator.h"
#include "object/object-stack.h"
#include "object/object-value.h"
#include "perception/identification.h"
#include "perception/object-perception.h"
#include "player/attack-defense-types.h"
#include "player/avatar.h"
#include "player/player-class.h"
#include "player/player-personalities-types.h"
#include "player/player-race-types.h"
#include "player/player-status.h"
#include "player/selfinfo.h"
#include "player/special-defense-types.h"
#include "racial/racial-android.h"
#include "realm/realm-hex-numbers.h"
#include "realm/realm-names-table.h"
#include "realm/realm-types.h"
#include "status/action-setter.h"
#include "status/experience.h"
#include "sv-definition/sv-lite-types.h"
#include "term/screen-processor.h"
#include "util/int-char-converter.h"
#include "util/quarks.h"
#include "view/display-inventory.h"
#include "view/display-messages.h"

/*!
 * @brief 持ち物一覧を表示するコマンドのメインルーチン / Display inventory_list
 * @return なし
 */
void do_cmd_inven(player_type *creature_ptr)
{
    char out_val[160];
    command_wrk = FALSE;
    if (easy_floor)
        command_wrk = USE_INVEN;

    screen_save();
    (void)show_inventory(creature_ptr, 0, USE_FULL, 0);
#ifdef JP
    sprintf(out_val, "持ち物： 合計 %3d.%1d kg (限界の%ld%%) コマンド: ", (int)lbtokg1(creature_ptr->total_weight), (int)lbtokg2(creature_ptr->total_weight),
#else
    sprintf(out_val, "Inventory: carrying %d.%d pounds (%ld%% of capacity). Command: ", (int)(creature_ptr->total_weight / 10),
        (int)(creature_ptr->total_weight % 10),
#endif
        (long int)(creature_ptr->total_weight * 100) / weight_limit(creature_ptr));

    prt(out_val, 0, 0);
    command_new = inkey();
    screen_load();
    if (command_new != ESCAPE) {
        command_see = TRUE;
        return;
    }

    TERM_LEN wid, hgt;
    term_get_size(&wid, &hgt);
    command_new = 0;
    command_gap = wid - 30;
}

/*!
 * @brief アイテムを落とすコマンドのメインルーチン / Drop an item
 * @return なし
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
    o_ptr = choose_object(creature_ptr, &item, q, s, (USE_EQUIP | USE_INVEN | IGNORE_BOTHHAND_SLOT), 0);
    if (!o_ptr)
        return;

    if ((item >= INVEN_RARM) && object_is_cursed(o_ptr)) {
        msg_print(_("ふーむ、どうやら呪われているようだ。", "Hmmm, it seems to be cursed."));
        return;
    }

    if (o_ptr->number > 1) {
        amt = get_quantity(NULL, o_ptr->number);
        if (amt <= 0)
            return;
    }

    take_turn(creature_ptr, 50);
    drop_from_inventory(creature_ptr, item, amt);
    if (item >= INVEN_RARM) {
        verify_equip_slot(creature_ptr, item);
        calc_android_exp(creature_ptr);
    }

    creature_ptr->redraw |= (PR_EQUIPPY);
}

/*!
 * @brief アイテムを破壊するコマンドのメインルーチン / Destroy an item
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void do_cmd_destroy(player_type *creature_ptr)
{
    OBJECT_IDX item;
    QUANTITY amt = 1;
    QUANTITY old_number;
    bool force = FALSE;
    object_type *o_ptr;
    object_type forge;
    object_type *q_ptr = &forge;
    GAME_TEXT o_name[MAX_NLEN];
    char out_val[MAX_NLEN + 40];
    if (creature_ptr->special_defense & KATA_MUSOU)
        set_action(creature_ptr, ACTION_NONE);

    if (command_arg > 0)
        force = TRUE;

    concptr q = _("どのアイテムを壊しますか? ", "Destroy which item? ");
    concptr s = _("壊せるアイテムを持っていない。", "You have nothing to destroy.");
    o_ptr = choose_object(creature_ptr, &item, q, s, (USE_INVEN | USE_FLOOR), 0);
    if (!o_ptr)
        return;

    if (!force && (confirm_destroy || (object_value(creature_ptr, o_ptr) > 0))) {
        describe_flavor(creature_ptr, o_name, o_ptr, OD_OMIT_PREFIX);
        sprintf(out_val, _("本当に%sを壊しますか? [y/n/Auto]", "Really destroy %s? [y/n/Auto]"), o_name);
        msg_print(NULL);
        message_add(out_val);
        creature_ptr->window |= (PW_MESSAGE);
        handle_stuff(creature_ptr);
        while (TRUE) {
            prt(out_val, 0, 0);
            char i = inkey();
            prt("", 0, 0);
            if (i == 'y' || i == 'Y')
                break;

            if (i == ESCAPE || i == 'n' || i == 'N')
                return;

            if (i == 'A') {
                if (autopick_autoregister(creature_ptr, o_ptr))
                    autopick_alter_item(creature_ptr, item, TRUE);

                return;
            }
        }
    }

    if (o_ptr->number > 1) {
        amt = get_quantity(NULL, o_ptr->number);
        if (amt <= 0)
            return;
    }

    old_number = o_ptr->number;
    o_ptr->number = amt;
    describe_flavor(creature_ptr, o_name, o_ptr, 0);
    o_ptr->number = old_number;
    take_turn(creature_ptr, 100);
    if (!can_player_destroy_object(creature_ptr, o_ptr)) {
        free_turn(creature_ptr);
        msg_format(_("%sは破壊不可能だ。", "You cannot destroy %s."), o_name);
        return;
    }

    object_copy(q_ptr, o_ptr);
    msg_format(_("%sを壊した。", "You destroy %s."), o_name);
    sound(SOUND_DESTITEM);
    reduce_charges(o_ptr, amt);
    vary_item(creature_ptr, item, -amt);
    if (item_tester_high_level_book(q_ptr)) {
        bool gain_expr = FALSE;
        if (creature_ptr->prace == RACE_ANDROID) {
        } else if ((creature_ptr->pclass == CLASS_WARRIOR) || (creature_ptr->pclass == CLASS_BERSERKER)) {
            gain_expr = TRUE;
        } else if (creature_ptr->pclass == CLASS_PALADIN) {
            if (is_good_realm(creature_ptr->realm1)) {
                if (!is_good_realm(tval2realm(q_ptr->tval)))
                    gain_expr = TRUE;
            } else {
                if (is_good_realm(tval2realm(q_ptr->tval)))
                    gain_expr = TRUE;
            }
        }

        if (gain_expr && (creature_ptr->exp < PY_MAX_EXP)) {
            s32b tester_exp = creature_ptr->max_exp / 20;
            if (tester_exp > 10000)
                tester_exp = 10000;

            if (q_ptr->sval < 3)
                tester_exp /= 4;

            if (tester_exp < 1)
                tester_exp = 1;

            msg_print(_("更に経験を積んだような気がする。", "You feel more experienced."));
            gain_exp(creature_ptr, tester_exp * amt);
        }

        if (item_tester_high_level_book(q_ptr) && q_ptr->tval == TV_LIFE_BOOK) {
            chg_virtue(creature_ptr, V_UNLIFE, 1);
            chg_virtue(creature_ptr, V_VITALITY, -1);
        } else if (item_tester_high_level_book(q_ptr) && q_ptr->tval == TV_DEATH_BOOK) {
            chg_virtue(creature_ptr, V_UNLIFE, -1);
            chg_virtue(creature_ptr, V_VITALITY, 1);
        }

        if (q_ptr->to_a || q_ptr->to_h || q_ptr->to_d)
            chg_virtue(creature_ptr, V_ENCHANT, -1);

        if (object_value_real(creature_ptr, q_ptr) > 30000)
            chg_virtue(creature_ptr, V_SACRIFICE, 2);
        else if (object_value_real(creature_ptr, q_ptr) > 10000)
            chg_virtue(creature_ptr, V_SACRIFICE, 1);
    }

    if (q_ptr->to_a != 0 || q_ptr->to_d != 0 || q_ptr->to_h != 0)
        chg_virtue(creature_ptr, V_HARMONY, 1);

    if (item >= INVEN_RARM)
        calc_android_exp(creature_ptr);
}

/*!
 * @brief アイテムを調査するコマンドのメインルーチン / Observe an item which has been *identify*-ed
 * @return なし
 */
void do_cmd_observe(player_type *creature_ptr)
{
    OBJECT_IDX item;
    object_type *o_ptr;
    GAME_TEXT o_name[MAX_NLEN];
    concptr q = _("どのアイテムを調べますか? ", "Examine which item? ");
    concptr s = _("調べられるアイテムがない。", "You have nothing to examine.");
    o_ptr = choose_object(creature_ptr, &item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT), 0);
    if (!o_ptr)
        return;

    if (!object_is_fully_known(o_ptr)) {
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
 * @return なし
 */
void do_cmd_uninscribe(player_type *creature_ptr)
{
    OBJECT_IDX item;
    object_type *o_ptr;
    concptr q = _("どのアイテムの銘を消しますか? ", "Un-inscribe which item? ");
    concptr s = _("銘を消せるアイテムがない。", "You have nothing to un-inscribe.");
    o_ptr = choose_object(creature_ptr, &item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT), 0);
    if (!o_ptr)
        return;

    if (!o_ptr->inscription) {
        msg_print(_("このアイテムには消すべき銘がない。", "That item had no inscription to remove."));
        return;
    }

    msg_print(_("銘を消した。", "Inscription removed."));
    o_ptr->inscription = 0;
    creature_ptr->update |= (PU_COMBINE);
    creature_ptr->window |= (PW_INVEN | PW_EQUIP);
    creature_ptr->update |= (PU_BONUS);
}

/*!
 * @brief アイテムの銘を刻むコマンドのメインルーチン
 * Inscribe an object with a comment
 * @return なし
 */
void do_cmd_inscribe(player_type *creature_ptr)
{
    OBJECT_IDX item;
    object_type *o_ptr;
    GAME_TEXT o_name[MAX_NLEN];
    char out_val[80];
    concptr q = _("どのアイテムに銘を刻みますか? ", "Inscribe which item? ");
    concptr s = _("銘を刻めるアイテムがない。", "You have nothing to inscribe.");
    o_ptr = choose_object(creature_ptr, &item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT), 0);
    if (!o_ptr)
        return;

    describe_flavor(creature_ptr, o_name, o_ptr, OD_OMIT_INSCRIPTION);
    msg_format(_("%sに銘を刻む。", "Inscribing %s."), o_name);
    msg_print(NULL);
    strcpy(out_val, "");
    if (o_ptr->inscription)
        strcpy(out_val, quark_str(o_ptr->inscription));

    if (get_string(_("銘: ", "Inscription: "), out_val, 80)) {
        o_ptr->inscription = quark_add(out_val);
        creature_ptr->update |= (PU_COMBINE);
        creature_ptr->window |= (PW_INVEN | PW_EQUIP);
        creature_ptr->update |= (PU_BONUS);
    }
}

/*!
 * @brief ランタンに燃料を加えるコマンドのメインルーチン
 * Refill the players lamp (from the pack or floor)
 * @return なし
 */
static void do_cmd_refill_lamp(player_type *user_ptr)
{
    OBJECT_IDX item;
    object_type *o_ptr;
    object_type *j_ptr;
    item_tester_hook = item_tester_refill_lantern;
    concptr q = _("どの油つぼから注ぎますか? ", "Refill with which flask? ");
    concptr s = _("油つぼがない。", "You have no flasks of oil.");
    o_ptr = choose_object(user_ptr, &item, q, s, (USE_INVEN | USE_FLOOR), 0);
    if (!o_ptr)
        return;

    take_turn(user_ptr, 50);
    j_ptr = &user_ptr->inventory_list[INVEN_LITE];
    j_ptr->xtra4 += o_ptr->xtra4;
    msg_print(_("ランプに油を注いだ。", "You fuel your lamp."));
    if ((o_ptr->name2 == EGO_LITE_DARKNESS) && (j_ptr->xtra4 > 0)) {
        j_ptr->xtra4 = 0;
        msg_print(_("ランプが消えてしまった！", "Your lamp has gone out!"));
    } else if ((o_ptr->name2 == EGO_LITE_DARKNESS) || (j_ptr->name2 == EGO_LITE_DARKNESS)) {
        j_ptr->xtra4 = 0;
        msg_print(_("しかしランプは全く光らない。", "Curiously, your lamp doesn't light."));
    } else if (j_ptr->xtra4 >= FUEL_LAMP) {
        j_ptr->xtra4 = FUEL_LAMP;
        msg_print(_("ランプの油は一杯だ。", "Your lamp is full."));
    }

    vary_item(user_ptr, item, -1);
    user_ptr->update |= (PU_TORCH);
}

/*!
 * @brief 松明を束ねるコマンドのメインルーチン
 * Refuel the players torch (from the pack or floor)
 * @return なし
 */
static void do_cmd_refill_torch(player_type *creature_ptr)
{
    OBJECT_IDX item;
    object_type *o_ptr;
    object_type *j_ptr;
    item_tester_hook = object_can_refill_torch;
    concptr q = _("どの松明で明かりを強めますか? ", "Refuel with which torch? ");
    concptr s = _("他に松明がない。", "You have no extra torches.");
    o_ptr = choose_object(creature_ptr, &item, q, s, (USE_INVEN | USE_FLOOR), 0);
    if (!o_ptr)
        return;

    take_turn(creature_ptr, 50);
    j_ptr = &creature_ptr->inventory_list[INVEN_LITE];
    j_ptr->xtra4 += o_ptr->xtra4 + 5;
    msg_print(_("松明を結合した。", "You combine the torches."));
    if ((o_ptr->name2 == EGO_LITE_DARKNESS) && (j_ptr->xtra4 > 0)) {
        j_ptr->xtra4 = 0;
        msg_print(_("松明が消えてしまった！", "Your torch has gone out!"));
    } else if ((o_ptr->name2 == EGO_LITE_DARKNESS) || (j_ptr->name2 == EGO_LITE_DARKNESS)) {
        j_ptr->xtra4 = 0;
        msg_print(_("しかし松明は全く光らない。", "Curiously, your torch doesn't light."));
    } else if (j_ptr->xtra4 >= FUEL_TORCH) {
        j_ptr->xtra4 = FUEL_TORCH;
        msg_print(_("松明の寿命は十分だ。", "Your torch is fully fueled."));
    } else
        msg_print(_("松明はいっそう明るく輝いた。", "Your torch glows more brightly."));

    vary_item(creature_ptr, item, -1);
    creature_ptr->update |= (PU_TORCH);
}

/*!
 * @brief 燃料を補充するコマンドのメインルーチン
 * Refill the players lamp, or restock his torches
 * @return なし
 */
void do_cmd_refill(player_type *creature_ptr)
{
    object_type *o_ptr;
    o_ptr = &creature_ptr->inventory_list[INVEN_LITE];
    if (creature_ptr->special_defense & KATA_MUSOU)
        set_action(creature_ptr, ACTION_NONE);

    if (o_ptr->tval != TV_LITE)
        msg_print(_("光源を装備していない。", "You are not wielding a light."));
    else if (o_ptr->sval == SV_LITE_LANTERN)
        do_cmd_refill_lamp(creature_ptr);
    else if (o_ptr->sval == SV_LITE_TORCH)
        do_cmd_refill_torch(creature_ptr);
    else
        msg_print(_("この光源は寿命を延ばせない。", "Your light cannot be refilled."));
}

/*!
 * @brief アイテムを汎用的に「使う」コマンドのメインルーチン /
 * Use an item
 * @return なし
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

    item_tester_hook = item_tester_hook_use;
    concptr q = _("どれを使いますか？", "Use which item? ");
    concptr s = _("使えるものがありません。", "You have nothing to use.");
    o_ptr = choose_object(creature_ptr, &item, q, s, (USE_INVEN | USE_EQUIP | USE_FLOOR | IGNORE_BOTHHAND_SLOT), 0);
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

        exe_read(creature_ptr, item, TRUE);
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
 * @return なし
 */
void do_cmd_activate(player_type *user_ptr)
{
    OBJECT_IDX item;
    if (user_ptr->wild_mode || cmd_limit_arena(user_ptr))
        return;

    if (user_ptr->special_defense & (KATA_MUSOU | KATA_KOUKIJIN))
        set_action(user_ptr, ACTION_NONE);

    item_tester_hook = item_tester_hook_activate;

    concptr q = _("どのアイテムを始動させますか? ", "Activate which item? ");
    concptr s = _("始動できるアイテムを装備していない。", "You have nothing to activate.");
    if (!choose_object(user_ptr, &item, q, s, (USE_EQUIP | IGNORE_BOTHHAND_SLOT), 0))
        return;

    exe_activate(user_ptr, item);
}
