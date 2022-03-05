#pragma once

#include "system/angband.h"

struct effect_monster_type;
class PlayerType;
ProcessResult effect_monster_lite_weak(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_lite(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_dark(PlayerType *player_ptr, effect_monster_type *em_ptr);
