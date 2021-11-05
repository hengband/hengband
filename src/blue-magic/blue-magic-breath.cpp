/*!
 * @file blue-magic-breath.cpp
 * @brief 青魔法のブレス系呪文定義
 */

#include "blue-magic/blue-magic-breath.h"
#include "blue-magic/blue-magic-util.h"
#include "mind/mind-blue-mage.h"
#include "monster-race/race-ability-flags.h"
#include "mspell/mspell-damage-calculator.h"
#include "spell-kind/spells-launcher.h"
#include "spell/spell-types.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

bool cast_blue_breath_acid(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir))
        return false;

    msg_print(_("酸のブレスを吐いた。", "You breathe acid."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, RF_ABILITY::BR_ACID, bmc_ptr->plev, DAM_ROLL);
    fire_breath(player_ptr, GF_ACID, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return true;
}

bool cast_blue_breath_elec(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir))
        return false;

    msg_print(_("稲妻のブレスを吐いた。", "You breathe lightning."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, RF_ABILITY::BR_ELEC, bmc_ptr->plev, DAM_ROLL);
    fire_breath(player_ptr, GF_ELEC, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return true;
}

bool cast_blue_breath_fire(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir))
        return false;

    msg_print(_("火炎のブレスを吐いた。", "You breathe fire."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, RF_ABILITY::BR_FIRE, bmc_ptr->plev, DAM_ROLL);
    fire_breath(player_ptr, GF_FIRE, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return true;
}

bool cast_blue_breath_cold(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir))
        return false;

    msg_print(_("冷気のブレスを吐いた。", "You breathe frost."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, RF_ABILITY::BR_COLD, bmc_ptr->plev, DAM_ROLL);
    fire_breath(player_ptr, GF_COLD, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return true;
}

bool cast_blue_breath_pois(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir))
        return false;

    msg_print(_("ガスのブレスを吐いた。", "You breathe gas."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, RF_ABILITY::BR_POIS, bmc_ptr->plev, DAM_ROLL);
    fire_breath(player_ptr, GF_POIS, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return true;
}

bool cast_blue_breath_nether(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir))
        return false;

    msg_print(_("地獄のブレスを吐いた。", "You breathe nether."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, RF_ABILITY::BR_NETH, bmc_ptr->plev, DAM_ROLL);
    fire_breath(player_ptr, GF_NETHER, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return true;
}

bool cast_blue_breath_lite(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir))
        return false;

    msg_print(_("閃光のブレスを吐いた。", "You breathe light."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, RF_ABILITY::BR_LITE, bmc_ptr->plev, DAM_ROLL);
    fire_breath(player_ptr, GF_LITE, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return true;
}

bool cast_blue_breath_dark(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir))
        return false;

    msg_print(_("暗黒のブレスを吐いた。", "You breathe darkness."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, RF_ABILITY::BR_DARK, bmc_ptr->plev, DAM_ROLL);
    fire_breath(player_ptr, GF_DARK, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return true;
}

bool cast_blue_breath_conf(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir))
        return false;

    msg_print(_("混乱のブレスを吐いた。", "You breathe confusion."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, RF_ABILITY::BR_CONF, bmc_ptr->plev, DAM_ROLL);
    fire_breath(player_ptr, GF_CONFUSION, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return true;
}

bool cast_blue_breath_sound(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir))
        return false;

    msg_print(_("轟音のブレスを吐いた。", "You breathe sound."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, RF_ABILITY::BR_SOUN, bmc_ptr->plev, DAM_ROLL);
    fire_breath(player_ptr, GF_SOUND, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return true;
}

bool cast_blue_breath_chaos(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir))
        return false;

    msg_print(_("カオスのブレスを吐いた。", "You breathe chaos."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, RF_ABILITY::BR_CHAO, bmc_ptr->plev, DAM_ROLL);
    fire_breath(player_ptr, GF_CHAOS, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return true;
}

bool cast_blue_breath_disenchant(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir))
        return false;

    msg_print(_("劣化のブレスを吐いた。", "You breathe disenchantment."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, RF_ABILITY::BR_DISE, bmc_ptr->plev, DAM_ROLL);
    fire_breath(player_ptr, GF_DISENCHANT, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return true;
}

bool cast_blue_breath_nexus(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir))
        return false;

    msg_print(_("因果混乱のブレスを吐いた。", "You breathe nexus."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, RF_ABILITY::BR_NEXU, bmc_ptr->plev, DAM_ROLL);
    fire_breath(player_ptr, GF_NEXUS, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return true;
}

bool cast_blue_breath_time(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir))
        return false;

    msg_print(_("時間逆転のブレスを吐いた。", "You breathe time."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, RF_ABILITY::BR_TIME, bmc_ptr->plev, DAM_ROLL);
    fire_breath(player_ptr, GF_TIME, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return true;
}

bool cast_blue_breath_inertia(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir))
        return false;

    msg_print(_("遅鈍のブレスを吐いた。", "You breathe inertia."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, RF_ABILITY::BR_INER, bmc_ptr->plev, DAM_ROLL);
    fire_breath(player_ptr, GF_INERTIAL, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return true;
}

bool cast_blue_breath_gravity(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir))
        return false;

    msg_print(_("重力のブレスを吐いた。", "You breathe gravity."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, RF_ABILITY::BR_GRAV, bmc_ptr->plev, DAM_ROLL);
    fire_breath(player_ptr, GF_GRAVITY, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return true;
}

bool cast_blue_breath_shards(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir))
        return false;

    msg_print(_("破片のブレスを吐いた。", "You breathe shards."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, RF_ABILITY::BR_SHAR, bmc_ptr->plev, DAM_ROLL);
    fire_breath(player_ptr, GF_SHARDS, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return true;
}

bool cast_blue_breath_plasma(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir))
        return false;

    msg_print(_("プラズマのブレスを吐いた。", "You breathe plasma."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, RF_ABILITY::BR_PLAS, bmc_ptr->plev, DAM_ROLL);
    fire_breath(player_ptr, GF_PLASMA, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return true;
}

bool cast_blue_breath_force(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir))
        return false;

    msg_print(_("フォースのブレスを吐いた。", "You breathe force."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, RF_ABILITY::BR_FORC, bmc_ptr->plev, DAM_ROLL);
    fire_breath(player_ptr, GF_FORCE, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return true;
}

bool cast_blue_breath_mana(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir))
        return false;

    msg_print(_("魔力のブレスを吐いた。", "You breathe mana."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, RF_ABILITY::BR_MANA, bmc_ptr->plev, DAM_ROLL);
    fire_breath(player_ptr, GF_MANA, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return true;
}

bool cast_blue_breath_nuke(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir))
        return false;

    msg_print(_("放射性廃棄物のブレスを吐いた。", "You breathe toxic waste."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, RF_ABILITY::BR_NUKE, bmc_ptr->plev, DAM_ROLL);
    fire_breath(player_ptr, GF_NUKE, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return true;
}

/*!
 * @brief 分解のブレスの青魔法
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param bmc_ptr 青魔法詠唱への参照ポインタ
 * @details 永久の刻は過ぎ去れリ.
 */
bool cast_blue_breath_disintegration(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir))
        return false;

    msg_print(_("分解のブレスを吐いた。", "You breathe disintegration."));
    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, RF_ABILITY::BR_DISI, bmc_ptr->plev, DAM_ROLL);
    fire_breath(player_ptr, GF_DISINTEGRATE, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return true;
}
