#pragma once

#include "system/angband.h"

enum summon_type : int;

class ItemEntity;
class PlayerType;
bool trump_summoning(PlayerType *player_ptr, int num, bool pet, POSITION y, POSITION x, DEPTH lev, summon_type type, BIT_FLAGS mode);
bool cast_summon_demon(PlayerType *player_ptr, int power);
bool cast_summon_undead(PlayerType *player_ptr, int power);
bool cast_summon_hound(PlayerType *player_ptr, int power);
bool cast_summon_elemental(PlayerType *player_ptr, int power);
bool cast_summon_octopus(PlayerType *player_ptr);
bool cast_summon_greater_demon(PlayerType *player_ptr);
bool summon_kin_player(PlayerType *player_ptr, DEPTH level, POSITION y, POSITION x, BIT_FLAGS mode);
void mitokohmon(PlayerType *player_ptr);
int summon_cyber(PlayerType *player_ptr, MONSTER_IDX who, POSITION y, POSITION x);
int activate_hi_summon(PlayerType *player_ptr, POSITION y, POSITION x, bool can_pet);
void cast_invoke_spirits(PlayerType *player_ptr, DIRECTION dir);
