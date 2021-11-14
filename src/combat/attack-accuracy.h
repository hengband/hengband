#pragma once

#include "system/angband.h"

struct player_attack_type;
class PlayerType;
bool test_hit_norm(PlayerType *player_ptr, int chance, ARMOUR_CLASS ac, bool visible);
PERCENTAGE hit_chance(PlayerType *player_ptr, int chance, ARMOUR_CLASS ac);
bool check_hit_from_monster_to_player(PlayerType *player_ptr, int power, DEPTH level, int stun);
bool check_hit_from_monster_to_monster(int power, DEPTH level, ARMOUR_CLASS ac, int stun);
bool process_attack_hit(PlayerType *player_ptr, player_attack_type *pa_ptr, int chance);
