#pragma once

#include<stdint.h>

enum target_type : uint32_t;
struct player_type;
bool target_set(player_type *player_ptr, target_type mode);
