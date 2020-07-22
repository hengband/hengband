#pragma once

#include "system/angband.h"

BIT_FLAGS get_curse(player_type *owner_ptr, int power, object_type *o_ptr);
void curse_equipment(player_type *owner_ptr, PERCENTAGE chance, PERCENTAGE heavy_chance);
