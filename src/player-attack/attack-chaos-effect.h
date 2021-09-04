#pragma once

#include "system/angband.h"

struct player_attack_type;
struct player_type;
void change_monster_stat(player_type *attacker_ptr, player_attack_type *pa_ptr, const POSITION y, const POSITION x, int *num);
