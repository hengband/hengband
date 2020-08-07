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
#include "core/asking-player.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "effect/spells-effect-util.h"
#include "game-option/disturbance-options.h"
#include "game-option/text-display-options.h"
#include "inventory/inventory-slot-types.h"
#include "io/command-repeater.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "main/sound-of-music.h"
#include "melee/melee-postprocess.h"
#include "mind/mind-cavalry.h"
#include "mind/mind-force-trainer.h"
#include "mind/mind-mage.h"
#include "mind/mind-mirror-master.h"
#include "mind/mind-monk.h"
#include "mind/mind-ninja.h"
#include "mind/mind-samurai.h"
#include "mind/mind-warrior-mage.h"
#include "mind/mind-warrior.h"
#include "mind/monk-attack.h"
#include "mind/stances-table.h"
#include "mutation/mutation-flag-types.h"
#include "object/item-tester-hooker.h"
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
#include "racial/racial-util.h"
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
#include "spell/spell-types.h"
#include "spell/spells-object.h"
#include "spell/spells-status.h"
#include "status/action-setter.h"
#include "status/bad-status-setter.h"
#include "status/buff-setter.h"
#include "status/experience.h"
#include "target/target-getter.h"
#include "term/screen-processor.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"

/*!
 * @brief レイシャル・パワーの発動成功率を計算する / Returns the chance to activate a racial power/mutation
 * @param rpi_ptr 発動したいレイシャル・パワー情報の構造体参照ポインタ
 * @return 成功率(%)を返す
 */
PERCENTAGE racial_chance(player_type *creature_ptr, rpi_type *rpi_ptr)
{
    if ((creature_ptr->lev < rpi_ptr->min_level) || creature_ptr->confused)
        return 0;

    PERCENTAGE difficulty = rpi_ptr->fail;
    if (difficulty == 0)
        return 100;

    if (creature_ptr->stun) {
        difficulty += (PERCENTAGE)creature_ptr->stun;
    } else if (creature_ptr->lev > rpi_ptr->min_level) {
        PERCENTAGE lev_adj = (PERCENTAGE)((creature_ptr->lev - rpi_ptr->min_level) / 3);
        if (lev_adj > 10)
            lev_adj = 10;

        difficulty -= lev_adj;
    }

    if (difficulty < 5)
        difficulty = 5;

    difficulty = difficulty / 2;
    const BASE_STATUS stat = creature_ptr->stat_cur[rpi_ptr->stat];
    int sum = 0;
    for (int i = 1; i <= stat; i++) {
        int val = i - difficulty;
        if (val > 0)
            sum += (val <= difficulty) ? val : difficulty;
    }

    if (difficulty == 0)
        return 100;
    else
        return ((sum * 100) / difficulty) / stat;
}

/*!
 * @brief レイシャル・パワーの発動の判定処理
 * @param rpi_ptr 発動したいレイシャル・パワー情報の構造体参照ポインタ
 * @return
 * 発動成功ならば1、発動失敗ならば-1、キャンセルならば0を返す。
 * return value indicates that we have succesfully used the power 1: Succeeded, 0: Cancelled, -1: Failed
 */
int check_racial_level(player_type *creature_ptr, rpi_type *rpi_ptr)
{
    PLAYER_LEVEL min_level = rpi_ptr->min_level;
    int use_stat = rpi_ptr->stat;
    int difficulty = rpi_ptr->fail;
    int use_hp = 0;
    rpi_ptr->racial_cost = rpi_ptr->cost;
    if (creature_ptr->csp < rpi_ptr->racial_cost)
        use_hp = rpi_ptr->racial_cost - creature_ptr->csp;

    if (creature_ptr->lev < min_level) {
        msg_format(_("この能力を使用するにはレベル %d に達していなければなりません。", "You need to attain level %d to use this power."), min_level);
        free_turn(creature_ptr);
        return FALSE;
    }

    if (cmd_limit_confused(creature_ptr)) {
        free_turn(creature_ptr);
        return FALSE;
    } else if (creature_ptr->chp < use_hp) {
        if (!get_check(_("本当に今の衰弱した状態でこの能力を使いますか？", "Really use the power in your weakened state? "))) {
            free_turn(creature_ptr);
            return FALSE;
        }
    }

    if (difficulty) {
        if (creature_ptr->stun) {
            difficulty += creature_ptr->stun;
        } else if (creature_ptr->lev > min_level) {
            int lev_adj = ((creature_ptr->lev - min_level) / 3);
            if (lev_adj > 10)
                lev_adj = 10;
            difficulty -= lev_adj;
        }

        if (difficulty < 5)
            difficulty = 5;
    }

    take_turn(creature_ptr, 100);
    if (randint1(creature_ptr->stat_cur[use_stat]) >= ((difficulty / 2) + randint1(difficulty / 2)))
        return 1;

    if (flush_failure)
        flush();

    msg_print(_("充分に集中できなかった。", "You've failed to concentrate hard enough."));
    return -1;
}

