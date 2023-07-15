#pragma once

#include "system/angband.h"

struct player_attack_type;
class PlayerType;
void change_monster_stat(PlayerType *player_ptr, player_attack_type *pa_ptr, const POSITION y, const POSITION x, int *num);
