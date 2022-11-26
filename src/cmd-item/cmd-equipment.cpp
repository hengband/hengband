#include "cmd-item/cmd-equipment.h"
#include "action/weapon-shield.h"
#include "artifact/fixed-art-types.h"
#include "autopick/autopick.h"
#include "avatar/avatar.h"
#include "core/asking-player.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "dungeon/quest.h" //!< @todo 違和感、何故アイテムを装備するとクエストの成功判定が走るのか？.
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
#include "locale/japanese.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/trc-types.h"
#include "object-hook/hook-armor.h"
#include "object-hook/hook-weapon.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-flags.h"
#include "object/object-info.h"
#include "object/object-mark-types.h"
#include "perception/object-perception.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/equipment-info.h"
#include "player-info/samurai-data-type.h"
#include "player-status/player-energy.h"
#include "player-status/player-hand-types.h"
#include "player/attack-defense-types.h"
#include "player/player-status.h"
#include "player/special-defense-types.h"
#include "racial/racial-android.h"
#include "spell-kind/spells-perception.h"
#include "status/action-setter.h"
#include "status/shape-changer.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "util/int-char-converter.h"
#include "view/display-inventory.h"
#include "view/display-messages.h"

/*!
 * @brief 装備時にアイテムを呪う処理
 */
