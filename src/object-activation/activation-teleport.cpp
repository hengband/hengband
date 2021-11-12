#include "object-activation/activation-teleport.h"
#include "cmd-io/cmd-save.h"
#include "core/asking-player.h"
#include "game-option/special-options.h"
#include "spell-kind/spells-grid.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "effect/attribute-types.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

bool activate_teleport_away(PlayerType *player_ptr)
{
    DIRECTION dir;
    if (!get_aim_dir(player_ptr, &dir))
        return false;

    (void)fire_beam(player_ptr, AttributeType::AWAY_ALL, dir, player_ptr->lev);
    return true;
}

bool activate_escape(PlayerType *player_ptr)
{
    switch (randint1(13)) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        teleport_player(player_ptr, 10, TELEPORT_SPONTANEOUS);
        return true;
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
        teleport_player(player_ptr, 222, TELEPORT_SPONTANEOUS);
        return true;
    case 11:
    case 12:
        (void)stair_creation(player_ptr);
        return true;
    default:
        if (!get_check(_("この階を去りますか？", "Leave this level? ")))
            return true;

        if (autosave_l)
            do_cmd_save_game(player_ptr, true);

        player_ptr->leaving = true;
        return true;
    }
}

bool activate_teleport_level(PlayerType *player_ptr)
{
    if (!get_check(_("本当に他の階にテレポートしますか？", "Are you sure? (Teleport Level)")))
        return false;

    teleport_level(player_ptr, 0);
    return true;
}

bool activate_dimension_door(PlayerType *player_ptr)
{
    msg_print(_("次元の扉が開いた。目的地を選んで下さい。", "You open a dimensional gate. Choose a destination."));
    return dimension_door(player_ptr);
}

bool activate_teleport(PlayerType *player_ptr)
{
    msg_print(_("周りの空間が歪んでいる...", "It twists space around you..."));
    teleport_player(player_ptr, 100, TELEPORT_SPONTANEOUS);
    return true;
}

bool activate_phase_door(PlayerType *player_ptr)
{
    teleport_player(player_ptr, 10, TELEPORT_SPONTANEOUS);
    return true;
}
