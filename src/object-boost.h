#pragma once
extern int m_bonus(int max, DEPTH level);
extern void one_sustain(object_type *o_ptr);
extern bool add_esp_strong(object_type *o_ptr);
extern void add_esp_weak(object_type *o_ptr, bool extra);
extern void one_dragon_ele_resistance(object_type *o_ptr);
extern void one_high_resistance(object_type *o_ptr);
extern void one_ele_resistance(object_type *o_ptr);
extern void dragon_resist(object_type * o_ptr);


extern void apply_magic_weapon(object_type *o_ptr, DEPTH level, int power);

