#pragma once

#include "system/angband.h"

struct player_attack_type;
struct player_type;
bool test_hit_norm(player_type *player_ptr, int chance, ARMOUR_CLASS ac, bool visible);
PERCENTAGE hit_chance(player_type *player_ptr, int chance, ARMOUR_CLASS ac);
bool check_hit_from_monster_to_player(player_type *player_ptr, int power, DEPTH level, int stun);
bool check_hit_from_monster_to_monster(int power, DEPTH level, ARMOUR_CLASS ac, int stun);
bool process_attack_hit(player_type *player_ptr, player_attack_type *pa_ptr, int chance);
