/*!
 * @brief 広域マップ定義
 * @author Hourier
 * @date 2025/02/06
 */

#pragma once

#include "util/point-2d.h"
#include <cstdint>
#include <string>
#include <vector>

enum class WildernessTerrain;
enum class DungeonId;
class Direction;
class WildernessGrid {
public:
    WildernessGrid() = default;
    WildernessTerrain terrain{};
    short town = 0;
    int road = 0;
    uint32_t seed = 0;
    int level = 0;
    DungeonId entrance{};
    std::string name = "";

    void initialize_seed();
};

class WildernessGrids {
public:
    ~WildernessGrids() = default;
    WildernessGrids(const WildernessGrids &) = delete;
    WildernessGrids(WildernessGrids &&) = delete;
    WildernessGrids &operator=(const WildernessGrids &) = delete;
    WildernessGrids &operator=(WildernessGrids &&) = delete;
    void init_height(int height);
    void init_width(int width);
    void initialize_grids(); //!< @details 全ての定義ファイルを読み込んでから初期化する.
    void initialize_seeds();

    static WildernessGrids &get_instance();
    const WildernessGrid &get_grid(const Pos2D &pos) const;
    WildernessGrid &get_grid(const Pos2D &pos);
    const Pos2D &get_player_position() const;
    const WildernessGrid &get_player_grid() const;
    bool is_height_initialized() const;
    bool is_width_initialized() const;
    bool has_player_located() const;
    bool is_player_in_bounds() const;
    const Pos2D &get_bottom_right() const;
    const std::vector<Pos2D> &get_positions() const;

    void set_player_position(const Pos2D &pos);
    void move_player_to(const Direction &dir);

private:
    WildernessGrids() = default;
    static WildernessGrids instance;
    std::vector<Pos2D> positions;
    Pos2D bottom_right = { 0, 0 };
    Pos2D current_pos = { 0, 0 };
};

extern std::vector<std::vector<WildernessGrid>> wilderness_grids;
