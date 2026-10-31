#pragma once

#include <nlohmann/json_fwd.hpp>
#include <string>
#include <string_view>

struct dungeon_grid;

bool has_legend_object_trap_conflict(const dungeon_grid &grid, bool object_is_quest_reward, bool fixed_trap, bool random_trap);
std::string_view describe_legend_object_trap_conflict();
std::string describe_town_fixed_trap_tag(const nlohmann::json &cell_data);
