#include "specific-object/muramasa.h"
#include "artifact/fixed-art-types.h"
#include "core/asking-player.h"
#include "spell/spells-object.h"
#include "status/base-status.h"
#include "system/object-type-definition.h"
#include "view/display-messages.h"

bool activate_muramasa(player_type *user_ptr, object_type *o_ptr)
{
    if (o_ptr->name1 != ART_MURAMASA)
        return FALSE;

    if (!get_check(_("本当に使いますか？", "Are you sure?!")))
        return TRUE;

    msg_print(_("村正が震えた．．．", "The Muramasa pulsates..."));
    do_inc_stat(user_ptr, A_STR);
    if (one_in_(2)) {
        msg_print(_("村正は壊れた！", "The Muramasa is destroyed!"));
        curse_weapon_object(user_ptr, TRUE, o_ptr);
    }

    return TRUE;
}
