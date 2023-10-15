/*!
 * @file blue-magic-ball-bolt.cpp
 * @brief 青魔法のボール/ボルト系呪文定義
 */

#include "blue-magic/blue-magic-ball-bolt.h"
#include "blue-magic/blue-magic-util.h"
#include "effect/attribute-types.h"
#include "monster-race/race-ability-flags.h"
#include "mspell/mspell-damage-calculator.h"
#include "spell-kind/spells-launcher.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

bool cast_blue_ball_acid(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    msg_print(_("アシッド・ボールの呪文を唱えた。", "You cast an acid ball."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, MonsterAbilityType::BA_ACID, bmc_ptr->plev, DAM_ROLL);
    fire_ball(player_ptr, AttributeType::ACID, bmc_ptr->dir, bmc_ptr->damage, 2);
    return true;
}

bool cast_blue_ball_elec(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    msg_print(_("サンダー・ボールの呪文を唱えた。", "You cast a lightning ball."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, MonsterAbilityType::BA_ELEC, bmc_ptr->plev, DAM_ROLL);
    fire_ball(player_ptr, AttributeType::ELEC, bmc_ptr->dir, bmc_ptr->damage, 2);
    return true;
}

bool cast_blue_ball_fire(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    msg_print(_("ファイア・ボールの呪文を唱えた。", "You cast a fire ball."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, MonsterAbilityType::BA_FIRE, bmc_ptr->plev, DAM_ROLL);
    fire_ball(player_ptr, AttributeType::FIRE, bmc_ptr->dir, bmc_ptr->damage, 2);
    return true;
}

bool cast_blue_ball_cold(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    msg_print(_("アイス・ボールの呪文を唱えた。", "You cast a frost ball."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, MonsterAbilityType::BA_COLD, bmc_ptr->plev, DAM_ROLL);
    fire_ball(player_ptr, AttributeType::COLD, bmc_ptr->dir, bmc_ptr->damage, 2);
    return true;
}

bool cast_blue_ball_pois(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    msg_print(_("悪臭雲の呪文を唱えた。", "You cast a stinking cloud."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, MonsterAbilityType::BA_POIS, bmc_ptr->plev, DAM_ROLL);
    fire_ball(player_ptr, AttributeType::POIS, bmc_ptr->dir, bmc_ptr->damage, 2);
    return true;
}

bool cast_blue_ball_nuke(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    msg_print(_("放射能球を放った。", "You cast a ball of radiation."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, MonsterAbilityType::BA_NUKE, bmc_ptr->plev, DAM_ROLL);
    fire_ball(player_ptr, AttributeType::NUKE, bmc_ptr->dir, bmc_ptr->damage, 2);
    return true;
}

bool cast_blue_ball_nether(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    msg_print(_("地獄球の呪文を唱えた。", "You cast a nether ball."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, MonsterAbilityType::BA_NETH, bmc_ptr->plev, DAM_ROLL);
    fire_ball(player_ptr, AttributeType::NETHER, bmc_ptr->dir, bmc_ptr->damage, 2);
    return true;
}

bool cast_blue_ball_chaos(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    msg_print(_("純ログルスを放った。", "You invoke a raw Logrus."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, MonsterAbilityType::BA_CHAO, bmc_ptr->plev, DAM_ROLL);
    fire_ball(player_ptr, AttributeType::CHAOS, bmc_ptr->dir, bmc_ptr->damage, 4);
    return true;
}

/*!
 * @brief ウォーター・ボールの青魔法
 * @brief 分解のブレスの青魔法
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param bmc_ptr 青魔法詠唱への参照ポインタ
 * @details All my worries are blown away.
 */
bool cast_blue_ball_water(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    msg_print(_("流れるような身振りをした。", "You gesture fluidly."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, MonsterAbilityType::BA_WATE, bmc_ptr->plev, DAM_ROLL);
    fire_ball(player_ptr, AttributeType::WATER, bmc_ptr->dir, bmc_ptr->damage, 4);
    return true;
}

bool cast_blue_ball_star_burst(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    msg_print(_("スターバーストの呪文を念じた。", "You invoke a starburst."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, MonsterAbilityType::BA_LITE, bmc_ptr->plev, DAM_ROLL);
    fire_ball(player_ptr, AttributeType::LITE, bmc_ptr->dir, bmc_ptr->damage, 4);
    return true;
}

bool cast_blue_ball_dark_storm(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    msg_print(_("暗黒の嵐の呪文を念じた。", "You invoke a darkness storm."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, MonsterAbilityType::BA_DARK, bmc_ptr->plev, DAM_ROLL);
    fire_ball(player_ptr, AttributeType::DARK, bmc_ptr->dir, bmc_ptr->damage, 4);
    return true;
}

bool cast_blue_ball_mana_storm(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    msg_print(_("魔力の嵐の呪文を念じた。", "You invoke a mana storm."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, MonsterAbilityType::BA_MANA, bmc_ptr->plev, DAM_ROLL);
    fire_ball(player_ptr, AttributeType::MANA, bmc_ptr->dir, bmc_ptr->damage, 4);
    return true;
}

bool cast_blue_ball_void(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    msg_print(_("虚無の嵐の呪文を念じた。", "You invoke a void storm."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, MonsterAbilityType::BA_VOID, bmc_ptr->plev, DAM_ROLL);
    fire_ball(player_ptr, AttributeType::VOID_MAGIC, bmc_ptr->dir, bmc_ptr->damage, 4);
    return true;
}

bool cast_blue_ball_abyss(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    msg_print(_("深淵の嵐の呪文を念じた。", "You invoke a abyss storm."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, MonsterAbilityType::BA_ABYSS, bmc_ptr->plev, DAM_ROLL);
    fire_ball(player_ptr, AttributeType::ABYSS, bmc_ptr->dir, bmc_ptr->damage, 4);
    return true;
}

bool cast_blue_bolt_acid(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    msg_print(_("アシッド・ボルトの呪文を唱えた。", "You cast an acid bolt."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, MonsterAbilityType::BO_ACID, bmc_ptr->plev, DAM_ROLL);
    fire_bolt(player_ptr, AttributeType::ACID, bmc_ptr->dir, bmc_ptr->damage);
    return true;
}

bool cast_blue_bolt_elec(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    msg_print(_("サンダー・ボルトの呪文を唱えた。", "You cast a lightning bolt."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, MonsterAbilityType::BO_ELEC, bmc_ptr->plev, DAM_ROLL);
    fire_bolt(player_ptr, AttributeType::ELEC, bmc_ptr->dir, bmc_ptr->damage);
    return true;
}

bool cast_blue_bolt_fire(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    msg_print(_("ファイア・ボルトの呪文を唱えた。", "You cast a fire bolt."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, MonsterAbilityType::BO_FIRE, bmc_ptr->plev, DAM_ROLL);
    fire_bolt(player_ptr, AttributeType::FIRE, bmc_ptr->dir, bmc_ptr->damage);
    return true;
}

bool cast_blue_bolt_cold(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    msg_print(_("アイス・ボルトの呪文を唱えた。", "You cast a frost bolt."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, MonsterAbilityType::BO_COLD, bmc_ptr->plev, DAM_ROLL);
    fire_bolt(player_ptr, AttributeType::COLD, bmc_ptr->dir, bmc_ptr->damage);
    return true;
}

bool cast_blue_bolt_nether(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    msg_print(_("地獄の矢の呪文を唱えた。", "You cast a nether bolt."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, MonsterAbilityType::BO_NETH, bmc_ptr->plev, DAM_ROLL);
    fire_bolt(player_ptr, AttributeType::NETHER, bmc_ptr->dir, bmc_ptr->damage);
    return true;
}

bool cast_blue_bolt_water(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    msg_print(_("ウォーター・ボルトの呪文を唱えた。", "You cast a water bolt."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, MonsterAbilityType::BO_WATE, bmc_ptr->plev, DAM_ROLL);
    fire_bolt(player_ptr, AttributeType::WATER, bmc_ptr->dir, bmc_ptr->damage);
    return true;
}

bool cast_blue_bolt_mana(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    msg_print(_("魔力の矢の呪文を唱えた。", "You cast a mana bolt."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, MonsterAbilityType::BO_MANA, bmc_ptr->plev, DAM_ROLL);
    fire_bolt(player_ptr, AttributeType::MANA, bmc_ptr->dir, bmc_ptr->damage);
    return true;
}

bool cast_blue_bolt_plasma(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    msg_print(_("プラズマ・ボルトの呪文を唱えた。", "You cast a plasma bolt."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, MonsterAbilityType::BO_PLAS, bmc_ptr->plev, DAM_ROLL);
    fire_bolt(player_ptr, AttributeType::PLASMA, bmc_ptr->dir, bmc_ptr->damage);
    return true;
}

bool cast_blue_bolt_icee(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    msg_print(_("極寒の矢の呪文を唱えた。", "You cast a ice bolt."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, MonsterAbilityType::BO_ICEE, bmc_ptr->plev, DAM_ROLL);
    fire_bolt(player_ptr, AttributeType::ICE, bmc_ptr->dir, bmc_ptr->damage);
    return true;
}

bool cast_blue_bolt_missile(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    msg_print(_("マジック・ミサイルの呪文を唱えた。", "You cast a magic missile."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, MonsterAbilityType::MISSILE, bmc_ptr->plev, DAM_ROLL);
    fire_bolt(player_ptr, AttributeType::MISSILE, bmc_ptr->dir, bmc_ptr->damage);
    return true;
}

bool cast_blue_bolt_abyss(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    msg_print(_("アビス・ボルトの呪文を唱えた。", "You cast a abyss bolt."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, MonsterAbilityType::BO_ABYSS, bmc_ptr->plev, DAM_ROLL);
    fire_bolt(player_ptr, AttributeType::ABYSS, bmc_ptr->dir, bmc_ptr->damage);
    return true;
}

bool cast_blue_bolt_void(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    msg_print(_("ヴォイド・ボルトの呪文を唱えた。", "You cast a void bolt."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, MonsterAbilityType::BO_VOID, bmc_ptr->plev, DAM_ROLL);
    fire_bolt(player_ptr, AttributeType::VOID_MAGIC, bmc_ptr->dir, bmc_ptr->damage);
    return true;
}
