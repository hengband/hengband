#include "blue-magic/blue-magic-ball-bolt.h"
#include "blue-magic/blue-magic-util.h"
#include "mspell/mspell-damage-calculator.h"
#include "mspell/mspell-type.h"
#include "spell-kind/spells-launcher.h"
#include "spell/spell-types.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

bool cast_blue_ball_acid(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("アシッド・ボールの呪文を唱えた。", "You cast an acid ball."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BALL_ACID, bmc_ptr->plev, DAM_ROLL);
    fire_ball(caster_ptr, GF_ACID, bmc_ptr->dir, bmc_ptr->damage, 2);
    return TRUE;
}

bool cast_blue_ball_elec(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("サンダー・ボールの呪文を唱えた。", "You cast a lightning ball."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BALL_ELEC, bmc_ptr->plev, DAM_ROLL);
    fire_ball(caster_ptr, GF_ELEC, bmc_ptr->dir, bmc_ptr->damage, 2);
    return TRUE;
}

bool cast_blue_ball_fire(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("ファイア・ボールの呪文を唱えた。", "You cast a fire ball."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BALL_FIRE, bmc_ptr->plev, DAM_ROLL);
    fire_ball(caster_ptr, GF_FIRE, bmc_ptr->dir, bmc_ptr->damage, 2);
    return TRUE;
}

bool cast_blue_ball_cold(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("アイス・ボールの呪文を唱えた。", "You cast a frost ball."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BALL_COLD, bmc_ptr->plev, DAM_ROLL);
    fire_ball(caster_ptr, GF_COLD, bmc_ptr->dir, bmc_ptr->damage, 2);
    return TRUE;
}

bool cast_blue_ball_pois(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("悪臭雲の呪文を唱えた。", "You cast a stinking cloud."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BALL_POIS, bmc_ptr->plev, DAM_ROLL);
    fire_ball(caster_ptr, GF_POIS, bmc_ptr->dir, bmc_ptr->damage, 2);
    return TRUE;
}

bool cast_blue_ball_nuke(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("放射能球を放った。", "You cast a ball of radiation."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BALL_NUKE, bmc_ptr->plev, DAM_ROLL);
    fire_ball(caster_ptr, GF_NUKE, bmc_ptr->dir, bmc_ptr->damage, 2);
    return TRUE;
}

bool cast_blue_ball_nether(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("地獄球の呪文を唱えた。", "You cast a nether ball."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BALL_NETHER, bmc_ptr->plev, DAM_ROLL);
    fire_ball(caster_ptr, GF_NETHER, bmc_ptr->dir, bmc_ptr->damage, 2);
    return TRUE;
}

bool cast_blue_ball_chaos(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("純ログルスを放った。", "You invoke a raw Logrus."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BALL_CHAOS, bmc_ptr->plev, DAM_ROLL);
    fire_ball(caster_ptr, GF_CHAOS, bmc_ptr->dir, bmc_ptr->damage, 4);
    return TRUE;
}

/*!
 * @brief ウォーター・ボールの青魔法
 * @brief 分解のブレスの青魔法
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param bmc_ptr 青魔法詠唱への参照ポインタ
 * @details All my worries are blown away.
 */
bool cast_blue_ball_water(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("流れるような身振りをした。", "You gesture fluidly."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BALL_WATER, bmc_ptr->plev, DAM_ROLL);
    fire_ball(caster_ptr, GF_WATER, bmc_ptr->dir, bmc_ptr->damage, 4);
    return TRUE;
}

bool cast_blue_ball_star_burst(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("スターバーストの呪文を念じた。", "You invoke a starburst."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_STARBURST, bmc_ptr->plev, DAM_ROLL);
    fire_ball(caster_ptr, GF_LITE, bmc_ptr->dir, bmc_ptr->damage, 4);
    return TRUE;
}

