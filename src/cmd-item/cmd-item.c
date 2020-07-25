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
#include "action/weapon-shield.h"
#include "art-definition/art-protector-types.h"
#include "autopick/autopick-registry.h"
#include "autopick/autopick.h"
#include "cmd-action/cmd-open-close.h"
#include "cmd-action/cmd-pet.h"
#include "cmd-item/cmd-activate.h"
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
#include "dungeon/quest.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "game-option/birth-options.h"
#include "game-option/input-options.h"
#include "game-option/keymap-directory-getter.h"
#include "inventory/inventory-describer.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "io/cursor.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "io/screen-util.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/snipe-types.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/trc-types.h"
#include "object-hook/hook-armor.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-expendable.h"
#include "object-hook/hook-magic.h"
#include "object-hook/hook-weapon.h"
#include "object-use/quaff-execution.h"
#include "object-use/read-execution.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-generator.h"
#include "object/object-info.h"
#include "object/object-mark-types.h"
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
#include "spell-kind/spells-perception.h"
#include "status/action-setter.h"
#include "status/experience.h"
#include "status/shape-changer.h"
#include "sv-definition/sv-lite-types.h"
#include "target/target-checker.h"
#include "target/target-setter.h"
#include "target/target-types.h"
#include "term/screen-processor.h"
#include "util/int-char-converter.h"
#include "util/quarks.h"
#include "view/display-inventory.h"
#include "view/display-messages.h"
#include "window/main-window-util.h"

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
 * @brief 装備一覧を表示するコマンドのメインルーチン / Display equipment
 * @return なし
 */
