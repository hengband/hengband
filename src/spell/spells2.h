#pragma once

#include "system/angband.h"

void call_chaos(player_type* caster_ptr);
bool hypodynamic_bolt(player_type* caster_ptr, DIRECTION dir, HIT_POINT dam);
bool death_ray(player_type* caster_ptr, DIRECTION dir, PLAYER_LEVEL plev);
bool activate_ty_curse(player_type* target_ptr, bool stop_ty, int* count);
int activate_hi_summon(player_type* caster_ptr, POSITION y, POSITION x, bool can_pet);
void wall_breaker(player_type* caster_ptr);
bool charm_monster(player_type* caster_ptr, DIRECTION dir, PLAYER_LEVEL plev);
bool control_one_undead(player_type* caster_ptr, DIRECTION dir, PLAYER_LEVEL plev);
bool control_one_demon(player_type* caster_ptr, DIRECTION dir, PLAYER_LEVEL plev);
bool charm_animal(player_type* caster_ptr, DIRECTION dir, PLAYER_LEVEL plev);
bool eat_magic(player_type* caster_ptr, int power);
void ring_of_power(player_type* caster_ptr, DIRECTION dir);
void wild_magic(player_type* caster_ptr, int spell);
void cast_meteor(player_type* caster_ptr, HIT_POINT dam, POSITION rad);
bool cast_wrath_of_the_god(player_type* caster_ptr, HIT_POINT dam, POSITION rad);
void cast_wonder(player_type* caster_ptr, DIRECTION dir);
void cast_invoke_spirits(player_type* caster_ptr, DIRECTION dir);
void cast_shuffle(player_type* caster_ptr);
bool vampirism(player_type* caster_ptr);
bool hit_and_away(player_type* caster_ptr);
bool android_inside_weapon(player_type* creature_ptr);
bool create_ration(player_type* crature_ptr);
void hayagake(player_type* creature_ptr);
bool double_attack(player_type* creature_ptr);
bool comvert_hp_to_mp(player_type* creature_ptr);
bool comvert_mp_to_hp(player_type* creature_ptr);
bool sword_dancing(player_type* creature_ptr);
bool clear_mind(player_type* creature_ptr);
bool vanish_dungeon(player_type* caster_ptr);
