#include "specific-object/bloody-moon.h"
#include "art-definition/art-weapon-types.h"
#include "core/player-update-types.h"
#include "racial/racial-android.h"
#include "spell/spells-object.h" // get_bloody_moon_flags() ‚ªˆË‘¶AŒã‚Åˆø‚Á‰z‚·.
#include "system/object-type-definition.h"
#include "view/display-messages.h"

bool activate_bloody_moon(player_type *user_ptr, object_type *o_ptr)
{
    if (o_ptr->name1 != ART_BLOOD)
        return FALSE;

    msg_print(_("Š™‚ª–¾‚é‚­‹P‚¢‚½...", "Your scythe glows brightly!"));
    get_bloody_moon_flags(o_ptr);
    if (user_ptr->prace == RACE_ANDROID)
        calc_android_exp(user_ptr);

    user_ptr->update |= PU_BONUS | PU_HP;
    return TRUE;
}
