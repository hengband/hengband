#pragma once

#include "system/angband.h"

enum door_kind_type : int;
class PlayerType;
void add_door(PlayerType *player_ptr, POSITION x, POSITION y);
void place_secret_door(PlayerType *player_ptr, POSITION y, POSITION x, door_kind_type type);
void place_locked_door(PlayerType *player_ptr, POSITION y, POSITION x);
void place_random_door(PlayerType *player_ptr, POSITION y, POSITION x, bool room);
void place_closed_door(PlayerType *player_ptr, POSITION y, POSITION x, door_kind_type type);
