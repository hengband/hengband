#pragma once

#include "autopick/autopick-methods-table.h"
#include "util/flag-group.h"
#include "util/point-2d.h"
#include <cstdint>
#include <tl/optional.hpp>

extern EnumClassFlagGroup<AutopickMethod> display_autopick;

class DisplaySymbolPair;
class FloorType;
class Grid;
class PlayerType;
class TerrainType;

/*!
 * @brief 地図のマスの地形を表示するかの判定に使う、マスによらない値の組
 * @details 地図全体を描くときなどは、走査の前に 1 回だけ作ってマスごとの判定に渡す。
 * 作ってから使い終わるまでの間に、プレイヤーの状態や荒野か・時刻が変わらないことを前提にする。
 */
class MapVisibilityContext {
public:
    explicit MapVisibilityContext(const PlayerType &player);
    MapVisibilityContext(const MapVisibilityContext &) = delete;
    MapVisibilityContext(MapVisibilityContext &&) = delete;
    MapVisibilityContext &operator=(const MapVisibilityContext &) = delete;
    MapVisibilityContext &operator=(MapVisibilityContext &&) = delete;

    const PlayerType &player; //!< プレイヤー
    const FloorType &floor; //!< プレイヤーのいる階
    const bool is_blind; //!< 盲目か
    const bool has_nocto; //!< 暗視があるか
    const bool is_wild_mode; //!< 荒野 (広域マップ) にいるか
    const bool is_daytime; //!< 昼間か
};

bool is_map_terrain_visible(const MapVisibilityContext &context, const Grid &grid, const TerrainType &terrain, const Pos2D &pos);
int decide_visible_map_terrain_lighting(const MapVisibilityContext &context, const Grid &grid, const TerrainType &terrain, const Pos2D &pos, tl::optional<uint8_t> monochrome = tl::nullopt);
bool is_map_terrain_visible(const PlayerType &player, const Pos2D &pos);
int decide_map_terrain_lighting(const MapVisibilityContext &context, const Pos2D &pos, tl::optional<uint8_t> monochrome = tl::nullopt);
int decide_map_terrain_lighting(const PlayerType &player, const Pos2D &pos, tl::optional<uint8_t> monochrome = tl::nullopt);
DisplaySymbolPair map_info(PlayerType *player_ptr, const MapVisibilityContext &context, const Pos2D &pos);
DisplaySymbolPair map_info(PlayerType *player_ptr, const Pos2D &pos);
tl::optional<uint8_t> get_monochrome_display_color(PlayerType *player_ptr);
