#pragma once

typedef struct player_attack_type player_attack_type;
typedef struct player_type player_type;
void process_monk_attack(player_type *attacker_ptr, player_attack_type *pa_ptr);
bool double_attack(player_type *creature_ptr);
