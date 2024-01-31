#pragma once
#include <system/h-type.h>

struct player_attack_type;
class PlayerType;
void process_monk_attack(PlayerType *player_ptr, player_attack_type *pa_ptr);
bool double_attack(PlayerType *player_ptr);
WEIGHT calc_monk_attack_weight(PlayerType *player_ptr);
