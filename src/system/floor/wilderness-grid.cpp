/*!
 * @brief 広域マップ実装
 * @author Hourier
 * @date 2025/02/06
 */

#include "system/floor/wilderness-grid.h"
#include "floor/geometry.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/enums/monrace/monrace-hook-types.h"
#include "system/enums/terrain/wilderness-terrain.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "term/z-rand.h"

std::vector<std::vector<WildernessGrid>> wilderness_grids;

int WildernessGrid::get_level() const
{
    return this->level;
}

void WildernessGrid::set_level(int level_parsing)
{
    this->level = level_parsing;
}

uint32_t WildernessGrid::get_seed() const
{
    return this->seed;
}

void WildernessGrid::set_seed(uint32_t saved_seed)
{
    this->seed = saved_seed;
}

DungeonId WildernessGrid::get_entrance() const
{
    return this->entrance;
}

void WildernessGrid::set_entrance(DungeonId entrance_parsing)
{
    this->entrance = entrance_parsing;
}

MonraceHook WildernessGrid::get_monrace_hook() const
{
    switch (this->terrain) {
    case WildernessTerrain::TOWN:
        return MonraceHook::TOWN;
    case WildernessTerrain::DEEP_WATER:
        return MonraceHook::OCEAN;
    case WildernessTerrain::SHALLOW_WATER:
    case WildernessTerrain::SWAMP:
        return MonraceHook::SHORE;
    case WildernessTerrain::DIRT:
    case WildernessTerrain::DESERT:
        return MonraceHook::WASTE;
    case WildernessTerrain::GRASS:
        return MonraceHook::GRASS;
    case WildernessTerrain::TREES:
        return MonraceHook::WOOD;
    case WildernessTerrain::SHALLOW_LAVA:
    case WildernessTerrain::DEEP_LAVA:
        return MonraceHook::VOLCANO;
    case WildernessTerrain::MOUNTAIN:
        return MonraceHook::MOUNTAIN;
    default:
        return MonraceHook::DUNGEON;
    }
}

WildernessTerrain WildernessGrid::get_terrain() const
{
    return this->terrain;
}

void WildernessGrid::set_terrain(WildernessTerrain wt)
{
    this->terrain = wt;
}

bool WildernessGrid::has_town() const
{
    return this->town > 0;
}

bool WildernessGrid::matches_town(short town_matching) const
{
    return this->town == town_matching;
}

short WildernessGrid::get_town() const
{
    return this->town;
}

void WildernessGrid::set_town(short town_parsing)
{
    this->town = town_parsing;
}

bool WildernessGrid::has_road() const
{
    return this->road > 0;
}

void WildernessGrid::set_road(int road_parsing)
{
    this->road = road_parsing;
}

void WildernessGrid::initialize(const WildernessGrid &letter)
{
    this->terrain = letter.terrain;
    this->level = letter.level;
    this->town = letter.town;
    this->road = letter.road;
}

void WildernessGrid::initialize_seed()
{
    this->seed = randint0(0x10000000);
}

WildernessGrids WildernessGrids::instance{};

void WildernessGrids::initialize_height(int height)
{
    this->area.bottom_right.y = height - 1;
}

void WildernessGrids::initialize_width(int width)
{
    this->area.bottom_right.x = width - 1;
}

void WildernessGrids::initialize_grids()
{
    wilderness_grids.assign(this->area.bottom_right.y + 1, std::vector<WildernessGrid>(this->area.bottom_right.x + 1));
}

void WildernessGrids::initialize_seeds()
{
    for (const auto &pos : this->area) {
        wilderness_grids[pos.y][pos.x].initialize_seed();
    }
}

void WildernessGrids::initialize_position()
{
    this->current_pos = this->starting_pos;
}

WildernessGrids &WildernessGrids::get_instance()
{
    return instance;
}

const WildernessGrid &WildernessGrids::get_grid(const Pos2D &pos) const
{
    return wilderness_grids.at(pos.y).at(pos.x);
}

WildernessGrid &WildernessGrids::get_grid(const Pos2D &pos)
{
    return wilderness_grids.at(pos.y).at(pos.x);
}

const Pos2D &WildernessGrids::get_player_position() const
{
    return this->current_pos;
}

void WildernessGrids::set_starting_player_position(const Pos2D &pos)
{
    this->starting_pos = pos;
}

void WildernessGrids::set_player_position(const Pos2D &pos)
{
    this->current_pos = pos;
}

const WildernessGrid &WildernessGrids::get_player_grid() const
{
    return wilderness_grids.at(this->current_pos.y).at(this->current_pos.x);
}

void WildernessGrids::move_player_to(const Direction &dir)
{
    this->current_pos += dir.vec();
}

bool WildernessGrids::should_reinitialize() const
{
    return this->reinitialization_flag;
}

void WildernessGrids::set_reinitialization(bool state)
{
    this->reinitialization_flag = state;
}

bool WildernessGrids::should_ambush() const
{
    return this->ambushes_flag;
}

void WildernessGrids::set_ambushes(bool state)
{
    this->ambushes_flag = state;
}

bool WildernessGrids::is_height_initialized() const
{
    return this->area.bottom_right.y > 0;
}

bool WildernessGrids::is_width_initialized() const
{
    return this->area.bottom_right.x > 0;
}

bool WildernessGrids::has_player_located() const
{
    return (this->current_pos.x > 0) && (this->current_pos.y > 0);
}

bool WildernessGrids::is_player_in_bounds() const
{
    auto is_in_bounds_x = this->current_pos.x >= 1;
    is_in_bounds_x &= this->current_pos.x <= this->area.bottom_right.x;
    auto is_in_bounds_y = this->current_pos.y >= 1;
    is_in_bounds_y &= this->current_pos.y <= this->area.bottom_right.y;
    return is_in_bounds_x && is_in_bounds_y;
}

const Rect2D &WildernessGrids::get_area() const
{
    return this->area;
}

MonraceHook WildernessGrids::get_monrace_hook() const
{
    return this->get_player_grid().get_monrace_hook();
}

WildernessLetters WildernessLetters::instance{};

WildernessLetters &WildernessLetters::get_instance()
{
    return instance;
}

void WildernessLetters::initialize()
{
    if (this->letters.empty()) {
        this->letters.resize(TerrainList::get_instance().size());
    }
}

const WildernessGrid &WildernessLetters::get_grid(int index) const
{
    return this->letters.at(index);
}

WildernessGrid &WildernessLetters::get_grid(int index)
{
    return this->letters.at(index);
}
