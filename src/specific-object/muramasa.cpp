#include "specific-object/muramasa.h"
#include "artifact/fixed-art-types.h"
#include "core/asking-player.h"
#include "spell/spells-object.h"
#include "status/base-status.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

bool activate_muramasa(PlayerType *player_ptr, ObjectType *o_ptr)
{
    if (o_ptr->fixed_artifact_idx != ART_MURAMASA)
        return false;

    if (!get_check(_("本当に使いますか？", "Are you sure?! ")))
        return true;

    msg_print(_("村正が震えた．．．", "The Muramasa pulsates..."));
    do_inc_stat(player_ptr, A_STR);
    if (one_in_(2)) {
        msg_print(_("村正は壊れた！", "The Muramasa is destroyed!"));
        curse_weapon_object(player_ptr, true, o_ptr);
    }

    return true;
}
