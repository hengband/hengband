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
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
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
#include "player-info/equipment-info.h"
#include "player-status/player-energy.h"
#include "player-status/player-hand-types.h"
#include "player/attack-defense-types.h"
#include "player/player-class.h"
#include "player/player-damage.h"
#include "player/player-race-types.h"
#include "player/player-race.h"
#include "player/player-status.h"
#include "racial/racial-android.h"
#include "racial/racial-balrog.h"
#include "racial/racial-draconian.h"
#include "racial/racial-kutar.h"
#include "racial/racial-vampire.h"
#include "realm/realm-names-table.h"
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
#include "spell/spell-types.h"
#include "spell/spells-status.h"
#include "status/action-setter.h"
#include "status/bad-status-setter.h"
#include "status/buff-setter.h"
#include "status/experience.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"

bool switch_class_racial_execution(player_type *creature_ptr, const int32_t command)
{
    DIRECTION dir = 0;
    switch (creature_ptr->pclass) {
    case CLASS_WARRIOR:
        return sword_dancing(creature_ptr);
    case CLASS_HIGH_MAGE:
        if (creature_ptr->realm1 == REALM_HEX) {
            bool retval = stop_hex_spell(creature_ptr);
            if (retval) {
                PlayerEnergy(creature_ptr).set_player_turn_energy(10);
            }

            return retval;
        }

        /* Fall through */
    case CLASS_MAGE:
    case CLASS_SORCERER:
        return eat_magic(creature_ptr, creature_ptr->lev * 2);
    case CLASS_PRIEST:
        if (!is_good_realm(creature_ptr->realm1)) {
            (void)dispel_monsters(creature_ptr, creature_ptr->lev * 4);
            turn_monsters(creature_ptr, creature_ptr->lev * 4);
            banish_monsters(creature_ptr, creature_ptr->lev * 4);
            return true;
        }

        return bless_weapon(creature_ptr);
    case CLASS_ROGUE:
        return hit_and_away(creature_ptr);
    case CLASS_RANGER:
    case CLASS_SNIPER:
        msg_print(_("敵を調査した...", "You examine your foes..."));
        probing(creature_ptr);
        return true;
    case CLASS_PALADIN:
        if (!get_aim_dir(creature_ptr, &dir))
            return false;

        fire_beam(creature_ptr, is_good_realm(creature_ptr->realm1) ? GF_HOLY_FIRE : GF_HELL_FIRE, dir, creature_ptr->lev * 3);
        return true;
    case CLASS_WARRIOR_MAGE:
        if (command == -3)
            return comvert_hp_to_mp(creature_ptr);
        else if (command == -4)
            return comvert_mp_to_hp(creature_ptr);

        return true;
    case CLASS_CHAOS_WARRIOR:
        return confusing_light(creature_ptr);
    case CLASS_MONK:
        if (none_bits(empty_hands(creature_ptr, true), EMPTY_HAND_MAIN)) {
            msg_print(_("素手じゃないとできません。", "You need to be barehanded."));
            return false;
        }

        if (creature_ptr->riding) {
            msg_print(_("乗馬中はできません。", "You need to get off a pet."));
            return false;
        }

        if (command == -3) {
            if (!choose_monk_stance(creature_ptr))
                return false;

            set_bits(creature_ptr->update, PU_BONUS);
            return true;
        }

        if (command == -4)
            return double_attack(creature_ptr);

        return true;
    case CLASS_MINDCRAFTER:
    case CLASS_FORCETRAINER:
        return clear_mind(creature_ptr);
    case CLASS_TOURIST:
        if (command == -3) {
            if (!get_aim_dir(creature_ptr, &dir))
                return false;

            project_length = 1;
            fire_beam(creature_ptr, GF_PHOTO, dir, 1);
            return true;
        }

        return (command != -4) || identify_fully(creature_ptr, false);
    case CLASS_IMITATOR:
        handle_stuff(creature_ptr);
        return do_cmd_mane(creature_ptr, true);
    case CLASS_BEASTMASTER:
        if (command == -3) {
            if (!get_aim_dir(creature_ptr, &dir))
                return false;

            (void)fire_ball_hide(creature_ptr, GF_CHARM_LIVING, dir, creature_ptr->lev, 0);
            return true;
        }

        if (command == -4)
            project_all_los(creature_ptr, GF_CHARM_LIVING, creature_ptr->lev);

        return true;
    case CLASS_ARCHER:
        return create_ammo(creature_ptr);
    case CLASS_MAGIC_EATER:
        if (command == -3)
            return import_magic_device(creature_ptr);

        return (command != -4) || (!cmd_limit_cast(creature_ptr) && do_cmd_magic_eater(creature_ptr, false, true));
    case CLASS_BARD:
        if ((get_singing_song_effect(creature_ptr) == 0) && (get_interrupting_song_effect(creature_ptr) == 0))
            return false;

        stop_singing(creature_ptr);
        PlayerEnergy(creature_ptr).set_player_turn_energy(10);
        return true;
    case CLASS_RED_MAGE:
        if (cmd_limit_cast(creature_ptr))
            return false;

        handle_stuff(creature_ptr);
        if (!do_cmd_cast(creature_ptr))
            return false;

        if (!creature_ptr->paralyzed && !cmd_limit_cast(creature_ptr)) {
            handle_stuff(creature_ptr);
            command_dir = 0;
            (void)do_cmd_cast(creature_ptr);
        }
        return true;
    case CLASS_SAMURAI:
        if (command == -3) {
            concentration(creature_ptr);
            return true;
        }

        if (command != -4)
            return true;

        if (!has_melee_weapon(creature_ptr, INVEN_MAIN_HAND) && !has_melee_weapon(creature_ptr, INVEN_SUB_HAND)) {
            msg_print(_("武器を持たないといけません。", "You need to wield a weapon."));
            return false;
        }

        if (!choose_kata(creature_ptr))
            return false;

        set_bits(creature_ptr->update, PU_BONUS);
        return true;
    case CLASS_BLUE_MAGE:
        set_action(creature_ptr, creature_ptr->action == ACTION_LEARN ? ACTION_NONE : ACTION_LEARN);
        PlayerEnergy(creature_ptr).reset_player_turn();
        return true;
    case CLASS_CAVALRY:
        return rodeo(creature_ptr);
    case CLASS_BERSERKER:
        return recall_player(creature_ptr, randint0(21) + 15);
    case CLASS_SMITH:
        if (creature_ptr->lev <= 29)
            return ident_spell(creature_ptr, true);

        return identify_fully(creature_ptr, true);
    case CLASS_MIRROR_MASTER:
        if (command == -3) {
            remove_all_mirrors(creature_ptr, true);
            return true;
        }

        if (command == -4)
            return mirror_concentration(creature_ptr);

        return true;
    case CLASS_NINJA:
        return hayagake(creature_ptr);
    case CLASS_ELEMENTALIST:
        if (command == -3)
            return clear_mind(creature_ptr);
        if (command == -4)
            return switch_element_execution(creature_ptr);
        return true;
    default:
        return true;
    }
}

