#pragma once

#include "system/angband.h"

class EffectMonster;
class PlayerType;
ProcessResult effect_monster_lite_weak(PlayerType *player_ptr, EffectMonster *em_ptr);
ProcessResult effect_monster_lite(PlayerType *player_ptr, EffectMonster *em_ptr);
ProcessResult effect_monster_dark(PlayerType *player_ptr, EffectMonster *em_ptr);
