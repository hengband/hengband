#include "target/projection-path-calculator.h"
#include "effect/effect-characteristics.h"
#include "effect/spells-effect-util.h"
#include "floor/cave.h"
#include "grid/feature-flag-types.h"
#include "spell-class/spells-mirror-master.h"
#include "system/angband-system.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

struct projection_path_type {
    std::vector<std::pair<int, int>> *position;
    POSITION range;
    BIT_FLAGS flag;
    POSITION y1;
    POSITION x1;
    POSITION y2;
    POSITION x2;
    POSITION y;
    POSITION x;
    POSITION ay;
    POSITION ax;
    POSITION sy;
    POSITION sx;
    int frac;
    int m;
    int half;
    int full;
    int k;
};

std::vector<std::pair<int, int>>::const_iterator projection_path::begin() const
{
    return this->position.cbegin();
}

std::vector<std::pair<int, int>>::const_iterator projection_path::end() const
{
    return this->position.cend();
}

const std::pair<int, int> &projection_path::front() const
{
    return this->position.front();
}

const std::pair<int, int> &projection_path::back() const
{
    return this->position.back();
}

const std::pair<int, int> &projection_path::operator[](int num) const
{
    return this->position[num];
}

int projection_path::path_num() const
{
    return static_cast<int>(this->position.size());
}

static projection_path_type *initialize_projection_path_type(
    projection_path_type *pp_ptr, std::vector<std::pair<int, int>> *position, POSITION range, BIT_FLAGS flag, POSITION y1, POSITION x1, POSITION y2, POSITION x2)
{
    pp_ptr->position = position;
    pp_ptr->range = range;
    pp_ptr->flag = flag;
    pp_ptr->y1 = y1;
    pp_ptr->x1 = x1;
    pp_ptr->y2 = y2;
    pp_ptr->x2 = x2;
    return pp_ptr;
}

static void set_asxy(projection_path_type *pp_ptr)
{
    if (pp_ptr->y2 < pp_ptr->y1) {
        pp_ptr->ay = pp_ptr->y1 - pp_ptr->y2;
        pp_ptr->sy = -1;
    } else {
        pp_ptr->ay = pp_ptr->y2 - pp_ptr->y1;
        pp_ptr->sy = 1;
    }

    if (pp_ptr->x2 < pp_ptr->x1) {
        pp_ptr->ax = pp_ptr->x1 - pp_ptr->x2;
        pp_ptr->sx = -1;
    } else {
        pp_ptr->ax = pp_ptr->x2 - pp_ptr->x1;
        pp_ptr->sx = 1;
    }
}

static bool project_stop(PlayerType *player_ptr, projection_path_type *pp_ptr)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;

    if (none_bits(pp_ptr->flag, PROJECT_THRU) && (pp_ptr->x == pp_ptr->x2) && (pp_ptr->y == pp_ptr->y2)) {
        return true;
    }

    if (any_bits(pp_ptr->flag, PROJECT_DISI)) {
        if (!pp_ptr->position->empty() && cave_stop_disintegration(floor_ptr, pp_ptr->y, pp_ptr->x)) {
            return true;
        }
    } else if (any_bits(pp_ptr->flag, PROJECT_LOS)) {
        if (!pp_ptr->position->empty() && !cave_los_bold(floor_ptr, pp_ptr->y, pp_ptr->x)) {
            return true;
        }
    } else if (none_bits(pp_ptr->flag, PROJECT_PATH)) {
        if (!pp_ptr->position->empty() && !cave_has_flag_bold(floor_ptr, pp_ptr->y, pp_ptr->x, TerrainCharacteristics::PROJECT)) {
            return true;
        }
    }

    if (any_bits(pp_ptr->flag, PROJECT_MIRROR)) {
        if (!pp_ptr->position->empty() && floor_ptr->grid_array[pp_ptr->y][pp_ptr->x].is_mirror()) {
            return true;
        }
    }

    if (any_bits(pp_ptr->flag, PROJECT_STOP) && !pp_ptr->position->empty() && (player_bold(player_ptr, pp_ptr->y, pp_ptr->x) || floor_ptr->grid_array[pp_ptr->y][pp_ptr->x].m_idx != 0)) {
        return true;
    }

    if (!in_bounds(floor_ptr, pp_ptr->y, pp_ptr->x)) {
        return true;
    }

    return false;
}

static void calc_frac(projection_path_type *pp_ptr, bool is_vertical)
{
    if (pp_ptr->m == 0) {
        return;
    }

    pp_ptr->frac += pp_ptr->m;
    if (pp_ptr->frac <= pp_ptr->half) {
        return;
    }

    if (is_vertical) {
        pp_ptr->x += pp_ptr->sx;
    } else {
        pp_ptr->y += pp_ptr->sy;
    }

    pp_ptr->frac -= pp_ptr->full;
    pp_ptr->k++;
}