/*!
 * @brief レイシャル・パワー発動処理
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param command 発動するレイシャルのID
 * @return 処理を実際に実行した場合はTRUE、キャンセルした場合FALSEを返す。
 */
bool exe_racial_power(player_type *creature_ptr, s32b command)
{
    PLAYER_LEVEL plev = creature_ptr->lev;
    DIRECTION dir = 0;
    if (command <= -3) {
        switch (creature_ptr->pclass) {
        case CLASS_WARRIOR:
            return sword_dancing(creature_ptr);
        case CLASS_HIGH_MAGE:
            if (creature_ptr->realm1 == REALM_HEX) {
                bool retval = stop_hex_spell(creature_ptr);
                if (retval)
                    creature_ptr->energy_use = 10;

                return retval;
            }

            /* Fall through */
        case CLASS_MAGE:
        case CLASS_SORCERER:
            if (!eat_magic(creature_ptr, creature_ptr->lev * 2))
                return FALSE;

            break;
        case CLASS_PRIEST:
            if (is_good_realm(creature_ptr->realm1)) {
                if (!bless_weapon(creature_ptr))
                    return FALSE;
            } else {
                (void)dispel_monsters(creature_ptr, plev * 4);
                turn_monsters(creature_ptr, plev * 4);
                banish_monsters(creature_ptr, plev * 4);
            }

            break;
        case CLASS_ROGUE:
            if (!hit_and_away(creature_ptr))
                return FALSE;

            break;
        case CLASS_RANGER:
        case CLASS_SNIPER:
            msg_print(_("敵を調査した...", "You examine your foes..."));
            probing(creature_ptr);
            break;
        case CLASS_PALADIN:
            if (!get_aim_dir(creature_ptr, &dir))
                return FALSE;

            fire_beam(creature_ptr, is_good_realm(creature_ptr->realm1) ? GF_HOLY_FIRE : GF_HELL_FIRE, dir, plev * 3);
            break;
        case CLASS_WARRIOR_MAGE:
            if (command == -3) {
                return comvert_hp_to_mp(creature_ptr);
            } else if (command == -4) {
                return comvert_mp_to_hp(creature_ptr);
            }

            break;
        case CLASS_CHAOS_WARRIOR:
            return confusing_light(creature_ptr);
        case CLASS_MONK:
            if (!(empty_hands(creature_ptr, TRUE) & EMPTY_HAND_RARM)) {
                msg_print(_("素手じゃないとできません。", "You need to be barehanded."));
                return FALSE;
            }

            if (creature_ptr->riding) {
                msg_print(_("乗馬中はできません。", "You need to get off a pet."));
                return FALSE;
            }

            if (command == -3) {
                if (!choose_monk_stance(creature_ptr))
                    return FALSE;
                creature_ptr->update |= (PU_BONUS);
            } else if (command == -4) {
                return double_attack(creature_ptr);
            }

            break;
        case CLASS_MINDCRAFTER:
        case CLASS_FORCETRAINER:
            return clear_mind(creature_ptr);
        case CLASS_TOURIST:
            if (command == -3) {
                if (!get_aim_dir(creature_ptr, &dir))
                    return FALSE;

                project_length = 1;
                fire_beam(creature_ptr, GF_PHOTO, dir, 1);
            } else if (command == -4) {
                if (!identify_fully(creature_ptr, FALSE, 0))
                    return FALSE;
            }

            break;
        case CLASS_IMITATOR:
            handle_stuff(creature_ptr);
            if (!do_cmd_mane(creature_ptr, TRUE))
                return FALSE;

            break;
        case CLASS_BEASTMASTER:
            if (command == -3) {
                if (!get_aim_dir(creature_ptr, &dir))
                    return FALSE;

                (void)fire_ball_hide(creature_ptr, GF_CHARM_LIVING, dir, creature_ptr->lev, 0);
            } else if (command == -4)
                project_all_los(creature_ptr, GF_CHARM_LIVING, creature_ptr->lev);

            break;
        case CLASS_ARCHER:
            if (!create_ammo(creature_ptr))
                return FALSE;

            break;
        case CLASS_MAGIC_EATER:
            if (command == -3) {
                if (!import_magic_device(creature_ptr))
                    return FALSE;
            } else if (command == -4) {
                if (cmd_limit_cast(creature_ptr) || !do_cmd_magic_eater(creature_ptr, FALSE, TRUE))
                    return FALSE;
            }

            break;
        case CLASS_BARD:
            if (!SINGING_SONG_EFFECT(creature_ptr) && !INTERUPTING_SONG_EFFECT(creature_ptr))
                return FALSE;

            stop_singing(creature_ptr);
            creature_ptr->energy_use = 10;
            break;
        case CLASS_RED_MAGE:
            if (cmd_limit_cast(creature_ptr))
                return FALSE;

            handle_stuff(creature_ptr);
            do_cmd_cast(creature_ptr);
            handle_stuff(creature_ptr);
            if (!creature_ptr->paralyzed && !cmd_limit_cast(creature_ptr))
                do_cmd_cast(creature_ptr);

            break;
        case CLASS_SAMURAI:
            if (command == -3) {
                concentration(creature_ptr);
            } else if (command == -4) {
                if (!has_melee_weapon(creature_ptr, INVEN_RARM) && !has_melee_weapon(creature_ptr, INVEN_LARM)) {
                    msg_print(_("武器を持たないといけません。", "You need to wield a weapon."));
                    return FALSE;
                }

                if (!choose_kata(creature_ptr))
                    return FALSE;

                creature_ptr->update |= (PU_BONUS);
            }

            break;
        case CLASS_BLUE_MAGE:
            if (creature_ptr->action == ACTION_LEARN)
                set_action(creature_ptr, ACTION_NONE);
            else
                set_action(creature_ptr, ACTION_LEARN);

            free_turn(creature_ptr);
            break;
        case CLASS_CAVALRY:
            return rodeo(creature_ptr);
        case CLASS_BERSERKER:
            if (!recall_player(creature_ptr, randint0(21) + 15))
                return FALSE;

            break;
        case CLASS_SMITH:
            if (creature_ptr->lev > 29) {
                if (!identify_fully(creature_ptr, TRUE, 0))
                    return FALSE;
            } else if (!ident_spell(creature_ptr, TRUE, 0))
                return FALSE;

            break;
        case CLASS_MIRROR_MASTER:
            if (command == -3)
                remove_all_mirrors(creature_ptr, TRUE);
            else if (command == -4)
                return mirror_concentration(creature_ptr);

            break;
        case CLASS_NINJA:
            hayagake(creature_ptr);
            break;
        }

        return TRUE;
    }

    if (creature_ptr->mimic_form) {
        switch (creature_ptr->mimic_form) {
        case MIMIC_DEMON:
        case MIMIC_DEMON_LORD: {
            return demonic_breath(creature_ptr);
        }
        case MIMIC_VAMPIRE:
            vampirism(creature_ptr);
            break;
        }

        return TRUE;
    }

    switch (creature_ptr->prace) {
    case RACE_DWARF:
        msg_print(_("周囲を調べた。", "You examine your surroundings."));
        (void)detect_traps(creature_ptr, DETECT_RAD_DEFAULT, TRUE);
        (void)detect_doors(creature_ptr, DETECT_RAD_DEFAULT);
        (void)detect_stairs(creature_ptr, DETECT_RAD_DEFAULT);
        break;
    case RACE_HOBBIT:
        return create_ration(creature_ptr);
    case RACE_GNOME:
        msg_print(_("パッ！", "Blink!"));
        teleport_player(creature_ptr, 10, TELEPORT_SPONTANEOUS);
        break;
    case RACE_HALF_ORC:
        msg_print(_("勇気を出した。", "You play tough."));
        (void)set_afraid(creature_ptr, 0);
        break;
    case RACE_HALF_TROLL:
        msg_print(_("うがぁぁ！", "RAAAGH!"));
        (void)berserk(creature_ptr, 10 + randint1(plev));
        break;
    case RACE_AMBERITE:
        if (command == -1) {
            msg_print(_("あなたは歩き周り始めた。", "You start walking around. "));
            reserve_alter_reality(creature_ptr);
        } else if (command == -2) {
            msg_print(_("あなたは「パターン」を心に描いてその上を歩いた...", "You picture the Pattern in your mind and walk it..."));
            (void)true_healing(creature_ptr, 0);
            (void)restore_all_status(creature_ptr);
            (void)restore_level(creature_ptr);
        }

        break;
    case RACE_BARBARIAN:
        msg_print(_("うぉぉおお！", "Raaagh!"));
        (void)berserk(creature_ptr, 10 + randint1(plev));
        break;
    case RACE_HALF_OGRE:
        msg_print(_("爆発のルーンを慎重に仕掛けた...", "You carefully set an explosive rune..."));
        explosive_rune(creature_ptr, creature_ptr->y, creature_ptr->x);
        break;
    case RACE_HALF_GIANT:
        if (!get_aim_dir(creature_ptr, &dir))
            return FALSE;

        (void)wall_to_mud(creature_ptr, dir, 20 + randint1(30));
        break;
    case RACE_HALF_TITAN:
        msg_print(_("敵を調査した...", "You examine your foes..."));
        probing(creature_ptr);
        break;
    case RACE_CYCLOPS:
        if (!get_aim_dir(creature_ptr, &dir))
            return FALSE;

        msg_print(_("巨大な岩を投げた。", "You throw a huge boulder."));
        fire_bolt(creature_ptr, GF_MISSILE, dir, (3 * plev) / 2);
        break;
    case RACE_YEEK:
        if (!get_aim_dir(creature_ptr, &dir))
            return FALSE;

        stop_mouth(creature_ptr);
        msg_print(_("身の毛もよだつ叫び声を上げた！", "You make a horrible scream!"));
        (void)fear_monster(creature_ptr, dir, plev);
        break;
    case RACE_KLACKON:
        if (!get_aim_dir(creature_ptr, &dir))
            return FALSE;

        stop_mouth(creature_ptr);
        msg_print(_("酸を吐いた。", "You spit acid."));
        if (plev < 25)
            fire_bolt(creature_ptr, GF_ACID, dir, plev);
        else
            fire_ball(creature_ptr, GF_ACID, dir, plev, 2);

        break;
    case RACE_KOBOLD:
        if (!get_aim_dir(creature_ptr, &dir))
            return FALSE;

        msg_print(_("毒のダーツを投げた。", "You throw a dart of poison."));
        fire_bolt(creature_ptr, GF_POIS, dir, plev);
        break;
    case RACE_NIBELUNG:
        msg_print(_("周囲を調査した。", "You examine your surroundings."));
        (void)detect_traps(creature_ptr, DETECT_RAD_DEFAULT, TRUE);
        (void)detect_doors(creature_ptr, DETECT_RAD_DEFAULT);
        (void)detect_stairs(creature_ptr, DETECT_RAD_DEFAULT);
        break;
    case RACE_DARK_ELF:
        if (!get_aim_dir(creature_ptr, &dir))
            return FALSE;

        msg_print(_("マジック・ミサイルを放った。", "You cast a magic missile."));
        fire_bolt_or_beam(creature_ptr, 10, GF_MISSILE, dir, damroll(3 + ((plev - 1) / 5), 4));
        break;
    case RACE_DRACONIAN:
        return draconian_breath(creature_ptr);
    case RACE_MIND_FLAYER:
        if (!get_aim_dir(creature_ptr, &dir))
            return FALSE;

        msg_print(_("あなたは集中し、目が赤く輝いた...", "You concentrate and your eyes glow red..."));
        fire_bolt(creature_ptr, GF_PSI, dir, plev);
        break;
    case RACE_IMP:
        if (!get_aim_dir(creature_ptr, &dir))
            return FALSE;

        if (plev >= 30) {
            msg_print(_("ファイア・ボールを放った。", "You cast a ball of fire."));
            fire_ball(creature_ptr, GF_FIRE, dir, plev, 2);
        } else {
            msg_print(_("ファイア・ボルトを放った。", "You cast a bolt of fire."));
            fire_bolt(creature_ptr, GF_FIRE, dir, plev);
        }

        break;
    case RACE_GOLEM:
        (void)set_shield(creature_ptr, randint1(20) + 30, FALSE);
        break;
    case RACE_SKELETON:
    case RACE_ZOMBIE:
        msg_print(_("あなたは失ったエネルギーを取り戻そうと試みた。", "You attempt to restore your lost energies."));
        (void)restore_level(creature_ptr);
        break;
    case RACE_VAMPIRE:
        vampirism(creature_ptr);
        break;
    case RACE_SPECTRE:
        if (!get_aim_dir(creature_ptr, &dir))
            return FALSE;

        stop_mouth(creature_ptr);
        msg_print(_("あなたはおどろおどろしい叫び声をあげた！", "You emit an eldritch howl!"));
        (void)fear_monster(creature_ptr, dir, plev);
        break;
    case RACE_SPRITE:
        msg_print(_("あなたは魔法の粉を投げつけた...", "You throw some magic dust..."));
        if (plev < 25)
            sleep_monsters_touch(creature_ptr);
        else
            (void)sleep_monsters(creature_ptr, plev);

        break;
    case RACE_BALROG:
        return demonic_breath(creature_ptr);
    case RACE_KUTAR:
        (void)set_leveling(creature_ptr, randint1(20) + 30, FALSE);
        break;
    case RACE_ANDROID:
        return android_inside_weapon(creature_ptr);
    default:
        msg_print(_("この種族は特殊な能力を持っていません。", "This race has no bonus power."));
        free_turn(creature_ptr);
        break;
    }

    return TRUE;
}
