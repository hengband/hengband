#include "blue-magic/blue-magic-caster.h"
#include "blue-magic/blue-magic-util.h"
#include "blue-magic/learnt-info.h"
#include "core/hp-mp-processor.h"
#include "floor/floor.h"
#include "grid/grid.h"
#include "io/targeting.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags1.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "mspell/mspell-damage-calculator.h"
#include "mspell/mspell-type.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell/spell-types.h"
#include "spell/spells-status.h"
#include "spell/spells-summon.h"
#include "status/bad-status-setter.h"
#include "status/body-improvement.h"
#include "status/buff-setter.h"
#include "system/floor-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief 青魔法の叫び
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return 常にTRUE
 */
static bool cast_blue_shriek(player_type *caster_ptr)
{
    msg_print(_("かん高い金切り声をあげた。", "You make a high pitched shriek."));
    aggravate_monsters(caster_ptr, 0);
    return TRUE;
}

static bool cast_blue_dispel(player_type *caster_ptr)
{
    if (!target_set(caster_ptr, TARGET_KILL))
        return FALSE;

    MONSTER_IDX m_idx = caster_ptr->current_floor_ptr->grid_array[target_row][target_col].m_idx;
    if ((m_idx == 0) || !player_has_los_bold(caster_ptr, target_row, target_col)
        || !projectable(caster_ptr, caster_ptr->y, caster_ptr->x, target_row, target_col))
        return TRUE;

    dispel_monster_status(caster_ptr, m_idx);
    return TRUE;
}

static bool cast_blue_rocket(player_type *caster_ptr, blue_magic_type *bm_ptr)
{
    if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
        return FALSE;

    msg_print(_("ロケットを発射した。", "You fire a rocket."));
    bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_ROCKET), bm_ptr->plev, DAM_ROLL);
    fire_rocket(caster_ptr, GF_ROCKET, bm_ptr->dir, bm_ptr->damage, 2);
    return TRUE;
}

static bool cast_blue_shoot(player_type *caster_ptr, blue_magic_type *bm_ptr)
{
    if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
        return FALSE;

    msg_print(_("矢を放った。", "You fire an arrow."));
    bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_SHOOT), bm_ptr->plev, DAM_ROLL);
    fire_bolt(caster_ptr, GF_ARROW, bm_ptr->dir, bm_ptr->damage);
    return TRUE;
}

static bool cast_blue_breath_acid(player_type *caster_ptr, blue_magic_type *bm_ptr)
{
    if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
        return FALSE;

    msg_print(_("酸のブレスを吐いた。", "You breathe acid."));
    bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BR_ACID), bm_ptr->plev, DAM_ROLL);
    fire_breath(caster_ptr, GF_ACID, bm_ptr->dir, bm_ptr->damage, (bm_ptr->plev > 40 ? 3 : 2));
    return TRUE;
}

static bool cast_blue_breath_elec(player_type *caster_ptr, blue_magic_type *bm_ptr)
{
    if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
        return FALSE;

    msg_print(_("稲妻のブレスを吐いた。", "You breathe lightning."));
    bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BR_ELEC), bm_ptr->plev, DAM_ROLL);
    fire_breath(caster_ptr, GF_ELEC, bm_ptr->dir, bm_ptr->damage, (bm_ptr->plev > 40 ? 3 : 2));
    return TRUE;
}

static bool cast_blue_breath_fire(player_type *caster_ptr, blue_magic_type *bm_ptr)
{
    if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
        return FALSE;

    msg_print(_("火炎のブレスを吐いた。", "You breathe fire."));
    bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BR_FIRE), bm_ptr->plev, DAM_ROLL);
    fire_breath(caster_ptr, GF_FIRE, bm_ptr->dir, bm_ptr->damage, (bm_ptr->plev > 40 ? 3 : 2));
    return TRUE;
}

static bool cast_blue_breath_cold(player_type *caster_ptr, blue_magic_type *bm_ptr)
{
    if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
        return FALSE;

    msg_print(_("冷気のブレスを吐いた。", "You breathe frost."));
    bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BR_COLD), bm_ptr->plev, DAM_ROLL);
    fire_breath(caster_ptr, GF_COLD, bm_ptr->dir, bm_ptr->damage, (bm_ptr->plev > 40 ? 3 : 2));
    return TRUE;
}

