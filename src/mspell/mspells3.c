/*!
 * @brief 青魔法の処理実装 / Blue magic
 * @date 2014/01/15
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * 2014 Deskull rearranged comment for Doxygen.
 */

#include "mspell/mspells3.h"
#include "action/action-limited.h"
#include "blue-magic/learnt-info.h"
#include "blue-magic/learnt-power-getter.h"
#include "cmd-action/cmd-spell.h"
#include "core/asking-player.h"
#include "core/hp-mp-processor.h"
#include "core/player-redraw-types.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "floor/floor.h"
#include "game-option/disturbance-options.h"
#include "game-option/input-options.h"
#include "game-option/text-display-options.h"
#include "grid/grid.h"
#include "io/command-repeater.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "io/targeting.h"
#include "lore/lore-calculator.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/mind-blue-mage.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-ability1.h"
#include "monster-race/race-flags-ability2.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags4.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "mspell/monster-power-table.h"
#include "mspell/mspell-damage-calculator.h"
#include "mspell/mspell-mask-definitions.h"
#include "mspell/mspell-type.h"
#include "player/attack-defense-types.h"
#include "player/avatar.h"
#include "player/player-status.h"
#include "realm/realm-types.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell/spell-info.h"
#include "spell/spell-types.h"
#include "spell/spells-status.h"
#include "spell/spells-summon.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "status/body-improvement.h"
#include "status/buff-setter.h"
#include "status/experience.h"
#include "system/floor-type-definition.h"
#include "term/screen-processor.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"

/*!
 * @brief 青魔法の発動 /
 * do_cmd_cast calls this function if the player's class is 'blue-mage'.
 * @param spell 発動するモンスター攻撃のID
 * @param success TRUEは成功時、FALSEは失敗時の処理を行う
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
 */
