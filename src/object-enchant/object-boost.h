#pragma once

#include "system/angband.h"

int m_bonus(int max, DEPTH level);
void one_sustain(object_type *o_ptr);
bool add_esp_strong(object_type *o_ptr);
void add_esp_weak(object_type *o_ptr, bool extra);
void one_dragon_ele_resistance(object_type *o_ptr);
void one_high_resistance(object_type *o_ptr);
void one_ele_resistance(object_type *o_ptr);
void dragon_resist(object_type *o_ptr);
void one_resistance(object_type *o_ptr);
void one_low_esp(object_type *o_ptr);
void one_ability(object_type *o_ptr);
void one_activation(object_type *o_ptr);
void one_lordly_high_resistance(object_type *o_ptr);
void apply_magic_weapon(player_type *owner_ptr, object_type *o_ptr, DEPTH level, int power);
