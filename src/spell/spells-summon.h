#pragma once

#include "system/angband.h"

enum summon_type : int;

struct object_type;
struct player_type;
bool trump_summoning(player_type *player_ptr, int num, bool pet, POSITION y, POSITION x, DEPTH lev, summon_type type, BIT_FLAGS mode);
bool cast_summon_demon(player_type *player_ptr, int power);
bool cast_summon_undead(player_type *player_ptr, int power);
bool cast_summon_hound(player_type *player_ptr, int power);
bool cast_summon_elemental(player_type *player_ptr, int power);
bool cast_summon_octopus(player_type *player_ptr);
bool cast_summon_greater_demon(player_type *player_ptr);
bool summon_kin_player(player_type *player_ptr, DEPTH level, POSITION y, POSITION x, BIT_FLAGS mode);
void mitokohmon(player_type *player_ptr);
int summon_cyber(player_type *player_ptr, MONSTER_IDX who, POSITION y, POSITION x);
int activate_hi_summon(player_type *player_ptr, POSITION y, POSITION x, bool can_pet);
void cast_invoke_spirits(player_type *player_ptr, DIRECTION dir);
