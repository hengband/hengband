﻿#include "object-activation/activation-util.h"
#include "object/object-info.h"
#include "system/baseitem-info.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"

ae_type *initialize_ae_type(PlayerType *player_ptr, ae_type *ae_ptr, const INVENTORY_IDX item)
{
    ae_ptr->o_ptr = ref_item(player_ptr, item);
    ae_ptr->lev = baseitems_info.at(ae_ptr->o_ptr->bi_id).level;
    return ae_ptr;
}
