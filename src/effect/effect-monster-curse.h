#pragma once

#include "system/angband.h"

typedef enum process_result process_resut;
typedef struct effect_monster_type effect_monster_type;
process_result effect_monster_curse_1(effect_monster_type *em_ptr);
process_result effect_monster_curse_2(effect_monster_type *em_ptr);
process_result effect_monster_curse_3(effect_monster_type *em_ptr);
process_result effect_monster_curse_4(effect_monster_type *em_ptr);
