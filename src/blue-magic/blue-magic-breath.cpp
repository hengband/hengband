#include "blue-magic/blue-magic-breath.h"
#include "blue-magic/blue-magic-util.h"
#include "mind/mind-blue-mage.h"
#include "mspell/mspell-damage-calculator.h"
#include "mspell/mspell-type.h"
#include "spell-kind/spells-launcher.h"
#include "spell/spell-types.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

bool cast_blue_breath_acid(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("酸のブレスを吐いた。", "You breathe acid."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BR_ACID, bmc_ptr->plev, DAM_ROLL);
    fire_breath(caster_ptr, GF_ACID, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return TRUE;
}

bool cast_blue_breath_elec(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("稲妻のブレスを吐いた。", "You breathe lightning."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BR_ELEC, bmc_ptr->plev, DAM_ROLL);
    fire_breath(caster_ptr, GF_ELEC, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return TRUE;
}

bool cast_blue_breath_fire(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("火炎のブレスを吐いた。", "You breathe fire."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BR_FIRE, bmc_ptr->plev, DAM_ROLL);
    fire_breath(caster_ptr, GF_FIRE, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return TRUE;
}

bool cast_blue_breath_cold(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("冷気のブレスを吐いた。", "You breathe frost."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BR_COLD, bmc_ptr->plev, DAM_ROLL);
    fire_breath(caster_ptr, GF_COLD, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return TRUE;
}

bool cast_blue_breath_pois(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("ガスのブレスを吐いた。", "You breathe gas."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BR_POIS, bmc_ptr->plev, DAM_ROLL);
    fire_breath(caster_ptr, GF_POIS, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return TRUE;
}

bool cast_blue_breath_nether(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("地獄のブレスを吐いた。", "You breathe nether."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BR_NETHER, bmc_ptr->plev, DAM_ROLL);
    fire_breath(caster_ptr, GF_NETHER, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return TRUE;
}

bool cast_blue_breath_lite(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("閃光のブレスを吐いた。", "You breathe light."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BR_LITE, bmc_ptr->plev, DAM_ROLL);
    fire_breath(caster_ptr, GF_LITE, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return TRUE;
}

bool cast_blue_breath_dark(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("暗黒のブレスを吐いた。", "You breathe darkness."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BR_DARK, bmc_ptr->plev, DAM_ROLL);
    fire_breath(caster_ptr, GF_DARK, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return TRUE;
}

bool cast_blue_breath_conf(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("混乱のブレスを吐いた。", "You breathe confusion."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BR_CONF, bmc_ptr->plev, DAM_ROLL);
    fire_breath(caster_ptr, GF_CONFUSION, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return TRUE;
}

bool cast_blue_breath_sound(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("轟音のブレスを吐いた。", "You breathe sound."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BR_SOUND, bmc_ptr->plev, DAM_ROLL);
    fire_breath(caster_ptr, GF_SOUND, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return TRUE;
}

bool cast_blue_breath_chaos(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("カオスのブレスを吐いた。", "You breathe chaos."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BR_CHAOS, bmc_ptr->plev, DAM_ROLL);
    fire_breath(caster_ptr, GF_CHAOS, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return TRUE;
}

bool cast_blue_breath_disenchant(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("劣化のブレスを吐いた。", "You breathe disenchantment."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BR_DISEN, bmc_ptr->plev, DAM_ROLL);
    fire_breath(caster_ptr, GF_DISENCHANT, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return TRUE;
}

bool cast_blue_breath_nexus(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("因果混乱のブレスを吐いた。", "You breathe nexus."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BR_NEXUS, bmc_ptr->plev, DAM_ROLL);
    fire_breath(caster_ptr, GF_NEXUS, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return TRUE;
}

bool cast_blue_breath_time(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("時間逆転のブレスを吐いた。", "You breathe time."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BR_TIME, bmc_ptr->plev, DAM_ROLL);
    fire_breath(caster_ptr, GF_TIME, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return TRUE;
}

bool cast_blue_breath_inertia(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("遅鈍のブレスを吐いた。", "You breathe inertia."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BR_INERTIA, bmc_ptr->plev, DAM_ROLL);
    fire_breath(caster_ptr, GF_INERTIAL, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return TRUE;
}

bool cast_blue_breath_gravity(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("重力のブレスを吐いた。", "You breathe gravity."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BR_GRAVITY, bmc_ptr->plev, DAM_ROLL);
    fire_breath(caster_ptr, GF_GRAVITY, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return TRUE;
}

bool cast_blue_breath_shards(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("破片のブレスを吐いた。", "You breathe shards."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BR_SHARDS, bmc_ptr->plev, DAM_ROLL);
    fire_breath(caster_ptr, GF_SHARDS, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return TRUE;
}

bool cast_blue_breath_plasma(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("プラズマのブレスを吐いた。", "You breathe plasma."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BR_PLASMA, bmc_ptr->plev, DAM_ROLL);
    fire_breath(caster_ptr, GF_PLASMA, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return TRUE;
}

bool cast_blue_breath_force(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("フォースのブレスを吐いた。", "You breathe force."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BR_FORCE, bmc_ptr->plev, DAM_ROLL);
    fire_breath(caster_ptr, GF_FORCE, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return TRUE;
}

bool cast_blue_breath_mana(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("魔力のブレスを吐いた。", "You breathe mana."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BR_MANA, bmc_ptr->plev, DAM_ROLL);
    fire_breath(caster_ptr, GF_MANA, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return TRUE;
}

bool cast_blue_breath_nuke(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("放射性廃棄物のブレスを吐いた。", "You breathe toxic waste."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BR_NUKE, bmc_ptr->plev, DAM_ROLL);
    fire_breath(caster_ptr, GF_NUKE, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return TRUE;
}

/*!
 * @brief 分解のブレスの青魔法
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param bmc_ptr 青魔法詠唱への参照ポインタ
 * @details 永久の刻は過ぎ去れリ.
 */
bool cast_blue_breath_disintegration(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    msg_print(_("分解のブレスを吐いた。", "You breathe disintegration."));
    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BR_DISI, bmc_ptr->plev, DAM_ROLL);
    fire_breath(caster_ptr, GF_DISINTEGRATE, bmc_ptr->dir, bmc_ptr->damage, (bmc_ptr->plev > 40 ? 3 : 2));
    return TRUE;
}