bool switch_mimic_racial_execution(player_type *creature_ptr)
{
    switch (creature_ptr->mimic_form) {
    case MIMIC_DEMON:
    case MIMIC_DEMON_LORD: {
        return demonic_breath(creature_ptr);
    }
    case MIMIC_VAMPIRE:
        vampirism(creature_ptr);
        return true;
    default:
        return true;
    }
}

bool switch_race_racial_execution(player_type *creature_ptr, const int32_t command)
{
    DIRECTION dir = 0;
    switch (creature_ptr->prace) {
    case player_race_type::DWARF:
        msg_print(_("周囲を調べた。", "You examine your surroundings."));
        (void)detect_traps(creature_ptr, DETECT_RAD_DEFAULT, true);
        (void)detect_doors(creature_ptr, DETECT_RAD_DEFAULT);
        (void)detect_stairs(creature_ptr, DETECT_RAD_DEFAULT);
        return true;
    case player_race_type::HOBBIT:
        return create_ration(creature_ptr);
    case player_race_type::GNOME:
        msg_print(_("パッ！", "Blink!"));
        teleport_player(creature_ptr, 10, TELEPORT_SPONTANEOUS);
        return true;
    case player_race_type::HALF_ORC:
        msg_print(_("勇気を出した。", "You play tough."));
        (void)set_afraid(creature_ptr, 0);
        return true;
    case player_race_type::HALF_TROLL:
        msg_print(_("うがぁぁ！", "RAAAGH!"));
        (void)berserk(creature_ptr, 10 + randint1(creature_ptr->lev));
        return true;
    case player_race_type::AMBERITE:
        if (command == -1) {
            msg_print(_("あなたは歩き周り始めた。", "You start walking around. "));
            reserve_alter_reality(creature_ptr, randint0(21) + 15);
            return true;
        }

        if (command != -2)
            return true;

        msg_print(_("あなたは「パターン」を心に描いてその上を歩いた...", "You picture the Pattern in your mind and walk it..."));
        (void)true_healing(creature_ptr, 0);
        (void)restore_all_status(creature_ptr);
        (void)restore_level(creature_ptr);
        return true;
    case player_race_type::BARBARIAN:
        msg_print(_("うぉぉおお！", "Raaagh!"));
        (void)berserk(creature_ptr, 10 + randint1(creature_ptr->lev));
        return true;
    case player_race_type::HALF_OGRE:
        msg_print(_("爆発のルーンを慎重に仕掛けた...", "You carefully set an explosive rune..."));
        (void)create_rune_explosion(creature_ptr, creature_ptr->y, creature_ptr->x);
        return true;
    case player_race_type::HALF_GIANT:
        if (!get_aim_dir(creature_ptr, &dir))
            return false;

        (void)wall_to_mud(creature_ptr, dir, 20 + randint1(30));
        return true;
    case player_race_type::HALF_TITAN:
        msg_print(_("敵を調査した...", "You examine your foes..."));
        (void)probing(creature_ptr);
        return true;
    case player_race_type::CYCLOPS:
        if (!get_aim_dir(creature_ptr, &dir))
            return false;

        msg_print(_("巨大な岩を投げた。", "You throw a huge boulder."));
        (void)fire_bolt(creature_ptr, GF_MISSILE, dir, (3 * creature_ptr->lev) / 2);
        return true;
    case player_race_type::YEEK:
        if (!get_aim_dir(creature_ptr, &dir))
            return false;

        stop_mouth(creature_ptr);
        msg_print(_("身の毛もよだつ叫び声を上げた！", "You make a horrible scream!"));
        (void)fear_monster(creature_ptr, dir, creature_ptr->lev);
        return true;
    case player_race_type::KLACKON:
        if (!get_aim_dir(creature_ptr, &dir))
            return false;

        stop_mouth(creature_ptr);
        msg_print(_("酸を吐いた。", "You spit acid."));
        if (creature_ptr->lev < 25)
            (void)fire_bolt(creature_ptr, GF_ACID, dir, creature_ptr->lev);
        else
            (void)fire_ball(creature_ptr, GF_ACID, dir, creature_ptr->lev, 2);

        return true;
    case player_race_type::KOBOLD:
        if (!get_aim_dir(creature_ptr, &dir))
            return false;

        msg_print(_("毒のダーツを投げた。", "You throw a poisoned dart."));
        (void)fire_bolt(creature_ptr, GF_POIS, dir, creature_ptr->lev);
        return true;
    case player_race_type::NIBELUNG:
        msg_print(_("周囲を調査した。", "You examine your surroundings."));
        (void)detect_traps(creature_ptr, DETECT_RAD_DEFAULT, true);
        (void)detect_doors(creature_ptr, DETECT_RAD_DEFAULT);
        (void)detect_stairs(creature_ptr, DETECT_RAD_DEFAULT);
        return true;
    case player_race_type::DARK_ELF:
        if (!get_aim_dir(creature_ptr, &dir))
            return false;

        msg_print(_("マジック・ミサイルを放った。", "You cast a magic missile."));
        (void)fire_bolt_or_beam(creature_ptr, 10, GF_MISSILE, dir, damroll(3 + ((creature_ptr->lev - 1) / 5), 4));
        return true;
    case player_race_type::DRACONIAN:
        return draconian_breath(creature_ptr);
    case player_race_type::MIND_FLAYER:
        if (!get_aim_dir(creature_ptr, &dir))
            return false;

        msg_print(_("あなたは集中し、目が赤く輝いた...", "You concentrate and your eyes glow red..."));
        (void)fire_bolt(creature_ptr, GF_PSI, dir, creature_ptr->lev);
        return true;
    case player_race_type::IMP:
        if (!get_aim_dir(creature_ptr, &dir))
            return false;

        if (creature_ptr->lev >= 30) {
            msg_print(_("ファイア・ボールを放った。", "You cast a ball of fire."));
            (void)fire_ball(creature_ptr, GF_FIRE, dir, creature_ptr->lev, 2);
        } else {
            msg_print(_("ファイア・ボルトを放った。", "You cast a bolt of fire."));
            (void)fire_bolt(creature_ptr, GF_FIRE, dir, creature_ptr->lev);
        }

        return true;
    case player_race_type::GOLEM:
        (void)set_shield(creature_ptr, randint1(20) + 30, false);
        return true;
    case player_race_type::SKELETON:
    case player_race_type::ZOMBIE:
        msg_print(_("あなたは失ったエネルギーを取り戻そうと試みた。", "You attempt to restore your lost energies."));
        (void)restore_level(creature_ptr);
        return true;
    case player_race_type::VAMPIRE:
        (void)vampirism(creature_ptr);
        return true;
    case player_race_type::SPECTRE:
        if (!get_aim_dir(creature_ptr, &dir))
            return false;

        stop_mouth(creature_ptr);
        msg_print(_("あなたはおどろおどろしい叫び声をあげた！", "You emit an eldritch howl!"));
        (void)fear_monster(creature_ptr, dir, creature_ptr->lev);
        return true;
    case player_race_type::SPRITE:
        msg_print(_("あなたは魔法の粉を投げつけた...", "You throw some magic dust..."));
        if (creature_ptr->lev < 25)
            (void)sleep_monsters_touch(creature_ptr);
        else
            (void)sleep_monsters(creature_ptr, creature_ptr->lev);

        return true;
    case player_race_type::BALROG:
        return demonic_breath(creature_ptr);
    case player_race_type::KUTAR:
        (void)set_leveling(creature_ptr, randint1(20) + 30, false);
        return true;
    case player_race_type::ANDROID:
        return android_inside_weapon(creature_ptr);
    default:
        msg_print(_("この種族は特殊な能力を持っていません。", "This race has no bonus power."));
        PlayerEnergy(creature_ptr).reset_player_turn();
        return true;
    }
}
