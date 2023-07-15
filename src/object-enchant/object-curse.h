#pragma once

#include "system/angband.h"

class ItemEntity;
class PlayerType;
enum class CurseTraitType;
CurseTraitType get_curse(int power, ItemEntity *o_ptr);
void curse_equipment(PlayerType *player_ptr, PERCENTAGE chance, PERCENTAGE heavy_chance);
