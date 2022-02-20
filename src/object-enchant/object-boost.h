#pragma once

#include "system/angband.h"

class ObjectType;
int m_bonus(int max, DEPTH level);
void one_sustain(ObjectType *o_ptr);
bool add_esp_strong(ObjectType *o_ptr);
void add_esp_weak(ObjectType *o_ptr, bool extra);
void add_high_telepathy(ObjectType *o_ptr);
void add_low_telepathy(ObjectType *o_ptr);
void one_dragon_ele_resistance(ObjectType *o_ptr);
void one_high_resistance(ObjectType *o_ptr);
void one_ele_resistance(ObjectType *o_ptr);
void dragon_resist(ObjectType *o_ptr);
void one_resistance(ObjectType *o_ptr);
void one_low_esp(ObjectType *o_ptr);
void one_ability(ObjectType *o_ptr);
void one_activation(ObjectType *o_ptr);
void one_lordly_high_resistance(ObjectType *o_ptr);
void make_weight_ligten(ObjectType *o_ptr);
void make_weight_heavy(ObjectType *o_ptr);
void add_xtra_ac(ObjectType *o_ptr);
