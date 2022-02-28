#pragma once

#include "system/angband.h"

class ObjectType;
class PlayerType;
bool activate_sunlight(PlayerType *player_ptr);
bool activate_confusion(PlayerType *player_ptr);
bool activate_banish_evil(PlayerType *player_ptr);
bool activate_scare(PlayerType *player_ptr);
bool activate_aggravation(PlayerType *player_ptr, ObjectType *o_ptr, concptr name);
bool activate_stone_mud(PlayerType *player_ptr);
bool activate_judgement(PlayerType *player_ptr, concptr name);
bool activate_telekinesis(PlayerType *player_ptr, concptr name);
bool activate_unique_detection(PlayerType *player_ptr);
bool activate_dispel_curse(PlayerType *player_ptr, concptr name);
bool activate_cure_lw(PlayerType *player_ptr);
bool activate_grand_cross(PlayerType *player_ptr);
bool activate_call_chaos(PlayerType *player_ptr);
bool activate_dispel_evil(PlayerType *player_ptr);
bool activate_dispel_good(PlayerType *player_ptr);
bool activate_all_monsters_detection(PlayerType *player_ptr);
bool activate_all_detection(PlayerType *player_ptr);
bool activate_extra_detection(PlayerType *player_ptr);
bool activate_fully_identification(PlayerType *player_ptr);
bool activate_identification(PlayerType *player_ptr);
bool activate_pesticide(PlayerType *player_ptr);
bool activate_whirlwind(PlayerType *player_ptr);
bool activate_blinding_light(PlayerType *player_ptr, concptr name);
bool activate_sleep(PlayerType *player_ptr);
bool activate_door_destroy(PlayerType *player_ptr);
bool activate_earthquake(PlayerType *player_ptr);
bool activate_recharge(PlayerType *player_ptr);
bool activate_recharge_extra(PlayerType *player_ptr, concptr name);
bool activate_shikofumi(PlayerType *player_ptr);
bool activate_terror(PlayerType *player_ptr);
bool activate_map_light(PlayerType *player_ptr);
bool activate_exploding_rune(PlayerType *player_ptr);
bool activate_protection_rune(PlayerType *player_ptr);
bool activate_protection_elbereth(PlayerType *player_ptr);
bool activate_light(PlayerType *player_ptr, concptr name);
bool activate_recall(PlayerType *player_ptr);
bool activate_tree_creation(PlayerType *player_ptr, ObjectType *o_ptr, concptr name);
bool activate_animate_dead(PlayerType *player_ptr, ObjectType *o_ptr);
bool activate_detect_treasure(PlayerType *player_ptr);
bool activate_create_ammo(PlayerType *player_ptr);
bool activate_dispel_magic(PlayerType *player_ptr);
