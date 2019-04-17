/*!
 * @file shoot.c
 */


#pragma once

extern HIT_POINT critical_shot(WEIGHT weight, int plus_ammo, int plus_bow, HIT_POINT dam);
extern ENERGY bow_energy(OBJECT_SUBTYPE_VALUE sval);
extern int bow_tmul(OBJECT_SUBTYPE_VALUE sval);
