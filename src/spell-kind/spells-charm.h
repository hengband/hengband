#pragma once

#include "system/angband.h"

class PlayerType;
bool charm_monster(PlayerType *player_ptr, DIRECTION dir, PLAYER_LEVEL plev);
bool control_one_undead(PlayerType *player_ptr, DIRECTION dir, PLAYER_LEVEL plev);
bool control_one_demon(PlayerType *player_ptr, DIRECTION dir, PLAYER_LEVEL plev);
bool charm_animal(PlayerType *player_ptr, DIRECTION dir, PLAYER_LEVEL plev);
