#pragma once

#include "system/angband.h"
#include "io/files-util.h"

struct player_type;
void dump_aux_player_status(player_type *player_ptr, FILE *fff, display_player_pf display_player);
