#pragma once

#include "system/angband.h"

struct object_type;
struct player_type;
enum class TRC;
TRC get_curse(int power, object_type *o_ptr);
void curse_equipment(player_type *player_ptr, PERCENTAGE chance, PERCENTAGE heavy_chance);
