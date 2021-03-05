﻿/*!
 * @brief ものまねの処理実装 / Imitation code
 * @date 2014/01/14
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * 2014 Deskull rearranged comment for Doxygen.\n
 */

#include "action/action-limited.h"
#include "cmd-action/cmd-spell.h"
#include "core/asking-player.h"
#include "core/hp-mp-processor.h"
#include "core/player-redraw-types.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "floor/cave.h"
#include "floor/floor-object.h"
#include "game-option/disturbance-options.h"
#include "game-option/text-display-options.h"
#include "grid/grid.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/mind-mage.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags1.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-processor.h"
#include "monster/monster-status.h"
#include "mspell/monster-power-table.h"
#include "mspell/mspell-type.h"
#include "player/player-status-table.h"
#include "player/player-status.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell/spell-types.h"
#include "spell/spells-status.h"
#include "spell/spells-summon.h"
#include "spell/summon-types.h"
#include "status/bad-status-setter.h"
#include "status/body-improvement.h"
#include "status/buff-setter.h"
#include "system/floor-type-definition.h"
#include "target/projection-path-calculator.h"
#include "target/target-checker.h"
#include "target/target-getter.h"
#include "target/target-setter.h"
#include "target/target-types.h"
#include "term/screen-processor.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"

static int damage;

/*!
 * @brief 受け取ったパラメータに応じてものまねの効果情報をまとめたフォーマットを返す
 * @param p 情報を返す文字列参照ポインタ
 * @param power ものまねの効力の種類
 * @param dam ものまねの威力
 * @return なし
 */
static void mane_info(player_type *caster_ptr, char *p, int power, HIT_POINT dam)
{
    PLAYER_LEVEL plev = caster_ptr->lev;

    strcpy(p, "");

    if ((power > 2 && power < 41) || (power > 41 && power < 59) || (power == 75))
        sprintf(p, " %s%d", KWD_DAM, (int)dam);
    else {
        switch (power) {
        case 41:
            sprintf(p, " %sd%d+%d", KWD_HEAL, plev * 3, plev);
            break;
        case 64:
            sprintf(p, " %sd%d+%d", KWD_DURATION, 20 + plev, plev);
            break;
        case 66:
            sprintf(p, " %s%d", KWD_HEAL, plev * 6);
            break;
        case 67:
            sprintf(p, " %sd7+7", KWD_DURATION);
            break;
        case 68:
            sprintf(p, " %s10", KWD_SPHERE);
            break;
        case 69:
            sprintf(p, " %s%d", KWD_SPHERE, plev * 5);
            break;
        case 79:
            sprintf(p, " %s5", KWD_SPHERE);
            break;
        default:
            break;
        }
    }
}

/*!
 * @brief どのものまねを発動するか選択する処理 /
 * Allow user to choose a imitation.
 * @param sn 実行したものまねのIDを返す参照ポインタ（キャンセルなどの場合-1を返す）
 * @param baigaesi TRUEならば倍返し上の処理として行う
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
 * @details
 * If a valid spell is chosen, saves it in '*sn' and returns TRUE
 * If the user hits escape, returns FALSE, and set '*sn' to -1
 * If there are no legal choices, returns FALSE, and sets '*sn' to -2
 *
 * The "prompt" should be "cast", "recite", or "study"
 * The "known" should be TRUE for cast/pray, FALSE for study
 *
 * nb: This function has a (trivial) display bug which will be obvious
 * when you run it. It's probably easy to fix but I haven't tried,
 * sorry.
 */