bool cast_learned_spell(player_type *caster_ptr, int spell, bool success)
{
    DIRECTION dir;
    PLAYER_LEVEL plev = get_pseudo_monstetr_level(caster_ptr);
    PLAYER_LEVEL summon_lev = caster_ptr->lev * 2 / 3 + randint1(caster_ptr->lev / 2);
    HIT_POINT damage = 0;
    bool pet = success;
    bool no_trump = FALSE;
    BIT_FLAGS p_mode, u_mode = 0L, g_mode;

    if (pet) {
        p_mode = PM_FORCE_PET;
        g_mode = 0;
    } else {
        p_mode = PM_NO_PET;
        g_mode = PM_ALLOW_GROUP;
    }

    if (!success || (randint1(50 + plev) < plev / 10))
        u_mode = PM_ALLOW_UNIQUE;

    floor_type *floor_ptr = caster_ptr->current_floor_ptr;
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
        m_idx = floor_ptr->grid_array[target_row][target_col].m_idx;
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

        msg_print(_("ロケットを発射した。", "You fire a rocket."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_ROCKET), plev, DAM_ROLL);
        fire_rocket(caster_ptr, GF_ROCKET, dir, damage, 2);
        break;
    case MS_SHOOT: {
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("矢を放った。", "You fire an arrow."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_SHOOT), plev, DAM_ROLL);
        fire_bolt(caster_ptr, GF_ARROW, dir, damage);
        break;
    }
    case MS_XXX2:
        break;
    case MS_XXX3:
        break;
    case MS_XXX4:
        break;
    case MS_BR_ACID:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("酸のブレスを吐いた。", "You breathe acid."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_ACID), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_ACID, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BR_ELEC:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("稲妻のブレスを吐いた。", "You breathe lightning."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_ELEC), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_ELEC, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BR_FIRE:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("火炎のブレスを吐いた。", "You breathe fire."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_FIRE), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_FIRE, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BR_COLD:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("冷気のブレスを吐いた。", "You breathe frost."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_COLD), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_COLD, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BR_POIS:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("ガスのブレスを吐いた。", "You breathe gas."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_POIS), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_POIS, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BR_NETHER:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("地獄のブレスを吐いた。", "You breathe nether."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_NETHER), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_NETHER, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BR_LITE:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("閃光のブレスを吐いた。", "You breathe light."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_LITE), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_LITE, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BR_DARK:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("暗黒のブレスを吐いた。", "You breathe darkness."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_DARK), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_DARK, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BR_CONF:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("混乱のブレスを吐いた。", "You breathe confusion."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_CONF), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_CONFUSION, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BR_SOUND:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("轟音のブレスを吐いた。", "You breathe sound."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_SOUND), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_SOUND, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BR_CHAOS:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("カオスのブレスを吐いた。", "You breathe chaos."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_CHAOS), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_CHAOS, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BR_DISEN:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("劣化のブレスを吐いた。", "You breathe disenchantment."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_DISEN), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_DISENCHANT, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BR_NEXUS:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("因果混乱のブレスを吐いた。", "You breathe nexus."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_NEXUS), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_NEXUS, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BR_TIME:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("時間逆転のブレスを吐いた。", "You breathe time."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_TIME), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_TIME, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BR_INERTIA:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("遅鈍のブレスを吐いた。", "You breathe inertia."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_INERTIA), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_INERTIAL, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BR_GRAVITY:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("重力のブレスを吐いた。", "You breathe gravity."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_GRAVITY), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_GRAVITY, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BR_SHARDS:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("破片のブレスを吐いた。", "You breathe shards."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_SHARDS), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_SHARDS, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BR_PLASMA:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("プラズマのブレスを吐いた。", "You breathe plasma."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_PLASMA), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_PLASMA, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BR_FORCE:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("フォースのブレスを吐いた。", "You breathe force."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_FORCE), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_FORCE, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BR_MANA:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("魔力のブレスを吐いた。", "You breathe mana."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_MANA), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_MANA, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BALL_NUKE:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("放射能球を放った。", "You cast a ball of radiation."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_NUKE), plev, DAM_ROLL);
        fire_ball(caster_ptr, GF_NUKE, dir, damage, 2);
        break;
    case MS_BR_NUKE:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("放射性廃棄物のブレスを吐いた。", "You breathe toxic waste."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_NUKE), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_NUKE, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BALL_CHAOS:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("純ログルスを放った。", "You invoke a raw Logrus."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_CHAOS), plev, DAM_ROLL);
        fire_ball(caster_ptr, GF_CHAOS, dir, damage, 4);
        break;
    case MS_BR_DISI:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("分解のブレスを吐いた。", "You breathe disintegration."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_DISI), plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_DISINTEGRATE, dir, damage, (plev > 40 ? 3 : 2));
        break;
    case MS_BALL_ACID:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("アシッド・ボールの呪文を唱えた。", "You cast an acid ball."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_ACID), plev, DAM_ROLL);
        fire_ball(caster_ptr, GF_ACID, dir, damage, 2);
        break;
    case MS_BALL_ELEC:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("サンダー・ボールの呪文を唱えた。", "You cast a lightning ball."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_ELEC), plev, DAM_ROLL);
        fire_ball(caster_ptr, GF_ELEC, dir, damage, 2);
        break;
    case MS_BALL_FIRE:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("ファイア・ボールの呪文を唱えた。", "You cast a fire ball."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_FIRE), plev, DAM_ROLL);
        fire_ball(caster_ptr, GF_FIRE, dir, damage, 2);
        break;
    case MS_BALL_COLD:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("アイス・ボールの呪文を唱えた。", "You cast a frost ball."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_COLD), plev, DAM_ROLL);
        fire_ball(caster_ptr, GF_COLD, dir, damage, 2);
        break;
    case MS_BALL_POIS:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("悪臭雲の呪文を唱えた。", "You cast a stinking cloud."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_POIS), plev, DAM_ROLL);
        fire_ball(caster_ptr, GF_POIS, dir, damage, 2);
        break;
    case MS_BALL_NETHER:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("地獄球の呪文を唱えた。", "You cast a nether ball."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_NETHER), plev, DAM_ROLL);
        fire_ball(caster_ptr, GF_NETHER, dir, damage, 2);
        break;
    case MS_BALL_WATER:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("流れるような身振りをした。", "You gesture fluidly."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_WATER), plev, DAM_ROLL);
        fire_ball(caster_ptr, GF_WATER, dir, damage, 4);
        break;
    case MS_BALL_MANA:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("魔力の嵐の呪文を念じた。", "You invoke a mana storm."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_MANA), plev, DAM_ROLL);
        fire_ball(caster_ptr, GF_MANA, dir, damage, 4);
        break;
    case MS_BALL_DARK:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("暗黒の嵐の呪文を念じた。", "You invoke a darkness storm."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_DARK), plev, DAM_ROLL);
        fire_ball(caster_ptr, GF_DARK, dir, damage, 4);
        break;
    case MS_DRAIN_MANA:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        damage = monspell_bluemage_damage(caster_ptr, (MS_DRAIN_MANA), plev, DAM_ROLL);
        fire_ball_hide(caster_ptr, GF_DRAIN_MANA, dir, damage, 0);
        break;
    case MS_MIND_BLAST:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        damage = monspell_bluemage_damage(caster_ptr, (MS_MIND_BLAST), plev, DAM_ROLL);
        fire_ball_hide(caster_ptr, GF_MIND_BLAST, dir, damage, 0);
        break;
    case MS_BRAIN_SMASH:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        damage = monspell_bluemage_damage(caster_ptr, (MS_BRAIN_SMASH), plev, DAM_ROLL);
        fire_ball_hide(caster_ptr, GF_BRAIN_SMASH, dir, damage, 0);
        break;
    case MS_CAUSE_1:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        damage = monspell_bluemage_damage(caster_ptr, (MS_CAUSE_1), plev, DAM_ROLL);
        fire_ball_hide(caster_ptr, GF_CAUSE_1, dir, damage, 0);
        break;
    case MS_CAUSE_2:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        damage = monspell_bluemage_damage(caster_ptr, (MS_CAUSE_2), plev, DAM_ROLL);
        fire_ball_hide(caster_ptr, GF_CAUSE_2, dir, damage, 0);
        break;
    case MS_CAUSE_3:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        damage = monspell_bluemage_damage(caster_ptr, (MS_CAUSE_3), plev, DAM_ROLL);
        fire_ball_hide(caster_ptr, GF_CAUSE_3, dir, damage, 0);
        break;
    case MS_CAUSE_4:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        damage = monspell_bluemage_damage(caster_ptr, (MS_CAUSE_4), plev, DAM_ROLL);
        fire_ball_hide(caster_ptr, GF_CAUSE_4, dir, damage, 0);
        break;
    case MS_BOLT_ACID:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("アシッド・ボルトの呪文を唱えた。", "You cast an acid bolt."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BOLT_ACID), plev, DAM_ROLL);
        fire_bolt(caster_ptr, GF_ACID, dir, damage);
        break;
    case MS_BOLT_ELEC:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("サンダー・ボルトの呪文を唱えた。", "You cast a lightning bolt."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BOLT_ELEC), plev, DAM_ROLL);
        fire_bolt(caster_ptr, GF_ELEC, dir, damage);
        break;
    case MS_BOLT_FIRE:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("ファイア・ボルトの呪文を唱えた。", "You cast a fire bolt."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BOLT_FIRE), plev, DAM_ROLL);
        fire_bolt(caster_ptr, GF_FIRE, dir, damage);
        break;
    case MS_BOLT_COLD:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("アイス・ボルトの呪文を唱えた。", "You cast a frost bolt."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BOLT_COLD), plev, DAM_ROLL);
        fire_bolt(caster_ptr, GF_COLD, dir, damage);
        break;
    case MS_STARBURST:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("スターバーストの呪文を念じた。", "You invoke a starburst."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_STARBURST), plev, DAM_ROLL);
        fire_ball(caster_ptr, GF_LITE, dir, damage, 4);
        break;
    case MS_BOLT_NETHER:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("地獄の矢の呪文を唱えた。", "You cast a nether bolt."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BOLT_NETHER), plev, DAM_ROLL);
        fire_bolt(caster_ptr, GF_NETHER, dir, damage);
        break;
    case MS_BOLT_WATER:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("ウォーター・ボルトの呪文を唱えた。", "You cast a water bolt."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BOLT_WATER), plev, DAM_ROLL);
        fire_bolt(caster_ptr, GF_WATER, dir, damage);
        break;
    case MS_BOLT_MANA:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("魔力の矢の呪文を唱えた。", "You cast a mana bolt."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BOLT_MANA), plev, DAM_ROLL);
        fire_bolt(caster_ptr, GF_MANA, dir, damage);
        break;
    case MS_BOLT_PLASMA:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("プラズマ・ボルトの呪文を唱えた。", "You cast a plasma bolt."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BOLT_PLASMA), plev, DAM_ROLL);
        fire_bolt(caster_ptr, GF_PLASMA, dir, damage);
        break;
    case MS_BOLT_ICE:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("極寒の矢の呪文を唱えた。", "You cast a ice bolt."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BOLT_ICE), plev, DAM_ROLL);
        fire_bolt(caster_ptr, GF_ICE, dir, damage);
        break;
    case MS_MAGIC_MISSILE:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("マジック・ミサイルの呪文を唱えた。", "You cast a magic missile."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_MAGIC_MISSILE), plev, DAM_ROLL);
        fire_bolt(caster_ptr, GF_MISSILE, dir, damage);
        break;
    case MS_SCARE:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

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

        msg_print(_("<破滅の手>を放った！", "You invoke the Hand of Doom!"));
        fire_ball_hide(caster_ptr, GF_HAND_DOOM, dir, plev * 3, 0);
        break;
    }
    case MS_HEAL:
        msg_print(_("自分の傷に念を集中した。", "You concentrate on your wounds!"));
        (void)hp_player(caster_ptr, plev * 4);
        (void)set_stun(caster_ptr, 0);
        (void)set_cut(caster_ptr, 0);
        break;
    case MS_INVULNER:
        msg_print(_("無傷の球の呪文を唱えた。", "You cast a Globe of Invulnerability."));
        (void)set_invuln(caster_ptr, randint1(4) + 4, FALSE);
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
        if (!floor_ptr->grid_array[target_row][target_col].m_idx)
            break;
        if (!player_has_los_bold(caster_ptr, target_row, target_col))
            break;
        if (!projectable(caster_ptr, caster_ptr->y, caster_ptr->x, target_row, target_col))
            break;
        m_ptr = &floor_ptr->m_list[floor_ptr->grid_array[target_row][target_col].m_idx];
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
        teleport_monster_to(caster_ptr, floor_ptr->grid_array[target_row][target_col].m_idx, caster_ptr->y, caster_ptr->x, 100, TELEPORT_PASSIVE);
        break;
    }
    case MS_TELE_AWAY:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        (void)fire_beam(caster_ptr, GF_AWAY_ALL, dir, 100);
        break;

    case MS_TELE_LEVEL:
        return teleport_level_other(caster_ptr);
        break;

    case MS_PSY_SPEAR:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        msg_print(_("光の剣を放った。", "You throw a psycho-spear."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_PSY_SPEAR), plev, DAM_ROLL);
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
        msg_print(_("しかし何も起きなかった。", "Nothing happen."));
        break;
    case MS_RAISE_DEAD:
        msg_print(_("死者復活の呪文を唱えた。", "You animate the dead."));
        (void)animate_dead(caster_ptr, 0, caster_ptr->y, caster_ptr->x);
        break;
    case MS_S_KIN: {
        msg_print(_("援軍を召喚した。", "You summon one of your kin."));
        for (int k = 0; k < 1; k++) {
            if (summon_kin_player(caster_ptr, summon_lev, caster_ptr->y, caster_ptr->x, (pet ? PM_FORCE_PET : 0L))) {
                if (!pet)
                    msg_print(_("召喚された仲間は怒っている！", "The summoned companion is angry!"));
            } else {
                no_trump = TRUE;
            }
        }

        break;
    }
    case MS_S_CYBER: {
        msg_print(_("サイバーデーモンを召喚した！", "You summon a Cyberdemon!"));
        for (int k = 0; k < 1; k++) {
            if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_CYBER, p_mode)) {
                if (!pet)
                    msg_print(_("召喚されたサイバーデーモンは怒っている！", "The summoned Cyberdemon are angry!"));
            } else {
                no_trump = TRUE;
            }
        }
        break;
    }
    case MS_S_MONSTER: {
        msg_print(_("仲間を召喚した。", "You summon help."));
        for (int k = 0; k < 1; k++) {
            if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, 0, p_mode)) {
                if (!pet)
                    msg_print(_("召喚されたモンスターは怒っている！", "The summoned monster is angry!"));
            } else {
                no_trump = TRUE;
            }
        }

        break;
    }
    case MS_S_MONSTERS: {
        msg_print(_("モンスターを召喚した！", "You summon monsters!"));
        for (int k = 0; k < plev / 15 + 2; k++) {
            if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, 0, (p_mode | u_mode))) {
                if (!pet)
                    msg_print(_("召喚されたモンスターは怒っている！", "The summoned monsters are angry!"));
            } else {
                no_trump = TRUE;
            }
        }

        break;
    }
    case MS_S_ANT: {
        msg_print(_("アリを召喚した。", "You summon ants."));
        if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_ANT, (PM_ALLOW_GROUP | p_mode))) {
            if (!pet)
                msg_print(_("召喚されたアリは怒っている！", "The summoned ants are angry!"));
        } else {
            no_trump = TRUE;
        }
        break;
    }
    case MS_S_SPIDER: {
        msg_print(_("蜘蛛を召喚した。", "You summon spiders."));
        if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_SPIDER, (PM_ALLOW_GROUP | p_mode))) {
            if (!pet)
                msg_print(_("召喚された蜘蛛は怒っている！", "Summoned spiders are angry!"));
        } else {
            no_trump = TRUE;
        }

        break;
    }
    case MS_S_HOUND: {
        msg_print(_("ハウンドを召喚した。", "You summon hounds."));
        if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_HOUND, (PM_ALLOW_GROUP | p_mode))) {
            if (!pet)
                msg_print(_("召喚されたハウンドは怒っている！", "Summoned hounds are angry!"));
        } else {
            no_trump = TRUE;
        }

        break;
    }
    case MS_S_HYDRA: {
        msg_print(_("ヒドラを召喚した。", "You summon a hydras."));
        if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_HYDRA, (g_mode | p_mode))) {
            if (!pet)
                msg_print(_("召喚されたヒドラは怒っている！", "Summoned hydras are angry!"));
        } else {
            no_trump = TRUE;
        }

        break;
    }
    case MS_S_ANGEL: {
        msg_print(_("天使を召喚した！", "You summon an angel!"));
        if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_ANGEL, (g_mode | p_mode))) {
            if (!pet)
                msg_print(_("召喚された天使は怒っている！", "The summoned angel is angry!"));
        } else {
            no_trump = TRUE;
        }
        break;
    }
    case MS_S_DEMON: {
        msg_print(_("混沌の宮廷から悪魔を召喚した！", "You summon a demon from the Courts of Chaos!"));
        if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_DEMON, (g_mode | p_mode))) {
            if (!pet)
                msg_print(_("召喚されたデーモンは怒っている！", "The summoned demon is angry!"));
        } else {
            no_trump = TRUE;
        }
        break;
    }
    case MS_S_UNDEAD: {
        msg_print(_("アンデッドの強敵を召喚した！", "You summon an undead adversary!"));
        if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_UNDEAD, (g_mode | p_mode))) {
            if (!pet)
                msg_print(_("召喚されたアンデッドは怒っている！", "The summoned undead is angry!"));
        } else {
            no_trump = TRUE;
        }
        break;
    }
    case MS_S_DRAGON: {
        msg_print(_("ドラゴンを召喚した！", "You summon a dragon!"));
        if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_DRAGON, (g_mode | p_mode))) {
            if (!pet)
                msg_print(_("召喚されたドラゴンは怒っている！", "The summoned dragon is angry!"));
        } else {
            no_trump = TRUE;
        }
        break;
    }
    case MS_S_HI_UNDEAD: {
        msg_print(_("強力なアンデッドを召喚した！", "You summon a greater undead!"));
        if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_HI_UNDEAD, (g_mode | p_mode | u_mode))) {
            if (!pet)
                msg_print(_("召喚された上級アンデッドは怒っている！", "The summoned greater undead is angry!"));
        } else {
            no_trump = TRUE;
        }
        break;
    }
    case MS_S_HI_DRAGON: {
        msg_print(_("古代ドラゴンを召喚した！", "You summon an ancient dragon!"));
        if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_HI_DRAGON, (g_mode | p_mode | u_mode))) {
            if (!pet)
                msg_print(_("召喚された古代ドラゴンは怒っている！", "The summoned ancient dragon is angry!"));
        } else {
            no_trump = TRUE;
        }
        break;
    }
    case MS_S_AMBERITE: {
        msg_print(_("アンバーの王族を召喚した！", "You summon a Lord of Amber!"));
        if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_AMBERITES, (g_mode | p_mode | u_mode))) {
            if (!pet)
                msg_print(_("召喚されたアンバーの王族は怒っている！", "The summoned Lord of Amber is angry!"));
        } else {
            no_trump = TRUE;
        }
        break;
    }
    case MS_S_UNIQUE: {
        int k, count = 0;
        msg_print(_("特別な強敵を召喚した！", "You summon a special opponent!"));
        for (k = 0; k < 1; k++) {
            if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_UNIQUE, (g_mode | p_mode | PM_ALLOW_UNIQUE))) {
                count++;
                if (!pet)
                    msg_print(_("召喚されたユニーク・モンスターは怒っている！", "The summoned special opponent is angry!"));
            }
        }

        for (k = count; k < 1; k++) {
            if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_HI_UNDEAD, (g_mode | p_mode | PM_ALLOW_UNIQUE))) {
                count++;
                if (!pet)
                    msg_print(_("召喚された上級アンデッドは怒っている！", "The summoned greater undead is angry!"));
            }
        }

        if (!count) {
            no_trump = TRUE;
        }

        break;
    }
    default:
        msg_print("hoge?");
    }

    if (no_trump) {
        msg_print(_("何も現れなかった。", "No one appeared."));
    }

    return TRUE;
}

