#pragma once

#include "system/angband.h"
#include <tl/optional.hpp>

class PlayerType;
tl::optional<int> display_player(PlayerType *player_ptr, const int tmp_mode);
void display_player_equippy(PlayerType *player_ptr, TERM_LEN y, TERM_LEN x, BIT_FLAGS16 mode);
