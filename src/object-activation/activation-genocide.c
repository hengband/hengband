#include "object-activation/activation-genocide.h"
#include "spell-kind/spells-genocide.h"
#include "view/display-messages.h"

bool activate_genocide(player_type *user_ptr)
{
    msg_print(_("深青色に輝いている...", "It glows deep blue..."));
    (void)symbol_genocide(user_ptr, 200, TRUE);
    return TRUE;
}

bool activate_mass_genocide(player_type *user_ptr)
{
    msg_print(_("ひどく鋭い音が流れ出た...", "It lets out a long, shrill note..."));
    (void)mass_genocide(user_ptr, 200, TRUE);
    return TRUE;
}