static bool cast_blue_breath_pois(player_type *caster_ptr, blue_magic_type *bm_ptr)
{
    if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
        return FALSE;

    msg_print(_("ガスのブレスを吐いた。", "You breathe gas."));
    bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BR_POIS), bm_ptr->plev, DAM_ROLL);
    fire_breath(caster_ptr, GF_POIS, bm_ptr->dir, bm_ptr->damage, (bm_ptr->plev > 40 ? 3 : 2));
    return TRUE;
}

/*!
 * @brief 青魔法の発動 /
 * do_cmd_cast calls this function if the player's class is 'blue-mage'.
 * @param spell 発動するモンスター攻撃のID
 * @param success TRUEは成功時、FALSEは失敗時の処理を行う
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
 */
bool cast_learned_spell(player_type *caster_ptr, int spell, const bool success)
{
    blue_magic_type tmp_bm;
    blue_magic_type *bm_ptr = initialize_blue_magic_type(caster_ptr, &tmp_bm, success, get_pseudo_monstetr_level);
    floor_type *floor_ptr = caster_ptr->current_floor_ptr;
    switch (spell) {
    case MS_SHRIEK:
        (void)cast_blue_shriek(caster_ptr);
        break;
    case MS_XXX1:
    case MS_XXX2:
    case MS_XXX3:
    case MS_XXX4:
        break;
    case MS_DISPEL:
        if (!cast_blue_dispel(caster_ptr))
            return FALSE;

        break;
    case MS_ROCKET:
        if (!cast_blue_rocket(caster_ptr, bm_ptr))
            return FALSE;

        break;
    case MS_SHOOT:
        if (!cast_blue_shoot(caster_ptr, bm_ptr))
            return FALSE;

        break;
    case MS_BR_ACID:
        if (!cast_blue_breath_acid(caster_ptr, bm_ptr))
            return FALSE;

        break;
    case MS_BR_ELEC:
        if (!cast_blue_breath_elec(caster_ptr, bm_ptr))
            return FALSE;

        break;
    case MS_BR_FIRE:
        if (!cast_blue_breath_fire(caster_ptr, bm_ptr))
            return FALSE;

        break;
    case MS_BR_COLD:
        if (!cast_blue_breath_cold(caster_ptr, bm_ptr))
            return FALSE;

        break;
    case MS_BR_POIS:
        if (!cast_blue_breath_pois(caster_ptr, bm_ptr))
            return FALSE;

        break;
    case MS_BR_NETHER:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("地獄のブレスを吐いた。", "You breathe nether."));
        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BR_NETHER), bm_ptr->plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_NETHER, bm_ptr->dir, bm_ptr->damage, (bm_ptr->plev > 40 ? 3 : 2));
        break;
    case MS_BR_LITE:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("閃光のブレスを吐いた。", "You breathe light."));
        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BR_LITE), bm_ptr->plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_LITE, bm_ptr->dir, bm_ptr->damage, (bm_ptr->plev > 40 ? 3 : 2));
        break;
    case MS_BR_DARK:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("暗黒のブレスを吐いた。", "You breathe darkness."));
        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BR_DARK), bm_ptr->plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_DARK, bm_ptr->dir, bm_ptr->damage, (bm_ptr->plev > 40 ? 3 : 2));
        break;
    case MS_BR_CONF:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("混乱のブレスを吐いた。", "You breathe confusion."));
        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BR_CONF), bm_ptr->plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_CONFUSION, bm_ptr->dir, bm_ptr->damage, (bm_ptr->plev > 40 ? 3 : 2));
        break;
    case MS_BR_SOUND:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("轟音のブレスを吐いた。", "You breathe sound."));
        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BR_SOUND), bm_ptr->plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_SOUND, bm_ptr->dir, bm_ptr->damage, (bm_ptr->plev > 40 ? 3 : 2));
        break;
    case MS_BR_CHAOS:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("カオスのブレスを吐いた。", "You breathe chaos."));
        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BR_CHAOS), bm_ptr->plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_CHAOS, bm_ptr->dir, bm_ptr->damage, (bm_ptr->plev > 40 ? 3 : 2));
        break;
    case MS_BR_DISEN:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("劣化のブレスを吐いた。", "You breathe disenchantment."));
        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BR_DISEN), bm_ptr->plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_DISENCHANT, bm_ptr->dir, bm_ptr->damage, (bm_ptr->plev > 40 ? 3 : 2));
        break;
    case MS_BR_NEXUS:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("因果混乱のブレスを吐いた。", "You breathe nexus."));
        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BR_NEXUS), bm_ptr->plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_NEXUS, bm_ptr->dir, bm_ptr->damage, (bm_ptr->plev > 40 ? 3 : 2));
        break;
    case MS_BR_TIME:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("時間逆転のブレスを吐いた。", "You breathe time."));
        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BR_TIME), bm_ptr->plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_TIME, bm_ptr->dir, bm_ptr->damage, (bm_ptr->plev > 40 ? 3 : 2));
        break;
    case MS_BR_INERTIA:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("遅鈍のブレスを吐いた。", "You breathe inertia."));
        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BR_INERTIA), bm_ptr->plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_INERTIAL, bm_ptr->dir, bm_ptr->damage, (bm_ptr->plev > 40 ? 3 : 2));
        break;
    case MS_BR_GRAVITY:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("重力のブレスを吐いた。", "You breathe gravity."));
        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BR_GRAVITY), bm_ptr->plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_GRAVITY, bm_ptr->dir, bm_ptr->damage, (bm_ptr->plev > 40 ? 3 : 2));
        break;
    case MS_BR_SHARDS:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("破片のブレスを吐いた。", "You breathe shards."));
        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BR_SHARDS), bm_ptr->plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_SHARDS, bm_ptr->dir, bm_ptr->damage, (bm_ptr->plev > 40 ? 3 : 2));
        break;
    case MS_BR_PLASMA:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("プラズマのブレスを吐いた。", "You breathe plasma."));
        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BR_PLASMA), bm_ptr->plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_PLASMA, bm_ptr->dir, bm_ptr->damage, (bm_ptr->plev > 40 ? 3 : 2));
        break;
    case MS_BR_FORCE:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("フォースのブレスを吐いた。", "You breathe force."));
        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BR_FORCE), bm_ptr->plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_FORCE, bm_ptr->dir, bm_ptr->damage, (bm_ptr->plev > 40 ? 3 : 2));
        break;
    case MS_BR_MANA:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("魔力のブレスを吐いた。", "You breathe mana."));
        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BR_MANA), bm_ptr->plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_MANA, bm_ptr->dir, bm_ptr->damage, (bm_ptr->plev > 40 ? 3 : 2));
        break;
    case MS_BALL_NUKE:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("放射能球を放った。", "You cast a ball of radiation."));
        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_NUKE), bm_ptr->plev, DAM_ROLL);
        fire_ball(caster_ptr, GF_NUKE, bm_ptr->dir, bm_ptr->damage, 2);
        break;
    case MS_BR_NUKE:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("放射性廃棄物のブレスを吐いた。", "You breathe toxic waste."));
        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BR_NUKE), bm_ptr->plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_NUKE, bm_ptr->dir, bm_ptr->damage, (bm_ptr->plev > 40 ? 3 : 2));
        break;
    case MS_BALL_CHAOS:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("純ログルスを放った。", "You invoke a raw Logrus."));
        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_CHAOS), bm_ptr->plev, DAM_ROLL);
        fire_ball(caster_ptr, GF_CHAOS, bm_ptr->dir, bm_ptr->damage, 4);
        break;
    case MS_BR_DISI:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("分解のブレスを吐いた。", "You breathe disintegration."));
        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BR_DISI), bm_ptr->plev, DAM_ROLL);
        fire_breath(caster_ptr, GF_DISINTEGRATE, bm_ptr->dir, bm_ptr->damage, (bm_ptr->plev > 40 ? 3 : 2));
        break;
    case MS_BALL_ACID:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("アシッド・ボールの呪文を唱えた。", "You cast an acid ball."));
        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_ACID), bm_ptr->plev, DAM_ROLL);
        fire_ball(caster_ptr, GF_ACID, bm_ptr->dir, bm_ptr->damage, 2);
        break;
    case MS_BALL_ELEC:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("サンダー・ボールの呪文を唱えた。", "You cast a lightning ball."));
        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_ELEC), bm_ptr->plev, DAM_ROLL);
        fire_ball(caster_ptr, GF_ELEC, bm_ptr->dir, bm_ptr->damage, 2);
        break;
    case MS_BALL_FIRE:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("ファイア・ボールの呪文を唱えた。", "You cast a fire ball."));
        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_FIRE), bm_ptr->plev, DAM_ROLL);
        fire_ball(caster_ptr, GF_FIRE, bm_ptr->dir, bm_ptr->damage, 2);
        break;
    case MS_BALL_COLD:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("アイス・ボールの呪文を唱えた。", "You cast a frost ball."));
        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_COLD), bm_ptr->plev, DAM_ROLL);
        fire_ball(caster_ptr, GF_COLD, bm_ptr->dir, bm_ptr->damage, 2);
        break;
    case MS_BALL_POIS:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("悪臭雲の呪文を唱えた。", "You cast a stinking cloud."));
        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_POIS), bm_ptr->plev, DAM_ROLL);
        fire_ball(caster_ptr, GF_POIS, bm_ptr->dir, bm_ptr->damage, 2);
        break;
    case MS_BALL_NETHER:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("地獄球の呪文を唱えた。", "You cast a nether ball."));
        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_NETHER), bm_ptr->plev, DAM_ROLL);
        fire_ball(caster_ptr, GF_NETHER, bm_ptr->dir, bm_ptr->damage, 2);
        break;
    case MS_BALL_WATER:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("流れるような身振りをした。", "You gesture fluidly."));
        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_WATER), bm_ptr->plev, DAM_ROLL);
        fire_ball(caster_ptr, GF_WATER, bm_ptr->dir, bm_ptr->damage, 4);
        break;
    case MS_BALL_MANA:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("魔力の嵐の呪文を念じた。", "You invoke a mana storm."));
        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_MANA), bm_ptr->plev, DAM_ROLL);
        fire_ball(caster_ptr, GF_MANA, bm_ptr->dir, bm_ptr->damage, 4);
        break;
    case MS_BALL_DARK:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("暗黒の嵐の呪文を念じた。", "You invoke a darkness storm."));
        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_DARK), bm_ptr->plev, DAM_ROLL);
        fire_ball(caster_ptr, GF_DARK, bm_ptr->dir, bm_ptr->damage, 4);
        break;
    case MS_DRAIN_MANA:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_DRAIN_MANA), bm_ptr->plev, DAM_ROLL);
        fire_ball_hide(caster_ptr, GF_DRAIN_MANA, bm_ptr->dir, bm_ptr->damage, 0);
        break;
    case MS_MIND_BLAST:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_MIND_BLAST), bm_ptr->plev, DAM_ROLL);
        fire_ball_hide(caster_ptr, GF_MIND_BLAST, bm_ptr->dir, bm_ptr->damage, 0);
        break;
    case MS_BRAIN_SMASH:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BRAIN_SMASH), bm_ptr->plev, DAM_ROLL);
        fire_ball_hide(caster_ptr, GF_BRAIN_SMASH, bm_ptr->dir, bm_ptr->damage, 0);
        break;
    case MS_CAUSE_1:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_CAUSE_1), bm_ptr->plev, DAM_ROLL);
        fire_ball_hide(caster_ptr, GF_CAUSE_1, bm_ptr->dir, bm_ptr->damage, 0);
        break;
    case MS_CAUSE_2:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_CAUSE_2), bm_ptr->plev, DAM_ROLL);
        fire_ball_hide(caster_ptr, GF_CAUSE_2, bm_ptr->dir, bm_ptr->damage, 0);
        break;
    case MS_CAUSE_3:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_CAUSE_3), bm_ptr->plev, DAM_ROLL);
        fire_ball_hide(caster_ptr, GF_CAUSE_3, bm_ptr->dir, bm_ptr->damage, 0);
        break;
    case MS_CAUSE_4:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_CAUSE_4), bm_ptr->plev, DAM_ROLL);
        fire_ball_hide(caster_ptr, GF_CAUSE_4, bm_ptr->dir, bm_ptr->damage, 0);
        break;
    case MS_BOLT_ACID:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("アシッド・ボルトの呪文を唱えた。", "You cast an acid bolt."));
        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BOLT_ACID), bm_ptr->plev, DAM_ROLL);
        fire_bolt(caster_ptr, GF_ACID, bm_ptr->dir, bm_ptr->damage);
        break;
    case MS_BOLT_ELEC:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("サンダー・ボルトの呪文を唱えた。", "You cast a lightning bolt."));
        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BOLT_ELEC), bm_ptr->plev, DAM_ROLL);
        fire_bolt(caster_ptr, GF_ELEC, bm_ptr->dir, bm_ptr->damage);
        break;
    case MS_BOLT_FIRE:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("ファイア・ボルトの呪文を唱えた。", "You cast a fire bolt."));
        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BOLT_FIRE), bm_ptr->plev, DAM_ROLL);
        fire_bolt(caster_ptr, GF_FIRE, bm_ptr->dir, bm_ptr->damage);
        break;
    case MS_BOLT_COLD:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("アイス・ボルトの呪文を唱えた。", "You cast a frost bolt."));
        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BOLT_COLD), bm_ptr->plev, DAM_ROLL);
        fire_bolt(caster_ptr, GF_COLD, bm_ptr->dir, bm_ptr->damage);
        break;
    case MS_STARBURST:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("スターバーストの呪文を念じた。", "You invoke a starburst."));
        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_STARBURST), bm_ptr->plev, DAM_ROLL);
        fire_ball(caster_ptr, GF_LITE, bm_ptr->dir, bm_ptr->damage, 4);
        break;
    case MS_BOLT_NETHER:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("地獄の矢の呪文を唱えた。", "You cast a nether bolt."));
        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BOLT_NETHER), bm_ptr->plev, DAM_ROLL);
        fire_bolt(caster_ptr, GF_NETHER, bm_ptr->dir, bm_ptr->damage);
        break;
    case MS_BOLT_WATER:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("ウォーター・ボルトの呪文を唱えた。", "You cast a water bolt."));
        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BOLT_WATER), bm_ptr->plev, DAM_ROLL);
        fire_bolt(caster_ptr, GF_WATER, bm_ptr->dir, bm_ptr->damage);
        break;
    case MS_BOLT_MANA:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("魔力の矢の呪文を唱えた。", "You cast a mana bolt."));
        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BOLT_MANA), bm_ptr->plev, DAM_ROLL);
        fire_bolt(caster_ptr, GF_MANA, bm_ptr->dir, bm_ptr->damage);
        break;
    case MS_BOLT_PLASMA:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("プラズマ・ボルトの呪文を唱えた。", "You cast a plasma bolt."));
        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BOLT_PLASMA), bm_ptr->plev, DAM_ROLL);
        fire_bolt(caster_ptr, GF_PLASMA, bm_ptr->dir, bm_ptr->damage);
        break;
    case MS_BOLT_ICE:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("極寒の矢の呪文を唱えた。", "You cast a ice bolt."));
        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_BOLT_ICE), bm_ptr->plev, DAM_ROLL);
        fire_bolt(caster_ptr, GF_ICE, bm_ptr->dir, bm_ptr->damage);
        break;
    case MS_MAGIC_MISSILE:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("マジック・ミサイルの呪文を唱えた。", "You cast a magic missile."));
        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_MAGIC_MISSILE), bm_ptr->plev, DAM_ROLL);
        fire_bolt(caster_ptr, GF_MISSILE, bm_ptr->dir, bm_ptr->damage);
        break;
    case MS_SCARE:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("恐ろしげな幻覚を作り出した。", "You cast a fearful illusion."));
        fear_monster(caster_ptr, bm_ptr->dir, bm_ptr->plev + 10);
        break;
    case MS_BLIND:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;
        confuse_monster(caster_ptr, bm_ptr->dir, bm_ptr->plev * 2);
        break;
    case MS_CONF:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("誘惑的な幻覚をつくり出した。", "You cast a mesmerizing illusion."));
        confuse_monster(caster_ptr, bm_ptr->dir, bm_ptr->plev * 2);
        break;
    case MS_SLOW:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;
        slow_monster(caster_ptr, bm_ptr->dir, bm_ptr->plev);
        break;
    case MS_SLEEP:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;
        sleep_monster(caster_ptr, bm_ptr->dir, bm_ptr->plev);
        break;
    case MS_SPEED:
        (void)set_fast(caster_ptr, randint1(20 + bm_ptr->plev) + bm_ptr->plev, FALSE);
        break;
    case MS_HAND_DOOM: {
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("<破滅の手>を放った！", "You invoke the Hand of Doom!"));
        fire_ball_hide(caster_ptr, GF_HAND_DOOM, bm_ptr->dir, bm_ptr->plev * 3, 0);
        break;
    }
    case MS_HEAL:
        msg_print(_("自分の傷に念を集中した。", "You concentrate on your wounds!"));
        (void)hp_player(caster_ptr, bm_ptr->plev * 4);
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
        teleport_player(caster_ptr, bm_ptr->plev * 5, TELEPORT_SPONTANEOUS);
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
            if ((r_ptr->flags1 & RF1_UNIQUE) || (r_ptr->flagsr & RFR_RES_ALL)) {
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
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        (void)fire_beam(caster_ptr, GF_AWAY_ALL, bm_ptr->dir, 100);
        break;
    case MS_TELE_LEVEL:
        return teleport_level_other(caster_ptr);
        break;
    case MS_PSY_SPEAR:
        if (!get_aim_dir(caster_ptr, &bm_ptr->dir))
            return FALSE;

        msg_print(_("光の剣を放った。", "You throw a psycho-spear."));
        bm_ptr->damage = monspell_bluemage_damage(caster_ptr, (MS_PSY_SPEAR), bm_ptr->plev, DAM_ROLL);
        (void)fire_beam(caster_ptr, GF_PSY_SPEAR, bm_ptr->dir, bm_ptr->damage);
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
            if (summon_kin_player(caster_ptr, bm_ptr->summon_lev, caster_ptr->y, caster_ptr->x, (bm_ptr->pet ? PM_FORCE_PET : 0L))) {
                if (!bm_ptr->pet)
                    msg_print(_("召喚された仲間は怒っている！", "The summoned companion is angry!"));
            } else {
                bm_ptr->no_trump = TRUE;
            }
        }

        break;
    }
    case MS_S_CYBER: {
        msg_print(_("サイバーデーモンを召喚した！", "You summon a Cyberdemon!"));
        for (int k = 0; k < 1; k++) {
            if (summon_specific(caster_ptr, (bm_ptr->pet ? -1 : 0), caster_ptr->y, caster_ptr->x, bm_ptr->summon_lev, SUMMON_CYBER, bm_ptr->p_mode)) {
                if (!bm_ptr->pet)
                    msg_print(_("召喚されたサイバーデーモンは怒っている！", "The summoned Cyberdemon are angry!"));
            } else {
                bm_ptr->no_trump = TRUE;
            }
        }
        break;
    }
    case MS_S_MONSTER: {
        msg_print(_("仲間を召喚した。", "You summon help."));
        for (int k = 0; k < 1; k++) {
            if (summon_specific(caster_ptr, (bm_ptr->pet ? -1 : 0), caster_ptr->y, caster_ptr->x, bm_ptr->summon_lev, 0, bm_ptr->p_mode)) {
                if (!bm_ptr->pet)
                    msg_print(_("召喚されたモンスターは怒っている！", "The summoned monster is angry!"));
            } else {
                bm_ptr->no_trump = TRUE;
            }
        }

        break;
    }
    case MS_S_MONSTERS: {
        msg_print(_("モンスターを召喚した！", "You summon monsters!"));
        for (int k = 0; k < bm_ptr->plev / 15 + 2; k++) {
            if (summon_specific(caster_ptr, (bm_ptr->pet ? -1 : 0), caster_ptr->y, caster_ptr->x, bm_ptr->summon_lev, 0, (bm_ptr->p_mode | bm_ptr->u_mode))) {
                if (!bm_ptr->pet)
                    msg_print(_("召喚されたモンスターは怒っている！", "The summoned monsters are angry!"));
            } else {
                bm_ptr->no_trump = TRUE;
            }
        }

        break;
    }
    case MS_S_ANT: {
        msg_print(_("アリを召喚した。", "You summon ants."));
        if (summon_specific(
                caster_ptr, (bm_ptr->pet ? -1 : 0), caster_ptr->y, caster_ptr->x, bm_ptr->summon_lev, SUMMON_ANT, (PM_ALLOW_GROUP | bm_ptr->p_mode))) {
            if (!bm_ptr->pet)
                msg_print(_("召喚されたアリは怒っている！", "The summoned ants are angry!"));
        } else {
            bm_ptr->no_trump = TRUE;
        }

        break;
    }
    case MS_S_SPIDER: {
        msg_print(_("蜘蛛を召喚した。", "You summon spiders."));
        if (summon_specific(
                caster_ptr, (bm_ptr->pet ? -1 : 0), caster_ptr->y, caster_ptr->x, bm_ptr->summon_lev, SUMMON_SPIDER, (PM_ALLOW_GROUP | bm_ptr->p_mode))) {
            if (!bm_ptr->pet)
                msg_print(_("召喚された蜘蛛は怒っている！", "Summoned spiders are angry!"));
        } else {
            bm_ptr->no_trump = TRUE;
        }

        break;
    }
    case MS_S_HOUND: {
        msg_print(_("ハウンドを召喚した。", "You summon hounds."));
        if (summon_specific(
                caster_ptr, (bm_ptr->pet ? -1 : 0), caster_ptr->y, caster_ptr->x, bm_ptr->summon_lev, SUMMON_HOUND, (PM_ALLOW_GROUP | bm_ptr->p_mode))) {
            if (!bm_ptr->pet)
                msg_print(_("召喚されたハウンドは怒っている！", "Summoned hounds are angry!"));
        } else {
            bm_ptr->no_trump = TRUE;
        }

        break;
    }
    case MS_S_HYDRA: {
        msg_print(_("ヒドラを召喚した。", "You summon a hydras."));
        if (summon_specific(
                caster_ptr, (bm_ptr->pet ? -1 : 0), caster_ptr->y, caster_ptr->x, bm_ptr->summon_lev, SUMMON_HYDRA, (bm_ptr->g_mode | bm_ptr->p_mode))) {
            if (!bm_ptr->pet)
                msg_print(_("召喚されたヒドラは怒っている！", "Summoned hydras are angry!"));
        } else {
            bm_ptr->no_trump = TRUE;
        }

        break;
    }
    case MS_S_ANGEL: {
        msg_print(_("天使を召喚した！", "You summon an angel!"));
        if (summon_specific(
                caster_ptr, (bm_ptr->pet ? -1 : 0), caster_ptr->y, caster_ptr->x, bm_ptr->summon_lev, SUMMON_ANGEL, (bm_ptr->g_mode | bm_ptr->p_mode))) {
            if (!bm_ptr->pet)
                msg_print(_("召喚された天使は怒っている！", "The summoned angel is angry!"));
        } else {
            bm_ptr->no_trump = TRUE;
        }

        break;
    }
    case MS_S_DEMON: {
        msg_print(_("混沌の宮廷から悪魔を召喚した！", "You summon a demon from the Courts of Chaos!"));
        if (summon_specific(
                caster_ptr, (bm_ptr->pet ? -1 : 0), caster_ptr->y, caster_ptr->x, bm_ptr->summon_lev, SUMMON_DEMON, (bm_ptr->g_mode | bm_ptr->p_mode))) {
            if (!bm_ptr->pet)
                msg_print(_("召喚されたデーモンは怒っている！", "The summoned demon is angry!"));
        } else {
            bm_ptr->no_trump = TRUE;
        }

        break;
    }
    case MS_S_UNDEAD: {
        msg_print(_("アンデッドの強敵を召喚した！", "You summon an undead adversary!"));
        if (summon_specific(
                caster_ptr, (bm_ptr->pet ? -1 : 0), caster_ptr->y, caster_ptr->x, bm_ptr->summon_lev, SUMMON_UNDEAD, (bm_ptr->g_mode | bm_ptr->p_mode))) {
            if (!bm_ptr->pet)
                msg_print(_("召喚されたアンデッドは怒っている！", "The summoned undead is angry!"));
        } else {
            bm_ptr->no_trump = TRUE;
        }

        break;
    }
    case MS_S_DRAGON: {
        msg_print(_("ドラゴンを召喚した！", "You summon a dragon!"));
        if (summon_specific(
                caster_ptr, (bm_ptr->pet ? -1 : 0), caster_ptr->y, caster_ptr->x, bm_ptr->summon_lev, SUMMON_DRAGON, (bm_ptr->g_mode | bm_ptr->p_mode))) {
            if (!bm_ptr->pet)
                msg_print(_("召喚されたドラゴンは怒っている！", "The summoned dragon is angry!"));
        } else {
            bm_ptr->no_trump = TRUE;
        }

        break;
    }
    case MS_S_HI_UNDEAD: {
        msg_print(_("強力なアンデッドを召喚した！", "You summon a greater undead!"));
        if (summon_specific(caster_ptr, (bm_ptr->pet ? -1 : 0), caster_ptr->y, caster_ptr->x, bm_ptr->summon_lev, SUMMON_HI_UNDEAD,
                (bm_ptr->g_mode | bm_ptr->p_mode | bm_ptr->u_mode))) {
            if (!bm_ptr->pet)
                msg_print(_("召喚された上級アンデッドは怒っている！", "The summoned greater undead is angry!"));
        } else {
            bm_ptr->no_trump = TRUE;
        }

        break;
    }
    case MS_S_HI_DRAGON: {
        msg_print(_("古代ドラゴンを召喚した！", "You summon an ancient dragon!"));
        if (summon_specific(caster_ptr, (bm_ptr->pet ? -1 : 0), caster_ptr->y, caster_ptr->x, bm_ptr->summon_lev, SUMMON_HI_DRAGON,
                (bm_ptr->g_mode | bm_ptr->p_mode | bm_ptr->u_mode))) {
            if (!bm_ptr->pet)
                msg_print(_("召喚された古代ドラゴンは怒っている！", "The summoned ancient dragon is angry!"));
        } else {
            bm_ptr->no_trump = TRUE;
        }

        break;
    }
    case MS_S_AMBERITE: {
        msg_print(_("アンバーの王族を召喚した！", "You summon a Lord of Amber!"));
        if (summon_specific(caster_ptr, (bm_ptr->pet ? -1 : 0), caster_ptr->y, caster_ptr->x, bm_ptr->summon_lev, SUMMON_AMBERITES,
                (bm_ptr->g_mode | bm_ptr->p_mode | bm_ptr->u_mode))) {
            if (!bm_ptr->pet)
                msg_print(_("召喚されたアンバーの王族は怒っている！", "The summoned Lord of Amber is angry!"));
        } else {
            bm_ptr->no_trump = TRUE;
        }

        break;
    }
    case MS_S_UNIQUE: {
        int k, count = 0;
        msg_print(_("特別な強敵を召喚した！", "You summon a special opponent!"));
        for (k = 0; k < 1; k++) {
            if (summon_specific(caster_ptr, (bm_ptr->pet ? -1 : 0), caster_ptr->y, caster_ptr->x, bm_ptr->summon_lev, SUMMON_UNIQUE,
                    (bm_ptr->g_mode | bm_ptr->p_mode | PM_ALLOW_UNIQUE))) {
                count++;
                if (!bm_ptr->pet)
                    msg_print(_("召喚されたユニーク・モンスターは怒っている！", "The summoned special opponent is angry!"));
            }
        }

        for (k = count; k < 1; k++) {
            if (summon_specific(caster_ptr, (bm_ptr->pet ? -1 : 0), caster_ptr->y, caster_ptr->x, bm_ptr->summon_lev, SUMMON_HI_UNDEAD,
                    (bm_ptr->g_mode | bm_ptr->p_mode | PM_ALLOW_UNIQUE))) {
                count++;
                if (!bm_ptr->pet)
                    msg_print(_("召喚された上級アンデッドは怒っている！", "The summoned greater undead is angry!"));
            }
        }

        if (!count)
            bm_ptr->no_trump = TRUE;

        break;
    }
    default:
        msg_print("hoge?");
    }

    if (bm_ptr->no_trump)
        msg_print(_("何も現れなかった。", "No one appeared."));

    return TRUE;
}
