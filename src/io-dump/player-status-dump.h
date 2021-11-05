#pragma once

#include "system/angband.h"
#include "io/files-util.h"

class PlayerType;
void dump_aux_player_status(PlayerType *player_ptr, FILE *fff, display_player_pf display_player);
