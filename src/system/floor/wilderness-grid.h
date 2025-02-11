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

enum class DungeonId;
enum class MonraceHook;
enum class WildernessTerrain;
class Direction;
class WildernessGrid {
public:
    WildernessGrid() = default;
    DungeonId entrance{};
    std::string name = "";

    MonraceHook get_monrace_hook() const;
    WildernessTerrain get_terrain() const;
    void set_terrain(WildernessTerrain wt);
    bool has_town() const;
    bool matches_town(short town_matching) const;
    short get_town() const;
    void set_town(short town_parsing);
    bool has_road() const;
    void set_road(int road_parsing);
    int get_level() const;
    void set_level(int level_parsing);
    uint32_t get_seed() const;
    void set_seed(uint32_t saved_seed);
    void initialize(const WildernessGrid &letter); //!< @details コピーではなく一部引き写し.
    void initialize_seed();

private:
    WildernessTerrain terrain{};
    int level = 0;
    uint32_t seed = 0;
    short town = 0;
    int road = 0;
};

class WildernessGrids {
public:
    ~WildernessGrids() = default;
    WildernessGrids(const WildernessGrids &) = delete;
    WildernessGrids(WildernessGrids &&) = delete;
    WildernessGrids &operator=(const WildernessGrids &) = delete;
    WildernessGrids &operator=(WildernessGrids &&) = delete;
    void initialize_height(int height);
    void initialize_width(int width);
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
    MonraceHook get_monrace_hook() const;

    void set_starting_player_position(const Pos2D &pos);
    void set_player_position(const Pos2D &pos);
    void move_player_to(const Direction &dir);
    bool should_reinitialize() const;
    void set_reinitialization(bool state);
    bool should_ambush() const;
    void set_ambushes(bool state);

private:
    WildernessGrids() = default;
    static WildernessGrids instance;

    Rect2D area = { 0, 0, 0, 0 };
    Pos2D current_pos = { 0, 0 };
    Pos2D starting_pos = { 0, 0 };
    bool reinitialization_flag = false;
    bool ambushes_flag = false;
};

extern std::vector<std::vector<WildernessGrid>> wilderness_grids;

class WildernessLetters {
public:
    ~WildernessLetters() = default;
    WildernessLetters(const WildernessLetters &) = delete;
    WildernessLetters(WildernessLetters &&) = delete;
    WildernessLetters &operator=(const WildernessLetters &) = delete;
    WildernessLetters &operator=(WildernessLetters &&) = delete;
    static WildernessLetters &get_instance();
    void initialize();

    const WildernessGrid &get_grid(int index) const;
    WildernessGrid &get_grid(int index);

private:
    WildernessLetters() = default;
    static WildernessLetters instance;

    std::vector<WildernessGrid> letters;
};
