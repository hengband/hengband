#include "mind/mind-hobbit.h"
#include "floor/floor-object.h"
#include "object/object-kind-hook.h"
#include "sv-definition/sv-food-types.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

bool create_ration(player_type *creature_ptr)
{
    object_type forge;
    object_type *q_ptr = &forge;
    q_ptr->prep(lookup_kind(TV_FOOD, SV_FOOD_RATION));
    (void)drop_near(creature_ptr, q_ptr, -1, creature_ptr->y, creature_ptr->x);
    msg_print(_("食事を料理して作った。", "You cook some food."));
    return true;
}
