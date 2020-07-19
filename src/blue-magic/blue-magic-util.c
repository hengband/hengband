#include "blue-magic/blue-magic-util.h"
#include "monster-floor/place-monster-types.h"

bmc_type *initialize_blue_magic_type(
    player_type *caster_ptr, bmc_type *bmc_ptr, const bool success, get_pseudo_monstetr_level_pf get_pseudo_monstetr_level)
{
    bmc_ptr->plev = (*get_pseudo_monstetr_level)(caster_ptr);
    bmc_ptr->summon_lev = caster_ptr->lev * 2 / 3 + randint1(caster_ptr->lev / 2);
    bmc_ptr->damage = 0;
    bmc_ptr->pet = success; // read-only.
    bmc_ptr->no_trump = FALSE;
    bmc_ptr->p_mode = bmc_ptr->pet ? PM_FORCE_PET : PM_NO_PET;
    bmc_ptr->u_mode = 0L;
    bmc_ptr->g_mode = bmc_ptr->pet ? 0 : PM_ALLOW_GROUP;
    if (!success || (randint1(50 + bmc_ptr->plev) < bmc_ptr->plev / 10))
        bmc_ptr->u_mode = PM_ALLOW_UNIQUE;

    return bmc_ptr;
}
