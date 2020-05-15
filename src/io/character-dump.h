#pragma once

#include "system/angband.h"
#include "files.h"

void make_character_dump(player_type *creature_ptr, FILE *fff, void(*update_playtime)(void), display_player_pf display_player, map_name_pf map_name);
