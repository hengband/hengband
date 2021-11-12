#pragma once

#include "system/angband.h"
#include "info-reader/parse-error-types.h"

class PlayerType;
parse_error_type parse_fixed_map(PlayerType *player_ptr, concptr name, int ymin, int xmin, int ymax, int xmax);
