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

    int get_level() const;
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
    void initialize_position();

    static WildernessGrids &get_instance();
    const WildernessGrid &get_grid(const Pos2D &pos) const;
    WildernessGrid &get_grid(const Pos2D &pos);
    const Pos2D &get_player_position() const;
    const WildernessGrid &get_player_grid() const;
    bool is_height_initialized() const;
    bool is_width_initialized() const;
    bool has_player_located() const;
    bool is_player_in_bounds() const;
    const Rect2D &get_area() const;

    void set_starting_player_position(const Pos2D &pos);
    void set_player_position(const Pos2D &pos);
    void move_player_to(const Direction &dir);
    bool should_reinitialize() const;
    void set_reinitialization(bool state);

private:
    WildernessGrids() = default;
    static WildernessGrids instance;

    Rect2D area = { 0, 0, 0, 0 };
    Pos2D current_pos = { 0, 0 };
    Pos2D starting_pos = { 0, 0 };
    bool reinitialization_flag = false;
};

extern std::vector<std::vector<WildernessGrid>> wilderness_grids;
