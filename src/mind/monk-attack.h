#pragma once

struct player_attack_type;
struct player_type;
void process_monk_attack(player_type *player_ptr, player_attack_type *pa_ptr);
bool double_attack(player_type *player_ptr);
