#pragma once

#include "system/angband.h"
#include "lore/lore-util.h"

void set_breath_types(player_type *player_ptr, lore_type *lore_ptr);
void set_ball_types(player_type *player_ptr, lore_type *lore_ptr);
void set_particular_types(player_type *player_ptr, lore_type *lore_ptr);
void set_bolt_types(player_type *player_ptr, lore_type *lore_ptr);
void set_status_types(lore_type *lore_ptr);
void set_teleport_types(lore_type *lore_ptr);
void set_floor_types(player_type *player_ptr, lore_type *lore_ptr);
void set_summon_types(lore_type *lore_ptr);
