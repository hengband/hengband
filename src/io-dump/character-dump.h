#pragma once

#include "system/angband.h"
#include "io/files-util.h"

class PlayerType;
void make_character_dump(PlayerType *player_ptr, FILE *fff, display_player_pf display_player);
