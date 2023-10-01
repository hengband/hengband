#include "cmd-item/cmd-equipment.h"
#include "action/weapon-shield.h"
#include "artifact/fixed-art-types.h"
#include "autopick/autopick.h"
#include "avatar/avatar.h"
#include "core/asking-player.h"
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
#include "system/redrawing-flags-updater.h"
#include "term/screen-processor.h"
#include "term/z-form.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"
#include "view/display-inventory.h"
#include "view/display-messages.h"

/*!
 * @brief 装備時にアイテムを呪う処理
 */
static void do_curse_on_equip(OBJECT_IDX slot, ItemEntity *o_ptr, PlayerType *player_ptr)
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
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
        rfu.set_flag(StatusRecalculatingFlag::BONUS);
        return;
    }

    auto should_curse = object_flags(o_ptr).has(TR_PERSISTENT_CURSE) || o_ptr->curse_flags.has(CurseTraitType::PERSISTENT_CURSE);
    should_curse &= o_ptr->curse_flags.has_not(CurseTraitType::HEAVY_CURSE);
    if (!should_curse) {
        return;
    }

    const auto item_name = describe_flavor(player_ptr, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
    o_ptr->curse_flags.set(CurseTraitType::HEAVY_CURSE);
    msg_format(_("悪意に満ちた黒いオーラが%sをとりまいた...", "There is a malignant black aura surrounding your %s..."), item_name.data());
    o_ptr->feeling = FEEL_NONE;
    rfu.set_flag(StatusRecalculatingFlag::BONUS);
}

/*!
 * @brief 装備一覧を表示するコマンドのメインルーチン / Display equipment
 */
