﻿#pragma once

#include "system/angband.h"

bool item_tester_hook_convertible(const player_type *player_ptr, object_type *o_ptr);
bool item_tester_hook_ammo(const player_type *player_ptr, object_type *o_ptr);
bool object_is_ammo(object_type *o_ptr);
