#pragma once

bool test_hit_norm(player_type *attacker_ptr, HIT_RELIABILITY chance, ARMOUR_CLASS ac, bool visible);
PERCENTAGE hit_chance(player_type *attacker_ptr, HIT_RELIABILITY chance, ARMOUR_CLASS ac);
