#pragma once

#include "system/angband.h"
#include <optional>

class EffectMonster;
class CapturedMonsterType;
class PlayerType;
ProcessResult switch_effects_monster(PlayerType *player_ptr, EffectMonster *em_ptr, std::optional<CapturedMonsterType *> cap_mon_ptr = std::nullopt);