void do_cmd_equip(player_type *creature_ptr)
{
    char out_val[160];
    command_wrk = TRUE;
    if (easy_floor)
        command_wrk = USE_EQUIP;

    screen_save();
    (void)show_equipment(creature_ptr, 0, USE_FULL, 0);
#ifdef JP
    sprintf(out_val, "装備： 合計 %3d.%1d kg (限界の%ld%%) コマンド: ", (int)lbtokg1(creature_ptr->total_weight), (int)lbtokg2(creature_ptr->total_weight),
        (long int)((creature_ptr->total_weight * 100) / weight_limit(creature_ptr)));
#else
    sprintf(out_val, "Equipment: carrying %d.%d pounds (%ld%% of capacity). Command: ", (int)(creature_ptr->total_weight / 10),
        (int)(creature_ptr->total_weight % 10), (long int)((creature_ptr->total_weight * 100) / weight_limit(creature_ptr)));
#endif

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

bool select_ring_slot = FALSE;

/*!
 * @brief 装備するコマンドのメインルーチン / Wield or wear a single item from the pack or floor
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void do_cmd_wield(player_type *creature_ptr)
{
    OBJECT_IDX item, slot;
    object_type forge;
    object_type *q_ptr;
    object_type *o_ptr;
    concptr act;
    GAME_TEXT o_name[MAX_NLEN];
    OBJECT_IDX need_switch_wielding = 0;
    if (creature_ptr->special_defense & KATA_MUSOU)
        set_action(creature_ptr, ACTION_NONE);

    item_tester_hook = item_tester_hook_wear;
    concptr q = _("どれを装備しますか? ", "Wear/Wield which item? ");
    concptr s = _("装備可能なアイテムがない。", "You have nothing you can wear or wield.");
    o_ptr = choose_object(creature_ptr, &item, q, s, (USE_INVEN | USE_FLOOR), 0);
    if (!o_ptr)
        return;

    slot = wield_slot(creature_ptr, o_ptr);

    switch (o_ptr->tval) {
    case TV_CAPTURE:
    case TV_SHIELD:
    case TV_CARD:
        if (has_melee_weapon(creature_ptr, INVEN_RARM) && has_melee_weapon(creature_ptr, INVEN_LARM)) {
            item_tester_hook = item_tester_hook_melee_weapon;
            q = _("どちらの武器と取り替えますか?", "Replace which weapon? ");
            s = _("おっと。", "Oops.");
            if (!choose_object(creature_ptr, &slot, q, s, (USE_EQUIP | IGNORE_BOTHHAND_SLOT), 0))
                return;

            if (slot == INVEN_RARM)
                need_switch_wielding = INVEN_LARM;
        } else if (has_melee_weapon(creature_ptr, INVEN_LARM))
            slot = INVEN_RARM;
        else if (creature_ptr->inventory_list[INVEN_RARM].k_idx && !object_is_melee_weapon(&creature_ptr->inventory_list[INVEN_RARM])
            && creature_ptr->inventory_list[INVEN_LARM].k_idx && !object_is_melee_weapon(&creature_ptr->inventory_list[INVEN_LARM])) {
            item_tester_hook = item_tester_hook_mochikae;
            q = _("どちらの手に装備しますか?", "Equip which hand? ");
            s = _("おっと。", "Oops.");
            if (!choose_object(creature_ptr, &slot, q, s, (USE_EQUIP), 0))
                return;
        }

        break;
    case TV_DIGGING:
    case TV_HAFTED:
    case TV_POLEARM:
    case TV_SWORD:
        if (slot == INVEN_LARM) {
            if (!get_check(_("二刀流で戦いますか？", "Dual wielding? ")))
                slot = INVEN_RARM;
        } else if (!creature_ptr->inventory_list[INVEN_RARM].k_idx && has_melee_weapon(creature_ptr, INVEN_LARM)) {
            if (!get_check(_("二刀流で戦いますか？", "Dual wielding? ")))
                slot = INVEN_LARM;
        } else if (creature_ptr->inventory_list[INVEN_LARM].k_idx && creature_ptr->inventory_list[INVEN_RARM].k_idx) {
            item_tester_hook = item_tester_hook_mochikae;
            q = _("どちらの手に装備しますか?", "Equip which hand? ");
            s = _("おっと。", "Oops.");
            if (!choose_object(creature_ptr, &slot, q, s, (USE_EQUIP), 0))
                return;

            if ((slot == INVEN_LARM) && !has_melee_weapon(creature_ptr, INVEN_RARM))
                need_switch_wielding = INVEN_RARM;
        }

        break;
    case TV_RING:
        if (creature_ptr->inventory_list[INVEN_LEFT].k_idx && creature_ptr->inventory_list[INVEN_RIGHT].k_idx)
            q = _("どちらの指輪と取り替えますか?", "Replace which ring? ");
        else
            q = _("どちらの手に装備しますか?", "Equip which hand? ");

        s = _("おっと。", "Oops.");
        select_ring_slot = TRUE;
        if (!choose_object(creature_ptr, &slot, q, s, (USE_EQUIP | IGNORE_BOTHHAND_SLOT), 0)) {
            select_ring_slot = FALSE;
            return;
        }

        select_ring_slot = FALSE;
        break;
    }

    if (object_is_cursed(&creature_ptr->inventory_list[slot])) {
        describe_flavor(creature_ptr, o_name, &creature_ptr->inventory_list[slot], (OD_OMIT_PREFIX | OD_NAME_ONLY));
#ifdef JP
        msg_format("%s%sは呪われているようだ。", describe_use(creature_ptr, slot), o_name);
#else
        msg_format("The %s you are %s appears to be cursed.", o_name, describe_use(creature_ptr, slot));
#endif
        return;
    }

    if (confirm_wear
        && ((object_is_cursed(o_ptr) && object_is_known(o_ptr))
            || ((o_ptr->ident & IDENT_SENSE) && (FEEL_BROKEN <= o_ptr->feeling) && (o_ptr->feeling <= FEEL_CURSED)))) {
        char dummy[MAX_NLEN + 80];
        describe_flavor(creature_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
        sprintf(dummy, _("本当に%s{呪われている}を使いますか？", "Really use the %s {cursed}? "), o_name);

        if (!get_check(dummy))
            return;
    }

    if ((o_ptr->name1 == ART_STONEMASK) && object_is_known(o_ptr) && (creature_ptr->prace != RACE_VAMPIRE) && (creature_ptr->prace != RACE_ANDROID)) {
        char dummy[MAX_NLEN + 100];
        describe_flavor(creature_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
        sprintf(dummy,
            _("%sを装備すると吸血鬼になります。よろしいですか？", "%s will transforms you into a vampire permanently when equiped. Do you become a vampire?"),
            o_name);

        if (!get_check(dummy))
            return;
    }

    if (need_switch_wielding && !object_is_cursed(&creature_ptr->inventory_list[need_switch_wielding])) {
        object_type *slot_o_ptr = &creature_ptr->inventory_list[slot];
        object_type *switch_o_ptr = &creature_ptr->inventory_list[need_switch_wielding];
        object_type object_tmp;
        object_type *otmp_ptr = &object_tmp;
        GAME_TEXT switch_name[MAX_NLEN];
        describe_flavor(creature_ptr, switch_name, switch_o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
        object_copy(otmp_ptr, switch_o_ptr);
        object_copy(switch_o_ptr, slot_o_ptr);
        object_copy(slot_o_ptr, otmp_ptr);
        msg_format(_("%sを%sに構えなおした。", "You wield %s at %s hand."), switch_name,
            (slot == INVEN_RARM) ? (left_hander ? _("左手", "left") : _("右手", "right")) : (left_hander ? _("右手", "right") : _("左手", "left")));
        slot = need_switch_wielding;
    }

    check_find_art_quest_completion(creature_ptr, o_ptr);
    if (creature_ptr->pseikaku == PERSONALITY_MUNCHKIN) {
        identify_item(creature_ptr, o_ptr);
        autopick_alter_item(creature_ptr, item, FALSE);
    }

    take_turn(creature_ptr, 100);
    q_ptr = &forge;
    object_copy(q_ptr, o_ptr);
    q_ptr->number = 1;
    if (item >= 0) {
        inven_item_increase(creature_ptr, item, -1);
        inven_item_optimize(creature_ptr, item);
    } else {
        floor_item_increase(creature_ptr->current_floor_ptr, 0 - item, -1);
        floor_item_optimize(creature_ptr, 0 - item);
    }

    o_ptr = &creature_ptr->inventory_list[slot];
    if (o_ptr->k_idx)
        (void)inven_takeoff(creature_ptr, slot, 255);

    object_copy(o_ptr, q_ptr);
    o_ptr->marked |= OM_TOUCHED;
    creature_ptr->total_weight += q_ptr->weight;
    creature_ptr->equip_cnt++;

#define STR_WIELD_RARM _("%s(%c)を右手に装備した。", "You are wielding %s (%c) in your right hand.")
#define STR_WIELD_LARM _("%s(%c)を左手に装備した。", "You are wielding %s (%c) in your left hand.")
#define STR_WIELD_ARMS _("%s(%c)を両手で構えた。", "You are wielding %s (%c) with both hands.")

    switch (slot) {
    case INVEN_RARM:
        if (object_allow_two_hands_wielding(o_ptr) && (empty_hands(creature_ptr, FALSE) == EMPTY_HAND_LARM) && can_two_hands_wielding(creature_ptr))
            act = STR_WIELD_ARMS;
        else
            act = (left_hander ? STR_WIELD_LARM : STR_WIELD_RARM);

        break;
    case INVEN_LARM:
        if (object_allow_two_hands_wielding(o_ptr) && (empty_hands(creature_ptr, FALSE) == EMPTY_HAND_RARM) && can_two_hands_wielding(creature_ptr))
            act = STR_WIELD_ARMS;
        else
            act = (left_hander ? STR_WIELD_RARM : STR_WIELD_LARM);

        break;
    case INVEN_BOW:
        act = _("%s(%c)を射撃用に装備した。", "You are shooting with %s (%c).");
        break;
    case INVEN_LITE:
        act = _("%s(%c)を光源にした。", "Your light source is %s (%c).");
        break;
    default:
        act = _("%s(%c)を装備した。", "You are wearing %s (%c).");
        break;
    }

    describe_flavor(creature_ptr, o_name, o_ptr, 0);
    msg_format(act, o_name, index_to_label(slot));
    if (object_is_cursed(o_ptr)) {
        msg_print(_("うわ！ すさまじく冷たい！", "Oops! It feels deathly cold!"));
        chg_virtue(creature_ptr, V_HARMONY, -1);
        o_ptr->ident |= (IDENT_SENSE);
    }

    if ((o_ptr->name1 == ART_STONEMASK) && (creature_ptr->prace != RACE_VAMPIRE) && (creature_ptr->prace != RACE_ANDROID))
        change_race(creature_ptr, RACE_VAMPIRE, "");

    creature_ptr->update |= PU_BONUS | PU_TORCH | PU_MANA;
    creature_ptr->redraw |= PR_EQUIPPY;
    creature_ptr->window |= PW_INVEN | PW_EQUIP | PW_PLAYER;
    calc_android_exp(creature_ptr);
}

/*!
 * @brief 装備を外すコマンドのメインルーチン / Take off an item
 * @return なし
 */
void do_cmd_takeoff(player_type *creature_ptr)
{
    OBJECT_IDX item;
    object_type *o_ptr;
    if (creature_ptr->special_defense & KATA_MUSOU)
        set_action(creature_ptr, ACTION_NONE);

    concptr q = _("どれを装備からはずしますか? ", "Take off which item? ");
    concptr s = _("はずせる装備がない。", "You are not wearing anything to take off.");
    o_ptr = choose_object(creature_ptr, &item, q, s, (USE_EQUIP | IGNORE_BOTHHAND_SLOT), 0);
    if (!o_ptr)
        return;

    if (object_is_cursed(o_ptr)) {
        if ((o_ptr->curse_flags & TRC_PERMA_CURSE) || (creature_ptr->pclass != CLASS_BERSERKER)) {
            msg_print(_("ふーむ、どうやら呪われているようだ。", "Hmmm, it seems to be cursed."));
            return;
        }

        if (((o_ptr->curse_flags & TRC_HEAVY_CURSE) && one_in_(7)) || one_in_(4)) {
            msg_print(_("呪われた装備を力づくで剥がした！", "You tore off a piece of cursed equipment by sheer strength!"));
            o_ptr->ident |= (IDENT_SENSE);
            o_ptr->curse_flags = 0L;
            o_ptr->feeling = FEEL_NONE;
            creature_ptr->update |= PU_BONUS;
            creature_ptr->window |= PW_EQUIP;
            msg_print(_("呪いを打ち破った。", "You break the curse."));
        } else {
            msg_print(_("装備を外せなかった。", "You couldn't remove the equipment."));
            take_turn(creature_ptr, 50);
            return;
        }
    }

    take_turn(creature_ptr, 50);
    (void)inven_takeoff(creature_ptr, item, 255);
    verify_equip_slot(creature_ptr, item);
    calc_android_exp(creature_ptr);
    creature_ptr->redraw |= (PR_EQUIPPY);
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
 * @brief ターゲットを設定するコマンドのメインルーチン
 * Target command
 * @return なし
 */
void do_cmd_target(player_type *creature_ptr)
{
    if (creature_ptr->wild_mode)
        return;

    if (target_set(creature_ptr, TARGET_KILL))
        msg_print(_("ターゲット決定。", "Target Selected."));
    else
        msg_print(_("ターゲット解除。", "Target Aborted."));
}

/*!
 * @brief 周囲を見渡すコマンドのメインルーチン
 * Look command
 * @return なし
 */
void do_cmd_look(player_type *creature_ptr)
{
    creature_ptr->window |= PW_MONSTER_LIST;
    handle_stuff(creature_ptr);
    if (target_set(creature_ptr, TARGET_LOOK))
        msg_print(_("ターゲット決定。", "Target Selected."));
}

/*!
 * @brief 位置を確認するコマンドのメインルーチン
 * Allow the player to examine other sectors on the map
 * @return なし
 */
void do_cmd_locate(player_type *creature_ptr)
{
    DIRECTION dir;
    POSITION y1, x1;
    GAME_TEXT tmp_val[80];
    GAME_TEXT out_val[160];
    TERM_LEN wid, hgt;
    get_screen_size(&wid, &hgt);
    POSITION y2 = y1 = panel_row_min;
    POSITION x2 = x1 = panel_col_min;
    while (TRUE) {
        if ((y2 == y1) && (x2 == x1))
            strcpy(tmp_val, _("真上", "\0"));
        else
            sprintf(tmp_val, "%s%s", ((y2 < y1) ? _("北", " North") : (y2 > y1) ? _("南", " South") : ""),
                ((x2 < x1) ? _("西", " West") : (x2 > x1) ? _("東", " East") : ""));

        sprintf(out_val, _("マップ位置 [%d(%02d),%d(%02d)] (プレイヤーの%s)  方向?", "Map sector [%d(%02d),%d(%02d)], which is%s your sector.  Direction?"),
            y2 / (hgt / 2), y2 % (hgt / 2), x2 / (wid / 2), x2 % (wid / 2), tmp_val);

        dir = 0;
        while (!dir) {
            char command;
            if (!get_com(out_val, &command, TRUE))
                break;

            dir = get_keymap_dir(command);
            if (!dir)
                bell();
        }

        if (!dir)
            break;

        if (change_panel(creature_ptr, ddy[dir], ddx[dir])) {
            y2 = panel_row_min;
            x2 = panel_col_min;
        }
    }

    verify_panel(creature_ptr);
    creature_ptr->update |= (PU_MONSTERS);
    creature_ptr->redraw |= (PR_MAP);
    creature_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
    handle_stuff(creature_ptr);
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
