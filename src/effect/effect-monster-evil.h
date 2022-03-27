#pragma once

#include "system/angband.h"

struct effect_monster_type;
class PlayerType;
ProcessResult effect_monster_away_undead(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_away_evil(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_away_all(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_turn_undead(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_turn_evil(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_turn_all(effect_monster_type *em_ptr);
ProcessResult effect_monster_disp_undead(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_disp_evil(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_disp_good(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_disp_living(effect_monster_type *em_ptr);
ProcessResult effect_monster_disp_demon(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_disp_all(effect_monster_type *em_ptr);
