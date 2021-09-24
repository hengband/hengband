#pragma once

#include <array>

enum class BlueMagicType {
    BOLT = 1,
    BALL = 2,
    BREATH = 3,
    SUMMON = 4,
    OTHER = 5,
};

struct player_type;
bool do_cmd_cast_learned(player_type *player_ptr);

inline constexpr std::array BLUE_MAGIC_TYPE_LIST = {
    BlueMagicType::BOLT,
    BlueMagicType::BALL,
    BlueMagicType::BREATH,
    BlueMagicType::SUMMON,
    BlueMagicType::OTHER,
};
