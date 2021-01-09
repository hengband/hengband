#include "cmd-item/cmd-equipment.h"
#include "action/weapon-shield.h"
#include "artifact/fixed-art-types.h"
#include "autopick/autopick.h"
#include "core/asking-player.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "dungeon/quest.h" // todo 違和感、何故アイテムを装備するとクエストの成功判定が走るのか？.
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "game-option/birth-options.h"
#include "game-option/input-options.h"
#include "inventory/inventory-describer.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/trc-types.h"
#include "object-hook/hook-armor.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-weapon.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-generator.h"
#include "object/object-info.h"
#include "object/object-mark-types.h"
#include "perception/object-perception.h"
#include "player-info/avatar.h"
#include "player/attack-defense-types.h"
#include "player/special-defense-types.h"
#include "racial/racial-android.h"
#include "spell-kind/spells-perception.h"
#include "status/action-setter.h"
#include "status/shape-changer.h"
#include "system/object-type-definition.h"
#include "term/screen-processor.h"
#include "util/int-char-converter.h"
#include "view/display-inventory.h"
#include "view/display-messages.h"

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
    WEIGHT weight = calc_inventory_weight(creature_ptr);
    WEIGHT weight_lim = calc_weight_limit(creature_ptr);
#ifdef JP
    sprintf(out_val, "装備： 合計 %3d.%1d kg (限界の%ld%%) コマンド: ", (int)lbtokg1(weight), (int)lbtokg2(weight), (long int)((weight * 100) / weight_lim));
#else
    sprintf(out_val, "Equipment: carrying %d.%d pounds (%ld%% of capacity). Command: ", (int)(weight / 10), (int)(weight % 10),
        (long int)((weight * 100) / weight_lim));
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
        creature_ptr->select_ring_slot = TRUE;
        if (!choose_object(creature_ptr, &slot, q, s, (USE_EQUIP | IGNORE_BOTHHAND_SLOT), 0)) {
            creature_ptr->select_ring_slot = FALSE;
            return;
        }

        creature_ptr->select_ring_slot = FALSE;
        break;
    }

    if (object_is_cursed(&creature_ptr->inventory_list[slot])) {
        describe_flavor(creature_ptr, o_name, &creature_ptr->inventory_list[slot], OD_OMIT_PREFIX | OD_NAME_ONLY);
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
    creature_ptr->redraw |= PR_EQUIPPY;
}