/*!
 * @brief 青魔法のラーニング判定と成功した場合のラーニング処理
 * @param monspell ラーニングを試みるモンスター攻撃のID
 * @return なし
 */
void learn_spell(player_type *learner_ptr, int monspell)
{
    if (learner_ptr->action != ACTION_LEARN)
        return;
    if (monspell < 0)
        return;
    if (learner_ptr->magic_num2[monspell])
        return;
    if (learner_ptr->confused || learner_ptr->blind || learner_ptr->image || learner_ptr->stun || learner_ptr->paralyzed)
        return;
    if (randint1(learner_ptr->lev + 70) > monster_powers[monspell].level + 40) {
        learner_ptr->magic_num2[monspell] = 1;
        msg_format(_("%sを学習した！", "You have learned %s!"), monster_powers[monspell].name);
        gain_exp(learner_ptr, monster_powers[monspell].level * monster_powers[monspell].smana);

        sound(SOUND_STUDY);

        learner_ptr->new_mane = TRUE;
        learner_ptr->redraw |= (PR_STATE);
    }
}

/*!
 * todo f4, f5, f6を構造体にまとめ直す
 * @brief モンスター特殊能力のフラグ配列から特定条件の魔法だけを抜き出す処理
 * Extract monster spells mask for the given mode
 * @param f4 モンスター特殊能力の4番目のフラグ配列
 * @param f5 モンスター特殊能力の5番目のフラグ配列
 * @param f6 モンスター特殊能力の6番目のフラグ配列
 * @param mode 抜き出したい条件
 * @return なし
 */
