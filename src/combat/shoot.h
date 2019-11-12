/*!
 * @file shoot.c
 */
#pragma once

extern bool test_hit_fire(player_type *shooter_ptr, int chance, monster_type *m_ptr, int vis, char* o_name);
extern HIT_POINT critical_shot(player_type *shooter_ptr, WEIGHT weight, int plus_ammo, int plus_bow, HIT_POINT dam);
extern ENERGY bow_energy(OBJECT_SUBTYPE_VALUE sval);
extern int bow_tmul(OBJECT_SUBTYPE_VALUE sval);
extern HIT_POINT calc_crit_ratio_shot(player_type *shooter_ptr, HIT_POINT plus_ammo, HIT_POINT plus_bow);
extern HIT_POINT calc_expect_crit_shot(player_type *shooter_ptr, WEIGHT weight, int plus_ammo, int plus_bow, HIT_POINT dam);
extern HIT_POINT calc_expect_crit(WEIGHT weight, int plus, HIT_POINT dam, s16b meichuu, bool dokubari);
