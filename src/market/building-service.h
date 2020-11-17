#pragma once

#include "system/angband.h"
#include "system/building-type-definition.h"

bool is_owner(player_type *player_ptr, building_type *bldg);
bool is_member(player_type *player_ptr, building_type *bldg);
void display_buikding_service(player_type *player_ptr, building_type *bldg);
