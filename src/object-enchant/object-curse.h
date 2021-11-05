#pragma once

#include "system/angband.h"

struct object_type;
class PlayerType;
enum class TRC;
TRC get_curse(int power, object_type *o_ptr);
void curse_equipment(PlayerType *player_ptr, PERCENTAGE chance, PERCENTAGE heavy_chance);
