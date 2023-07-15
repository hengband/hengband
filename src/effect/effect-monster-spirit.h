#pragma once

#include "system/angband.h"

class EffectMonster;
class PlayerType;
ProcessResult effect_monster_drain_mana(PlayerType *player_ptr, EffectMonster *em_ptr);
ProcessResult effect_monster_mind_blast(PlayerType *player_ptr, EffectMonster *em_ptr);
ProcessResult effect_monster_brain_smash(PlayerType *player_ptr, EffectMonster *em_ptr);
