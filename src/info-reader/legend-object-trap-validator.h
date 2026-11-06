#pragma once

#include <nlohmann/json_fwd.hpp>
#include <string>
#include <string_view>

struct dungeon_grid;

//! 凡例のセルの trap のタグをいつ地形へ解決するか
enum class LegendTrapTagResolution {
    DEFERRED, //!< 読み込みの後で解決する。未知のタグも固定の罠になる
    IMMEDIATE, //!< 読み込みの時に解決する。未知のタグは読み込みで拒否される
};

bool has_legend_object_trap_conflict(const dungeon_grid &grid, bool object_is_quest_reward, bool fixed_trap, bool random_trap);
std::string_view describe_legend_object_trap_conflict();
std::string describe_town_fixed_trap_tag(const nlohmann::json &cell_data, LegendTrapTagResolution resolution);