void do_cmd_equip(PlayerType *player_ptr)
{
    command_wrk = true;
    if (easy_floor) {
        command_wrk = USE_EQUIP;
    }

    screen_save();
    (void)show_equipment(player_ptr, 0, USE_FULL, AllMatchItemTester());
    auto weight = calc_inventory_weight(player_ptr);
    auto weight_lim = calc_weight_limit(player_ptr);
    const auto mes = _("装備： 合計 %3d.%1d kg (限界の%d%%) コマンド: ", "Equipment: carrying %d.%d pounds (%d%% of capacity). Command: ");
#ifdef JP
    const auto out_val = format(mes, lb_to_kg_integer(weight), lb_to_kg_fraction(weight), weight * 100 / weight_lim);
#else
    const auto out_val = format(mes, weight / 10, weight % 10, weight * 100 / weight_lim);
#endif

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
    OBJECT_IDX need_switch_wielding = 0;
    PlayerClass(player_ptr).break_samurai_stance({ SamuraiStanceType::MUSOU });

    constexpr auto selection_q = _("どれを装備しますか? ", "Wear/Wield which item? ");
    constexpr auto selection_s = _("装備可能なアイテムがない。", "You have nothing you can wear or wield.");
    o_ptr = choose_object(player_ptr, &item, selection_q, selection_s, (USE_INVEN | USE_FLOOR), FuncItemTester(item_tester_hook_wear, player_ptr));
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
            constexpr auto q = _("どちらの武器と取り替えますか?", "Replace which weapon? ");
            constexpr auto s = _("おっと。", "Oops.");
            if (!choose_object(player_ptr, &slot, q, s, (USE_EQUIP | IGNORE_BOTHHAND_SLOT), FuncItemTester(&ItemEntity::is_melee_weapon))) {
                return;
            }

            if (slot == INVEN_MAIN_HAND) {
                need_switch_wielding = INVEN_SUB_HAND;
            }
        } else if (has_melee_weapon(player_ptr, INVEN_SUB_HAND)) {
            slot = INVEN_MAIN_HAND;
        } else if (o_ptr_mh->is_valid() && o_ptr_sh->is_valid() &&
                   ((tval == ItemKindType::CAPTURE) || (!o_ptr_mh->is_melee_weapon() && !o_ptr_sh->is_melee_weapon()))) {
            constexpr auto q = _("どちらの手に装備しますか?", "Equip which hand? ");
            constexpr auto s = _("おっと。", "Oops.");
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
            if (!input_check(_("二刀流で戦いますか？", "Dual wielding? "))) {
                slot = INVEN_MAIN_HAND;
            }
        } else if (!o_ptr_mh->is_valid() && has_melee_weapon(player_ptr, INVEN_SUB_HAND)) {
            if (!input_check(_("二刀流で戦いますか？", "Dual wielding? "))) {
                slot = INVEN_SUB_HAND;
            }
        } else if (o_ptr_mh->is_valid() && o_ptr_sh->is_valid()) {
            constexpr auto q = _("どちらの手に装備しますか?", "Equip which hand? ");
            constexpr auto s = _("おっと。", "Oops.");
            if (!choose_object(player_ptr, &slot, q, s, (USE_EQUIP), FuncItemTester(&ItemEntity::is_wieldable_in_etheir_hand))) {
                return;
            }

            if ((slot == INVEN_SUB_HAND) && !has_melee_weapon(player_ptr, INVEN_MAIN_HAND)) {
                need_switch_wielding = INVEN_MAIN_HAND;
            }
        }

        break;
    case ItemKindType::RING: {
        std::string q;
        if (player_ptr->inventory_list[INVEN_SUB_RING].is_valid() && player_ptr->inventory_list[INVEN_MAIN_RING].is_valid()) {
            q = _("どちらの指輪と取り替えますか?", "Replace which ring? ");
        } else {
            q = _("どちらの手に装備しますか?", "Equip which hand? ");
        }

        constexpr auto s = _("おっと。", "Oops.");
        player_ptr->select_ring_slot = true;
        if (!choose_object(player_ptr, &slot, q.data(), s, (USE_EQUIP | IGNORE_BOTHHAND_SLOT))) {
            player_ptr->select_ring_slot = false;
            return;
        }

        player_ptr->select_ring_slot = false;
        break;
    }
    default:
        break;
    }

    if (player_ptr->inventory_list[slot].is_cursed()) {
        const auto item_name = describe_flavor(player_ptr, &player_ptr->inventory_list[slot], OD_OMIT_PREFIX | OD_NAME_ONLY);
#ifdef JP
        msg_format("%s%sは呪われているようだ。", describe_use(player_ptr, slot), item_name.data());
#else
        msg_format("The %s you are %s appears to be cursed.", item_name.data(), describe_use(player_ptr, slot));
#endif
        return;
    }

    auto should_equip_cursed = o_ptr->is_cursed() && o_ptr->is_known();
    should_equip_cursed |= any_bits(o_ptr->ident, IDENT_SENSE) && (FEEL_BROKEN <= o_ptr->feeling) && (o_ptr->feeling <= FEEL_CURSED);
    should_equip_cursed &= confirm_wear;
    if (should_equip_cursed) {
        const auto item_name = describe_flavor(player_ptr, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
        if (!input_check(format(_("本当に%s{呪われている}を使いますか？", "Really use the %s {cursed}? "), item_name.data()))) {
            return;
        }
    }

    PlayerRace pr(player_ptr);
    auto should_change_vampire = o_ptr->is_specific_artifact(FixedArtifactId::STONEMASK);
    should_change_vampire &= o_ptr->is_known();
    should_change_vampire &= !pr.equals(PlayerRaceType::VAMPIRE);
    should_change_vampire &= !pr.equals(PlayerRaceType::ANDROID);
    if (should_change_vampire) {
        const auto item_name = describe_flavor(player_ptr, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
        constexpr auto mes = _("%sを装備すると吸血鬼になります。よろしいですか？",
            "%s will transform you into a vampire permanently when equipped. Do you become a vampire? ");
        if (!input_check(format(mes, item_name.data()))) {
            return;
        }
    }

    sound(SOUND_WIELD);
    if (need_switch_wielding && !player_ptr->inventory_list[need_switch_wielding].is_cursed()) {
        ItemEntity *slot_o_ptr = &player_ptr->inventory_list[slot];
        ItemEntity *switch_o_ptr = &player_ptr->inventory_list[need_switch_wielding];
        ItemEntity object_tmp;
        ItemEntity *otmp_ptr = &object_tmp;
        const auto item_name = describe_flavor(player_ptr, switch_o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
        otmp_ptr->copy_from(switch_o_ptr);
        switch_o_ptr->copy_from(slot_o_ptr);
        slot_o_ptr->copy_from(otmp_ptr);
        msg_format(_("%sを%sに構えなおした。", "You wield %s at %s hand."), item_name.data(),
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
    if (o_ptr->is_valid()) {
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

    const auto item_name = describe_flavor(player_ptr, o_ptr, 0);
    msg_format(act, item_name.data(), index_to_label(slot));
    if (o_ptr->is_cursed()) {
        msg_print(_("うわ！ すさまじく冷たい！", "Oops! It feels deathly cold!"));
        chg_virtue(player_ptr, Virtue::HARMONY, -1);
        o_ptr->ident |= (IDENT_SENSE);
    }

    do_curse_on_equip(slot, o_ptr, player_ptr);
    if (o_ptr->is_specific_artifact(FixedArtifactId::STONEMASK)) {
        auto is_specific_race = pr.equals(PlayerRaceType::VAMPIRE);
        is_specific_race |= pr.equals(PlayerRaceType::ANDROID);
        if (!is_specific_race) {
            change_race(player_ptr, PlayerRaceType::VAMPIRE, "");
        }
    }

    calc_android_exp(player_ptr);
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::TORCH,
        StatusRecalculatingFlag::MP,
    };
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flags(flags_srf);
    rfu.set_flag(MainWindowRedrawingFlag::EQUIPPY);
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::INVENTORY,
        SubWindowRedrawingFlag::EQUIPMENT,
        SubWindowRedrawingFlag::PLAYER,
    };
    rfu.set_flags(flags_swrf);
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

    constexpr auto q = _("どれを装備からはずしますか? ", "Take off which item? ");
    constexpr auto s = _("はずせる装備がない。", "You are not wearing anything to take off.");
    o_ptr = choose_object(player_ptr, &item, q, s, (USE_EQUIP | IGNORE_BOTHHAND_SLOT));
    if (!o_ptr) {
        return;
    }

    PlayerEnergy energy(player_ptr);
    auto &rfu = RedrawingFlagsUpdater::get_instance();
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
            rfu.set_flag(StatusRecalculatingFlag::BONUS);
            rfu.set_flag(SubWindowRedrawingFlag::EQUIPMENT);
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
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::TORCH,
        StatusRecalculatingFlag::MP,
    };
    rfu.set_flags(flags_srf);
    rfu.set_flag(MainWindowRedrawingFlag::EQUIPPY);
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::INVENTORY,
        SubWindowRedrawingFlag::EQUIPMENT,
        SubWindowRedrawingFlag::PLAYER,
    };
    rfu.set_flags(flags_swrf);
}
