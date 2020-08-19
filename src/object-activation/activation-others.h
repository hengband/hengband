#pragma once

#include "system/angband.h"

bool activate_sunlight(player_type *user_ptr);
bool activate_confusion(player_type *user_ptr);
bool activate_teleport_away(player_type *user_ptr);
bool activate_banish_evil(player_type *user_ptr);
bool activate_scare(player_type *user_ptr);
bool activate_aggravation(player_type *user_ptr, object_type *o_ptr, concptr name);
bool activate_stone_mud(player_type *user_ptr);
bool activate_judgement(player_type *user_ptr, concptr name);
bool activate_telekinesis(player_type *user_ptr, concptr name);
bool activate_unique_detection(player_type *user_ptr);
bool activate_escape(player_type *user_ptr);
bool activate_teleport_level(player_type *user_ptr);
bool activate_dimension_door(player_type *user_ptr);
bool activate_teleport(player_type *user_ptr);
bool activate_dispel_curse(player_type *user_ptr, concptr name);
bool activate_cure_lw(player_type *user_ptr);
bool activate_grand_cross(player_type *user_ptr);
