#include "specific-object/muramasa.h"
#include "artifact/fixed-art-types.h"
#include "core/asking-player.h"
#include "spell/spells-object.h"
#include "status/base-status.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

bool activate_muramasa(PlayerType *player_ptr, ItemEntity *o_ptr)
{
    if (!o_ptr->is_specific_artifact(FixedArtifactId::MURAMASA)) {
        return false;
    }

    if (!input_check(_("本当に使いますか？", "Are you sure?! "))) {
        return true;
    }

    msg_print(_("村正が震えた．．．", "The Muramasa pulsates..."));
    do_inc_stat(player_ptr, A_STR);
    if (one_in_(2)) {
        msg_print(_("村正は壊れた！", "The Muramasa is destroyed!"));
        curse_weapon_object(player_ptr, true, o_ptr);
    }

    return true;
}
