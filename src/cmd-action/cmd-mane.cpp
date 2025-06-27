/*!
 * @brief ものまねの処理実装 / Imitation code
 * @date 2014/01/14
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * 2014 Deskull rearranged comment for Doxygen.\n
 */

#include "cmd-action/cmd-mane.h"
#include "action/action-limited.h"
#include "artifact/fixed-art-types.h"
#include "cmd-action/cmd-spell.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "floor/floor-object.h"
#include "game-option/disturbance-options.h"
#include "game-option/text-display-options.h"
#include "hpmp/hp-mp-processor.h"
#include "inventory/inventory-slot-types.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/mind-mage.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/race-ability-flags.h"
#include "monster-race/race-ability-mask.h"
#include "monster-race/race-flags-resistance.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-processor.h"
#include "monster/monster-status.h"
#include "mspell/monster-power-table.h"
#include "player-base/player-class.h"
#include "player-info/mane-data-type.h"
#include "player-status/player-energy.h"
#include "player/player-status-table.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell/spells-status.h"
#include "spell/spells-summon.h"
#include "spell/summon-types.h"
#include "status/bad-status-setter.h"
#include "status/body-improvement.h"
#include "status/buff-setter.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/projection-path-calculator.h"
#include "target/target-checker.h"
#include "target/target-getter.h"
#include "target/target-setter.h"
#include "target/target-types.h"
#include "term/screen-processor.h"
#include "term/z-form.h"
#include "timed-effect/timed-effects.h"
#include "util/enum-converter.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include <iterator>
#include <unordered_set>

namespace {
const std::unordered_set<MonsterAbilityType> AIMING_SPELLS = {
    MonsterAbilityType::ROCKET,
    MonsterAbilityType::SHOOT,
    MonsterAbilityType::BR_ACID,
    MonsterAbilityType::BR_ELEC,
    MonsterAbilityType::BR_FIRE,
    MonsterAbilityType::BR_COLD,
    MonsterAbilityType::BR_POIS,
    MonsterAbilityType::BR_NETH,
    MonsterAbilityType::BR_LITE,
    MonsterAbilityType::BR_DARK,
    MonsterAbilityType::BR_CONF,
    MonsterAbilityType::BR_SOUN,
    MonsterAbilityType::BR_CHAO,
    MonsterAbilityType::BR_DISE,
    MonsterAbilityType::BR_NEXU,
    MonsterAbilityType::BR_TIME,
    MonsterAbilityType::BR_INER,
    MonsterAbilityType::BR_GRAV,
    MonsterAbilityType::BR_SHAR,
    MonsterAbilityType::BR_PLAS,
    MonsterAbilityType::BR_FORC,
    MonsterAbilityType::BR_MANA,
    MonsterAbilityType::BA_NUKE,
    MonsterAbilityType::BR_NUKE,
    MonsterAbilityType::BA_CHAO,
    MonsterAbilityType::BR_DISI,
    MonsterAbilityType::BR_VOID,
    MonsterAbilityType::BR_ABYSS,
    MonsterAbilityType::BA_ACID,
    MonsterAbilityType::BA_ELEC,
    MonsterAbilityType::BA_FIRE,
    MonsterAbilityType::BA_COLD,
    MonsterAbilityType::BA_POIS,
    MonsterAbilityType::BA_NETH,
    MonsterAbilityType::BA_WATE,
    MonsterAbilityType::BA_MANA,
    MonsterAbilityType::BA_DARK,
    MonsterAbilityType::BA_VOID,
    MonsterAbilityType::BA_ABYSS,
    MonsterAbilityType::BA_METEOR,
    MonsterAbilityType::DRAIN_MANA,
    MonsterAbilityType::MIND_BLAST,
    MonsterAbilityType::BRAIN_SMASH,
    MonsterAbilityType::CAUSE_1,
    MonsterAbilityType::CAUSE_2,
    MonsterAbilityType::CAUSE_3,
    MonsterAbilityType::CAUSE_4,
    MonsterAbilityType::BO_ACID,
    MonsterAbilityType::BO_ELEC,
    MonsterAbilityType::BO_FIRE,
    MonsterAbilityType::BO_COLD,
    MonsterAbilityType::BA_LITE,
    MonsterAbilityType::BO_NETH,
    MonsterAbilityType::BO_WATE,
    MonsterAbilityType::BO_MANA,
    MonsterAbilityType::BO_PLAS,
    MonsterAbilityType::BO_ICEE,
    MonsterAbilityType::BO_VOID,
    MonsterAbilityType::BO_ABYSS,
    MonsterAbilityType::BO_METEOR,
    MonsterAbilityType::BO_LITE,
    MonsterAbilityType::MISSILE,
    MonsterAbilityType::SCARE,
    MonsterAbilityType::BLIND,
    MonsterAbilityType::CONF,
    MonsterAbilityType::SLOW,
    MonsterAbilityType::HOLD,
    MonsterAbilityType::HAND_DOOM,
    MonsterAbilityType::TELE_AWAY,
    MonsterAbilityType::PSY_SPEAR,
};
}

static int damage;

