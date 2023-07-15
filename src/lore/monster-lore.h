#pragma once

#include "lore/lore-util.h"
#include "system/angband.h"

class PlayerType;
void process_monster_lore(PlayerType *player_ptr, MonsterRaceId r_idx, monster_lore_mode mode);
