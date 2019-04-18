/*!
 * @file shoot.c
 */
#pragma once

extern bool test_hit_fire(int chance, monster_type *m_ptr, int vis, char* o_name);
extern HIT_POINT critical_shot(WEIGHT weight, int plus_ammo, int plus_bow, HIT_POINT dam);
extern ENERGY bow_energy(OBJECT_SUBTYPE_VALUE sval);
extern int bow_tmul(OBJECT_SUBTYPE_VALUE sval);
