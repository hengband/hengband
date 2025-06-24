#include "mind/mind-hobbit.h"
#include "floor/floor-object.h"
#include "sv-definition/sv-food-types.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

bool create_ration(PlayerType *player_ptr)
{
    ItemEntity item({ ItemKindType::FOOD, SV_FOOD_RATION });
    (void)drop_near(player_ptr, item, player_ptr->get_position());
    msg_print(_("食事を料理して作った。", "You cook some food."));
    return true;
}
