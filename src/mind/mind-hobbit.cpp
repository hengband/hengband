#include "mind/mind-hobbit.h"
#include "floor/floor-object.h"
#include "object/object-kind-hook.h"
#include "sv-definition/sv-food-types.h"
#include "system/baseitem-info-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

bool create_ration(PlayerType *player_ptr)
{
    ObjectType forge;
    auto *q_ptr = &forge;
    q_ptr->prep(lookup_baseitem_id({ ItemKindType::FOOD, SV_FOOD_RATION }));
    (void)drop_near(player_ptr, q_ptr, -1, player_ptr->y, player_ptr->x);
    msg_print(_("食事を料理して作った。", "You cook some food."));
    return true;
}
