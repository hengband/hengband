#pragma once

#include "system/angband.h"

errr load_town(void);
errr load_quest_info(u16b *max_quests_load, byte *max_rquests_load);
void analyze_quests(player_type *creature_ptr, const u16b max_quests_load, const byte max_rquests_load);
void rd_unique_info(void);