static void calc_projection_to_target(PlayerType *player_ptr, projection_path_type *pp_ptr, bool is_vertical)
{
    while (true) {
        pp_ptr->position->emplace_back(pp_ptr->y, pp_ptr->x);
        if (static_cast<int>(pp_ptr->position->size()) + pp_ptr->k / 2 >= pp_ptr->range) {
            break;
        }

        if (project_stop(player_ptr, pp_ptr)) {
            break;
        }

        calc_frac(pp_ptr, is_vertical);
        if (is_vertical) {
            pp_ptr->y += pp_ptr->sy;
        } else {
            pp_ptr->x += pp_ptr->sx;
        }
    }
}

static bool calc_vertical_projection(PlayerType *player_ptr, projection_path_type *pp_ptr)
{
    if (pp_ptr->ay <= pp_ptr->ax) {
        return false;
    }

    pp_ptr->m = pp_ptr->ax * pp_ptr->ax * 2;
    pp_ptr->y = pp_ptr->y1 + pp_ptr->sy;
    pp_ptr->x = pp_ptr->x1;
    pp_ptr->frac = pp_ptr->m;
    if (pp_ptr->frac > pp_ptr->half) {
        pp_ptr->x += pp_ptr->sx;
        pp_ptr->frac -= pp_ptr->full;
        pp_ptr->k++;
    }

    calc_projection_to_target(player_ptr, pp_ptr, true);
    return true;
}

static bool calc_horizontal_projection(PlayerType *player_ptr, projection_path_type *pp_ptr)
{
    if (pp_ptr->ax <= pp_ptr->ay) {
        return false;
    }

    pp_ptr->m = pp_ptr->ay * pp_ptr->ay * 2;
    pp_ptr->y = pp_ptr->y1;
    pp_ptr->x = pp_ptr->x1 + pp_ptr->sx;
    pp_ptr->frac = pp_ptr->m;
    if (pp_ptr->frac > pp_ptr->half) {
        pp_ptr->y += pp_ptr->sy;
        pp_ptr->frac -= pp_ptr->full;
        pp_ptr->k++;
    }

    calc_projection_to_target(player_ptr, pp_ptr, false);
    return true;
}

static void calc_projection_others(PlayerType *player_ptr, projection_path_type *pp_ptr)
{
    while (true) {
        pp_ptr->position->emplace_back(pp_ptr->y, pp_ptr->x);
        if (static_cast<int>(pp_ptr->position->size()) * 3 / 2 >= pp_ptr->range) {
            break;
        }

        if (project_stop(player_ptr, pp_ptr)) {
            break;
        }

        pp_ptr->y += pp_ptr->sy;
        pp_ptr->x += pp_ptr->sx;
    }
}

/*!
 * @brief 始点から終点への直線経路を返す /
 * Determine the path taken by a projection.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param range 距離
 * @param y1 始点Y座標
 * @param x1 始点X座標
 * @param y2 終点Y座標
 * @param x2 終点X座標
 * @param flag フラグID
 * @return リストの長さ
 */
projection_path::projection_path(PlayerType *player_ptr, POSITION range, POSITION y1, POSITION x1, POSITION y2, POSITION x2, BIT_FLAGS flag)
{
    this->position.clear();
    if ((x1 == x2) && (y1 == y2)) {
        return;
    }

    projection_path_type tmp_projection_path;
    auto *pp_ptr = initialize_projection_path_type(&tmp_projection_path, &this->position, range, flag, y1, x1, y2, x2);
    set_asxy(pp_ptr);
    pp_ptr->half = pp_ptr->ay * pp_ptr->ax;
    pp_ptr->full = pp_ptr->half << 1;
    pp_ptr->k = 0;

    if (calc_vertical_projection(player_ptr, pp_ptr)) {
        return;
    }

    if (calc_horizontal_projection(player_ptr, pp_ptr)) {
        return;
    }

    pp_ptr->y = y1 + pp_ptr->sy;
    pp_ptr->x = x1 + pp_ptr->sx;
    calc_projection_others(player_ptr, pp_ptr);
}

/*
 * Determine if a bolt spell cast from (y1,x1) to (y2,x2) will arrive
 * at the final destination, assuming no monster gets in the way.
 *
 * This is slightly (but significantly) different from "los(y1,x1,y2,x2)".
 */
bool projectable(PlayerType *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2)
{
    projection_path grid_g(player_ptr, (project_length ? project_length : AngbandSystem::get_instance().get_max_range()), y1, x1, y2, x2, 0);
    if (grid_g.path_num() == 0) {
        return true;
    }

    const auto &[y, x] = grid_g.back();
    if ((y != y2) || (x != x2)) {
        return false;
    }

    return true;
}

/*
 * Convert a "grid" (G) into a "location" (Y)
 */
POSITION get_grid_y(uint16_t grid)
{
    return (int)(grid / 256U);
}

/*
 * Convert a "grid" (G) into a "location" (X)
 */
POSITION get_grid_x(uint16_t grid)
{
    return (int)(grid % 256U);
}
