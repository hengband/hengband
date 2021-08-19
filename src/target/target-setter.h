#pragma once

#include<stdint.h>

enum target_type : uint32_t;
typedef struct player_type player_type;
bool target_set(player_type *creature_ptr, target_type mode);
