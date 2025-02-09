/*!
 * @brief 広域マップ実装
 * @author Hourier
 * @date 2025/02/06
 */

#include "system/floor/wilderness-grid.h"
#include "floor/geometry.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "term/z-rand.h"

std::vector<std::vector<WildernessGrid>> wilderness_grids;

int WildernessGrid::get_level() const
{
    return this->level;
}

void WildernessGrid::initialize_seed()
{
    this->seed = randint0(0x10000000);
}

WildernessGrids WildernessGrids::instance{};

void WildernessGrids::init_height(int height)
{
    this->area.bottom_right.y = height - 1;
}

void WildernessGrids::init_width(int width)
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
    return wilderness_grids[pos.y][pos.x];
}

WildernessGrid &WildernessGrids::get_grid(const Pos2D &pos)
{
    return wilderness_grids[pos.y][pos.x];
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
