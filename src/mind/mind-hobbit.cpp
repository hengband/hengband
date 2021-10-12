#include "mind/mind-hobbit.h"
#include "floor/floor-object.h"
#include "object/object-kind-hook.h"
#include "sv-definition/sv-food-types.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

bool create_ration(player_type *player_ptr)
{
    object_type forge;
    object_type *q_ptr = &forge;
    q_ptr->prep(lookup_kind(ItemKindType::FOOD, SV_FOOD_RATION));
    (void)drop_near(player_ptr, q_ptr, -1, player_ptr->y, player_ptr->x);
    msg_print(_("食事を料理して作った。", "You cook some food."));
    return true;
}
