#pragma once

#include "system/angband.h"

typedef struct monap_type monap_type;
struct player_type;
void process_blind_attack(player_type *player_ptr, monap_type *monap_ptr);
void process_terrify_attack(player_type *player_ptr, monap_type *monap_ptr);
void process_paralyze_attack(player_type *player_ptr, monap_type *monap_ptr);
void process_lose_all_attack(player_type *player_ptr, monap_type *monap_ptr);
void process_stun_attack(player_type *player_ptr, monap_type *monap_ptr);
void process_monster_attack_time(player_type *player_ptr, monap_type *monap_ptr);
