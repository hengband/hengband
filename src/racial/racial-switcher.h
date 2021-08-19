﻿#pragma once

#include "system/angband.h"

typedef struct player_type player_type;
bool switch_class_racial_execution(player_type *creature_ptr, const int command);
bool switch_mimic_racial_execution(player_type *creature_ptr);
bool switch_race_racial_execution(player_type *creature_ptr, const int command);
