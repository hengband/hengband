#pragma once

#include "system/angband.h"

void curse_artifact(const player_type *player_ptr, object_type *o_ptr);
void get_random_name(object_type *o_ptr, char *return_name, bool armour, int power);
bool has_extreme_damage_rate(const player_type *player_ptr, object_type *o_ptr);
