#pragma once

#include "system/angband.h"

class PlayerType;
void vault_monsters(PlayerType *player_ptr, POSITION y1, POSITION x1, int num);
void vault_objects(PlayerType *player_ptr, POSITION y, POSITION x, int num);
void vault_traps(PlayerType *player_ptr, POSITION y, POSITION x, POSITION yd, POSITION xd, int num);
