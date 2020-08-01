#include "blue-magic/blue-magic-spirit-curse.h"
#include "blue-magic/blue-magic-util.h"
#include "mspell/mspell-damage-calculator.h"
#include "mspell/mspell-type.h"
#include "spell-kind/spells-launcher.h"
#include "spell/spell-types.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

bool cast_blue_drain_mana(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_DRAIN_MANA, bmc_ptr->plev, DAM_ROLL);
    fire_ball_hide(caster_ptr, GF_DRAIN_MANA, bmc_ptr->dir, bmc_ptr->damage, 0);
    return TRUE;
}

bool cast_blue_mind_blast(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_MIND_BLAST, bmc_ptr->plev, DAM_ROLL);
    fire_ball_hide(caster_ptr, GF_MIND_BLAST, bmc_ptr->dir, bmc_ptr->damage, 0);
    return TRUE;
}

bool cast_blue_brain_smash(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_BRAIN_SMASH, bmc_ptr->plev, DAM_ROLL);
    fire_ball_hide(caster_ptr, GF_BRAIN_SMASH, bmc_ptr->dir, bmc_ptr->damage, 0);
    return TRUE;
}

bool cast_blue_curse_1(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_CAUSE_1, bmc_ptr->plev, DAM_ROLL);
    fire_ball_hide(caster_ptr, GF_CAUSE_1, bmc_ptr->dir, bmc_ptr->damage, 0);
    return TRUE;
}

bool cast_blue_curse_2(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_CAUSE_2, bmc_ptr->plev, DAM_ROLL);
    fire_ball_hide(caster_ptr, GF_CAUSE_2, bmc_ptr->dir, bmc_ptr->damage, 0);
    return TRUE;
}

bool cast_blue_curse_3(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_CAUSE_3, bmc_ptr->plev, DAM_ROLL);
    fire_ball_hide(caster_ptr, GF_CAUSE_3, bmc_ptr->dir, bmc_ptr->damage, 0);
    return TRUE;
}

bool cast_blue_curse_4(player_type *caster_ptr, bmc_type *bmc_ptr)
{
    if (!get_aim_dir(caster_ptr, &bmc_ptr->dir))
        return FALSE;

    bmc_ptr->damage = monspell_bluemage_damage(caster_ptr, MS_CAUSE_4, bmc_ptr->plev, DAM_ROLL);
    fire_ball_hide(caster_ptr, GF_CAUSE_4, bmc_ptr->dir, bmc_ptr->damage, 0);
    return TRUE;
}
