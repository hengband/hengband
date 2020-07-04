#pragma once

#include "system/angband.h"

bool artifact_scroll(player_type* caster_ptr);
bool mundane_spell(player_type* ownner_ptr, bool only_equip);
bool recharge(player_type* caster_ptr, int power);
void massacre(player_type* caster_ptr);
bool shock_power(player_type* caster_ptr);
