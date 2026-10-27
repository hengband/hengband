#pragma once

#include <string_view>
#include <tl/expected.hpp>

struct dungeon_grid;

enum class LegendEgoError {
    UNDEFINED_ID,
    WORTHLESS,
    NO_PLACED_OBJECT,
    RANDOM_OBJECT,
    QUEST_REWARD,
    GOLD,
    WITH_TRAP,
    INCOMPATIBLE,
};

tl::expected<void, LegendEgoError> validate_legend_ego(const dungeon_grid &grid, bool object_is_quest_reward, bool has_trap);
std::string_view describe_legend_ego_error(LegendEgoError error);
