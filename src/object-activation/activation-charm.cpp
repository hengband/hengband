#include "object-activation/activation-charm.h"
#include "spell-kind/spells-charm.h"
#include "spell-kind/spells-sight.h"
#include "target/target-getter.h"

bool activate_charm_animal(player_type *user_ptr)
{
    DIRECTION dir;
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    (void)charm_animal(user_ptr, dir, user_ptr->lev);
    return TRUE;
}

bool activate_charm_undead(player_type *user_ptr)
{
    DIRECTION dir;
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    (void)control_one_undead(user_ptr, dir, user_ptr->lev);
    return TRUE;
}

bool activate_charm_other(player_type *user_ptr)
{
    DIRECTION dir;
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    (void)charm_monster(user_ptr, dir, user_ptr->lev * 2);
    return TRUE;
}

bool activate_charm_animals(player_type *user_ptr)
{
    (void)charm_animals(user_ptr, user_ptr->lev * 2);
    return TRUE;
}

bool activate_charm_others(player_type *user_ptr)
{
    (void)charm_monsters(user_ptr, user_ptr->lev * 2);
    return TRUE;
}
