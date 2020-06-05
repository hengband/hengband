#pragma once

#include "system/angband.h"

bool eat_magic(player_type* caster_ptr, int power);
void wild_magic(player_type* caster_ptr, int spell);
bool cast_wrath_of_the_god(player_type* caster_ptr, HIT_POINT dam, POSITION rad);
void cast_wonder(player_type* caster_ptr, DIRECTION dir);
bool vampirism(player_type* caster_ptr);
bool android_inside_weapon(player_type* creature_ptr);
void hayagake(player_type* creature_ptr);
bool double_attack(player_type* creature_ptr);
bool vanish_dungeon(player_type* caster_ptr);
