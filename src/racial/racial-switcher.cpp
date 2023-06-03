/*!
 * @brief レイシャルと突然変異の技能処理 / Racial powers (and mutations)
 * @date 2014/01/08
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * 2014 Deskull rearranged comment for Doxygen. \n
 */

#include "racial/racial-switcher.h"
#include "action/action-limited.h"
#include "action/mutation-execution.h"
#include "cmd-action/cmd-mane.h"
#include "cmd-action/cmd-spell.h"
#include "cmd-item/cmd-magiceat.h"
#include "cmd-item/cmd-zapwand.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "effect/spells-effect-util.h"
#include "game-option/text-display-options.h"
#include "inventory/inventory-slot-types.h"
#include "io/command-repeater.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "main/sound-of-music.h"
#include "melee/melee-postprocess.h"
#include "mind/mind-archer.h"
#include "mind/mind-cavalry.h"
#include "mind/mind-elementalist.h"
#include "mind/mind-force-trainer.h"
#include "mind/mind-hobbit.h"
#include "mind/mind-mage.h"
#include "mind/mind-magic-eater.h"
#include "mind/mind-mirror-master.h"
#include "mind/mind-monk.h"
#include "mind/mind-ninja.h"
#include "mind/mind-priest.h"
#include "mind/mind-samurai.h"
#include "mind/mind-warrior-mage.h"
#include "mind/mind-warrior.h"
#include "mind/monk-attack.h"
#include "mind/stances-table.h"
#include "mutation/mutation-flag-types.h"
#include "object/item-tester-hooker.h"
#include "player-info/class-info.h"
#include "player-info/equipment-info.h"
#include "player-info/race-info.h"
#include "player-info/race-types.h"
#include "player-status/player-energy.h"
#include "player-status/player-hand-types.h"
#include "player/attack-defense-types.h"
#include "player/player-damage.h"
#include "player/player-status.h"
#include "racial/racial-android.h"
#include "racial/racial-balrog.h"
#include "racial/racial-draconian.h"
#include "racial/racial-kutar.h"
#include "racial/racial-vampire.h"
#include "realm/realm-names-table.h"
#include "spell-class/spells-mirror-master.h"
#include "spell-kind/spells-beam.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-grid.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell-realm/spells-hex.h"
#include "spell-realm/spells-song.h"
#include "spell/spells-status.h"
#include "status/action-setter.h"
#include "status/bad-status-setter.h"
#include "status/buff-setter.h"
#include "status/experience.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/target-getter.h"
#include "timed-effect/player-paralysis.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"

