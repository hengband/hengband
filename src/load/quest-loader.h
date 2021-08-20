#pragma once

#include "system/angband.h"

typedef struct player_type player_type;
errr load_town(void);
errr load_quest_info(uint16_t *max_quests_load, byte *max_rquests_load);
void analyze_quests(player_type *creature_ptr, const uint16_t max_quests_load, const byte max_rquests_load);
void rd_unique_info(void);
