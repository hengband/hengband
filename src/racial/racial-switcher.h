#pragma once

#include "system/angband.h"

class PlayerType;
bool switch_class_racial_execution(PlayerType *player_ptr, const int32_t command);
bool switch_mimic_racial_execution(PlayerType *player_ptr);
bool switch_race_racial_execution(PlayerType *player_ptr, const int32_t command);
