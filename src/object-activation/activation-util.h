﻿#pragma once

#include "system/angband.h"

// Activation Execution.
struct object_type;
typedef struct ae_type {
    DIRECTION dir;
    bool success;
    object_type *o_ptr;
    DEPTH lev;
    int chance;
    int fail;
} ae_type;

class PlayerType;
ae_type *initialize_ae_type(PlayerType *player_ptr, ae_type *ae_ptr, const INVENTORY_IDX item);