/*!
 * @brief 受け取ったパラメータに応じてものまねの効果情報をまとめたフォーマットを返す
 * @param power ものまねの効力の種類
 * @param dam ものまねの威力
 * @param std::string ものまねの効果を表す文字列
 */
static std::string mane_info(PlayerType *player_ptr, MonsterAbilityType power, int dam)
{
    PLAYER_LEVEL plev = player_ptr->lev;

    using Mat = MonsterAbilityType;
    const auto flags =
        (RF_ABILITY_BALL_MASK |
            RF_ABILITY_BOLT_MASK |
            RF_ABILITY_BEAM_MASK)
            .set(
                { Mat::MIND_BLAST, Mat::BRAIN_SMASH, Mat::CAUSE_1, Mat::CAUSE_2, Mat::CAUSE_3, Mat::CAUSE_4 });
    if (flags.has(power)) {
        return format(" %s%d", KWD_DAM, (int)dam);
    }
    switch (power) {
    case MonsterAbilityType::DRAIN_MANA:
        return format(" %sd%d+%d", KWD_HEAL, plev * 3, plev);
    case MonsterAbilityType::HASTE:
        return format(" %sd%d+%d", KWD_DURATION, 20 + plev, plev);
    case MonsterAbilityType::HEAL:
        return format(" %s%d", KWD_HEAL, plev * 6);
    case MonsterAbilityType::INVULNER:
        return format(" %sd7+7", KWD_DURATION);
    case MonsterAbilityType::BLINK:
        return format(" %s10", KWD_SPHERE);
    case MonsterAbilityType::TPORT:
        return format(" %s%d", KWD_SPHERE, plev * 5);
    case MonsterAbilityType::RAISE_DEAD:
        return format(" %s5", KWD_SPHERE);
    default:
        return std::string();
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
static int get_mane_power(PlayerType *player_ptr, int *sn, bool baigaesi)
{
    int i = 0;
    int num = 0;
    TERM_LEN y = 1;
    TERM_LEN x = 18;
    PERCENTAGE minfail = 0;
    PLAYER_LEVEL plev = player_ptr->lev;
    PERCENTAGE chance = 0;
    concptr p = _("能力", "power");

    monster_power spell;
    bool flag, redraw;

    /* Assume cancelled */
    *sn = (-1);

    flag = false;
    redraw = false;

    auto mane_data = PlayerClass(player_ptr).get_specific_data<mane_data_type>();

    num = mane_data->mane_list.size();

    /* Build a prompt (accept all spells) */
    constexpr auto fmt = _("(%c-%c, '*'で一覧, ESC) どの%sをまねますか？", "(%c-%c, *=List, ESC=exit) Use which %s? ");
    const auto prompt = format(fmt, I2A(0), I2A(num - 1), p);

    char choice = always_show_list ? ESCAPE : '\1';
    while (!flag) {
        if (choice == ESCAPE) {
            choice = ' ';
        } else {
            const auto new_choice = input_command(prompt, true);
            if (!new_choice) {
                break;
            }

            choice = *new_choice;
        }

        /* Request redraw */
        if ((choice == ' ') || (choice == '*') || (choice == '?')) {
            /* Show the list */
            if (!redraw) {
                redraw = true;
                screen_save();

                /* Display a list of spells */
                prt("", y, x);
                put_str(_("名前", "Name"), y, x + 5);
                put_str(_("失率 効果", "Fail Info"), y, x + 36);

                /* Dump the spells */
                for (i = 0; i < num; i++) {
                    const auto &mane = mane_data->mane_list[i];
                    /* Access the spell */
                    spell = monster_powers.at(mane.spell);

                    chance = spell.manefail;

                    /* Reduce failure rate by "effective" level adjustment */
                    if (plev > spell.level) {
                        chance -= 3 * (plev - spell.level);
                    }

                    /* Reduce failure rate by INT/WIS adjustment */
                    chance -= 3 * (adj_mag_stat[player_ptr->stat_index[spell.use_stat]] + adj_mag_stat[player_ptr->stat_index[A_DEX]] - 2) / 2;

                    if (spell.manedam) {
                        chance = chance * (baigaesi ? mane.damage * 2 : mane.damage) / spell.manedam;
                    }

                    chance += player_ptr->to_m_chance;

                    if (player_ptr->is_wielding(FixedArtifactId::GOGO_PENDANT)) {
                        chance -= 10;
                    }

                    /* Extract the minimum failure rate */
                    minfail = adj_mag_fail[player_ptr->stat_index[spell.use_stat]];

                    /* Minimum failure rate */
                    if (chance < minfail) {
                        chance = minfail;
                    }

                    chance += player_ptr->effects()->stun().get_magic_chance_penalty();
                    if (chance > 95) {
                        chance = 95;
                    }

                    /* Get info */
                    const auto comment = mane_info(player_ptr, mane.spell, (baigaesi ? mane.damage * 2 : mane.damage));

                    /* Dump the spell --(-- */
                    prt(format("  %c) %-30s %3d%%%s", I2A(i), spell.name, chance, comment.data()), y + i + 1, x);
                }

                /* Clear the bottom line */
                prt("", y + i + 1, x);
            }

            /* Hide the list */
            else {
                /* Hide list */
                redraw = false;
                screen_load();
            }

            /* Redo asking */
            continue;
        }

        /* Extract request */
        i = A2I(choice);

        /* Totally Illegal */
        if ((i < 0) || (i >= num)) {
            bell();
            continue;
        }

        /* Save the spell index */
        spell = monster_powers.at(mane_data->mane_list[i].spell);

        /* Stop the loop */
        flag = true;
    }
    if (redraw) {
        screen_load();
    }

    RedrawingFlagsUpdater::get_instance().set_flag(SubWindowRedrawingFlag::SPELL);
    handle_stuff(player_ptr);

    /* Abort if needed */
    if (!flag) {
        return false;
    }

    /* Save the choice */
    (*sn) = i;

    damage = (baigaesi ? mane_data->mane_list[i].damage * 2 : mane_data->mane_list[i].damage);

    /* Success */
    return true;
}

/*!
 * @brief ものまね処理の発動 /
 * do_cmd_cast calls this function if the player's class is 'imitator'.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param spell 発動するモンスター攻撃のID
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
 */
static bool use_mane(PlayerType *player_ptr, MonsterAbilityType spell)
{
    PLAYER_LEVEL plev = player_ptr->lev;
    BIT_FLAGS mode = (PM_ALLOW_GROUP | PM_FORCE_PET);
    BIT_FLAGS u_mode = 0L;

    if (randint1(50 + plev) < plev / 10) {
        u_mode = PM_ALLOW_UNIQUE;
    }

    auto dir = Direction::none();
    if (AIMING_SPELLS.contains(spell)) {
        dir = get_aim_dir(player_ptr);
        if (!dir) {
            return false;
        }
    }

    /* spell code */
    switch (spell) {
    case MonsterAbilityType::SHRIEK:
        msg_print(_("かん高い金切り声をあげた。", "You make a high pitched shriek."));
        aggravate_monsters(player_ptr, 0);
        break;

    case MonsterAbilityType::XXX1:
        break;

    case MonsterAbilityType::DISPEL: {
        const auto pos = target_set(player_ptr, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }

        const auto &floor = *player_ptr->current_floor_ptr;
        const auto &grid = floor.get_grid(*pos);
        const auto m_idx = grid.m_idx;
        const auto p_pos = player_ptr->get_position();
        auto should_dispel = m_idx == 0;
        should_dispel &= grid.has_los();
        should_dispel &= projectable(floor, p_pos, *pos);
        if (!should_dispel) {
            break;
        }

        dispel_monster_status(player_ptr, m_idx);
        break;
    }

    case MonsterAbilityType::ROCKET:
        msg_print(_("ロケットを発射した。", "You fire a rocket."));
        fire_rocket(player_ptr, AttributeType::ROCKET, dir, damage, 2);
        break;

    case MonsterAbilityType::SHOOT:
        msg_print(_("矢を放った。", "You fire an arrow."));
        fire_bolt(player_ptr, AttributeType::MONSTER_SHOOT, dir, damage);
        break;

    case MonsterAbilityType::XXX2:
        break;

    case MonsterAbilityType::XXX3:
        break;

    case MonsterAbilityType::XXX4:
        break;

    case MonsterAbilityType::BR_ACID:
        msg_print(_("酸のブレスを吐いた。", "You breathe acid."));
        fire_breath(player_ptr, AttributeType::ACID, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MonsterAbilityType::BR_ELEC:
        msg_print(_("稲妻のブレスを吐いた。", "You breathe lightning."));
        fire_breath(player_ptr, AttributeType::ELEC, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MonsterAbilityType::BR_FIRE:
        msg_print(_("火炎のブレスを吐いた。", "You breathe fire."));
        fire_breath(player_ptr, AttributeType::FIRE, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MonsterAbilityType::BR_COLD:
        msg_print(_("冷気のブレスを吐いた。", "You breathe frost."));
        fire_breath(player_ptr, AttributeType::COLD, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MonsterAbilityType::BR_POIS:
        msg_print(_("ガスのブレスを吐いた。", "You breathe gas."));
        fire_breath(player_ptr, AttributeType::POIS, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MonsterAbilityType::BR_NETH:
        msg_print(_("地獄のブレスを吐いた。", "You breathe nether."));
        fire_breath(player_ptr, AttributeType::NETHER, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MonsterAbilityType::BR_LITE:
        msg_print(_("閃光のブレスを吐いた。", "You breathe light."));
        fire_breath(player_ptr, AttributeType::LITE, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MonsterAbilityType::BR_DARK:
        msg_print(_("暗黒のブレスを吐いた。", "You breathe darkness."));
        fire_breath(player_ptr, AttributeType::DARK, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MonsterAbilityType::BR_CONF:
        msg_print(_("混乱のブレスを吐いた。", "You breathe confusion."));
        fire_breath(player_ptr, AttributeType::CONFUSION, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MonsterAbilityType::BR_SOUN:
        msg_print(_("轟音のブレスを吐いた。", "You breathe sound."));
        fire_breath(player_ptr, AttributeType::SOUND, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MonsterAbilityType::BR_CHAO:
        msg_print(_("カオスのブレスを吐いた。", "You breathe chaos."));
        fire_breath(player_ptr, AttributeType::CHAOS, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MonsterAbilityType::BR_DISE:
        msg_print(_("劣化のブレスを吐いた。", "You breathe disenchantment."));
        fire_breath(player_ptr, AttributeType::DISENCHANT, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MonsterAbilityType::BR_NEXU:
        msg_print(_("因果混乱のブレスを吐いた。", "You breathe nexus."));
        fire_breath(player_ptr, AttributeType::NEXUS, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MonsterAbilityType::BR_TIME:
        msg_print(_("時間逆転のブレスを吐いた。", "You breathe time."));
        fire_breath(player_ptr, AttributeType::TIME, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MonsterAbilityType::BR_INER:
        msg_print(_("遅鈍のブレスを吐いた。", "You breathe inertia."));
        fire_breath(player_ptr, AttributeType::INERTIAL, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MonsterAbilityType::BR_GRAV:
        msg_print(_("重力のブレスを吐いた。", "You breathe gravity."));
        fire_breath(player_ptr, AttributeType::GRAVITY, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MonsterAbilityType::BR_SHAR:
        msg_print(_("破片のブレスを吐いた。", "You breathe shards."));
        fire_breath(player_ptr, AttributeType::SHARDS, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MonsterAbilityType::BR_PLAS:
        msg_print(_("プラズマのブレスを吐いた。", "You breathe plasma."));
        fire_breath(player_ptr, AttributeType::PLASMA, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MonsterAbilityType::BR_FORC:
        msg_print(_("フォースのブレスを吐いた。", "You breathe force."));
        fire_breath(player_ptr, AttributeType::FORCE, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MonsterAbilityType::BR_MANA:
        msg_print(_("魔力のブレスを吐いた。", "You breathe mana."));
        fire_breath(player_ptr, AttributeType::MANA, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MonsterAbilityType::BA_NUKE:
        msg_print(_("放射能球を放った。", "You cast a ball of radiation."));
        fire_ball(player_ptr, AttributeType::NUKE, dir, damage, 2);
        break;

    case MonsterAbilityType::BR_NUKE:
        msg_print(_("放射性廃棄物のブレスを吐いた。", "You breathe toxic waste."));
        fire_breath(player_ptr, AttributeType::NUKE, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MonsterAbilityType::BA_CHAO:
        msg_print(_("純ログルスを放った。", "You invoke a raw Logrus."));
        fire_ball(player_ptr, AttributeType::CHAOS, dir, damage, 4);
        break;
    case MonsterAbilityType::BR_DISI:
        msg_print(_("分解のブレスを吐いた。", "You breathe disintegration."));
        fire_breath(player_ptr, AttributeType::DISINTEGRATE, dir, damage, (plev > 35 ? 3 : 2));
        break;
    case MonsterAbilityType::BR_VOID:
        msg_print(_("虚無のブレスを吐いた。", "You breathe void."));
        fire_breath(player_ptr, AttributeType::VOID_MAGIC, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MonsterAbilityType::BR_ABYSS:
        msg_print(_("深淵のブレスを吐いた。", "You breathe abyss."));
        fire_breath(player_ptr, AttributeType::ABYSS, dir, damage, (plev > 35 ? 3 : 2));
        break;

    case MonsterAbilityType::BA_ACID:
        msg_print(_("アシッド・ボールの呪文を唱えた。", "You cast an acid ball."));
        fire_ball(player_ptr, AttributeType::ACID, dir, damage, 2);
        break;
    case MonsterAbilityType::BA_ELEC:
        msg_print(_("サンダー・ボールの呪文を唱えた。", "You cast a lightning ball."));
        fire_ball(player_ptr, AttributeType::ELEC, dir, damage, 2);
        break;
    case MonsterAbilityType::BA_FIRE:
        msg_print(_("ファイア・ボールの呪文を唱えた。", "You cast a fire ball."));
        fire_ball(player_ptr, AttributeType::FIRE, dir, damage, 2);
        break;
    case MonsterAbilityType::BA_COLD:
        msg_print(_("アイス・ボールの呪文を唱えた。", "You cast a frost ball."));
        fire_ball(player_ptr, AttributeType::COLD, dir, damage, 2);
        break;
    case MonsterAbilityType::BA_POIS:
        msg_print(_("悪臭雲の呪文を唱えた。", "You cast a stinking cloud."));
        fire_ball(player_ptr, AttributeType::POIS, dir, damage, 2);
        break;
    case MonsterAbilityType::BA_NETH:
        msg_print(_("地獄球の呪文を唱えた。", "You cast a nether ball."));
        fire_ball(player_ptr, AttributeType::NETHER, dir, damage, 2);
        break;
    case MonsterAbilityType::BA_WATE:
        msg_print(_("流れるような身振りをした。", "You gesture fluidly."));
        fire_ball(player_ptr, AttributeType::WATER, dir, damage, 4);
        break;
    case MonsterAbilityType::BA_MANA:
        msg_print(_("魔力の嵐の呪文を念じた。", "You invoke a mana storm."));
        fire_ball(player_ptr, AttributeType::MANA, dir, damage, 4);
        break;
    case MonsterAbilityType::BA_DARK:
        msg_print(_("暗黒の嵐の呪文を念じた。", "You invoke a darkness storm."));
        fire_ball(player_ptr, AttributeType::DARK, dir, damage, 4);
        break;
    case MonsterAbilityType::BA_VOID:
        msg_print(_("虚無の嵐の呪文を念じた。", "You cast a void ball."));
        fire_ball(player_ptr, AttributeType::VOID_MAGIC, dir, damage, 4);
        break;
    case MonsterAbilityType::BA_ABYSS:
        msg_print(_("深淵の嵐の呪文を念じた。", "You cast a abyss ball."));
        fire_ball(player_ptr, AttributeType::ABYSS, dir, damage, 4);
        break;
    case MonsterAbilityType::BA_METEOR:
        msg_print(_("メテオスウォームの呪文を念じた。", "You cast a meteor swarm."));
        fire_ball(player_ptr, AttributeType::METEOR, dir, damage, 4);
        break;
    case MonsterAbilityType::DRAIN_MANA:
        fire_ball_hide(player_ptr, AttributeType::DRAIN_MANA, dir, randint1(plev * 3) + plev, 0);
        break;
    case MonsterAbilityType::MIND_BLAST:
        fire_ball_hide(player_ptr, AttributeType::MIND_BLAST, dir, damage, 0);
        break;
    case MonsterAbilityType::BRAIN_SMASH:
        fire_ball_hide(player_ptr, AttributeType::BRAIN_SMASH, dir, damage, 0);
        break;
    case MonsterAbilityType::CAUSE_1:
        fire_ball_hide(player_ptr, AttributeType::CAUSE_1, dir, damage, 0);
        break;
    case MonsterAbilityType::CAUSE_2:
        fire_ball_hide(player_ptr, AttributeType::CAUSE_2, dir, damage, 0);
        break;
    case MonsterAbilityType::CAUSE_3:
        fire_ball_hide(player_ptr, AttributeType::CAUSE_3, dir, damage, 0);
        break;
    case MonsterAbilityType::CAUSE_4:
        fire_ball_hide(player_ptr, AttributeType::CAUSE_4, dir, damage, 0);
        break;
    case MonsterAbilityType::BO_ACID:
        msg_print(_("アシッド・ボルトの呪文を唱えた。", "You cast an acid bolt."));
        fire_bolt(player_ptr, AttributeType::ACID, dir, damage);
        break;
    case MonsterAbilityType::BO_ELEC:
        msg_print(_("サンダー・ボルトの呪文を唱えた。", "You cast a lightning bolt."));
        fire_bolt(player_ptr, AttributeType::ELEC, dir, damage);
        break;
    case MonsterAbilityType::BO_FIRE:
        msg_print(_("ファイア・ボルトの呪文を唱えた。", "You cast a fire bolt."));
        fire_bolt(player_ptr, AttributeType::FIRE, dir, damage);
        break;
    case MonsterAbilityType::BO_COLD:
        msg_print(_("アイス・ボルトの呪文を唱えた。", "You cast a frost bolt."));
        fire_bolt(player_ptr, AttributeType::COLD, dir, damage);
        break;
    case MonsterAbilityType::BA_LITE:
        msg_print(_("スターバーストの呪文を念じた。", "You invoke a starburst."));
        fire_ball(player_ptr, AttributeType::LITE, dir, damage, 4);
        break;
    case MonsterAbilityType::BO_NETH:
        msg_print(_("地獄の矢の呪文を唱えた。", "You cast a nether bolt."));
        fire_bolt(player_ptr, AttributeType::NETHER, dir, damage);
        break;
    case MonsterAbilityType::BO_WATE:
        msg_print(_("ウォーター・ボルトの呪文を唱えた。", "You cast a water bolt."));
        fire_bolt(player_ptr, AttributeType::WATER, dir, damage);
        break;
    case MonsterAbilityType::BO_MANA:
        msg_print(_("魔力の矢の呪文を唱えた。", "You cast a mana bolt."));
        fire_bolt(player_ptr, AttributeType::MANA, dir, damage);
        break;
    case MonsterAbilityType::BO_PLAS:
        msg_print(_("プラズマ・ボルトの呪文を唱えた。", "You cast a plasma bolt."));
        fire_bolt(player_ptr, AttributeType::PLASMA, dir, damage);
        break;
    case MonsterAbilityType::BO_ICEE:
        msg_print(_("極寒の矢の呪文を唱えた。", "You cast a ice bolt."));
        fire_bolt(player_ptr, AttributeType::ICE, dir, damage);
        break;
    case MonsterAbilityType::BO_VOID:
        msg_print(_("虚無の矢の呪文を唱えた。", "You cast a void bolt."));
        fire_bolt(player_ptr, AttributeType::VOID_MAGIC, dir, damage);
        break;
    case MonsterAbilityType::BO_ABYSS:
        msg_print(_("深淵の矢の呪文を唱えた。", "You cast a abyss bolt."));
        fire_bolt(player_ptr, AttributeType::ABYSS, dir, damage);
        break;
    case MonsterAbilityType::BO_METEOR:
        msg_print(_("メテオストライクの呪文を唱えた。", "You cast a meteor strike."));
        fire_bolt(player_ptr, AttributeType::METEOR, dir, damage);
        break;
    case MonsterAbilityType::BO_LITE:
        msg_print(_("スターライトアローの呪文を唱えた。", "You cast a starlight arrow."));
        fire_bolt(player_ptr, AttributeType::LITE, dir, damage);
        break;
    case MonsterAbilityType::MISSILE:
        msg_print(_("マジック・ミサイルの呪文を唱えた。", "You cast a magic missile."));
        fire_bolt(player_ptr, AttributeType::MISSILE, dir, damage);
        break;
    case MonsterAbilityType::SCARE:
        msg_print(_("恐ろしげな幻覚を作り出した。", "You cast a fearful illusion."));
        fear_monster(player_ptr, dir, plev + 10);
        break;
    case MonsterAbilityType::BLIND:
        confuse_monster(player_ptr, dir, plev * 2);
        break;
    case MonsterAbilityType::CONF:
        msg_print(_("誘惑的な幻覚をつくり出した。", "You cast a mesmerizing illusion."));
        confuse_monster(player_ptr, dir, plev * 2);
        break;
    case MonsterAbilityType::SLOW:
        slow_monster(player_ptr, dir, plev);
        break;
    case MonsterAbilityType::HOLD:
        sleep_monster(player_ptr, dir, plev);
        break;
    case MonsterAbilityType::HASTE:
        (void)set_acceleration(player_ptr, randint1(20 + plev) + plev, false);
        break;
    case MonsterAbilityType::HAND_DOOM: {
        msg_print(_("<破滅の手>を放った！", "You invoke the Hand of Doom!"));
        fire_ball_hide(player_ptr, AttributeType::HAND_DOOM, dir, 200, 0);
        break;
    }
    case MonsterAbilityType::HEAL: {
        msg_print(_("自分の傷に念を集中した。", "You concentrate on your wounds!"));
        (void)hp_player(player_ptr, plev * 6);
        BadStatusSetter bss(player_ptr);
        (void)bss.set_stun(0);
        (void)bss.set_cut(0);
        break;
    }
    case MonsterAbilityType::INVULNER:
        msg_print(_("無傷の球の呪文を唱えた。", "You cast a Globe of Invulnerability."));
        (void)set_invuln(player_ptr, randint1(7) + 7, false);
        break;
    case MonsterAbilityType::BLINK:
        teleport_player(player_ptr, 10, TELEPORT_SPONTANEOUS);
        break;
    case MonsterAbilityType::TPORT:
        teleport_player(player_ptr, plev * 5, TELEPORT_SPONTANEOUS);
        break;
    case MonsterAbilityType::WORLD:
        (void)time_walk(player_ptr);
        break;
    case MonsterAbilityType::SPECIAL:
        break;
    case MonsterAbilityType::TELE_TO: {
        const auto pos = target_set(player_ptr, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }

        const auto &floor = *player_ptr->current_floor_ptr;
        const auto &grid_target = floor.get_grid(*pos);
        const auto p_pos = player_ptr->get_position();
        auto should_teleport = grid_target.has_monster();
        should_teleport &= grid_target.has_los();
        should_teleport &= projectable(floor, p_pos, *pos);
        if (!should_teleport) {
            break;
        }

        const auto &monster = floor.m_list[grid_target.m_idx];
        auto &monrace = monster.get_monrace();
        const auto m_name = monster_desc(player_ptr, monster, 0);
        if (monrace.resistance_flags.has(MonsterResistanceType::RESIST_TELEPORT)) {
            if (monrace.kind_flags.has(MonsterKindType::UNIQUE) || monrace.resistance_flags.has(MonsterResistanceType::RESIST_ALL)) {
                if (is_original_ap_and_seen(player_ptr, monster)) {
                    monrace.r_resistance_flags.set(MonsterResistanceType::RESIST_TELEPORT);
                }
                msg_format(_("%sには効果がなかった！", "%s is unaffected!"), m_name.data());

                break;
            } else if (monrace.level > randint1(100)) {
                if (is_original_ap_and_seen(player_ptr, monster)) {
                    monrace.r_resistance_flags.set(MonsterResistanceType::RESIST_TELEPORT);
                }
                msg_format(_("%sには耐性がある！", "%s resists!"), m_name.data());

                break;
            }
        }
        msg_format(_("%sを引き戻した。", "You command %s to return."), m_name.data());

        teleport_monster_to(player_ptr, grid_target.m_idx, player_ptr->y, player_ptr->x, 100, TELEPORT_PASSIVE);
        break;
    }
    case MonsterAbilityType::TELE_AWAY:
        (void)fire_beam(player_ptr, AttributeType::AWAY_ALL, dir, plev);
        break;

    case MonsterAbilityType::TELE_LEVEL:
        return teleport_level_other(player_ptr);
        break;

    case MonsterAbilityType::PSY_SPEAR:
        msg_print(_("光の剣を放った。", "You throw a psycho-spear."));
        (void)fire_beam(player_ptr, AttributeType::PSY_SPEAR, dir, damage);
        break;

    case MonsterAbilityType::DARKNESS:
        msg_print(_("暗闇の中で手を振った。", "You gesture in shadow."));
        (void)unlite_area(player_ptr, 10, 3);
        break;

    case MonsterAbilityType::TRAPS: {
        const auto pos = target_set(player_ptr, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("呪文を唱えて邪悪に微笑んだ。", "You cast a spell and cackle evilly."));
        trap_creation(player_ptr, pos->y, pos->x);
        break;
    }
    case MonsterAbilityType::FORGET:
        msg_print(_("しかし何も起きなかった。", "Nothing happens."));
        break;
    case MonsterAbilityType::RAISE_DEAD:
        msg_print(_("死者復活の呪文を唱えた。", "You animate the dead."));
        (void)animate_dead(player_ptr, 0, player_ptr->y, player_ptr->x);
        break;
    case MonsterAbilityType::S_KIN: {
        const auto pos = target_set(player_ptr, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }

        msg_print(_("援軍を召喚した。", "You summon minions."));
        for (auto k = 0; k < 4; k++) {
            (void)summon_kin_player(player_ptr, plev, pos->y, pos->x, (PM_FORCE_PET | PM_ALLOW_GROUP));
        }
        break;
    }
    case MonsterAbilityType::S_CYBER: {
        int max_cyber = (player_ptr->current_floor_ptr->dun_level / 50) + randint1(3);
        const auto pos = target_set(player_ptr, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("サイバーデーモンを召喚した！", "You summon Cyberdemons!"));
        if (max_cyber > 4) {
            max_cyber = 4;
        }
        for (auto k = 0; k < max_cyber; k++) {
            summon_specific(player_ptr, pos->y, pos->x, plev, SUMMON_CYBER, mode);
        }
        break;
    }
    case MonsterAbilityType::S_MONSTER: {
        const auto pos = target_set(player_ptr, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("仲間を召喚した。", "You summon help."));
        for (auto k = 0; k < 1; k++) {
            summon_specific(player_ptr, pos->y, pos->x, plev, SUMMON_NONE, (mode | u_mode));
        }
        break;
    }
    case MonsterAbilityType::S_MONSTERS: {
        const auto pos = target_set(player_ptr, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("モンスターを召喚した！", "You summon monsters!"));
        for (auto k = 0; k < 6; k++) {
            summon_specific(player_ptr, pos->y, pos->x, plev, SUMMON_NONE, (mode | u_mode));
        }
        break;
    }
    case MonsterAbilityType::S_ANT: {
        const auto pos = target_set(player_ptr, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("アリを召喚した。", "You summon ants."));
        for (auto k = 0; k < 6; k++) {
            summon_specific(player_ptr, pos->y, pos->x, plev, SUMMON_ANT, mode);
        }
        break;
    }
    case MonsterAbilityType::S_SPIDER: {
        const auto pos = target_set(player_ptr, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("蜘蛛を召喚した。", "You summon spiders."));
        for (auto k = 0; k < 6; k++) {
            summon_specific(player_ptr, pos->y, pos->x, plev, SUMMON_SPIDER, mode);
        }
        break;
    }
    case MonsterAbilityType::S_HOUND: {
        const auto pos = target_set(player_ptr, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("ハウンドを召喚した。", "You summon hounds."));
        for (auto k = 0; k < 4; k++) {
            summon_specific(player_ptr, pos->y, pos->x, plev, SUMMON_HOUND, mode);
        }
        break;
    }
    case MonsterAbilityType::S_HYDRA: {
        const auto pos = target_set(player_ptr, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("ヒドラを召喚した。", "You summon hydras."));
        for (auto k = 0; k < 4; k++) {
            summon_specific(player_ptr, pos->y, pos->x, plev, SUMMON_HYDRA, mode);
        }
        break;
    }
    case MonsterAbilityType::S_ANGEL: {
        const auto pos = target_set(player_ptr, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("天使を召喚した！", "You summon an angel!"));
        for (auto k = 0; k < 1; k++) {
            summon_specific(player_ptr, pos->y, pos->x, plev, SUMMON_ANGEL, mode);
        }
        break;
    }
    case MonsterAbilityType::S_DEMON: {
        const auto pos = target_set(player_ptr, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("混沌の宮廷から悪魔を召喚した！", "You summon a demon from the Courts of Chaos!"));
        for (auto k = 0; k < 1; k++) {
            summon_specific(player_ptr, pos->y, pos->x, plev, SUMMON_DEMON, (mode | u_mode));
        }
        break;
    }
    case MonsterAbilityType::S_UNDEAD: {
        const auto pos = target_set(player_ptr, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("アンデッドの強敵を召喚した！", "You summon an undead adversary!"));
        for (auto k = 0; k < 1; k++) {
            summon_specific(player_ptr, pos->y, pos->x, plev, SUMMON_UNDEAD, (mode | u_mode));
        }
        break;
    }
    case MonsterAbilityType::S_DRAGON: {
        const auto pos = target_set(player_ptr, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("ドラゴンを召喚した！", "You summon a dragon!"));
        for (auto k = 0; k < 1; k++) {
            summon_specific(player_ptr, pos->y, pos->x, plev, SUMMON_DRAGON, (mode | u_mode));
        }
        break;
    }
    case MonsterAbilityType::S_HI_UNDEAD: {
        const auto pos = target_set(player_ptr, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("強力なアンデッドを召喚した！", "You summon greater undead!"));
        for (auto k = 0; k < 6; k++) {
            summon_specific(player_ptr, pos->y, pos->x, plev, SUMMON_HI_UNDEAD, (mode | u_mode));
        }
        break;
    }
    case MonsterAbilityType::S_HI_DRAGON: {
        const auto pos = target_set(player_ptr, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("古代ドラゴンを召喚した！", "You summon ancient dragons!"));
        for (auto k = 0; k < 4; k++) {
            summon_specific(player_ptr, pos->y, pos->x, plev, SUMMON_HI_DRAGON, (mode | u_mode));
        }
        break;
    }
    case MonsterAbilityType::S_AMBERITES: {
        const auto pos = target_set(player_ptr, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("アンバーの王族を召喚した！", "You summon Lords of Amber!"));
        for (auto k = 0; k < 4; k++) {
            summon_specific(player_ptr, pos->y, pos->x, plev, SUMMON_AMBERITES, (mode | PM_ALLOW_UNIQUE));
        }
        break;
    }
    case MonsterAbilityType::S_UNIQUE: {
        int count = 0;
        const auto pos = target_set(player_ptr, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("特別な強敵を召喚した！", "You summon special opponents!"));
        for (auto k = 0; k < 4; k++) {
            if (summon_specific(player_ptr, pos->y, pos->x, plev, SUMMON_UNIQUE, (mode | PM_ALLOW_UNIQUE))) {
                count++;
            }
        }
        for (auto k = count; k < 4; k++) {
            summon_specific(player_ptr, pos->y, pos->x, plev, SUMMON_HI_UNDEAD, (mode | u_mode));
        }
        break;
    }
    case MonsterAbilityType::S_DEAD_UNIQUE: {
        const auto pos = target_set(player_ptr, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("特別な強敵を蘇生した！", "You summon special dead opponents!"));
        for (auto k = 0; k < 4; k++) {
            summon_specific(player_ptr, pos->y, pos->x, plev, SUMMON_DEAD_UNIQUE, (mode | PM_ALLOW_UNIQUE | PM_CLONE));
        }
        break;
    }
    default:
        msg_print("hoge?");
    }

    return true;
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
bool do_cmd_mane(PlayerType *player_ptr, bool baigaesi)
{
    int n = 0;
    PERCENTAGE chance;
    PERCENTAGE minfail = 0;
    PLAYER_LEVEL plev = player_ptr->lev;
    monster_power spell;
    bool cast;

    if (cmd_limit_confused(player_ptr)) {
        return false;
    }

    auto mane_data = PlayerClass(player_ptr).get_specific_data<mane_data_type>();

    if (mane_data->mane_list.empty()) {
        msg_print(_("まねられるものが何もない！", "You don't remember any action!"));
        return false;
    }

    if (!get_mane_power(player_ptr, &n, baigaesi)) {
        return false;
    }

    spell = monster_powers.at(mane_data->mane_list[n].spell);

    /* Spell failure chance */
    chance = spell.manefail;

    /* Reduce failure rate by "effective" level adjustment */
    if (plev > spell.level) {
        chance -= 3 * (plev - spell.level);
    }

    /* Reduce failure rate by 1 stat and DEX adjustment */
    chance -= 3 * (adj_mag_stat[player_ptr->stat_index[spell.use_stat]] + adj_mag_stat[player_ptr->stat_index[A_DEX]] - 2) / 2;

    if (spell.manedam) {
        chance = chance * damage / spell.manedam;
    }

    chance += player_ptr->to_m_chance;

    /* Extract the minimum failure rate */
    minfail = adj_mag_fail[player_ptr->stat_index[spell.use_stat]];

    /* Minimum failure rate */
    if (chance < minfail) {
        chance = minfail;
    }

    chance += player_ptr->effects()->stun().get_magic_chance_penalty();
    if (chance > 95) {
        chance = 95;
    }

    /* Failed spell */
    if (evaluate_percent(chance)) {
        if (flush_failure) {
            flush();
        }
        msg_print(_("ものまねに失敗した！", "You failed to concentrate hard enough!"));
        sound(SoundKind::FAIL);
    } else {
        sound(SoundKind::ZAP);
        cast = use_mane(player_ptr, mane_data->mane_list[n].spell);
        if (!cast) {
            return false;
        }
    }

    mane_data->mane_list.erase(std::next(mane_data->mane_list.begin(), n));
    PlayerEnergy(player_ptr).set_player_turn_energy(100);
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::IMITATION);
    static constexpr auto flags = {
        SubWindowRedrawingFlag::PLAYER,
        SubWindowRedrawingFlag::SPELL,
    };
    rfu.set_flags(flags);
    return true;
}
