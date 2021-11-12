#pragma once

#include "system/angband.h"

struct object_type;
class PlayerType;
enum class CurseTraitType;
CurseTraitType get_curse(int power, object_type *o_ptr);
void curse_equipment(PlayerType *player_ptr, PERCENTAGE chance, PERCENTAGE heavy_chance);
