﻿#pragma once

#include "system/angband.h"

typedef struct object_type object_type;
bool store_will_buy(const player_type *player_ptr, object_type *o_ptr);
void mass_produce(const player_type *player_ptr, object_type *o_ptr);
