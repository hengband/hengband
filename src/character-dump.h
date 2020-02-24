#pragma once

#include "angband.h"

 errr make_character_dump(player_type *creature_ptr, FILE *fff, void(*update_playtime)(void), void(*display_player)(player_type*, int));
