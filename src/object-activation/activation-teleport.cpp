#include "object-activation/activation-teleport.h"
#include "cmd-io/cmd-save.h"
#include "core/asking-player.h"
#include "game-option/special-options.h"
#include "spell-kind/spells-grid.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell/spell-types.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

bool activate_teleport_away(player_type *user_ptr)
{
    DIRECTION dir;
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    (void)fire_beam(user_ptr, GF_AWAY_ALL, dir, user_ptr->lev);
    return TRUE;
}

bool activate_escape(player_type *user_ptr)
{
    switch (randint1(13)) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        teleport_player(user_ptr, 10, TELEPORT_SPONTANEOUS);
        return TRUE;
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
        teleport_player(user_ptr, 222, TELEPORT_SPONTANEOUS);
        return TRUE;
    case 11:
    case 12:
        (void)stair_creation(user_ptr);
        return TRUE;
    default:
        if (!get_check(_("この階を去りますか？", "Leave this level? ")))
            return TRUE;

        if (autosave_l)
            do_cmd_save_game(user_ptr, TRUE);

        user_ptr->leaving = TRUE;
        return TRUE;
    }
}

bool activate_teleport_level(player_type *user_ptr)
{
    if (!get_check(_("本当に他の階にテレポートしますか？", "Are you sure? (Teleport Level)")))
        return FALSE;

    teleport_level(user_ptr, 0);
    return TRUE;
}

bool activate_dimension_door(player_type *user_ptr)
{
    msg_print(_("次元の扉が開いた。目的地を選んで下さい。", "You open a dimensional gate. Choose a destination."));
    return dimension_door(user_ptr);
}

bool activate_teleport(player_type *user_ptr)
{
    msg_print(_("周りの空間が歪んでいる...", "It twists space around you..."));
    teleport_player(user_ptr, 100, TELEPORT_SPONTANEOUS);
    return TRUE;
}

bool activate_phase_door(player_type *user_ptr)
{
    teleport_player(user_ptr, 10, TELEPORT_SPONTANEOUS);
    return TRUE;
}