static void do_curse_on_equip(OBJECT_IDX slot, ItemEntity *o_ptr, PlayerType *player_ptr)
{
    if (set_anubis_and_chariot(player_ptr) && ((slot == INVEN_MAIN_HAND) || (slot == INVEN_SUB_HAND))) {

        ItemEntity *anubis = &(player_ptr->inventory_list[INVEN_MAIN_HAND]);
        ItemEntity *chariot = &(player_ptr->inventory_list[INVEN_SUB_HAND]);

        anubis->curse_flags.set(CurseTraitType::PERSISTENT_CURSE);
        anubis->curse_flags.set(CurseTraitType::HEAVY_CURSE);
        chariot->curse_flags.set(CurseTraitType::PERSISTENT_CURSE);
        chariot->curse_flags.set(CurseTraitType::HEAVY_CURSE);
        chariot->curse_flags.set(CurseTraitType::BERS_RAGE);
        chariot->curse_flags.set(CurseTraitType::VUL_CURSE);

        msg_format(_("『銀の戦車』プラス『アヌビス神』二刀流ッ！", "*Silver Chariot* plus *Anubis God* Two Swords!"));
        player_ptr->update |= (PU_BONUS);
        return;
    }

    if ((object_flags(o_ptr).has(TR_PERSISTENT_CURSE) || o_ptr->curse_flags.has(CurseTraitType::PERSISTENT_CURSE)) && o_ptr->curse_flags.has_not(CurseTraitType::HEAVY_CURSE)) {

        GAME_TEXT o_name[MAX_NLEN];
        describe_flavor(player_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
        o_ptr->curse_flags.set(CurseTraitType::HEAVY_CURSE);
        msg_format(_("悪意に満ちた黒いオーラが%sをとりまいた...", "There is a malignant black aura surrounding your %s..."), o_name);
        o_ptr->feeling = FEEL_NONE;
        player_ptr->update |= (PU_BONUS);
    }
}

/*!
 * @brief 装備一覧を表示するコマンドのメインルーチン / Display equipment
 */
void do_cmd_equip(PlayerType *player_ptr)
{
    char out_val[160];
    command_wrk = true;
    if (easy_floor) {
        command_wrk = USE_EQUIP;
    }

    screen_save();
    (void)show_equipment(player_ptr, 0, USE_FULL, AllMatchItemTester());
    auto weight = calc_inventory_weight(player_ptr);
    auto weight_lim = calc_weight_limit(player_ptr);
#ifdef JP
    sprintf(out_val, "装備： 合計 %3d.%1d kg (限界の%d%%) コマンド: ", lb_to_kg_integer(weight), lb_to_kg_fraction(weight), weight * 100 / weight_lim);
#else
    sprintf(out_val, "Equipment: carrying %d.%d pounds (%d%% of capacity). Command: ", weight / 10, weight % 10, weight * 100 / weight_lim);
#endif

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
 * @brief 装備するコマンドのメインルーチン / Wield or wear a single item from the pack or floor
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void do_cmd_wield(PlayerType *player_ptr)
{
    OBJECT_IDX item, slot;
    ItemEntity forge;
    ItemEntity *q_ptr;
    ItemEntity *o_ptr;
    concptr act;
    GAME_TEXT o_name[MAX_NLEN];
    OBJECT_IDX need_switch_wielding = 0;
    PlayerClass(player_ptr).break_samurai_stance({ SamuraiStanceType::MUSOU });

    concptr q = _("どれを装備しますか? ", "Wear/Wield which item? ");
    concptr s = _("装備可能なアイテムがない。", "You have nothing you can wear or wield.");
    o_ptr = choose_object(player_ptr, &item, q, s, (USE_INVEN | USE_FLOOR), FuncItemTester(item_tester_hook_wear, player_ptr));
    if (!o_ptr) {
        return;
    }

    slot = wield_slot(player_ptr, o_ptr);

    const auto o_ptr_mh = &player_ptr->inventory_list[INVEN_MAIN_HAND];
    const auto o_ptr_sh = &player_ptr->inventory_list[INVEN_SUB_HAND];
    const auto tval = o_ptr->bi_key.tval();
    switch (tval) {
    case ItemKindType::CAPTURE:
    case ItemKindType::SHIELD:
    case ItemKindType::CARD:
        if (has_melee_weapon(player_ptr, INVEN_MAIN_HAND) && has_melee_weapon(player_ptr, INVEN_SUB_HAND)) {
            q = _("どちらの武器と取り替えますか?", "Replace which weapon? ");
            s = _("おっと。", "Oops.");
            if (!choose_object(player_ptr, &slot, q, s, (USE_EQUIP | IGNORE_BOTHHAND_SLOT), FuncItemTester(&ItemEntity::is_melee_weapon))) {
                return;
            }

            if (slot == INVEN_MAIN_HAND) {
                need_switch_wielding = INVEN_SUB_HAND;
            }
        } else if (has_melee_weapon(player_ptr, INVEN_SUB_HAND)) {
            slot = INVEN_MAIN_HAND;
        } else if (o_ptr_mh->bi_id && o_ptr_sh->bi_id &&
                   ((tval == ItemKindType::CAPTURE) || (!o_ptr_mh->is_melee_weapon() && !o_ptr_sh->is_melee_weapon()))) {
            q = _("どちらの手に装備しますか?", "Equip which hand? ");
            s = _("おっと。", "Oops.");
            if (!choose_object(player_ptr, &slot, q, s, (USE_EQUIP), FuncItemTester(&ItemEntity::is_wieldable_in_etheir_hand))) {
                return;
            }
        }

        break;
    case ItemKindType::DIGGING:
    case ItemKindType::HAFTED:
    case ItemKindType::POLEARM:
    case ItemKindType::SWORD:
        if (slot == INVEN_SUB_HAND) {
            if (!get_check(_("二刀流で戦いますか？", "Dual wielding? "))) {
                slot = INVEN_MAIN_HAND;
            }
        } else if (!o_ptr_mh->bi_id && has_melee_weapon(player_ptr, INVEN_SUB_HAND)) {
            if (!get_check(_("二刀流で戦いますか？", "Dual wielding? "))) {
                slot = INVEN_SUB_HAND;
            }
        } else if (o_ptr_mh->bi_id && o_ptr_sh->bi_id) {
            q = _("どちらの手に装備しますか?", "Equip which hand? ");
            s = _("おっと。", "Oops.");
            if (!choose_object(player_ptr, &slot, q, s, (USE_EQUIP), FuncItemTester(&ItemEntity::is_wieldable_in_etheir_hand))) {
                return;
            }

            if ((slot == INVEN_SUB_HAND) && !has_melee_weapon(player_ptr, INVEN_MAIN_HAND)) {
                need_switch_wielding = INVEN_MAIN_HAND;
            }
        }

        break;
    case ItemKindType::RING:
        if (player_ptr->inventory_list[INVEN_SUB_RING].bi_id && player_ptr->inventory_list[INVEN_MAIN_RING].bi_id) {
            q = _("どちらの指輪と取り替えますか?", "Replace which ring? ");
        } else {
            q = _("どちらの手に装備しますか?", "Equip which hand? ");
        }

        s = _("おっと。", "Oops.");
        player_ptr->select_ring_slot = true;
        if (!choose_object(player_ptr, &slot, q, s, (USE_EQUIP | IGNORE_BOTHHAND_SLOT))) {
            player_ptr->select_ring_slot = false;
            return;
        }

        player_ptr->select_ring_slot = false;
        break;

    default:
        break;
    }

    if (player_ptr->inventory_list[slot].is_cursed()) {
        describe_flavor(player_ptr, o_name, &player_ptr->inventory_list[slot], OD_OMIT_PREFIX | OD_NAME_ONLY);
#ifdef JP
        msg_format("%s%sは呪われているようだ。", describe_use(player_ptr, slot), o_name);
#else
        msg_format("The %s you are %s appears to be cursed.", o_name, describe_use(player_ptr, slot));
#endif
        return;
    }

    if (confirm_wear && ((o_ptr->is_cursed() && o_ptr->is_known()) || ((o_ptr->ident & IDENT_SENSE) && (FEEL_BROKEN <= o_ptr->feeling) && (o_ptr->feeling <= FEEL_CURSED)))) {
        char dummy[MAX_NLEN + 80];
        describe_flavor(player_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
        sprintf(dummy, _("本当に%s{呪われている}を使いますか？", "Really use the %s {cursed}? "), o_name);

        if (!get_check(dummy)) {
            return;
        }
    }

    PlayerRace pr(player_ptr);
    if (o_ptr->is_specific_artifact(FixedArtifactId::STONEMASK) && o_ptr->is_known() && !pr.equals(PlayerRaceType::VAMPIRE) && !pr.equals(PlayerRaceType::ANDROID)) {
        char dummy[MAX_NLEN + 100];
        describe_flavor(player_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
        sprintf(dummy,
            _("%sを装備すると吸血鬼になります。よろしいですか？", "%s will transform you into a vampire permanently when equipped. Do you become a vampire? "),
            o_name);

        if (!get_check(dummy)) {
            return;
        }
    }

    sound(SOUND_WIELD);
    if (need_switch_wielding && !player_ptr->inventory_list[need_switch_wielding].is_cursed()) {
        ItemEntity *slot_o_ptr = &player_ptr->inventory_list[slot];
        ItemEntity *switch_o_ptr = &player_ptr->inventory_list[need_switch_wielding];
        ItemEntity object_tmp;
        ItemEntity *otmp_ptr = &object_tmp;
        GAME_TEXT switch_name[MAX_NLEN];
        describe_flavor(player_ptr, switch_name, switch_o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
        otmp_ptr->copy_from(switch_o_ptr);
        switch_o_ptr->copy_from(slot_o_ptr);
        slot_o_ptr->copy_from(otmp_ptr);
        msg_format(_("%sを%sに構えなおした。", "You wield %s at %s hand."), switch_name,
            (slot == INVEN_MAIN_HAND) ? (left_hander ? _("左手", "left") : _("右手", "right")) : (left_hander ? _("右手", "right") : _("左手", "left")));
        slot = need_switch_wielding;
    }

    check_find_art_quest_completion(player_ptr, o_ptr);
    if (player_ptr->ppersonality == PERSONALITY_MUNCHKIN) {
        identify_item(player_ptr, o_ptr);
        autopick_alter_item(player_ptr, item, false);
    }

    PlayerEnergy(player_ptr).set_player_turn_energy(100);
    q_ptr = &forge;
    q_ptr->copy_from(o_ptr);
    q_ptr->number = 1;
    if (item >= 0) {
        inven_item_increase(player_ptr, item, -1);
        inven_item_optimize(player_ptr, item);
    } else {
        floor_item_increase(player_ptr, 0 - item, -1);
        floor_item_optimize(player_ptr, 0 - item);
    }

    o_ptr = &player_ptr->inventory_list[slot];
    if (o_ptr->bi_id) {
        (void)inven_takeoff(player_ptr, slot, 255);
    }

    o_ptr->copy_from(q_ptr);
    o_ptr->marked.set(OmType::TOUCHED);
    player_ptr->equip_cnt++;

#define STR_WIELD_HAND_RIGHT _("%s(%c)を右手に装備した。", "You are wielding %s (%c) in your right hand.")
#define STR_WIELD_HAND_LEFT _("%s(%c)を左手に装備した。", "You are wielding %s (%c) in your left hand.")
#define STR_WIELD_HANDS_TWO _("%s(%c)を両手で構えた。", "You are wielding %s (%c) with both hands.")

    switch (slot) {
    case INVEN_MAIN_HAND:
        if (o_ptr->allow_two_hands_wielding() && (empty_hands(player_ptr, false) == EMPTY_HAND_SUB) && can_two_hands_wielding(player_ptr)) {
            act = STR_WIELD_HANDS_TWO;
        } else {
            act = (left_hander ? STR_WIELD_HAND_LEFT : STR_WIELD_HAND_RIGHT);
        }

        break;
    case INVEN_SUB_HAND:
        if (o_ptr->allow_two_hands_wielding() && (empty_hands(player_ptr, false) == EMPTY_HAND_MAIN) && can_two_hands_wielding(player_ptr)) {
            act = STR_WIELD_HANDS_TWO;
        } else {
            act = (left_hander ? STR_WIELD_HAND_RIGHT : STR_WIELD_HAND_LEFT);
        }

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

    describe_flavor(player_ptr, o_name, o_ptr, 0);
    msg_format(act, o_name, index_to_label(slot));
    if (o_ptr->is_cursed()) {
        msg_print(_("うわ！ すさまじく冷たい！", "Oops! It feels deathly cold!"));
        chg_virtue(player_ptr, V_HARMONY, -1);
        o_ptr->ident |= (IDENT_SENSE);
    }

    do_curse_on_equip(slot, o_ptr, player_ptr);

    if (o_ptr->is_specific_artifact(FixedArtifactId::STONEMASK) && !pr.equals(PlayerRaceType::VAMPIRE) && !pr.equals(PlayerRaceType::ANDROID)) {
        change_race(player_ptr, PlayerRaceType::VAMPIRE, "");
    }

    calc_android_exp(player_ptr);
    player_ptr->update |= PU_BONUS | PU_TORCH | PU_MANA;
    player_ptr->redraw |= PR_EQUIPPY;
    player_ptr->window_flags |= PW_INVEN | PW_EQUIP | PW_PLAYER;
}

/*!
 * @brief 装備を外すコマンドのメインルーチン / Take off an item
 */
void do_cmd_takeoff(PlayerType *player_ptr)
{
    OBJECT_IDX item;
    ItemEntity *o_ptr;
    PlayerClass pc(player_ptr);
    pc.break_samurai_stance({ SamuraiStanceType::MUSOU });

    concptr q = _("どれを装備からはずしますか? ", "Take off which item? ");
    concptr s = _("はずせる装備がない。", "You are not wearing anything to take off.");
    o_ptr = choose_object(player_ptr, &item, q, s, (USE_EQUIP | IGNORE_BOTHHAND_SLOT));
    if (!o_ptr) {
        return;
    }

    PlayerEnergy energy(player_ptr);
    if (o_ptr->is_cursed()) {
        if (o_ptr->curse_flags.has(CurseTraitType::PERMA_CURSE) || !pc.equals(PlayerClassType::BERSERKER)) {
            msg_print(_("ふーむ、どうやら呪われているようだ。", "Hmmm, it seems to be cursed."));
            return;
        }

        if ((o_ptr->curse_flags.has(CurseTraitType::HEAVY_CURSE) && one_in_(7)) || one_in_(4)) {
            msg_print(_("呪われた装備を力づくで剥がした！", "You tore off a piece of cursed equipment by sheer strength!"));
            o_ptr->ident |= (IDENT_SENSE);
            o_ptr->curse_flags.clear();
            o_ptr->feeling = FEEL_NONE;
            player_ptr->update |= PU_BONUS;
            player_ptr->window_flags |= PW_EQUIP;
            msg_print(_("呪いを打ち破った。", "You break the curse."));
        } else {
            msg_print(_("装備を外せなかった。", "You couldn't remove the equipment."));
            energy.set_player_turn_energy(50);
            return;
        }
    }

    sound(SOUND_TAKE_OFF);
    energy.set_player_turn_energy(50);
    (void)inven_takeoff(player_ptr, item, 255);
    verify_equip_slot(player_ptr, item);
    calc_android_exp(player_ptr);
    player_ptr->update |= PU_BONUS | PU_TORCH | PU_MANA;
    player_ptr->redraw |= PR_EQUIPPY;
    player_ptr->window_flags |= PW_INVEN | PW_EQUIP | PW_PLAYER;
}
