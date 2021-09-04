#pragma once

#include "system/angband.h"

extern bool show_gold_on_floor;

enum target_type : uint32_t;
struct player_type;
char examine_grid(player_type *subject_ptr, const POSITION y, const POSITION x, target_type mode, concptr info);
