#pragma once

#include "system/angband.h"

class EffectMonster;
class PlayerType;
ProcessResult effect_monster_psi(PlayerType *player_ptr, EffectMonster *em_ptr);
ProcessResult effect_monster_psi_drain(PlayerType *player_ptr, EffectMonster *em_ptr);
ProcessResult effect_monster_telekinesis(PlayerType *player_ptr, EffectMonster *em_ptr);
