#include "object-activation/activation-charm.h"
#include "spell-kind/spells-charm.h"
#include "spell-kind/spells-sight.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"

bool activate_charm_animal(player_type *player_ptr)
{
    DIRECTION dir;
    if (!get_aim_dir(player_ptr, &dir))
        return false;

    (void)charm_animal(player_ptr, dir, player_ptr->lev);
    return true;
}

bool activate_charm_undead(player_type *player_ptr)
{
    DIRECTION dir;
    if (!get_aim_dir(player_ptr, &dir))
        return false;

    (void)control_one_undead(player_ptr, dir, player_ptr->lev);
    return true;
}

bool activate_charm_other(player_type *player_ptr)
{
    DIRECTION dir;
    if (!get_aim_dir(player_ptr, &dir))
        return false;

    (void)charm_monster(player_ptr, dir, player_ptr->lev * 2);
    return true;
}

bool activate_charm_animals(player_type *player_ptr)
{
    (void)charm_animals(player_ptr, player_ptr->lev * 2);
    return true;
}

bool activate_charm_others(player_type *player_ptr)
{
    (void)charm_monsters(player_ptr, player_ptr->lev * 2);
    return true;
}