/*
 */
void set_rf_masks(BIT_FLAGS *f4, BIT_FLAGS *f5, BIT_FLAGS *f6, BIT_FLAGS mode)
{
    switch (mode) {
    case MONSPELL_TYPE_BOLT:
        *f4 = ((RF4_BOLT_MASK | RF4_BEAM_MASK) & ~(RF4_ROCKET));
        *f5 = RF5_BOLT_MASK | RF5_BEAM_MASK;
        *f6 = RF6_BOLT_MASK | RF6_BEAM_MASK;
        break;

    case MONSPELL_TYPE_BALL:
        *f4 = (RF4_BALL_MASK & ~(RF4_BREATH_MASK));
        *f5 = (RF5_BALL_MASK & ~(RF5_BREATH_MASK));
        *f6 = (RF6_BALL_MASK & ~(RF6_BREATH_MASK));
        break;

    case MONSPELL_TYPE_BREATH:
        *f4 = (BIT_FLAGS)RF4_BREATH_MASK;
        *f5 = RF5_BREATH_MASK;
        *f6 = RF6_BREATH_MASK;
        break;

    case MONSPELL_TYPE_SUMMON:
        *f4 = RF4_SUMMON_MASK;
        *f5 = RF5_SUMMON_MASK;
        *f6 = (BIT_FLAGS)RF6_SUMMON_MASK;
        break;

    case MONSPELL_TYPE_OTHER:
        *f4 = RF4_ATTACK_MASK & ~(RF4_BOLT_MASK | RF4_BEAM_MASK | RF4_BALL_MASK | RF4_INDIRECT_MASK);
        *f5 = RF5_ATTACK_MASK & ~(RF5_BOLT_MASK | RF5_BEAM_MASK | RF5_BALL_MASK | RF5_INDIRECT_MASK);
        *f6 = RF6_ATTACK_MASK & ~(RF6_BOLT_MASK | RF6_BEAM_MASK | RF6_BALL_MASK | RF6_INDIRECT_MASK);
        break;
    }
}
