﻿#pragma once

#include "system/angband.h"

class player_type;
bool switch_class_racial_execution(player_type *player_ptr, const int32_t command);
bool switch_mimic_racial_execution(player_type *player_ptr);
bool switch_race_racial_execution(player_type *player_ptr, const int32_t command);
