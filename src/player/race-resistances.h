#pragma once

#include "system/angband.h"

typedef struct player_type player_type;
void player_immunity(player_type *creature_ptr, TrFlags &flags);
void tim_player_immunity(player_type *creature_ptr, TrFlags &flags);
void known_obj_immunity(player_type *creature_ptr, TrFlags &flags);
void player_vulnerability_flags(player_type *creature_ptr, TrFlags &flags);