bool cast_blue_ball_dark_storm(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("暗黒の嵐の呪文を念じた。", "You invoke a darkness storm."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BALL_DARK, bmc_ptr->plev, DAM_ROLL);
    fire_ball(caster_ptr, GF_DARK, bmc_ptr->dir, bmc_ptr->damage, 4);
    return TRUE;
}

bool cast_blue_ball_mana_storm(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("魔力の嵐の呪文を念じた。", "You invoke a mana storm."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BALL_MANA, bmc_ptr->plev, DAM_ROLL);
    fire_ball(caster_ptr, GF_MANA, bmc_ptr->dir, bmc_ptr->damage, 4);
    return TRUE;
}

bool cast_blue_bolt_acid(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("アシッド・ボルトの呪文を唱えた。", "You cast an acid bolt."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BOLT_ACID, bmc_ptr->plev, DAM_ROLL);
    fire_bolt(caster_ptr, GF_ACID, bmc_ptr->dir, bmc_ptr->damage);
    return TRUE;
}

bool cast_blue_bolt_elec(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("サンダー・ボルトの呪文を唱えた。", "You cast a lightning bolt."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BOLT_ELEC, bmc_ptr->plev, DAM_ROLL);
    fire_bolt(caster_ptr, GF_ELEC, bmc_ptr->dir, bmc_ptr->damage);
    return TRUE;
}

bool cast_blue_bolt_fire(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("ファイア・ボルトの呪文を唱えた。", "You cast a fire bolt."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BOLT_FIRE, bmc_ptr->plev, DAM_ROLL);
    fire_bolt(caster_ptr, GF_FIRE, bmc_ptr->dir, bmc_ptr->damage);
    return TRUE;
}

bool cast_blue_bolt_cold(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("アイス・ボルトの呪文を唱えた。", "You cast a frost bolt."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BOLT_COLD, bmc_ptr->plev, DAM_ROLL);
    fire_bolt(caster_ptr, GF_COLD, bmc_ptr->dir, bmc_ptr->damage);
    return TRUE;
}

bool cast_blue_bolt_nether(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("地獄の矢の呪文を唱えた。", "You cast a nether bolt."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BOLT_NETHER, bmc_ptr->plev, DAM_ROLL);
    fire_bolt(caster_ptr, GF_NETHER, bmc_ptr->dir, bmc_ptr->damage);
    return TRUE;
}

bool cast_blue_bolt_water(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("ウォーター・ボルトの呪文を唱えた。", "You cast a water bolt."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BOLT_WATER, bmc_ptr->plev, DAM_ROLL);
    fire_bolt(caster_ptr, GF_WATER, bmc_ptr->dir, bmc_ptr->damage);
    return TRUE;
}

bool cast_blue_bolt_mana(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("魔力の矢の呪文を唱えた。", "You cast a mana bolt."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BOLT_MANA, bmc_ptr->plev, DAM_ROLL);
    fire_bolt(caster_ptr, GF_MANA, bmc_ptr->dir, bmc_ptr->damage);
    return TRUE;
}

bool cast_blue_bolt_plasma(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("プラズマ・ボルトの呪文を唱えた。", "You cast a plasma bolt."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BOLT_PLASMA, bmc_ptr->plev, DAM_ROLL);
    fire_bolt(caster_ptr, GF_PLASMA, bmc_ptr->dir, bmc_ptr->damage);
    return TRUE;
}

bool cast_blue_bolt_icee(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("極寒の矢の呪文を唱えた。", "You cast a ice bolt."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BOLT_ICE, bmc_ptr->plev, DAM_ROLL);
    fire_bolt(caster_ptr, GF_ICE, bmc_ptr->dir, bmc_ptr->damage);
    return TRUE;
}

bool cast_blue_bolt_missile(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("マジック・ミサイルの呪文を唱えた。", "You cast a magic missile."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_MAGIC_MISSILE, bmc_ptr->plev, DAM_ROLL);
    fire_bolt(caster_ptr, GF_MISSILE, bmc_ptr->dir, bmc_ptr->damage);
    return TRUE;
}
