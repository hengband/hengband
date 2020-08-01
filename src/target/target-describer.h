#pragma once

#include "system/angband.h"

extern bool show_gold_on_floor;

typedef enum target_type target_type;
char examine_grid(player_type *subject_ptr, const POSITION y, const POSITION x, target_type mode, concptr info);
