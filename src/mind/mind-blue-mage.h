#pragma once

#include <array>

enum class BlueMagicType {
    BOLT = 1,
    BALL = 2,
    BREATH = 3,
    SUMMON = 4,
    OTHER = 5,
};

class PlayerType;
bool do_cmd_cast_learned(PlayerType *player_ptr);

inline constexpr std::array<BlueMagicType, 5> BLUE_MAGIC_TYPE_LIST = { {
    BlueMagicType::BOLT,
    BlueMagicType::BALL,
    BlueMagicType::BREATH,
    BlueMagicType::SUMMON,
    BlueMagicType::OTHER,
} };
