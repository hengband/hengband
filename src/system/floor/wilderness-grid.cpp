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
    this->bottom_right.y = height;
}

void WildernessGrids::init_width(int width)
{
    this->bottom_right.x = width;
}

void WildernessGrids::initialize_grids()
{
    wilderness_grids.assign(this->bottom_right.y, std::vector<WildernessGrid>(this->bottom_right.x));
    for (auto y = 0; y < this->bottom_right.y; y++) {
        for (auto x = 0; x < this->bottom_right.x; x++) {
            this->positions.emplace_back(y, x);
        }
    }
}

void WildernessGrids::initialize_seeds()
{
    for (const auto &pos : this->positions) {
        wilderness_grids[pos.y][pos.x].initialize_seed();
    }
}

void WildernessGrids::initialize_position()
{
    constexpr Pos2D initial_position(48, 5);
    this->current_pos = initial_position;
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

bool WildernessGrids::is_height_initialized() const
{
    return this->bottom_right.y > 0;
}

bool WildernessGrids::is_width_initialized() const
{
    return this->bottom_right.x > 0;
}

bool WildernessGrids::has_player_located() const
{
    return (this->current_pos.x > 0) && (this->current_pos.y > 0);
}

bool WildernessGrids::is_player_in_bounds() const
{
    auto is_in_bounds_x = this->current_pos.x >= 1;
    is_in_bounds_x &= this->current_pos.x <= this->bottom_right.x;
    auto is_in_bounds_y = this->current_pos.y >= 1;
    is_in_bounds_y &= this->current_pos.y <= this->bottom_right.y;
    return is_in_bounds_x && is_in_bounds_y;
}

const Pos2D &WildernessGrids::get_bottom_right() const
{
    return this->bottom_right;
}

const std::vector<Pos2D> &WildernessGrids::get_positions() const
{
    return this->positions;
}
