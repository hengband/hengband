#pragma once

#include "lore/lore-util.h"
#include "system/angband.h"

class PlayerType;
void process_monster_lore(PlayerType *player_ptr, MonraceId r_idx, monster_lore_mode mode);
