#pragma once

#include "system/angband.h"
#include <tl/optional.hpp>
#include <vector>

class PlayerType;
struct saved_floor_type;
errr rd_saved_floor(PlayerType *player_ptr, saved_floor_type *sf_ptr);
bool load_floor(PlayerType *player_ptr, saved_floor_type *sf_ptr, BIT_FLAGS mode);
saved_floor_type rd_saved_floor_header();
tl::optional<std::vector<byte>> read_saved_floor_data(const saved_floor_type &sf);
