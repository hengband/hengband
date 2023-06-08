#pragma once

#include "system/angband.h"

class EffectMonster;
class PlayerType;
ProcessResult effect_monster_away_undead(PlayerType *player_ptr, EffectMonster *em_ptr);
ProcessResult effect_monster_away_evil(PlayerType *player_ptr, EffectMonster *em_ptr);
ProcessResult effect_monster_away_all(PlayerType *player_ptr, EffectMonster *em_ptr);
ProcessResult effect_monster_turn_undead(PlayerType *player_ptr, EffectMonster *em_ptr);
ProcessResult effect_monster_turn_evil(PlayerType *player_ptr, EffectMonster *em_ptr);
ProcessResult effect_monster_turn_all(EffectMonster *em_ptr);
ProcessResult effect_monster_disp_undead(PlayerType *player_ptr, EffectMonster *em_ptr);
ProcessResult effect_monster_disp_evil(PlayerType *player_ptr, EffectMonster *em_ptr);
ProcessResult effect_monster_disp_good(PlayerType *player_ptr, EffectMonster *em_ptr);
ProcessResult effect_monster_disp_living(EffectMonster *em_ptr);
ProcessResult effect_monster_disp_demon(PlayerType *player_ptr, EffectMonster *em_ptr);
ProcessResult effect_monster_disp_all(EffectMonster *em_ptr);
