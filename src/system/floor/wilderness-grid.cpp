/*!
 * @brief 広域マップ実装
 * @author Hourier
 * @date 2025/02/06
 */

#include "system/floor/wilderness-grid.h"

std::vector<std::vector<WildernessGrid>> wilderness_grids;

WildernessGrids WildernessGrids::instance{};

void WildernessGrids::init_height(int height)
{
    this->bottom_right.y = height;
}

void WildernessGrids::init_width(int width)
{
    this->bottom_right.x = width;
}

void WildernessGrids::initialize_grids()
{
    wilderness_grids.assign(this->bottom_right.y, std::vector<WildernessGrid>(this->bottom_right.x));
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

void WildernessGrids::move_player_to(const Pos2DVec &vec)
{
    this->current_pos += vec;
}

bool WildernessGrids::is_height_initialized() const
{
    return this->bottom_right.y > 0;
}

bool WildernessGrids::is_width_initialized() const
{
    return this->bottom_right.x > 0;
}

const Pos2D &WildernessGrids::get_bottom_right() const
{
    return this->bottom_right;
}
