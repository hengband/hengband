#pragma once

#include "system/angband.h"

class EffectMonster;
class PlayerType;
ProcessResult effect_monster_old_poly(EffectMonster *em_ptr);
ProcessResult effect_monster_old_clone(PlayerType *player_ptr, EffectMonster *em_ptr);
ProcessResult effect_monster_star_heal(PlayerType *player_ptr, EffectMonster *em_ptr);
ProcessResult effect_monster_old_heal(PlayerType *player_ptr, EffectMonster *em_ptr);
ProcessResult effect_monster_old_speed(PlayerType *player_ptr, EffectMonster *em_ptr);
ProcessResult effect_monster_old_slow(PlayerType *player_ptr, EffectMonster *em_ptr);
ProcessResult effect_monster_old_sleep(PlayerType *player_ptr, EffectMonster *em_ptr);
ProcessResult effect_monster_old_conf(PlayerType *player_ptr, EffectMonster *em_ptr);
ProcessResult effect_monster_stasis(EffectMonster *em_ptr, bool to_evil);
ProcessResult effect_monster_stun(EffectMonster *em_ptr);
