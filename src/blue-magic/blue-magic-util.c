#include "blue-magic/blue-magic-util.h"
#include "monster-floor/place-monster-types.h"

blue_magic_type *initialize_blue_magic_type(
    player_type *caster_ptr, blue_magic_type *bm_ptr, const bool success, get_pseudo_monstetr_level_pf get_pseudo_monstetr_level)
{
    bm_ptr->plev = (*get_pseudo_monstetr_level)(caster_ptr);
    bm_ptr->summon_lev = caster_ptr->lev * 2 / 3 + randint1(caster_ptr->lev / 2);
    bm_ptr->damage = 0;
    bm_ptr->pet = success; // read-only.
    bm_ptr->no_trump = FALSE;
    bm_ptr->p_mode = bm_ptr->pet ? PM_FORCE_PET : PM_NO_PET;
    bm_ptr->u_mode = 0L;
    bm_ptr->g_mode = bm_ptr->pet ? 0 : PM_ALLOW_GROUP;
    if (!success || (randint1(50 + bm_ptr->plev) < bm_ptr->plev / 10))
        bm_ptr->u_mode = PM_ALLOW_UNIQUE;

    return bm_ptr;
}
