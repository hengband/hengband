#pragma once

#include "system/angband.h"
#include <optional>

enum class ProcessResult;
struct effect_monster_type;
class CapturedMonsterType;
class PlayerType;
ProcessResult effect_monster_charm(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_control_undead(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_control_demon(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_control_animal(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_charm_living(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_domination(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_crusade(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_capture(PlayerType *player_ptr, effect_monster_type *em_ptr, std::optional<CapturedMonsterType *> cap_mon_ptr);
