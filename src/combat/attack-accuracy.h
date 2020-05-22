#pragma once

bool test_hit_norm(player_type *attacker_ptr, HIT_RELIABILITY chance, ARMOUR_CLASS ac, bool visible);
PERCENTAGE hit_chance(player_type *attacker_ptr, HIT_RELIABILITY chance, ARMOUR_CLASS ac);
int check_hit(player_type *target_ptr, int power, DEPTH level, int stun);
int check_hit2(int power, DEPTH level, ARMOUR_CLASS ac, int stun);