static int get_mane_power(player_type *caster_ptr, int *sn, bool baigaesi)
{
    int i = 0;
    int num = 0;
    TERM_LEN y = 1;
    TERM_LEN x = 18;
    PERCENTAGE minfail = 0;
    PLAYER_LEVEL plev = caster_ptr->lev;
    PERCENTAGE chance = 0;
    int ask;
    char choice;
    char out_val[MAX_MONSTER_NAME];
    char comment[80];
    concptr p = _("能力", "power");

    monster_power spell;
    bool flag, redraw;

    /* Assume cancelled */
    *sn = (-1);

    flag = FALSE;
    redraw = FALSE;

    num = caster_ptr->mane_num;

    /* Build a prompt (accept all spells) */
    (void)strnfmt(out_val, 78, _("(%c-%c, '*'で一覧, ESC) どの%sをまねますか？", "(%c-%c, *=List, ESC=exit) Use which %s? "), I2A(0), I2A(num - 1), p);

    choice = always_show_list ? ESCAPE : 1;
    while (!flag) {
        if (choice == ESCAPE)
            choice = ' ';
        else if (!get_com(out_val, &choice, TRUE))
            break;

        /* Request redraw */
        if ((choice == ' ') || (choice == '*') || (choice == '?')) {
            /* Show the list */
            if (!redraw) {
                char psi_desc[160];
                redraw = TRUE;
                screen_save();

                /* Display a list of spells */
                prt("", y, x);
                put_str(_("名前", "Name"), y, x + 5);
                put_str(_("失率 効果", "Fail Info"), y, x + 36);

                /* Dump the spells */
                for (i = 0; i < num; i++) {
                    /* Access the spell */
                    spell = monster_powers[caster_ptr->mane_spell[i]];

                    chance = spell.manefail;

                    /* Reduce failure rate by "effective" level adjustment */
                    if (plev > spell.level)
                        chance -= 3 * (plev - spell.level);

                    /* Reduce failure rate by INT/WIS adjustment */
                    chance -= 3 * (adj_mag_stat[caster_ptr->stat_ind[spell.use_stat]] + adj_mag_stat[caster_ptr->stat_ind[A_DEX]] - 2) / 2;

                    if (spell.manedam)
                        chance = chance * (baigaesi ? caster_ptr->mane_dam[i] * 2 : caster_ptr->mane_dam[i]) / spell.manedam;

                    chance += caster_ptr->to_m_chance;

                    /* Extract the minimum failure rate */
                    minfail = adj_mag_fail[caster_ptr->stat_ind[spell.use_stat]];

                    /* Minimum failure rate */
                    if (chance < minfail)
                        chance = minfail;

                    /* Stunning makes spells harder */
                    if (caster_ptr->stun > 50)
                        chance += 25;
                    else if (caster_ptr->stun)
                        chance += 15;

                    /* Always a 5 percent chance of working */
                    if (chance > 95)
                        chance = 95;

                    /* Get info */
                    mane_info(caster_ptr, comment, caster_ptr->mane_spell[i], (baigaesi ? caster_ptr->mane_dam[i] * 2 : caster_ptr->mane_dam[i]));

                    /* Dump the spell --(-- */
                    sprintf(psi_desc, "  %c) %-30s %3d%%%s", I2A(i), spell.name, chance, comment);
                    prt(psi_desc, y + i + 1, x);
                }

                /* Clear the bottom line */
                prt("", y + i + 1, x);
            }

            /* Hide the list */
            else {
                /* Hide list */
                redraw = FALSE;
                screen_load();
            }

            /* Redo asking */
            continue;
        }

        /* Note verify */
        ask = isupper(choice);

        /* Lowercase */
        if (ask)
            choice = (char)tolower(choice);

        /* Extract request */
        i = (islower(choice) ? A2I(choice) : -1);

        /* Totally Illegal */
        if ((i < 0) || (i >= num)) {
            bell();
            continue;
        }

        /* Save the spell index */
        spell = monster_powers[caster_ptr->mane_spell[i]];

        /* Verify it */
        if (ask) {
            char tmp_val[160];

            /* Prompt */
            (void)strnfmt(tmp_val, 78, _("%sをまねますか？", "Use %s? "), monster_powers[caster_ptr->mane_spell[i]].name);

            /* Belay that order */
            if (!get_check(tmp_val))
                continue;
        }

        /* Stop the loop */
        flag = TRUE;
    }
    if (redraw)
        screen_load();

    caster_ptr->window_flags |= (PW_SPELL);
    handle_stuff(caster_ptr);

    /* Abort if needed */
    if (!flag)
        return FALSE;

    /* Save the choice */
    (*sn) = i;

    damage = (baigaesi ? caster_ptr->mane_dam[i] * 2 : caster_ptr->mane_dam[i]);

    /* Success */
    return TRUE;
}

/*!
 * @brief ものまね処理の発動 /
 * do_cmd_cast calls this function if the player's class is 'imitator'.
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param spell 発動するモンスター攻撃のID
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
 */
