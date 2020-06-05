#pragma once

#include "system/angband.h"

bool eat_magic(player_type* caster_ptr, int power);
void wild_magic(player_type* caster_ptr, int spell);
void cast_meteor(player_type* caster_ptr, HIT_POINT dam, POSITION rad);
bool cast_wrath_of_the_god(player_type* caster_ptr, HIT_POINT dam, POSITION rad);
void cast_wonder(player_type* caster_ptr, DIRECTION dir);
bool vampirism(player_type* caster_ptr);
bool android_inside_weapon(player_type* creature_ptr);
bool create_ration(player_type* crature_ptr);
void hayagake(player_type* creature_ptr);
bool double_attack(player_type* creature_ptr);
bool comvert_hp_to_mp(player_type* creature_ptr);
bool comvert_mp_to_hp(player_type* creature_ptr);
bool sword_dancing(player_type* creature_ptr);
bool clear_mind(player_type* creature_ptr);
bool vanish_dungeon(player_type* caster_ptr);
