/*!
 * @file blue-magic-util.cpp
 * @brief 青魔法の構造体、初期化処理定義
 */

#include "blue-magic/blue-magic-util.h"
#include "monster-floor/place-monster-types.h"
#include "system/player-type-definition.h"

bmc_type *initialize_blue_magic_type(
    PlayerType *player_ptr, bmc_type *bmc_ptr, const bool success, get_pseudo_monstetr_level_pf get_pseudo_monstetr_level)
{
    bmc_ptr->plev = (*get_pseudo_monstetr_level)(player_ptr);
    bmc_ptr->summon_lev = player_ptr->lev * 2 / 3 + randint1(player_ptr->lev / 2);
    bmc_ptr->damage = 0;
    bmc_ptr->pet = success; // read-only.
    bmc_ptr->no_trump = false;
    bmc_ptr->p_mode = bmc_ptr->pet ? PM_FORCE_PET : PM_NO_PET;
    bmc_ptr->u_mode = 0L;
    bmc_ptr->g_mode = bmc_ptr->pet ? 0 : PM_ALLOW_GROUP;
    if (!success || (randint1(50 + bmc_ptr->plev) < bmc_ptr->plev / 10))
        bmc_ptr->u_mode = PM_ALLOW_UNIQUE;

    return bmc_ptr;
}
