#include "object-activation/activation-genocide.h"
#include "spell-kind/spells-genocide.h"
#include "view/display-messages.h"

bool activate_genocide(player_type *user_ptr)
{
    msg_print(_("[ÂF‚É‹P‚¢‚Ä‚¢‚é...", "It glows deep blue..."));
    (void)symbol_genocide(user_ptr, 200, TRUE);
    return TRUE;
}

bool activate_mass_genocide(player_type *user_ptr)
{
    msg_print(_("‚Ğ‚Ç‚­‰s‚¢‰¹‚ª—¬‚êo‚½...", "It lets out a long, shrill note..."));
    (void)mass_genocide(user_ptr, 200, TRUE);
    return TRUE;
}
