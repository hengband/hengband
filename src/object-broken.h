#pragma once

extern bool hates_acid(object_type *o_ptr);
extern bool hates_elec(object_type *o_ptr);
extern bool hates_fire(object_type *o_ptr);
extern bool hates_cold(object_type *o_ptr);
extern int set_acid_destroy(object_type *o_ptr);
extern int set_elec_destroy(object_type *o_ptr);
extern int set_fire_destroy(object_type *o_ptr);
extern int set_cold_destroy(object_type *o_ptr);

extern bool potion_smash_effect(MONSTER_IDX who, POSITION y, POSITION x, KIND_OBJECT_IDX k_idx);
PERCENTAGE breakage_chance(object_type *o_ptr, bool has_archer_bonus, SPELL_IDX snipe_type);
