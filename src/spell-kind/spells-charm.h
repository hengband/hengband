﻿#pragma once

#include "system/angband.h"

class player_type;
bool charm_monster(player_type *player_ptr, DIRECTION dir, PLAYER_LEVEL plev);
bool control_one_undead(player_type *player_ptr, DIRECTION dir, PLAYER_LEVEL plev);
bool control_one_demon(player_type *player_ptr, DIRECTION dir, PLAYER_LEVEL plev);
bool charm_animal(player_type *player_ptr, DIRECTION dir, PLAYER_LEVEL plev);
