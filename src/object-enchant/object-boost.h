#pragma once

#include "system/angband.h"

class ItemEntity;
int m_bonus(int max, DEPTH level);
void one_sustain(ItemEntity *o_ptr);
bool add_esp_strong(ItemEntity *o_ptr);
void add_esp_weak(ItemEntity *o_ptr, bool extra);
void add_high_telepathy(ItemEntity *o_ptr);
void add_low_telepathy(ItemEntity *o_ptr);
void one_dragon_ele_resistance(ItemEntity *o_ptr);
void one_high_resistance(ItemEntity *o_ptr);
void one_ele_resistance(ItemEntity *o_ptr);
void dragon_resist(ItemEntity *o_ptr);
void one_resistance(ItemEntity *o_ptr);
void one_low_esp(ItemEntity *o_ptr);
void one_ability(ItemEntity *o_ptr);
void one_activation(ItemEntity *o_ptr);
void one_lordly_high_resistance(ItemEntity *o_ptr);
void make_weight_ligten(ItemEntity *o_ptr);
void make_weight_heavy(ItemEntity *o_ptr);
void add_xtra_ac(ItemEntity *o_ptr);
