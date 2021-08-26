#pragma once

#include "system/angband.h"

typedef struct object_type object_type;
typedef struct player_type player_type;
bool hates_acid(object_type *o_ptr);
bool hates_elec(object_type *o_ptr);
bool hates_fire(object_type *o_ptr);
bool hates_cold(object_type *o_ptr);
int set_acid_destroy(object_type *o_ptr);
int set_elec_destroy(object_type *o_ptr);
int set_fire_destroy(object_type *o_ptr);
int set_cold_destroy(object_type *o_ptr);

bool potion_smash_effect(player_type *owner_ptr, MONSTER_IDX who, POSITION y, POSITION x, KIND_OBJECT_IDX k_idx);
PERCENTAGE breakage_chance(player_type *owner_ptr, object_type *o_ptr, bool has_archer_bonus, SPELL_IDX snipe_type);
