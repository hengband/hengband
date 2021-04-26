#pragma once

#include "system/angband.h"

typedef struct player_type player_type;
bool create_rune_protection_one(player_type *caster_ptr);
bool create_rune_explosion(player_type *caster_ptr, POSITION y, POSITION x);
void stair_creation(player_type *caster_ptr);
