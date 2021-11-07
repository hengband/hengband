#pragma once
#include "system/angband.h"

typedef struct monster_death_type monster_death_type;
struct player_type;
void switch_special_death(player_type *player_ptr, monster_death_type *md_ptr, EFFECT_ID effect_type);
