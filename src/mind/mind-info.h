#pragma once

#include "system/angband.h"
#include <string>

class PlayerType;
enum class MindKindType;

/*! 特殊技能の失敗率と消費量 */
struct MindPowerChance {
    PERCENTAGE chance; //!< 失敗率
    MANA_POINT mana_cost; //!< 消費 MP (狂戦士と忍者は消費 HP)
};

MindPowerChance calc_mind_power_chance(PlayerType *player_ptr, MindKindType use_mind, int index);
std::string mindcraft_info(PlayerType *player_ptr, MindKindType use_mind, int power);
