#pragma once

#include "target/target.h"
#include <cstdint>

enum target_type : uint32_t;
class PlayerType;
Target target_set(PlayerType *player_ptr, target_type mode);
