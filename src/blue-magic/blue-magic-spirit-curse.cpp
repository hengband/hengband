/*!
 * @file blue-magic-spirit-curse.cpp
 * @brief 青魔法の呪い系処理定義
 */

#include "blue-magic/blue-magic-spirit-curse.h"
#include "blue-magic/blue-magic-util.h"
#include "effect/attribute-types.h"
#include "monster-race/race-ability-flags.h"
#include "mspell/mspell-damage-calculator.h"
#include "spell-kind/spells-launcher.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

bool cast_blue_drain_mana(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, MonsterAbilityType::DRAIN_MANA, bmc_ptr->plev, DAM_ROLL);
    fire_ball_hide(player_ptr, AttributeType::DRAIN_MANA, bmc_ptr->dir, bmc_ptr->damage, 0);
    return true;
}

bool cast_blue_mind_blast(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, MonsterAbilityType::MIND_BLAST, bmc_ptr->plev, DAM_ROLL);
    fire_ball_hide(player_ptr, AttributeType::MIND_BLAST, bmc_ptr->dir, bmc_ptr->damage, 0);
    return true;
}

bool cast_blue_brain_smash(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, MonsterAbilityType::BRAIN_SMASH, bmc_ptr->plev, DAM_ROLL);
    fire_ball_hide(player_ptr, AttributeType::BRAIN_SMASH, bmc_ptr->dir, bmc_ptr->damage, 0);
    return true;
}

bool cast_blue_curse_1(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, MonsterAbilityType::CAUSE_1, bmc_ptr->plev, DAM_ROLL);
    fire_ball_hide(player_ptr, AttributeType::CAUSE_1, bmc_ptr->dir, bmc_ptr->damage, 0);
    return true;
}

bool cast_blue_curse_2(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, MonsterAbilityType::CAUSE_2, bmc_ptr->plev, DAM_ROLL);
    fire_ball_hide(player_ptr, AttributeType::CAUSE_2, bmc_ptr->dir, bmc_ptr->damage, 0);
    return true;
}

bool cast_blue_curse_3(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, MonsterAbilityType::CAUSE_3, bmc_ptr->plev, DAM_ROLL);
    fire_ball_hide(player_ptr, AttributeType::CAUSE_3, bmc_ptr->dir, bmc_ptr->damage, 0);
    return true;
}

bool cast_blue_curse_4(PlayerType *player_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(player_ptr, &bmc_ptr->dir)) {
        return false;
    }

    bmc_ptr->damage = monspell_bluemage_damage(player_ptr, MonsterAbilityType::CAUSE_4, bmc_ptr->plev, DAM_ROLL);
    fire_ball_hide(player_ptr, AttributeType::CAUSE_4, bmc_ptr->dir, bmc_ptr->damage, 0);
    return true;
}
