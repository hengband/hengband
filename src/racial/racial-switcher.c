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

/* Racial Power Info / レイシャル・パワー情報の構造体定義 */
typedef struct rpi_type {
    GAME_TEXT racial_name[MAX_NLEN];
    PLAYER_LEVEL min_level; //!<体得レベル
    int cost;
    int stat;
    PERCENTAGE fail;
    int number;
    int racial_cost;
} rpi_type;

/*!
 * @brief レイシャル・パワーの発動成功率を計算する / Returns the chance to activate a racial power/mutation
 * @param rpi_ptr 発動したいレイシャル・パワー情報の構造体参照ポインタ
 * @return 成功率(%)を返す
 */
static PERCENTAGE racial_chance(player_type *creature_ptr, rpi_type *rpi_ptr)
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
static int racial_aux(player_type *creature_ptr, rpi_type *rpi_ptr)
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
static bool exe_racial_power(player_type *creature_ptr, s32b command)
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

/*!
 * @brief レイシャル・パワーコマンドのメインルーチン / Allow user to choose a power (racial / mutation) to activate
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void do_cmd_racial_power(player_type *creature_ptr)
{
    int num;
    COMMAND_CODE command_code = 0;
    int ask = TRUE;
    PLAYER_LEVEL lvl = creature_ptr->lev;
    bool flag;
    bool redraw;
    bool cast = FALSE;
    bool is_warrior = ((creature_ptr->pclass == CLASS_WARRIOR || creature_ptr->pclass == CLASS_BERSERKER) ? TRUE : FALSE);
    char choice;
    char out_val[160];
    int menu_line = (use_menu ? 1 : 0);

    if (creature_ptr->wild_mode)
        return;

    rpi_type power_desc[36];
    for (int i = 0; i < 36; i++) {
        strcpy(power_desc[i].racial_name, "");
        power_desc[i].number = 0;
    }

    num = 0;

    if (cmd_limit_confused(creature_ptr)) {
        free_turn(creature_ptr);
        return;
    }

    if (creature_ptr->special_defense & (KATA_MUSOU | KATA_KOUKIJIN))
        set_action(creature_ptr, ACTION_NONE);

    switch (creature_ptr->pclass) {
    case CLASS_WARRIOR: {
        strcpy(power_desc[num].racial_name, _("剣の舞い", "Sword Dancing"));
        power_desc[num].min_level = 40;
        power_desc[num].cost = 75;
        power_desc[num].stat = A_DEX;
        power_desc[num].fail = 35;
        power_desc[num++].number = -3;
        break;
    }
    case CLASS_HIGH_MAGE:
        if (creature_ptr->realm1 == REALM_HEX) {
            strcpy(power_desc[num].racial_name, _("詠唱をやめる", "Stop spell casting"));
            power_desc[num].min_level = 1;
            power_desc[num].cost = 0;
            power_desc[num].stat = A_INT;
            power_desc[num].fail = 0;
            power_desc[num++].number = -3;
            break;
        }
        /* Fall through */
    case CLASS_MAGE:
    case CLASS_SORCERER: {
        strcpy(power_desc[num].racial_name, _("魔力食い", "Eat Magic"));
        power_desc[num].min_level = 25;
        power_desc[num].cost = 1;
        power_desc[num].stat = A_INT;
        power_desc[num].fail = 25;
        power_desc[num++].number = -3;
        break;
    }
    case CLASS_PRIEST: {
        if (is_good_realm(creature_ptr->realm1)) {
            strcpy(power_desc[num].racial_name, _("武器祝福", "Bless Weapon"));
            power_desc[num].min_level = 35;
            power_desc[num].cost = 70;
            power_desc[num].stat = A_WIS;
            power_desc[num].fail = 50;
            power_desc[num++].number = -3;
        } else {
            strcpy(power_desc[num].racial_name, _("召魂", "Evocation"));
            power_desc[num].min_level = 42;
            power_desc[num].cost = 40;
            power_desc[num].stat = A_WIS;
            power_desc[num].fail = 35;
            power_desc[num++].number = -3;
        }
        break;
    }
    case CLASS_ROGUE: {
        strcpy(power_desc[num].racial_name, _("ヒット＆アウェイ", "Hit and Away"));
        power_desc[num].min_level = 8;
        power_desc[num].cost = 12;
        power_desc[num].stat = A_DEX;
        power_desc[num].fail = 14;
        power_desc[num++].number = -3;
        break;
    }
    case CLASS_RANGER:
    case CLASS_SNIPER: {
        strcpy(power_desc[num].racial_name, _("モンスター調査", "Probe Monster"));
        power_desc[num].min_level = 15;
        power_desc[num].cost = 20;
        power_desc[num].stat = A_INT;
        power_desc[num].fail = 12;
        power_desc[num++].number = -3;
        break;
    }
    case CLASS_PALADIN: {
        if (is_good_realm(creature_ptr->realm1)) {
            strcpy(power_desc[num].racial_name, _("ホーリー・ランス", "Holy Lance"));
            power_desc[num].min_level = 30;
            power_desc[num].cost = 30;
            power_desc[num].stat = A_WIS;
            power_desc[num].fail = 30;
            power_desc[num++].number = -3;
        } else {
            strcpy(power_desc[num].racial_name, _("ヘル・ランス", "Hell Lance"));
            power_desc[num].min_level = 30;
            power_desc[num].cost = 30;
            power_desc[num].stat = A_WIS;
            power_desc[num].fail = 30;
            power_desc[num++].number = -3;
        }
        break;
    }
    case CLASS_WARRIOR_MAGE: {
        strcpy(power_desc[num].racial_name, _("変換: ＨＰ→ＭＰ", "Convert HP to SP"));
        power_desc[num].min_level = 25;
        power_desc[num].cost = 0;
        power_desc[num].stat = A_INT;
        power_desc[num].fail = 10;
        power_desc[num++].number = -3;

        strcpy(power_desc[num].racial_name, _("変換: ＭＰ→ＨＰ", "Convert SP to HP"));
        power_desc[num].min_level = 25;
        power_desc[num].cost = 0;
        power_desc[num].stat = A_INT;
        power_desc[num].fail = 10;
        power_desc[num++].number = -4;
        break;
    }
    case CLASS_CHAOS_WARRIOR: {
        strcpy(power_desc[num].racial_name, _("幻惑の光", "Confusing Light"));
        power_desc[num].min_level = 40;
        power_desc[num].cost = 50;
        power_desc[num].stat = A_INT;
        power_desc[num].fail = 25;
        power_desc[num++].number = -3;
        break;
    }
    case CLASS_MONK: {
        strcpy(power_desc[num].racial_name, _("構える", "Assume a Stance"));
        power_desc[num].min_level = 25;
        power_desc[num].cost = 0;
        power_desc[num].stat = A_DEX;
        power_desc[num].fail = 0;
        power_desc[num++].number = -3;

        strcpy(power_desc[num].racial_name, _("百裂拳", "Double Attack"));
        power_desc[num].min_level = 30;
        power_desc[num].cost = 30;
        power_desc[num].stat = A_STR;
        power_desc[num].fail = 20;
        power_desc[num++].number = -4;
        break;
    }
    case CLASS_MINDCRAFTER:
    case CLASS_FORCETRAINER: {
        strcpy(power_desc[num].racial_name, _("明鏡止水", "Clear Mind"));
        power_desc[num].min_level = 15;
        power_desc[num].cost = 0;
        power_desc[num].stat = A_WIS;
        power_desc[num].fail = 10;
        power_desc[num++].number = -3;
        break;
    }
    case CLASS_TOURIST: {
        strcpy(power_desc[num].racial_name, _("写真撮影", "Take a Photograph"));
        power_desc[num].min_level = 1;
        power_desc[num].cost = 0;
        power_desc[num].stat = A_DEX;
        power_desc[num].fail = 0;
        power_desc[num++].number = -3;

        strcpy(power_desc[num].racial_name, _("真・鑑定", "Identify True"));
        power_desc[num].min_level = 25;
        power_desc[num].cost = 20;
        power_desc[num].stat = A_INT;
        power_desc[num].fail = 20;
        power_desc[num++].number = -4;
        break;
    }
    case CLASS_IMITATOR: {
        strcpy(power_desc[num].racial_name, _("倍返し", "Double Revenge"));
        power_desc[num].min_level = 30;
        power_desc[num].cost = 100;
        power_desc[num].stat = A_DEX;
        power_desc[num].fail = 30;
        power_desc[num++].number = -3;
        break;
    }
    case CLASS_BEASTMASTER: {
        strcpy(power_desc[num].racial_name, _("生物支配", "Dominate a Living Thing"));
        power_desc[num].min_level = 1;
        power_desc[num].cost = (creature_ptr->lev + 3) / 4;
        power_desc[num].stat = A_CHR;
        power_desc[num].fail = 10;
        power_desc[num++].number = -3;

        strcpy(power_desc[num].racial_name, _("真・生物支配", "Dominate Living Things"));
        power_desc[num].min_level = 30;
        power_desc[num].cost = (creature_ptr->lev + 20) / 2;
        power_desc[num].stat = A_CHR;
        power_desc[num].fail = 10;
        power_desc[num++].number = -4;
        break;
    }
    case CLASS_ARCHER: {
        strcpy(power_desc[num].racial_name, _("弾/矢の製造", "Create Ammo"));
        power_desc[num].min_level = 1;
        power_desc[num].cost = 0;
        power_desc[num].stat = A_DEX;
        power_desc[num].fail = 0;
        power_desc[num++].number = -3;
        break;
    }
    case CLASS_MAGIC_EATER: {
        strcpy(power_desc[num].racial_name, _("魔力の取り込み", "Absorb Magic"));
        power_desc[num].min_level = 1;
        power_desc[num].cost = 0;
        power_desc[num].stat = A_INT;
        power_desc[num].fail = 0;
        power_desc[num++].number = -3;

        strcpy(power_desc[num].racial_name, _("強力発動", "Powerful Activation"));
        power_desc[num].min_level = 10;
        power_desc[num].cost = 10 + (lvl - 10) / 2;
        power_desc[num].stat = A_INT;
        power_desc[num].fail = 0;
        power_desc[num++].number = -4;
        break;
    }
    case CLASS_BARD: {
        strcpy(power_desc[num].racial_name, _("歌を止める", "Stop Singing"));
        power_desc[num].min_level = 1;
        power_desc[num].cost = 0;
        power_desc[num].stat = A_CHR;
        power_desc[num].fail = 0;
        power_desc[num++].number = -3;
        break;
    }
    case CLASS_RED_MAGE: {
        strcpy(power_desc[num].racial_name, _("連続魔", "Double Magic"));
        power_desc[num].min_level = 48;
        power_desc[num].cost = 20;
        power_desc[num].stat = A_INT;
        power_desc[num].fail = 0;
        power_desc[num++].number = -3;
        break;
    }
    case CLASS_SAMURAI: {
        strcpy(power_desc[num].racial_name, _("気合いため", "Concentration"));
        power_desc[num].min_level = 1;
        power_desc[num].cost = 0;
        power_desc[num].stat = A_WIS;
        power_desc[num].fail = 0;
        power_desc[num++].number = -3;

        strcpy(power_desc[num].racial_name, _("型", "Assume a Stance"));
        power_desc[num].min_level = 25;
        power_desc[num].cost = 0;
        power_desc[num].stat = A_DEX;
        power_desc[num].fail = 0;
        power_desc[num++].number = -4;
        break;
    }
    case CLASS_BLUE_MAGE: {
        strcpy(power_desc[num].racial_name, _("ラーニング", "Learning"));
        power_desc[num].min_level = 1;
        power_desc[num].cost = 0;
        power_desc[num].stat = A_INT;
        power_desc[num].fail = 0;
        power_desc[num++].number = -3;
        break;
    }
    case CLASS_CAVALRY: {
        strcpy(power_desc[num].racial_name, _("荒馬ならし", "Rodeo"));
        power_desc[num].min_level = 10;
        power_desc[num].cost = 0;
        power_desc[num].stat = A_STR;
        power_desc[num].fail = 10;
        power_desc[num++].number = -3;
        break;
    }
    case CLASS_BERSERKER: {
        strcpy(power_desc[num].racial_name, _("帰還", "Recall"));
        power_desc[num].min_level = 10;
        power_desc[num].cost = 10;
        power_desc[num].stat = A_DEX;
        power_desc[num].fail = 20;
        power_desc[num++].number = -3;
        break;
    }
    case CLASS_MIRROR_MASTER: {
        strcpy(power_desc[num].racial_name, _("鏡割り", "Break Mirrors"));
        power_desc[num].min_level = 1;
        power_desc[num].cost = 0;
        power_desc[num].stat = A_INT;
        power_desc[num].fail = 0;
        power_desc[num++].number = -3;

        strcpy(power_desc[num].racial_name, _("静水", "Mirror Concentration"));
        power_desc[num].min_level = 30;
        power_desc[num].cost = 0;
        power_desc[num].stat = A_INT;
        power_desc[num].fail = 20;
        power_desc[num++].number = -4;
        break;
    }
    case CLASS_SMITH: {
        strcpy(power_desc[num].racial_name, _("目利き", "Judgment"));
        power_desc[num].min_level = 5;
        power_desc[num].cost = 15;
        power_desc[num].stat = A_INT;
        power_desc[num].fail = 20;
        power_desc[num++].number = -3;
        break;
    }
    case CLASS_NINJA: {
        strcpy(power_desc[num].racial_name, _("速駆け", "Quick Walk"));
        power_desc[num].min_level = 20;
        power_desc[num].cost = 0;
        power_desc[num].stat = A_DEX;
        power_desc[num].fail = 0;
        power_desc[num++].number = -3;
        break;
    }
    default:
        strcpy(power_desc[0].racial_name, _("(なし)", "(none)"));
    }

    if (creature_ptr->mimic_form) {
        switch (creature_ptr->mimic_form) {
        case MIMIC_DEMON:
        case MIMIC_DEMON_LORD:
            sprintf(power_desc[num].racial_name, _("地獄/火炎のブレス (ダメージ %d)", "Nether or Fire Breath (dam %d)"), lvl * 3);
            power_desc[num].min_level = 15;
            power_desc[num].cost = 10 + lvl / 3;
            power_desc[num].stat = A_CON;
            power_desc[num].fail = 20;
            power_desc[num++].number = -1;
            break;
        case MIMIC_VAMPIRE:
            strcpy(power_desc[num].racial_name, _("吸血", "Vampiric Drain"));
            power_desc[num].min_level = 2;
            power_desc[num].cost = 1 + (lvl / 3);
            power_desc[num].stat = A_CON;
            power_desc[num].fail = 9;
            power_desc[num++].number = -1;
            break;
        }
    } else {
        switch (creature_ptr->prace) {
        case RACE_DWARF:
            strcpy(power_desc[num].racial_name, _("ドアと罠 感知", "Detect Doors+Traps"));
            power_desc[num].min_level = 5;
            power_desc[num].cost = 5;
            power_desc[num].stat = A_WIS;
            power_desc[num].fail = 12;
            power_desc[num++].number = -1;
            break;
        case RACE_NIBELUNG:
            strcpy(power_desc[num].racial_name, _("ドアと罠 感知", "Detect Doors+Traps"));
            power_desc[num].min_level = 10;
            power_desc[num].cost = 5;
            power_desc[num].stat = A_WIS;
            power_desc[num].fail = 10;
            power_desc[num++].number = -1;
            break;
        case RACE_HOBBIT:
            strcpy(power_desc[num].racial_name, _("食糧生成", "Create Food"));
            power_desc[num].min_level = 15;
            power_desc[num].cost = 10;
            power_desc[num].stat = A_INT;
            power_desc[num].fail = 10;
            power_desc[num++].number = -1;
            break;
        case RACE_GNOME:
            sprintf(power_desc[num].racial_name, _("ショート・テレポート", "Blink"));
            power_desc[num].min_level = 5;
            power_desc[num].cost = 5;
            power_desc[num].stat = A_INT;
            power_desc[num].fail = 12;
            power_desc[num++].number = -1;
            break;
        case RACE_HALF_ORC:
            strcpy(power_desc[num].racial_name, _("恐怖除去", "Remove Fear"));
            power_desc[num].min_level = 3;
            power_desc[num].cost = 5;
            power_desc[num].stat = A_WIS;
            power_desc[num].fail = is_warrior ? 5 : 10;
            power_desc[num++].number = -1;
            break;
        case RACE_HALF_TROLL:
            strcpy(power_desc[num].racial_name, _("狂戦士化", "Berserk"));
            power_desc[num].min_level = 10;
            power_desc[num].cost = 12;
            power_desc[num].stat = A_STR;
            power_desc[num].fail = is_warrior ? 6 : 12;
            power_desc[num++].number = -1;
            break;
        case RACE_BARBARIAN:
            strcpy(power_desc[num].racial_name, _("狂戦士化", "Berserk"));
            power_desc[num].min_level = 8;
            power_desc[num].cost = 10;
            power_desc[num].stat = A_STR;
            power_desc[num].fail = is_warrior ? 6 : 12;
            power_desc[num++].number = -1;
            break;
        case RACE_AMBERITE:
            strcpy(power_desc[num].racial_name, _("シャドウ・シフト", "Shadow Shifting"));
            power_desc[num].min_level = 30;
            power_desc[num].cost = 50;
            power_desc[num].stat = A_INT;
            power_desc[num].fail = 50;
            power_desc[num++].number = -1;

            strcpy(power_desc[num].racial_name, _("パターン・ウォーク", "Pattern Mindwalking"));
            power_desc[num].min_level = 40;
            power_desc[num].cost = 75;
            power_desc[num].stat = A_WIS;
            power_desc[num].fail = 50;
            power_desc[num++].number = -2;
            break;
        case RACE_HALF_OGRE:
            strcpy(power_desc[num].racial_name, _("爆発のルーン", "Explosive Rune"));
            power_desc[num].min_level = 25;
            power_desc[num].cost = 35;
            power_desc[num].stat = A_INT;
            power_desc[num].fail = 15;
            power_desc[num++].number = -1;
            break;
        case RACE_HALF_GIANT:
            strcpy(power_desc[num].racial_name, _("岩石溶解", "Stone to Mud"));
            power_desc[num].min_level = 20;
            power_desc[num].cost = 10;
            power_desc[num].stat = A_STR;
            power_desc[num].fail = 12;
            power_desc[num++].number = -1;
            break;
        case RACE_HALF_TITAN:
            strcpy(power_desc[num].racial_name, _("スキャン・モンスター", "Probing"));
            power_desc[num].min_level = 15;
            power_desc[num].cost = 10;
            power_desc[num].stat = A_INT;
            power_desc[num].fail = 12;
            power_desc[num++].number = -1;
            break;
        case RACE_CYCLOPS:
            sprintf(power_desc[num].racial_name, _("岩石投げ（ダメージ %d）", "Throw Boulder (dam %d)"), (3 * lvl) / 2);
            power_desc[num].min_level = 20;
            power_desc[num].cost = 15;
            power_desc[num].stat = A_STR;
            power_desc[num].fail = 12;
            power_desc[num++].number = -1;
            break;
        case RACE_YEEK:
            strcpy(power_desc[num].racial_name, _("モンスター恐慌", "Scare Monster"));
            power_desc[num].min_level = 15;
            power_desc[num].cost = 15;
            power_desc[num].stat = A_WIS;
            power_desc[num].fail = 10;
            power_desc[num++].number = -1;
            break;
        case RACE_SPECTRE:
            strcpy(power_desc[num].racial_name, _("モンスター恐慌", "Scare Monster"));
            power_desc[num].min_level = 4;
            power_desc[num].cost = 6;
            power_desc[num].stat = A_INT;
            power_desc[num].fail = 3;
            power_desc[num++].number = -1;
            break;
        case RACE_KLACKON:
            sprintf(power_desc[num].racial_name, _("酸の唾 (ダメージ %d)", "Spit Acid (dam %d)"), lvl);
            power_desc[num].min_level = 9;
            power_desc[num].cost = 9;
            power_desc[num].stat = A_DEX;
            power_desc[num].fail = 14;
            power_desc[num++].number = -1;
            break;
        case RACE_KOBOLD:
            sprintf(power_desc[num].racial_name, _("毒のダーツ (ダメージ %d)", "Poison Dart (dam %d)"), lvl);
            power_desc[num].min_level = 12;
            power_desc[num].cost = 8;
            power_desc[num].stat = A_DEX;
            power_desc[num].fail = 14;
            power_desc[num++].number = -1;
            break;
        case RACE_DARK_ELF:
            sprintf(power_desc[num].racial_name, _("マジック・ミサイル (ダメージ %dd%d)", "Magic Missile (dm %dd%d)"), 3 + ((lvl - 1) / 5), 4);
            power_desc[num].min_level = 2;
            power_desc[num].cost = 2;
            power_desc[num].stat = A_INT;
            power_desc[num].fail = 9;
            power_desc[num++].number = -1;
            break;
        case RACE_DRACONIAN:
            sprintf(power_desc[num].racial_name, _("ブレス (ダメージ %d)", "Breath Weapon (dam %d)"), lvl * 2);
            power_desc[num].min_level = 1;
            power_desc[num].cost = lvl;
            power_desc[num].stat = A_CON;
            power_desc[num].fail = 12;
            power_desc[num++].number = -1;
            break;
        case RACE_MIND_FLAYER:
            sprintf(power_desc[num].racial_name, _("精神攻撃 (ダメージ %d)", "Mind Blast (dam %d)"), lvl);
            power_desc[num].min_level = 15;
            power_desc[num].cost = 12;
            power_desc[num].stat = A_INT;
            power_desc[num].fail = 14;
            power_desc[num++].number = -1;
            break;
        case RACE_IMP:
            sprintf(power_desc[num].racial_name, _("ファイア・ボルト/ボール (ダメージ %d)", "Fire Bolt/Ball (dam %d)"), lvl);
            power_desc[num].min_level = 9;
            power_desc[num].cost = 15;
            power_desc[num].stat = A_WIS;
            power_desc[num].fail = 15;
            power_desc[num++].number = -1;
            break;
        case RACE_GOLEM:
            strcpy(power_desc[num].racial_name, _("肌石化 (期間 1d20+30)", "Stone Skin (dur 1d20+30)"));
            power_desc[num].min_level = 20;
            power_desc[num].cost = 15;
            power_desc[num].stat = A_CON;
            power_desc[num].fail = 8;
            power_desc[num++].number = -1;
            break;
        case RACE_SKELETON:
        case RACE_ZOMBIE:
            strcpy(power_desc[num].racial_name, _("経験値復活", "Restore Experience"));
            power_desc[num].min_level = 30;
            power_desc[num].cost = 30;
            power_desc[num].stat = A_WIS;
            power_desc[num].fail = 18;
            power_desc[num++].number = -1;
            break;
        case RACE_VAMPIRE:
            strcpy(power_desc[num].racial_name, _("吸血", "Vampiric Drain"));
            power_desc[num].min_level = 2;
            power_desc[num].cost = 1 + (lvl / 3);
            power_desc[num].stat = A_CON;
            power_desc[num].fail = 9;
            power_desc[num++].number = -1;
            break;
        case RACE_SPRITE:
            strcpy(power_desc[num].racial_name, _("眠り粉", "Sleeping Dust"));
            power_desc[num].min_level = 12;
            power_desc[num].cost = 12;
            power_desc[num].stat = A_INT;
            power_desc[num].fail = 15;
            power_desc[num++].number = -1;
            break;
        case RACE_BALROG:
            sprintf(power_desc[num].racial_name, _("地獄/火炎のブレス (ダメージ %d)", "Nether or Fire Breath (dam %d)"), lvl * 3);
            power_desc[num].min_level = 15;
            power_desc[num].cost = 10 + lvl / 3;
            power_desc[num].stat = A_CON;
            power_desc[num].fail = 20;
            power_desc[num++].number = -1;
            break;
        case RACE_KUTAR:
            strcpy(power_desc[num].racial_name, _("横に伸びる", "Expand Horizontally (dur 30+1d20)"));
            power_desc[num].min_level = 20;
            power_desc[num].cost = 15;
            power_desc[num].stat = A_CHR;
            power_desc[num].fail = 8;
            power_desc[num++].number = -1;
            break;
        case RACE_ANDROID:
            if (creature_ptr->lev < 10) {
                strcpy(power_desc[num].racial_name, _("レイガン", "Ray Gun"));
                power_desc[num].min_level = 1;
                power_desc[num].cost = 7;
                power_desc[num].fail = 8;
            } else if (creature_ptr->lev < 25) {
                strcpy(power_desc[num].racial_name, _("ブラスター", "Blaster"));
                power_desc[num].min_level = 10;
                power_desc[num].cost = 13;
                power_desc[num].fail = 10;
            } else if (creature_ptr->lev < 35) {
                strcpy(power_desc[num].racial_name, _("バズーカ", "Bazooka"));
                power_desc[num].min_level = 25;
                power_desc[num].cost = 26;
                power_desc[num].fail = 12;
            } else if (creature_ptr->lev < 45) {
                strcpy(power_desc[num].racial_name, _("ビームキャノン", "Beam Cannon"));
                power_desc[num].min_level = 35;
                power_desc[num].cost = 40;
                power_desc[num].fail = 15;
            } else {
                strcpy(power_desc[num].racial_name, _("ロケット", "Rocket"));
                power_desc[num].min_level = 45;
                power_desc[num].cost = 60;
                power_desc[num].fail = 18;
            }
            power_desc[num].stat = A_STR;
            power_desc[num++].number = -1;
            break;
        default: {
            break;
        }
        }
    }

    if (creature_ptr->muta1) {
        if (creature_ptr->muta1 & MUT1_SPIT_ACID) {
            strcpy(power_desc[num].racial_name, _("酸の唾", "Spit Acid"));
            power_desc[num].min_level = 9;
            power_desc[num].cost = 9;
            power_desc[num].stat = A_DEX;
            power_desc[num].fail = 15;
            power_desc[num++].number = MUT1_SPIT_ACID;
        }

        if (creature_ptr->muta1 & MUT1_BR_FIRE) {
            strcpy(power_desc[num].racial_name, _("炎のブレス", "Fire Breath"));
            power_desc[num].min_level = 20;
            power_desc[num].cost = lvl;
            power_desc[num].stat = A_CON;
            power_desc[num].fail = 18;
            power_desc[num++].number = MUT1_BR_FIRE;
        }

        if (creature_ptr->muta1 & MUT1_HYPN_GAZE) {
            strcpy(power_desc[num].racial_name, _("催眠睨み", "Hypnotic Gaze"));
            power_desc[num].min_level = 12;
            power_desc[num].cost = 12;
            power_desc[num].stat = A_CHR;
            power_desc[num].fail = 18;
            power_desc[num++].number = MUT1_HYPN_GAZE;
        }

        if (creature_ptr->muta1 & MUT1_TELEKINES) {
            strcpy(power_desc[num].racial_name, _("念動力", "Telekinesis"));
            power_desc[num].min_level = 9;
            power_desc[num].cost = 9;
            power_desc[num].stat = A_WIS;
            power_desc[num].fail = 14;
            power_desc[num++].number = MUT1_TELEKINES;
        }

        if (creature_ptr->muta1 & MUT1_VTELEPORT) {
            strcpy(power_desc[num].racial_name, _("テレポート", "Teleport"));
            power_desc[num].min_level = 7;
            power_desc[num].cost = 7;
            power_desc[num].stat = A_WIS;
            power_desc[num].fail = 15;
            power_desc[num++].number = MUT1_VTELEPORT;
        }

        if (creature_ptr->muta1 & MUT1_MIND_BLST) {
            strcpy(power_desc[num].racial_name, _("精神攻撃", "Mind Blast"));
            power_desc[num].min_level = 5;
            power_desc[num].cost = 3;
            power_desc[num].stat = A_WIS;
            power_desc[num].fail = 15;
            power_desc[num++].number = MUT1_MIND_BLST;
        }

        if (creature_ptr->muta1 & MUT1_RADIATION) {
            strcpy(power_desc[num].racial_name, _("放射能", "Emit Radiation"));
            power_desc[num].min_level = 15;
            power_desc[num].cost = 15;
            power_desc[num].stat = A_CON;
            power_desc[num].fail = 14;
            power_desc[num++].number = MUT1_RADIATION;
        }

        if (creature_ptr->muta1 & MUT1_VAMPIRISM) {
            strcpy(power_desc[num].racial_name, _("吸血", "Vampiric Drain"));
            power_desc[num].min_level = 2;
            power_desc[num].cost = (1 + (lvl / 3));
            power_desc[num].stat = A_CON;
            power_desc[num].fail = 9;
            power_desc[num++].number = MUT1_VAMPIRISM;
        }

        if (creature_ptr->muta1 & MUT1_SMELL_MET) {
            strcpy(power_desc[num].racial_name, _("金属嗅覚", "Smell Metal"));
            power_desc[num].min_level = 3;
            power_desc[num].cost = 2;
            power_desc[num].stat = A_INT;
            power_desc[num].fail = 12;
            power_desc[num++].number = MUT1_SMELL_MET;
        }

        if (creature_ptr->muta1 & MUT1_SMELL_MON) {
            strcpy(power_desc[num].racial_name, _("敵臭嗅覚", "Smell Monsters"));
            power_desc[num].min_level = 5;
            power_desc[num].cost = 4;
            power_desc[num].stat = A_INT;
            power_desc[num].fail = 15;
            power_desc[num++].number = MUT1_SMELL_MON;
        }

        if (creature_ptr->muta1 & MUT1_BLINK) {
            strcpy(power_desc[num].racial_name, _("ショート・テレポート", "Blink"));
            power_desc[num].min_level = 3;
            power_desc[num].cost = 3;
            power_desc[num].stat = A_WIS;
            power_desc[num].fail = 12;
            power_desc[num++].number = MUT1_BLINK;
        }

        if (creature_ptr->muta1 & MUT1_EAT_ROCK) {
            strcpy(power_desc[num].racial_name, _("岩食い", "Eat Rock"));
            power_desc[num].min_level = 8;
            power_desc[num].cost = 12;
            power_desc[num].stat = A_CON;
            power_desc[num].fail = 18;
            power_desc[num++].number = MUT1_EAT_ROCK;
        }

        if (creature_ptr->muta1 & MUT1_SWAP_POS) {
            strcpy(power_desc[num].racial_name, _("位置交換", "Swap Position"));
            power_desc[num].min_level = 15;
            power_desc[num].cost = 12;
            power_desc[num].stat = A_DEX;
            power_desc[num].fail = 16;
            power_desc[num++].number = MUT1_SWAP_POS;
        }

        if (creature_ptr->muta1 & MUT1_SHRIEK) {
            strcpy(power_desc[num].racial_name, _("叫び", "Shriek"));
            power_desc[num].min_level = 20;
            power_desc[num].cost = 14;
            power_desc[num].stat = A_CON;
            power_desc[num].fail = 16;
            power_desc[num++].number = MUT1_SHRIEK;
        }

        if (creature_ptr->muta1 & MUT1_ILLUMINE) {
            strcpy(power_desc[num].racial_name, _("照明", "Illuminate"));
            power_desc[num].min_level = 3;
            power_desc[num].cost = 2;
            power_desc[num].stat = A_INT;
            power_desc[num].fail = 10;
            power_desc[num++].number = MUT1_ILLUMINE;
        }

        if (creature_ptr->muta1 & MUT1_DET_CURSE) {
            strcpy(power_desc[num].racial_name, _("呪い感知", "Detect Curses"));
            power_desc[num].min_level = 7;
            power_desc[num].cost = 14;
            power_desc[num].stat = A_WIS;
            power_desc[num].fail = 14;
            power_desc[num++].number = MUT1_DET_CURSE;
        }

        if (creature_ptr->muta1 & MUT1_BERSERK) {
            strcpy(power_desc[num].racial_name, _("狂戦士化", "Berserk"));
            power_desc[num].min_level = 8;
            power_desc[num].cost = 8;
            power_desc[num].stat = A_STR;
            power_desc[num].fail = 14;
            power_desc[num++].number = MUT1_BERSERK;
        }

        if (creature_ptr->muta1 & MUT1_POLYMORPH) {
            strcpy(power_desc[num].racial_name, _("変身", "Polymorph"));
            power_desc[num].min_level = 18;
            power_desc[num].cost = 20;
            power_desc[num].stat = A_CON;
            power_desc[num].fail = 18;
            power_desc[num++].number = MUT1_POLYMORPH;
        }

        if (creature_ptr->muta1 & MUT1_MIDAS_TCH) {
            strcpy(power_desc[num].racial_name, _("ミダスの手", "Midas Touch"));
            power_desc[num].min_level = 10;
            power_desc[num].cost = 5;
            power_desc[num].stat = A_INT;
            power_desc[num].fail = 12;
            power_desc[num++].number = MUT1_MIDAS_TCH;
        }

        if (creature_ptr->muta1 & MUT1_GROW_MOLD) {
            strcpy(power_desc[num].racial_name, _("カビ発生", "Grow Mold"));
            power_desc[num].min_level = 1;
            power_desc[num].cost = 6;
            power_desc[num].stat = A_CON;
            power_desc[num].fail = 14;
            power_desc[num++].number = MUT1_GROW_MOLD;
        }

        if (creature_ptr->muta1 & MUT1_RESIST) {
            strcpy(power_desc[num].racial_name, _("エレメント耐性", "Resist Elements"));
            power_desc[num].min_level = 10;
            power_desc[num].cost = 12;
            power_desc[num].stat = A_CON;
            power_desc[num].fail = 12;
            power_desc[num++].number = MUT1_RESIST;
        }

        if (creature_ptr->muta1 & MUT1_EARTHQUAKE) {
            strcpy(power_desc[num].racial_name, _("地震", "Earthquake"));
            power_desc[num].min_level = 12;
            power_desc[num].cost = 12;
            power_desc[num].stat = A_STR;
            power_desc[num].fail = 16;
            power_desc[num++].number = MUT1_EARTHQUAKE;
        }

        if (creature_ptr->muta1 & MUT1_EAT_MAGIC) {
            strcpy(power_desc[num].racial_name, _("魔力食い", "Eat Magic"));
            power_desc[num].min_level = 17;
            power_desc[num].cost = 1;
            power_desc[num].stat = A_WIS;
            power_desc[num].fail = 15;
            power_desc[num++].number = MUT1_EAT_MAGIC;
        }

        if (creature_ptr->muta1 & MUT1_WEIGH_MAG) {
            strcpy(power_desc[num].racial_name, _("魔力感知", "Weigh Magic"));
            power_desc[num].min_level = 6;
            power_desc[num].cost = 6;
            power_desc[num].stat = A_INT;
            power_desc[num].fail = 10;
            power_desc[num++].number = MUT1_WEIGH_MAG;
        }

        if (creature_ptr->muta1 & MUT1_STERILITY) {
            strcpy(power_desc[num].racial_name, _("増殖阻止", "Sterilize"));
            power_desc[num].min_level = 12;
            power_desc[num].cost = 23;
            power_desc[num].stat = A_CHR;
            power_desc[num].fail = 15;
            power_desc[num++].number = MUT1_STERILITY;
        }

        if (creature_ptr->muta1 & MUT1_HIT_AND_AWAY) {
            strcpy(power_desc[num].racial_name, _("ヒット＆アウェイ", "Panic Hit"));
            power_desc[num].min_level = 10;
            power_desc[num].cost = 12;
            power_desc[num].stat = A_DEX;
            power_desc[num].fail = 14;
            power_desc[num++].number = MUT1_HIT_AND_AWAY;
        }

        if (creature_ptr->muta1 & MUT1_DAZZLE) {
            strcpy(power_desc[num].racial_name, _("眩惑", "Dazzle"));
            power_desc[num].min_level = 7;
            power_desc[num].cost = 15;
            power_desc[num].stat = A_CHR;
            power_desc[num].fail = 8;
            power_desc[num++].number = MUT1_DAZZLE;
        }

        if (creature_ptr->muta1 & MUT1_LASER_EYE) {
            strcpy(power_desc[num].racial_name, _("レーザー・アイ", "Laser Eye"));
            power_desc[num].min_level = 7;
            power_desc[num].cost = 10;
            power_desc[num].stat = A_WIS;
            power_desc[num].fail = 9;
            power_desc[num++].number = MUT1_LASER_EYE;
        }

        if (creature_ptr->muta1 & MUT1_RECALL) {
            strcpy(power_desc[num].racial_name, _("帰還", "Recall"));
            power_desc[num].min_level = 17;
            power_desc[num].cost = 50;
            power_desc[num].stat = A_INT;
            power_desc[num].fail = 16;
            power_desc[num++].number = MUT1_RECALL;
        }

        if (creature_ptr->muta1 & MUT1_BANISH) {
            strcpy(power_desc[num].racial_name, _("邪悪消滅", "Banish Evil"));
            power_desc[num].min_level = 25;
            power_desc[num].cost = 25;
            power_desc[num].stat = A_WIS;
            power_desc[num].fail = 18;
            power_desc[num++].number = MUT1_BANISH;
        }

        if (creature_ptr->muta1 & MUT1_COLD_TOUCH) {
            strcpy(power_desc[num].racial_name, _("凍結の手", "Cold Touch"));
            power_desc[num].min_level = 2;
            power_desc[num].cost = 2;
            power_desc[num].stat = A_CON;
            power_desc[num].fail = 11;
            power_desc[num++].number = MUT1_COLD_TOUCH;
        }

        if (creature_ptr->muta1 & MUT1_LAUNCHER) {
            strcpy(power_desc[num].racial_name, _("アイテム投げ", "Throw Object"));
            power_desc[num].min_level = 1;
            power_desc[num].cost = lvl;
            power_desc[num].stat = A_STR;
            power_desc[num].fail = 6;
            power_desc[num++].number = 3;
        }
    }

    flag = FALSE;
    redraw = FALSE;

    (void)strnfmt(out_val, 78, _("(特殊能力 %c-%c, *'で一覧, ESCで中断) どの特殊能力を使いますか？", "(Powers %c-%c, *=List, ESC=exit) Use which power? "),
        I2A(0), (num <= 26) ? I2A(num - 1) : '0' + num - 27);

    if (!repeat_pull(&command_code) || command_code < 0 || command_code >= num) {
        if (use_menu)
            screen_save();

        choice = (always_show_list || use_menu) ? ESCAPE : 1;
        while (!flag) {
            if (choice == ESCAPE)
                choice = ' ';
            else if (!get_com(out_val, &choice, FALSE))
                break;

            if (use_menu && choice != ' ') {
                switch (choice) {
                case '0': {
                    screen_load();
                    free_turn(creature_ptr);
                    return;
                }

                case '8':
                case 'k':
                case 'K': {
                    menu_line += (num - 1);
                    break;
                }

                case '2':
                case 'j':
                case 'J': {
                    menu_line++;
                    break;
                }

                case '6':
                case 'l':
                case 'L':
                case '4':
                case 'h':
                case 'H': {
                    if (menu_line > 18)
                        menu_line -= 18;
                    else if (menu_line + 18 <= num)
                        menu_line += 18;
                    break;
                }

                case 'x':
                case 'X':
                case '\r': {
                    command_code = menu_line - 1;
                    ask = FALSE;
                    break;
                }
                }
                if (menu_line > num)
                    menu_line -= num;
            }

            if ((choice == ' ') || (choice == '*') || (choice == '?') || (use_menu && ask)) {
                if (!redraw || use_menu) {
                    byte y = 1, x = 0;
                    int ctr = 0;
                    char dummy[80];
                    char letter;
                    TERM_LEN x1, y1;
                    strcpy(dummy, "");
                    redraw = TRUE;
                    if (!use_menu)
                        screen_save();

                    if (num < 18)
                        prt(_("                            Lv   MP 失率", "                            Lv Cost Fail"), y++, x);
                    else
                        prt(_("                            Lv   MP 失率                            Lv   MP 失率",
                                "                            Lv Cost Fail                            Lv Cost Fail"),
                            y++, x);

                    while (ctr < num) {
                        x1 = ((ctr < 18) ? x : x + 40);
                        y1 = ((ctr < 18) ? y + ctr : y + ctr - 18);
                        if (use_menu) {
                            if (ctr == (menu_line - 1))
                                strcpy(dummy, _(" 》 ", " >  "));
                            else
                                strcpy(dummy, "    ");
                        } else {
                            if (ctr < 26)
                                letter = I2A(ctr);
                            else
                                letter = '0' + ctr - 26;
                            sprintf(dummy, " %c) ", letter);
                        }

                        strcat(dummy,
                            format("%-23.23s %2d %4d %3d%%", power_desc[ctr].racial_name, power_desc[ctr].min_level, power_desc[ctr].cost,
                                100 - racial_chance(creature_ptr, &power_desc[ctr])));
                        prt(dummy, y1, x1);
                        ctr++;
                    }
                } else {
                    redraw = FALSE;
                    screen_load();
                }

                continue;
            }

            if (!use_menu) {
                if (choice == '\r' && num == 1)
                    choice = 'a';

                if (isalpha(choice)) {
                    ask = (isupper(choice));
                    if (ask)
                        choice = (char)tolower(choice);

                    command_code = (islower(choice) ? A2I(choice) : -1);
                } else {
                    ask = FALSE;
                    command_code = choice - '0' + 26;
                }
            }

            if ((command_code < 0) || (command_code >= num)) {
                bell();
                continue;
            }

            if (ask) {
                char tmp_val[160];
                (void)strnfmt(tmp_val, 78, _("%sを使いますか？ ", "Use %s? "), power_desc[command_code].racial_name);
                if (!get_check(tmp_val))
                    continue;
            }

            flag = TRUE;
        }

        if (redraw)
            screen_load();

        if (!flag) {
            free_turn(creature_ptr);
            return;
        }

        repeat_push(command_code);
    }

    switch (racial_aux(creature_ptr, &power_desc[command_code])) {
    case 1:
        if (power_desc[command_code].number < 0)
            cast = exe_racial_power(creature_ptr, power_desc[command_code].number);
        else
            cast = exe_mutation_power(creature_ptr, power_desc[command_code].number);
        break;
    case 0:
        cast = FALSE;
        break;
    case -1:
        cast = TRUE;
        break;
    }

    if (!cast) {
        free_turn(creature_ptr);
        return;    
    }
        
    int racial_cost = power_desc[command_code].racial_cost;
    if (racial_cost == 0)
        return;

    int actual_racial_cost = racial_cost / 2 + randint1(racial_cost / 2);
    if (creature_ptr->csp < actual_racial_cost) {
        actual_racial_cost -= creature_ptr->csp;
        creature_ptr->csp = 0;
        take_hit(creature_ptr, DAMAGE_USELIFE, actual_racial_cost, _("過度の集中", "concentrating too hard"), -1);
    } else
        creature_ptr->csp -= actual_racial_cost;

    creature_ptr->redraw |= (PR_HP | PR_MANA);
    creature_ptr->window |= (PW_PLAYER | PW_SPELL);
}
