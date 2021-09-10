/*!
 * @file blue-magic-spirit-curse.cpp
 * @brief 青魔法の呪い系処理定義
 */


#include "blue-magic/blue-magic-spirit-curse.h"
#include "blue-magic/blue-magic-util.h"
#include "monster-race/race-ability-flags.h"
#include "mspell/mspell-damage-calculator.h"
#include "spell-kind/spells-launcher.h"
#include "spell/spell-types.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

bool cast_blue_drain_mana(player_type *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir))
        return false;

    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, RF_ABILITY::DRAIN_MANA, bmc_ptr->plev, DAM_ROLL);
    fire_ball_hide(player_ptr, GF_DRAIN_MANA, bmc_ptr->dir, bmc_ptr->damage, 0);
    return true;
}

bool cast_blue_mind_blast(player_type *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir))
        return false;

    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, RF_ABILITY::MIND_BLAST, bmc_ptr->plev, DAM_ROLL);
    fire_ball_hide(player_ptr, GF_MIND_BLAST, bmc_ptr->dir, bmc_ptr->damage, 0);
    return true;
}

bool cast_blue_brain_smash(player_type *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir))
        return false;

    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, RF_ABILITY::BRAIN_SMASH, bmc_ptr->plev, DAM_ROLL);
    fire_ball_hide(player_ptr, GF_BRAIN_SMASH, bmc_ptr->dir, bmc_ptr->damage, 0);
    return true;
}

bool cast_blue_curse_1(player_type *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir))
        return false;

    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, RF_ABILITY::CAUSE_1, bmc_ptr->plev, DAM_ROLL);
    fire_ball_hide(player_ptr, GF_CAUSE_1, bmc_ptr->dir, bmc_ptr->damage, 0);
    return true;
}

bool cast_blue_curse_2(player_type *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir))
        return false;

    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, RF_ABILITY::CAUSE_2, bmc_ptr->plev, DAM_ROLL);
    fire_ball_hide(player_ptr, GF_CAUSE_2, bmc_ptr->dir, bmc_ptr->damage, 0);
    return true;
}

bool cast_blue_curse_3(player_type *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir))
        return false;

    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, RF_ABILITY::CAUSE_3, bmc_ptr->plev, DAM_ROLL);
    fire_ball_hide(player_ptr, GF_CAUSE_3, bmc_ptr->dir, bmc_ptr->damage, 0);
    return true;
}

bool cast_blue_curse_4(player_type *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir))
        return false;

    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, RF_ABILITY::CAUSE_4, bmc_ptr->plev, DAM_ROLL);
    fire_ball_hide(player_ptr, GF_CAUSE_4, bmc_ptr->dir, bmc_ptr->damage, 0);
    return true;
}
