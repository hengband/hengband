#pragma once

bool test_hit_norm(player_type *attacker_ptr, HIT_RELIABILITY chance, ARMOUR_CLASS ac, bool visible);
PERCENTAGE hit_chance(player_type *attacker_ptr, HIT_RELIABILITY chance, ARMOUR_CLASS ac);
int check_hit_from_monster_to_player(player_type *target_ptr, int power, DEPTH level, int stun);
int check_hit_from_monster_to_monster(int power, DEPTH level, ARMOUR_CLASS ac, int stun);
