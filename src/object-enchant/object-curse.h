#pragma once

#include "system/angband.h"

typedef struct object_type object_type;
typedef struct player_type player_type;
enum class TRC;
TRC get_curse(int power, object_type *o_ptr);
void curse_equipment(player_type *owner_ptr, PERCENTAGE chance, PERCENTAGE heavy_chance);