static bool use_mane(player_type *caster_ptr, int spell)
{
    DIRECTION dir;
    PLAYER_LEVEL plev = caster_ptr->lev;
    BIT_FLAGS mode = (PM_ALLOW_GROUP | PM_FORCE_PET);
    BIT_FLAGS u_mode = 0L;

    if (randint1(50 + plev) < plev / 10)
        u_mode = PM_ALLOW_UNIQUE;

    /* spell code */
    switch (spell) {
    case MS_SHRIEK:
        msg_print(_("かん高い金切り声をあげた。", "You make a high pitched shriek."));
        aggravate_monsters(caster_ptr, 0);
        break;

    case MS_XXX1:
        break;

    case MS_DISPEL: {
        MONSTER_IDX m_idx;

        if (!target_set(caster_ptr, TARGET_KILL))
            return FALSE;
        m_idx = caster_ptr->current_floor_ptr->grid_array[target_row][target_col].m_idx;
        if (!m_idx)
            break;
        if (!player_has_los_bold(caster_ptr, target_row, target_col))
            break;
        if (!projectable(caster_ptr, caster_ptr->y, caster_ptr->x, target_row, target_col))
            break;
        dispel_monster_status(caster_ptr, m_idx);
        break;
    }

    case MS_ROCKET:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("ロケットを発射した。", "You fire a rocket."));
        fire_rocket(caster_ptr, GF_ROCKET, dir, damage, 2);
        break;

    case MS_SHOOT:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("矢を放った。", "You fire an arrow."));
        fire_bolt(caster_ptr, GF_ARROW, dir, damage);
        break;

    case MS_XXX2:
        break;

    case MS_XXX3:
        break;

    case MS_XXX4:
        break;

    case MS_BR_ACID:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("酸のブレスを吐いた。", "You breathe acid."));
        fire_breath(caster_ptr, GF_ACID, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MS_BR_ELEC:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("稲妻のブレスを吐いた。", "You breathe lightning."));
        fire_breath(caster_ptr, GF_ELEC, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MS_BR_FIRE:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("火炎のブレスを吐いた。", "You breathe fire."));
        fire_breath(caster_ptr, GF_FIRE, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MS_BR_COLD:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("冷気のブレスを吐いた。", "You breathe frost."));
        fire_breath(caster_ptr, GF_COLD, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MS_BR_POIS:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("ガスのブレスを吐いた。", "You breathe gas."));
        fire_breath(caster_ptr, GF_POIS, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MS_BR_NETHER:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("地獄のブレスを吐いた。", "You breathe nether."));
        fire_breath(caster_ptr, GF_NETHER, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MS_BR_LITE:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("閃光のブレスを吐いた。", "You breathe light."));
        fire_breath(caster_ptr, GF_LITE, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MS_BR_DARK:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("暗黒のブレスを吐いた。", "You breathe darkness."));
        fire_breath(caster_ptr, GF_DARK, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MS_BR_CONF:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("混乱のブレスを吐いた。", "You breathe confusion."));
        fire_breath(caster_ptr, GF_CONFUSION, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MS_BR_SOUND:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("轟音のブレスを吐いた。", "You breathe sound."));
        fire_breath(caster_ptr, GF_SOUND, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MS_BR_CHAOS:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("カオスのブレスを吐いた。", "You breathe chaos."));
        fire_breath(caster_ptr, GF_CHAOS, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MS_BR_DISEN:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("劣化のブレスを吐いた。", "You breathe disenchantment."));
        fire_breath(caster_ptr, GF_DISENCHANT, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MS_BR_NEXUS:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("因果混乱のブレスを吐いた。", "You breathe nexus."));
        fire_breath(caster_ptr, GF_NEXUS, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MS_BR_TIME:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("時間逆転のブレスを吐いた。", "You breathe time."));
        fire_breath(caster_ptr, GF_TIME, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MS_BR_INERTIA:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("遅鈍のブレスを吐いた。", "You breathe inertia."));
        fire_breath(caster_ptr, GF_INERTIAL, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MS_BR_GRAVITY:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("重力のブレスを吐いた。", "You breathe gravity."));
        fire_breath(caster_ptr, GF_GRAVITY, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MS_BR_SHARDS:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("破片のブレスを吐いた。", "You breathe shards."));
        fire_breath(caster_ptr, GF_SHARDS, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MS_BR_PLASMA:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("プラズマのブレスを吐いた。", "You breathe plasma."));

        fire_breath(caster_ptr, GF_PLASMA, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MS_BR_FORCE:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("フォースのブレスを吐いた。", "You breathe force."));

        fire_breath(caster_ptr, GF_FORCE, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MS_BR_MANA:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("魔力のブレスを吐いた。", "You breathe mana."));

        fire_breath(caster_ptr, GF_MANA, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MS_BALL_NUKE:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("放射能球を放った。", "You cast a ball of radiation."));

        fire_ball(caster_ptr, GF_NUKE, dir, damage, 2);
        break;

    case MS_BR_NUKE:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("放射性廃棄物のブレスを吐いた。", "You breathe toxic waste."));

        fire_breath(caster_ptr, GF_NUKE, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MS_BALL_CHAOS:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("純ログルスを放った。", "You invoke a raw Logrus."));

        fire_ball(caster_ptr, GF_CHAOS, dir, damage, 4);
        break;
    case MS_BR_DISI:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("分解のブレスを吐いた。", "You breathe disintegration."));

        fire_breath(caster_ptr, GF_DISINTEGRATE, dir, damage, (plev > 35 ? 3 : 2));
        break;
    case MS_BALL_ACID:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("アシッド・ボールの呪文を唱えた。", "You cast an acid ball."));

        fire_ball(caster_ptr, GF_ACID, dir, damage, 2);
        break;
    case MS_BALL_ELEC:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("サンダー・ボールの呪文を唱えた。", "You cast a lightning ball."));

        fire_ball(caster_ptr, GF_ELEC, dir, damage, 2);
        break;
    case MS_BALL_FIRE:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("ファイア・ボールの呪文を唱えた。", "You cast a fire ball."));

        fire_ball(caster_ptr, GF_FIRE, dir, damage, 2);
        break;
    case MS_BALL_COLD:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("アイス・ボールの呪文を唱えた。", "You cast a frost ball."));

        fire_ball(caster_ptr, GF_COLD, dir, damage, 2);
        break;
    case MS_BALL_POIS:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("悪臭雲の呪文を唱えた。", "You cast a stinking cloud."));

        fire_ball(caster_ptr, GF_POIS, dir, damage, 2);
        break;
    case MS_BALL_NETHER:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("地獄球の呪文を唱えた。", "You cast a nether ball."));

        fire_ball(caster_ptr, GF_NETHER, dir, damage, 2);
        break;
    case MS_BALL_WATER:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("流れるような身振りをした。", "You gesture fluidly."));

        fire_ball(caster_ptr, GF_WATER, dir, damage, 4);
        break;
    case MS_BALL_MANA:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("魔力の嵐の呪文を念じた。", "You invoke a mana storm."));

        fire_ball(caster_ptr, GF_MANA, dir, damage, 4);
        break;
    case MS_BALL_DARK:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("暗黒の嵐の呪文を念じた。", "You invoke a darkness storm."));

        fire_ball(caster_ptr, GF_DARK, dir, damage, 4);
        break;
    case MS_DRAIN_MANA:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        fire_ball_hide(caster_ptr, GF_DRAIN_MANA, dir, randint1(plev * 3) + plev, 0);
        break;
    case MS_MIND_BLAST:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        fire_ball_hide(caster_ptr, GF_MIND_BLAST, dir, damage, 0);
        break;
    case MS_BRAIN_SMASH:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        fire_ball_hide(caster_ptr, GF_BRAIN_SMASH, dir, damage, 0);
        break;
    case MS_CAUSE_1:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        fire_ball_hide(caster_ptr, GF_CAUSE_1, dir, damage, 0);
        break;
    case MS_CAUSE_2:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        fire_ball_hide(caster_ptr, GF_CAUSE_2, dir, damage, 0);
        break;
    case MS_CAUSE_3:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        fire_ball_hide(caster_ptr, GF_CAUSE_3, dir, damage, 0);
        break;
    case MS_CAUSE_4:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        fire_ball_hide(caster_ptr, GF_CAUSE_4, dir, damage, 0);
        break;
    case MS_BOLT_ACID:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("アシッド・ボルトの呪文を唱えた。", "You cast an acid bolt."));

        fire_bolt(caster_ptr, GF_ACID, dir, damage);
        break;
    case MS_BOLT_ELEC:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("サンダー・ボルトの呪文を唱えた。", "You cast a lightning bolt."));

        fire_bolt(caster_ptr, GF_ELEC, dir, damage);
        break;
    case MS_BOLT_FIRE:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("ファイア・ボルトの呪文を唱えた。", "You cast a fire bolt."));

        fire_bolt(caster_ptr, GF_FIRE, dir, damage);
        break;
    case MS_BOLT_COLD:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("アイス・ボルトの呪文を唱えた。", "You cast a frost bolt."));

        fire_bolt(caster_ptr, GF_COLD, dir, damage);
        break;
    case MS_STARBURST:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("スターバーストの呪文を念じた。", "You invoke a starburst."));

        fire_ball(caster_ptr, GF_LITE, dir, damage, 4);
        break;
    case MS_BOLT_NETHER:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("地獄の矢の呪文を唱えた。", "You cast a nether bolt."));

        fire_bolt(caster_ptr, GF_NETHER, dir, damage);
        break;
    case MS_BOLT_WATER:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("ウォーター・ボルトの呪文を唱えた。", "You cast a water bolt."));

        fire_bolt(caster_ptr, GF_WATER, dir, damage);
        break;
    case MS_BOLT_MANA:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("魔力の矢の呪文を唱えた。", "You cast a mana bolt."));

        fire_bolt(caster_ptr, GF_MANA, dir, damage);
        break;
    case MS_BOLT_PLASMA:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("プラズマ・ボルトの呪文を唱えた。", "You cast a plasma bolt."));

        fire_bolt(caster_ptr, GF_PLASMA, dir, damage);
        break;
    case MS_BOLT_ICE:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("極寒の矢の呪文を唱えた。", "You cast a ice bolt."));

        fire_bolt(caster_ptr, GF_ICE, dir, damage);
        break;
    case MS_MAGIC_MISSILE:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("マジック・ミサイルの呪文を唱えた。", "You cast a magic missile."));

        fire_bolt(caster_ptr, GF_MISSILE, dir, damage);
        break;
    case MS_SCARE:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("恐ろしげな幻覚を作り出した。", "You cast a fearful illusion."));

        fear_monster(caster_ptr, dir, plev + 10);
        break;
    case MS_BLIND:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        confuse_monster(caster_ptr, dir, plev * 2);
        break;
    case MS_CONF:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("誘惑的な幻覚をつくり出した。", "You cast a mesmerizing illusion."));

        confuse_monster(caster_ptr, dir, plev * 2);
        break;
    case MS_SLOW:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        slow_monster(caster_ptr, dir, plev);
        break;
    case MS_SLEEP:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        sleep_monster(caster_ptr, dir, plev);
        break;
    case MS_SPEED:
        (void)set_fast(caster_ptr, randint1(20 + plev) + plev, FALSE);
        break;
    case MS_HAND_DOOM: {
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("<破滅の手>を放った！", "You invoke the Hand of Doom!"));

        fire_ball_hide(caster_ptr, GF_HAND_DOOM, dir, 200, 0);
        break;
    }
    case MS_HEAL:
        msg_print(_("自分の傷に念を集中した。", "You concentrate on your wounds!"));
        (void)hp_player(caster_ptr, plev * 6);
        (void)set_stun(caster_ptr, 0);
        (void)set_cut(caster_ptr, 0);
        break;
    case MS_INVULNER:
        msg_print(_("無傷の球の呪文を唱えた。", "You cast a Globe of Invulnerability."));
        (void)set_invuln(caster_ptr, randint1(7) + 7, FALSE);
        break;
    case MS_BLINK:
        teleport_player(caster_ptr, 10, TELEPORT_SPONTANEOUS);
        break;
    case MS_TELEPORT:
        teleport_player(caster_ptr, plev * 5, TELEPORT_SPONTANEOUS);
        break;
    case MS_WORLD:
        (void)time_walk(caster_ptr);
        break;
    case MS_SPECIAL:
        break;
    case MS_TELE_TO: {
        monster_type *m_ptr;
        monster_race *r_ptr;
        GAME_TEXT m_name[MAX_NLEN];

        if (!target_set(caster_ptr, TARGET_KILL))
            return FALSE;
        if (!caster_ptr->current_floor_ptr->grid_array[target_row][target_col].m_idx)
            break;
        if (!player_has_los_bold(caster_ptr, target_row, target_col))
            break;
        if (!projectable(caster_ptr, caster_ptr->y, caster_ptr->x, target_row, target_col))
            break;
        m_ptr = &caster_ptr->current_floor_ptr->m_list[caster_ptr->current_floor_ptr->grid_array[target_row][target_col].m_idx];
        r_ptr = &r_info[m_ptr->r_idx];
        monster_desc(caster_ptr, m_name, m_ptr, 0);
        if (r_ptr->flagsr & RFR_RES_TELE) {
            if ((r_ptr->flags1 & (RF1_UNIQUE)) || (r_ptr->flagsr & RFR_RES_ALL)) {
                if (is_original_ap_and_seen(caster_ptr, m_ptr))
                    r_ptr->r_flagsr |= RFR_RES_TELE;
                msg_format(_("%sには効果がなかった！", "%s is unaffected!"), m_name);

                break;
            } else if (r_ptr->level > randint1(100)) {
                if (is_original_ap_and_seen(caster_ptr, m_ptr))
                    r_ptr->r_flagsr |= RFR_RES_TELE;
                msg_format(_("%sには耐性がある！", "%s resists!"), m_name);

                break;
            }
        }
        msg_format(_("%sを引き戻した。", "You command %s to return."), m_name);

        teleport_monster_to(
            caster_ptr, caster_ptr->current_floor_ptr->grid_array[target_row][target_col].m_idx, caster_ptr->y, caster_ptr->x, 100, TELEPORT_PASSIVE);
        break;
    }
    case MS_TELE_AWAY:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        (void)fire_beam(caster_ptr, GF_AWAY_ALL, dir, plev);
        break;

    case MS_TELE_LEVEL:
        return teleport_level_other(caster_ptr);
        break;

    case MS_PSY_SPEAR:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;
        else
            msg_print(_("光の剣を放った。", "You throw a psycho-spear."));
        (void)fire_beam(caster_ptr, GF_PSY_SPEAR, dir, damage);
        break;

    case MS_DARKNESS:
        msg_print(_("暗闇の中で手を振った。", "You gesture in shadow."));
        (void)unlite_area(caster_ptr, 10, 3);
        break;

    case MS_MAKE_TRAP:
        if (!target_set(caster_ptr, TARGET_KILL))
            return FALSE;
        msg_print(_("呪文を唱えて邪悪に微笑んだ。", "You cast a spell and cackle evilly."));
        trap_creation(caster_ptr, target_row, target_col);
        break;
    case MS_FORGET:
        msg_print(_("しかし何も起きなかった。", "Nothing happens."));
        break;
    case MS_RAISE_DEAD:
        msg_print(_("死者復活の呪文を唱えた。", "You animate the dead."));
        (void)animate_dead(caster_ptr, 0, caster_ptr->y, caster_ptr->x);
        break;
    case MS_S_KIN: {
        int k;
        if (!target_set(caster_ptr, TARGET_KILL))
            return FALSE;

        msg_print(_("援軍を召喚した。", "You summon minions."));
        for (k = 0; k < 4; k++) {
            (void)summon_kin_player(caster_ptr, plev, target_row, target_col, (PM_FORCE_PET | PM_ALLOW_GROUP));
        }
        break;
    }
    case MS_S_CYBER: {
        int k;
        int max_cyber = (caster_ptr->current_floor_ptr->dun_level / 50) + randint1(3);
        if (!target_set(caster_ptr, TARGET_KILL))
            return FALSE;
        msg_print(_("サイバーデーモンを召喚した！", "You summon Cyberdemons!"));
        if (max_cyber > 4)
            max_cyber = 4;
        for (k = 0; k < max_cyber; k++)
            summon_specific(caster_ptr, -1, target_row, target_col, plev, SUMMON_CYBER, mode);
        break;
    }
    case MS_S_MONSTER: {
        int k;
        if (!target_set(caster_ptr, TARGET_KILL))
            return FALSE;
        msg_print(_("仲間を召喚した。", "You summon help."));
        for (k = 0; k < 1; k++)
            summon_specific(caster_ptr, -1, target_row, target_col, plev, SUMMON_NONE, (mode | u_mode));
        break;
    }
    case MS_S_MONSTERS: {
        int k;
        if (!target_set(caster_ptr, TARGET_KILL))
            return FALSE;
        msg_print(_("モンスターを召喚した！", "You summon monsters!"));
        for (k = 0; k < 6; k++)
            summon_specific(caster_ptr, -1, target_row, target_col, plev, SUMMON_NONE, (mode | u_mode));
        break;
    }
    case MS_S_ANT: {
        int k;
        if (!target_set(caster_ptr, TARGET_KILL))
            return FALSE;
        msg_print(_("アリを召喚した。", "You summon ants."));
        for (k = 0; k < 6; k++)
            summon_specific(caster_ptr, -1, target_row, target_col, plev, SUMMON_ANT, mode);
        break;
    }
    case MS_S_SPIDER: {
        int k;
        if (!target_set(caster_ptr, TARGET_KILL))
            return FALSE;
        msg_print(_("蜘蛛を召喚した。", "You summon spiders."));
        for (k = 0; k < 6; k++)
            summon_specific(caster_ptr, -1, target_row, target_col, plev, SUMMON_SPIDER, mode);
        break;
    }
    case MS_S_HOUND: {
        int k;
        if (!target_set(caster_ptr, TARGET_KILL))
            return FALSE;
        msg_print(_("ハウンドを召喚した。", "You summon hounds."));
        for (k = 0; k < 4; k++)
            summon_specific(caster_ptr, -1, target_row, target_col, plev, SUMMON_HOUND, mode);
        break;
    }
    case MS_S_HYDRA: {
        int k;
        if (!target_set(caster_ptr, TARGET_KILL))
            return FALSE;
        msg_print(_("ヒドラを召喚した。", "You summon hydras."));
        for (k = 0; k < 4; k++)
            summon_specific(caster_ptr, -1, target_row, target_col, plev, SUMMON_HYDRA, mode);
        break;
    }
    case MS_S_ANGEL: {
        int k;
        if (!target_set(caster_ptr, TARGET_KILL))
            return FALSE;
        msg_print(_("天使を召喚した！", "You summon an angel!"));
        for (k = 0; k < 1; k++)
            summon_specific(caster_ptr, -1, target_row, target_col, plev, SUMMON_ANGEL, mode);
        break;
    }
    case MS_S_DEMON: {
        int k;
        if (!target_set(caster_ptr, TARGET_KILL))
            return FALSE;
        msg_print(_("混沌の宮廷から悪魔を召喚した！", "You summon a demon from the Courts of Chaos!"));
        for (k = 0; k < 1; k++)
            summon_specific(caster_ptr, -1, target_row, target_col, plev, SUMMON_DEMON, (mode | u_mode));
        break;
    }
    case MS_S_UNDEAD: {
        int k;
        if (!target_set(caster_ptr, TARGET_KILL))
            return FALSE;
        msg_print(_("アンデッドの強敵を召喚した！", "You summon an undead adversary!"));
        for (k = 0; k < 1; k++)
            summon_specific(caster_ptr, -1, target_row, target_col, plev, SUMMON_UNDEAD, (mode | u_mode));
        break;
    }
    case MS_S_DRAGON: {
        int k;
        if (!target_set(caster_ptr, TARGET_KILL))
            return FALSE;
        msg_print(_("ドラゴンを召喚した！", "You summon a dragon!"));
        for (k = 0; k < 1; k++)
            summon_specific(caster_ptr, -1, target_row, target_col, plev, SUMMON_DRAGON, (mode | u_mode));
        break;
    }
    case MS_S_HI_UNDEAD: {
        int k;
        if (!target_set(caster_ptr, TARGET_KILL))
            return FALSE;
        msg_print(_("強力なアンデッドを召喚した！", "You summon greater undead!"));
        for (k = 0; k < 6; k++)
            summon_specific(caster_ptr, -1, target_row, target_col, plev, SUMMON_HI_UNDEAD, (mode | u_mode));
        break;
    }
    case MS_S_HI_DRAGON: {
        int k;
        if (!target_set(caster_ptr, TARGET_KILL))
            return FALSE;
        msg_print(_("古代ドラゴンを召喚した！", "You summon ancient dragons!"));
        for (k = 0; k < 4; k++)
            summon_specific(caster_ptr, -1, target_row, target_col, plev, SUMMON_HI_DRAGON, (mode | u_mode));
        break;
    }
    case MS_S_AMBERITE: {
        int k;
        if (!target_set(caster_ptr, TARGET_KILL))
            return FALSE;
        msg_print(_("アンバーの王族を召喚した！", "You summon Lords of Amber!"));
        for (k = 0; k < 4; k++)
            summon_specific(caster_ptr, -1, target_row, target_col, plev, SUMMON_AMBERITES, (mode | PM_ALLOW_UNIQUE));
        break;
    }
    case MS_S_UNIQUE: {
        int k, count = 0;
        if (!target_set(caster_ptr, TARGET_KILL))
            return FALSE;
        msg_print(_("特別な強敵を召喚した！", "You summon special opponents!"));
        for (k = 0; k < 4; k++)
            if (summon_specific(caster_ptr, -1, target_row, target_col, plev, SUMMON_UNIQUE, (mode | PM_ALLOW_UNIQUE)))
                count++;
        for (k = count; k < 4; k++)
            summon_specific(caster_ptr, -1, target_row, target_col, plev, SUMMON_HI_UNDEAD, (mode | u_mode));
        break;
    }
    default:
        msg_print("hoge?");
    }

    return TRUE;
}

