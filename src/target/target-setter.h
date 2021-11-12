#pragma once

#include<stdint.h>

enum target_type : uint32_t;
class PlayerType;
bool target_set(PlayerType *player_ptr, target_type mode);
