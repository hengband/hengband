#pragma once

#include "system/angband.h"

struct monster_type;
struct object_type;;
struct player_type;
bool test_hit_fire(player_type *shooter_ptr, int chance, monster_type *m_ptr, int vis, char *o_name);
HIT_POINT critical_shot(player_type *shooter_ptr, WEIGHT weight, int plus_ammo, int plus_bow, HIT_POINT dam);
ENERGY bow_energy(OBJECT_SUBTYPE_VALUE sval);
int bow_tmul(OBJECT_SUBTYPE_VALUE sval);
HIT_POINT calc_crit_ratio_shot(player_type *shooter_ptr, HIT_POINT plus_ammo, HIT_POINT plus_bow);
HIT_POINT calc_expect_crit_shot(player_type *shooter_ptr, WEIGHT weight, int plus_ammo, int plus_bow, HIT_POINT dam);
HIT_POINT calc_expect_crit(player_type *shooter_ptr, WEIGHT weight, int plus, HIT_POINT dam, int16_t meichuu, bool dokubari, bool impact);
void exe_fire(player_type *shooter_ptr, INVENTORY_IDX item, object_type *j_ptr, SPELL_IDX snipe_type);
