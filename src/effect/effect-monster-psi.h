#pragma once

#include "system/angband.h"

struct effect_monster_type;
class PlayerType;
ProcessResult effect_monster_psi(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_psi_drain(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_telekinesis(PlayerType *player_ptr, effect_monster_type *em_ptr);