/*!
 * @brief ものまねコマンドのメインルーチン /
 * do_cmd_cast calls this function if the player's class is 'imitator'.
 * @param baigaesi TRUEならば倍返し上の処理として行う
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
 * @details
 * If a valid spell is chosen, saves it in '*sn' and returns TRUE
 * If the user hits escape, returns FALSE, and set '*sn' to -1
 * If there are no legal choices, returns FALSE, and sets '*sn' to -2
 *
 * The "prompt" should be "cast", "recite", or "study"
 * The "known" should be TRUE for cast/pray, FALSE for study
 *
 * nb: This function has a (trivial) display bug which will be obvious
 * when you run it. It's probably easy to fix but I haven't tried,
 * sorry.
 */
bool do_cmd_mane(player_type *creature_ptr, bool baigaesi)
{
    int n = 0, j;
    PERCENTAGE chance;
    PERCENTAGE minfail = 0;
    PLAYER_LEVEL plev = creature_ptr->lev;
    monster_power spell;
    bool cast;

    if (cmd_limit_confused(creature_ptr))
        return FALSE;

    if (!creature_ptr->mane_num) {
        msg_print(_("まねられるものが何もない！", "You don't remember any action!"));
        return FALSE;
    }

    if (!get_mane_power(creature_ptr, &n, baigaesi))
        return FALSE;

    spell = monster_powers[creature_ptr->mane_spell[n]];

    /* Spell failure chance */
    chance = spell.manefail;

    /* Reduce failure rate by "effective" level adjustment */
    if (plev > spell.level)
        chance -= 3 * (plev - spell.level);

    /* Reduce failure rate by 1 stat and DEX adjustment */
    chance -= 3 * (adj_mag_stat[creature_ptr->stat_ind[spell.use_stat]] + adj_mag_stat[creature_ptr->stat_ind[A_DEX]] - 2) / 2;

    if (spell.manedam)
        chance = chance * damage / spell.manedam;

    chance += creature_ptr->to_m_chance;

    /* Extract the minimum failure rate */
    minfail = adj_mag_fail[creature_ptr->stat_ind[spell.use_stat]];

    /* Minimum failure rate */
    if (chance < minfail)
        chance = minfail;

    /* Stunning makes spells harder */
    if (creature_ptr->stun > 50)
        chance += 25;
    else if (creature_ptr->stun)
        chance += 15;

    /* Always a 5 percent chance of working */
    if (chance > 95)
        chance = 95;

    /* Failed spell */
    if (randint0(100) < chance) {
        if (flush_failure)
            flush();
        msg_print(_("ものまねに失敗した！", "You failed to concentrate hard enough!"));
        sound(SOUND_FAIL);
    } else {
        sound(SOUND_ZAP);
        cast = use_mane(creature_ptr, creature_ptr->mane_spell[n]);
        if (!cast)
            return FALSE;
    }

    creature_ptr->mane_num--;
    for (j = n; j < creature_ptr->mane_num; j++) {
        creature_ptr->mane_spell[j] = creature_ptr->mane_spell[j + 1];
        creature_ptr->mane_dam[j] = creature_ptr->mane_dam[j + 1];
    }

    take_turn(creature_ptr, 100);

    creature_ptr->redraw |= (PR_IMITATION);
    creature_ptr->window_flags |= (PW_PLAYER);
    creature_ptr->window_flags |= (PW_SPELL);

    return TRUE;
}
