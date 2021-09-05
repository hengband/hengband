#pragma once

#include "system/angband.h"

struct player_type;
void vault_monsters(player_type *player_ptr, POSITION y1, POSITION x1, int num);
void vault_objects(player_type *player_ptr, POSITION y, POSITION x, int num);
void vault_traps(player_type *player_ptr, POSITION y, POSITION x, POSITION yd, POSITION xd, int num);
