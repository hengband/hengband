#pragma once

#include "system/angband.h"

class PlayerType;
errr load_town(void);
errr load_quest_info(uint16_t *max_quests_load, byte *max_rquests_load);
void analyze_quests(PlayerType *player_ptr, const uint16_t max_quests_load, const byte max_rquests_load);