bool switch_class_racial_execution(PlayerType *player_ptr, const int32_t command)
{
    DIRECTION dir = 0;
    switch (player_ptr->pclass) {
    case PlayerClassType::WARRIOR:
        return sword_dancing(player_ptr);
    case PlayerClassType::HIGH_MAGE:
        if (player_ptr->realm1 == REALM_HEX) {
            auto retval = SpellHex(player_ptr).stop_spells_with_selection();
            if (retval) {
                PlayerEnergy(player_ptr).set_player_turn_energy(10);
            }

            return retval;
        }

        [[fallthrough]];
    case PlayerClassType::MAGE:
    case PlayerClassType::SORCERER:
        return eat_magic(player_ptr, player_ptr->lev * 2);
    case PlayerClassType::PRIEST:
        if (!is_good_realm(player_ptr->realm1)) {
            (void)dispel_monsters(player_ptr, player_ptr->lev * 4);
            turn_monsters(player_ptr, player_ptr->lev * 4);
            banish_monsters(player_ptr, player_ptr->lev * 4);
            return true;
        }

        return bless_weapon(player_ptr);
    case PlayerClassType::ROGUE:
        return hit_and_away(player_ptr);
    case PlayerClassType::RANGER:
    case PlayerClassType::SNIPER:
        msg_print(_("敵を調査した...", "You examine your foes..."));
        probing(player_ptr);
        return true;
    case PlayerClassType::PALADIN:
        if (!get_aim_dir(player_ptr, &dir)) {
            return false;
        }

        fire_beam(player_ptr, is_good_realm(player_ptr->realm1) ? AttributeType::HOLY_FIRE : AttributeType::HELL_FIRE, dir, player_ptr->lev * 3);
        return true;
    case PlayerClassType::WARRIOR_MAGE:
        if (command == -3) {
            return comvert_hp_to_mp(player_ptr);
        } else if (command == -4) {
            return comvert_mp_to_hp(player_ptr);
        }

        return true;
    case PlayerClassType::CHAOS_WARRIOR:
        return confusing_light(player_ptr);
    case PlayerClassType::MONK:
        if (none_bits(empty_hands(player_ptr, true), EMPTY_HAND_MAIN)) {
            msg_print(_("素手じゃないとできません。", "You need to be barehanded."));
            return false;
        }

        if (player_ptr->riding) {
            msg_print(_("乗馬中はできません。", "You need to get off a pet."));
            return false;
        }

        if (command == -3) {
            if (!choose_monk_stance(player_ptr)) {
                return false;
            }

            RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
            return true;
        }

        if (command == -4) {
            return double_attack(player_ptr);
        }

        return true;
    case PlayerClassType::MINDCRAFTER:
    case PlayerClassType::FORCETRAINER:
        return clear_mind(player_ptr);
    case PlayerClassType::TOURIST:
        if (command == -3) {
            if (!get_aim_dir(player_ptr, &dir)) {
                return false;
            }

            project_length = 1;
            fire_beam(player_ptr, AttributeType::PHOTO, dir, 1);
            return true;
        }

        return (command != -4) || identify_fully(player_ptr, false);
    case PlayerClassType::IMITATOR:
        handle_stuff(player_ptr);
        return do_cmd_mane(player_ptr, true);
    case PlayerClassType::BEASTMASTER:
        if (command == -3) {
            if (!get_aim_dir(player_ptr, &dir)) {
                return false;
            }

            (void)fire_ball_hide(player_ptr, AttributeType::CHARM_LIVING, dir, player_ptr->lev, 0);
            return true;
        }

        if (command == -4) {
            project_all_los(player_ptr, AttributeType::CHARM_LIVING, player_ptr->lev);
        }

        return true;
    case PlayerClassType::ARCHER:
        return create_ammo(player_ptr);
    case PlayerClassType::MAGIC_EATER:
        if (command == -3) {
            return import_magic_device(player_ptr);
        }

        return (command != -4) || (!cmd_limit_cast(player_ptr) && do_cmd_magic_eater(player_ptr, false, true));
    case PlayerClassType::BARD:
        if ((get_singing_song_effect(player_ptr) == 0) && (get_interrupting_song_effect(player_ptr) == 0)) {
            return false;
        }

        stop_singing(player_ptr);
        PlayerEnergy(player_ptr).set_player_turn_energy(10);
        return true;
    case PlayerClassType::RED_MAGE:
        if (cmd_limit_cast(player_ptr)) {
            return false;
        }

        handle_stuff(player_ptr);
        if (!do_cmd_cast(player_ptr)) {
            return false;
        }

        if (!player_ptr->effects()->paralysis()->is_paralyzed() && !cmd_limit_cast(player_ptr)) {
            handle_stuff(player_ptr);
            command_dir = 0;
            (void)do_cmd_cast(player_ptr);
        }
        return true;
    case PlayerClassType::SAMURAI:
        if (command == -3) {
            concentration(player_ptr);
            return true;
        }

        if (command != -4) {
            return true;
        }

        if (!has_melee_weapon(player_ptr, INVEN_MAIN_HAND) && !has_melee_weapon(player_ptr, INVEN_SUB_HAND)) {
            msg_print(_("武器を持たないといけません。", "You need to wield a weapon."));
            return false;
        }

        if (!choose_samurai_stance(player_ptr)) {
            return false;
        }

        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
        return true;
    case PlayerClassType::BLUE_MAGE:
        set_action(player_ptr, player_ptr->action == ACTION_LEARN ? ACTION_NONE : ACTION_LEARN);
        PlayerEnergy(player_ptr).reset_player_turn();
        return true;
    case PlayerClassType::CAVALRY:
        return rodeo(player_ptr);
    case PlayerClassType::BERSERKER:
        return recall_player(player_ptr, randint0(21) + 15);
    case PlayerClassType::SMITH:
        if (player_ptr->lev <= 29) {
            return ident_spell(player_ptr, true);
        }

        return identify_fully(player_ptr, true);
    case PlayerClassType::MIRROR_MASTER: {
        SpellsMirrorMaster smm(player_ptr);
        if (command == -3) {
            smm.remove_all_mirrors(true);
            return true;
        }

        if (command == -4) {
            return smm.mirror_concentration();
        }

        return true;
    }
    case PlayerClassType::NINJA:
        return hayagake(player_ptr);
    case PlayerClassType::ELEMENTALIST:
        if (command == -3) {
            return clear_mind(player_ptr);
        }
        if (command == -4) {
            return switch_element_execution(player_ptr);
        }
        return true;
    default:
        return true;
    }
}

