#pragma once

#include "system/angband.h"

typedef struct monap_type monap_type;
typedef struct player_type player_type;
void process_eat_gold(player_type *target_ptr, monap_type *monap_ptr);
bool check_eat_item(player_type *target_ptr, monap_type *monap_ptr);
void process_eat_item(player_type *target_ptr, monap_type *monap_ptr);
void process_eat_food(player_type *target_ptr, monap_type *monap_ptr);
void process_eat_lite(player_type *target_ptr, monap_type *monap_ptr);

bool process_un_power(player_type *target_ptr, monap_type *monap_ptr);
bool check_drain_hp(player_type *target_ptr, const int32_t d);
void process_drain_life(player_type *target_ptr, monap_type *monap_ptr, const bool resist_drain);
void process_drain_mana(player_type *target_ptr, monap_type *monap_ptr);
void process_monster_attack_hungry(player_type *target_ptr, monap_type *monap_ptr);
