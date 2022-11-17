#pragma once

#include "system/angband.h"

// Activation Execution.
class ItemEntity;
struct ae_type {
    DIRECTION dir;
    bool success;
    ItemEntity *o_ptr;
    DEPTH lev;
    int chance;
    int fail;
};

class PlayerType;
ae_type *initialize_ae_type(PlayerType *player_ptr, ae_type *ae_ptr, const INVENTORY_IDX item);