bool switch_mimic_racial_execution(PlayerType *player_ptr)
{
    switch (player_ptr->mimic_form) {
    case MimicKindType::DEMON:
    case MimicKindType::DEMON_LORD: {
        return demonic_breath(player_ptr);
    }
    case MimicKindType::VAMPIRE:
        vampirism(player_ptr);
        return true;
    default:
        return true;
    }
}

bool switch_race_racial_execution(PlayerType *player_ptr, const int32_t command)
{
    DIRECTION dir = 0;
    switch (player_ptr->prace) {
    case PlayerRaceType::DWARF:
        msg_print(_("周囲を調べた。", "You examine your surroundings."));
        (void)detect_traps(player_ptr, DETECT_RAD_DEFAULT, true);
        (void)detect_doors(player_ptr, DETECT_RAD_DEFAULT);
        (void)detect_stairs(player_ptr, DETECT_RAD_DEFAULT);
        return true;
    case PlayerRaceType::HOBBIT:
        return create_ration(player_ptr);
    case PlayerRaceType::GNOME:
        msg_print(_("パッ！", "Blink!"));
        teleport_player(player_ptr, 10, TELEPORT_SPONTANEOUS);
        return true;
    case PlayerRaceType::HALF_ORC:
        msg_print(_("勇気を出した。", "You play tough."));
        (void)BadStatusSetter(player_ptr).set_fear(0);
        return true;
    case PlayerRaceType::HALF_TROLL:
        msg_print(_("うがぁぁ！", "RAAAGH!"));
        (void)berserk(player_ptr, 10 + randint1(player_ptr->lev));
        return true;
    case PlayerRaceType::AMBERITE:
        if (command == -1) {
            msg_print(_("あなたは歩き周り始めた。", "You start walking around. "));
            reserve_alter_reality(player_ptr, randint0(21) + 15);
            return true;
        }

        if (command != -2) {
            return true;
        }

        msg_print(_("あなたは「パターン」を心に描いてその上を歩いた...", "You picture the Pattern in your mind and walk it..."));
        (void)true_healing(player_ptr, 0);
        (void)restore_all_status(player_ptr);
        (void)restore_level(player_ptr);
        return true;
    case PlayerRaceType::BARBARIAN:
        msg_print(_("うぉぉおお！", "Raaagh!"));
        (void)berserk(player_ptr, 10 + randint1(player_ptr->lev));
        return true;
    case PlayerRaceType::HALF_OGRE:
        msg_print(_("爆発のルーンを慎重に仕掛けた...", "You carefully set an explosive rune..."));
        (void)create_rune_explosion(player_ptr, player_ptr->y, player_ptr->x);
        return true;
    case PlayerRaceType::HALF_GIANT:
        if (!get_aim_dir(player_ptr, &dir)) {
            return false;
        }

        (void)wall_to_mud(player_ptr, dir, 20 + randint1(30));
        return true;
    case PlayerRaceType::HALF_TITAN:
        msg_print(_("敵を調査した...", "You examine your foes..."));
        (void)probing(player_ptr);
        return true;
    case PlayerRaceType::CYCLOPS:
        if (!get_aim_dir(player_ptr, &dir)) {
            return false;
        }

        msg_print(_("巨大な岩を投げた。", "You throw a huge boulder."));
        (void)fire_bolt(player_ptr, AttributeType::MISSILE, dir, (3 * player_ptr->lev) / 2);
        return true;
    case PlayerRaceType::YEEK:
        if (!get_aim_dir(player_ptr, &dir)) {
            return false;
        }

        stop_mouth(player_ptr);
        msg_print(_("身の毛もよだつ叫び声を上げた！", "You make a horrible scream!"));
        (void)fear_monster(player_ptr, dir, player_ptr->lev);
        return true;
    case PlayerRaceType::KLACKON:
        if (!get_aim_dir(player_ptr, &dir)) {
            return false;
        }

        stop_mouth(player_ptr);
        msg_print(_("酸を吐いた。", "You spit acid."));
        if (player_ptr->lev < 25) {
            (void)fire_bolt(player_ptr, AttributeType::ACID, dir, player_ptr->lev);
        } else {
            (void)fire_ball(player_ptr, AttributeType::ACID, dir, player_ptr->lev, 2);
        }

        return true;
    case PlayerRaceType::KOBOLD:
        if (!get_aim_dir(player_ptr, &dir)) {
            return false;
        }

        msg_print(_("毒のダーツを投げた。", "You throw a poisoned dart."));
        (void)fire_bolt(player_ptr, AttributeType::POIS, dir, player_ptr->lev);
        return true;
    case PlayerRaceType::NIBELUNG:
        msg_print(_("周囲を調査した。", "You examine your surroundings."));
        (void)detect_traps(player_ptr, DETECT_RAD_DEFAULT, true);
        (void)detect_doors(player_ptr, DETECT_RAD_DEFAULT);
        (void)detect_stairs(player_ptr, DETECT_RAD_DEFAULT);
        return true;
    case PlayerRaceType::DARK_ELF:
        if (!get_aim_dir(player_ptr, &dir)) {
            return false;
        }

        msg_print(_("マジック・ミサイルを放った。", "You cast a magic missile."));
        (void)fire_bolt_or_beam(player_ptr, 10, AttributeType::MISSILE, dir, damroll(3 + ((player_ptr->lev - 1) / 5), 4));
        return true;
    case PlayerRaceType::DRACONIAN:
        return draconian_breath(player_ptr);
    case PlayerRaceType::MIND_FLAYER:
        if (!get_aim_dir(player_ptr, &dir)) {
            return false;
        }

        msg_print(_("あなたは集中し、目が赤く輝いた...", "You concentrate and your eyes glow red..."));
        (void)fire_bolt(player_ptr, AttributeType::PSI, dir, player_ptr->lev);
        return true;
    case PlayerRaceType::IMP:
        if (!get_aim_dir(player_ptr, &dir)) {
            return false;
        }

        if (player_ptr->lev >= 30) {
            msg_print(_("ファイア・ボールを放った。", "You cast a ball of fire."));
            (void)fire_ball(player_ptr, AttributeType::FIRE, dir, player_ptr->lev, 2);
        } else {
            msg_print(_("ファイア・ボルトを放った。", "You cast a bolt of fire."));
            (void)fire_bolt(player_ptr, AttributeType::FIRE, dir, player_ptr->lev);
        }

        return true;
    case PlayerRaceType::GOLEM:
        (void)set_shield(player_ptr, randint1(20) + 30, false);
        return true;
    case PlayerRaceType::SKELETON:
    case PlayerRaceType::ZOMBIE:
        msg_print(_("あなたは失ったエネルギーを取り戻そうと試みた。", "You attempt to restore your lost energies."));
        (void)restore_level(player_ptr);
        return true;
    case PlayerRaceType::VAMPIRE:
        (void)vampirism(player_ptr);
        return true;
    case PlayerRaceType::SPECTRE:
        if (!get_aim_dir(player_ptr, &dir)) {
            return false;
        }

        stop_mouth(player_ptr);
        msg_print(_("あなたはおどろおどろしい叫び声をあげた！", "You emit an eldritch howl!"));
        (void)fear_monster(player_ptr, dir, player_ptr->lev);
        return true;
    case PlayerRaceType::SPRITE:
        msg_print(_("あなたは魔法の粉を投げつけた...", "You throw some magic dust..."));
        if (player_ptr->lev < 25) {
            (void)sleep_monsters_touch(player_ptr);
        } else {
            (void)sleep_monsters(player_ptr, player_ptr->lev);
        }

        return true;
    case PlayerRaceType::BALROG:
        return demonic_breath(player_ptr);
    case PlayerRaceType::KUTAR:
        (void)set_leveling(player_ptr, randint1(20) + 30, false);
        return true;
    case PlayerRaceType::ANDROID:
        return android_inside_weapon(player_ptr);
    default:
        msg_print(_("この種族は特殊な能力を持っていません。", "This race has no bonus power."));
        PlayerEnergy(player_ptr).reset_player_turn();
        return true;
    }
}
