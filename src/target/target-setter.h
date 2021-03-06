#pragma once

#include "system/angband.h"

enum target_type : uint8_t;
bool target_set(player_type *creature_ptr, target_type mode);
