﻿#pragma once

#include "system/angband.h"

struct object_type;;
class player_type;
bool activate_sunlight(player_type *player_ptr);
bool activate_confusion(player_type *player_ptr);
bool activate_banish_evil(player_type *player_ptr);
bool activate_scare(player_type *player_ptr);
bool activate_aggravation(player_type *player_ptr, object_type *o_ptr, concptr name);
bool activate_stone_mud(player_type *player_ptr);
bool activate_judgement(player_type *player_ptr, concptr name);
bool activate_telekinesis(player_type *player_ptr, concptr name);
bool activate_unique_detection(player_type *player_ptr);
bool activate_dispel_curse(player_type *player_ptr, concptr name);
bool activate_cure_lw(player_type *player_ptr);
bool activate_grand_cross(player_type *player_ptr);
bool activate_call_chaos(player_type *player_ptr);
bool activate_dispel_evil(player_type *player_ptr);
bool activate_dispel_good(player_type *player_ptr);
bool activate_all_monsters_detection(player_type *player_ptr);
bool activate_all_detection(player_type *player_ptr);
bool activate_extra_detection(player_type *player_ptr);
bool activate_fully_identification(player_type *player_ptr);
bool activate_identification(player_type *player_ptr);
bool activate_pesticide(player_type *player_ptr);
bool activate_whirlwind(player_type *player_ptr);
bool activate_blinding_light(player_type *player_ptr, concptr name);
bool activate_sleep(player_type *player_ptr);
bool activate_door_destroy(player_type *player_ptr);
bool activate_earthquake(player_type *player_ptr);
bool activate_recharge(player_type *player_ptr);
bool activate_recharge_extra(player_type *player_ptr, concptr name);
bool activate_shikofumi(player_type *player_ptr);
bool activate_terror(player_type *player_ptr);
bool activate_map_light(player_type *player_ptr);
bool activate_exploding_rune(player_type *player_ptr);
bool activate_protection_rune(player_type *player_ptr);
bool activate_protection_elbereth(player_type *player_ptr);
bool activate_light(player_type *player_ptr, concptr name);
bool activate_recall(player_type *player_ptr);
bool activate_tree_creation(player_type *player_ptr, object_type *o_ptr, concptr name);
bool activate_animate_dead(player_type *player_ptr, object_type *o_ptr);
bool activate_detect_treasure(player_type *player_ptr);
