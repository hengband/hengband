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

class projection_path_type {
public:
    projection_path_type(std::vector<Pos2D> *position, int range, uint32_t flag, Pos2D pos_src, Pos2D pos_dst)
        : position(position)
        , range(range)
        , flag(flag)
        , pos_src(pos_src)
        , pos_dst(pos_dst)
    {
        int ay;
        int sy;
        if (this->pos_dst.y < this->pos_src.y) {
            ay = this->pos_src.y - this->pos_dst.y;
            sy = -1;
        } else {
            ay = this->pos_dst.y - this->pos_src.y;
            sy = 1;
        }

        int ax;
        int sx;
        if (this->pos_dst.x < this->pos_src.x) {
            ax = this->pos_src.x - this->pos_dst.x;
            sx = -1;
        } else {
            ax = this->pos_dst.x - this->pos_src.x;
            sx = 1;
        }

        this->pos_a = Pos2D(ay, ax);
        this->pos_s = Pos2D(sy, sx);

        this->half = this->pos_a.y * this->pos_a.x;
        this->full = this->half << 1;
        this->k = 0;
    }

    std::vector<Pos2D> *position;
    int range;
    uint32_t flag;
    Pos2D pos_src;
    Pos2D pos_dst;
    Pos2D pos_a;
    Pos2D pos_s;
    Pos2D pos{};
    int frac = 0;
    int m = 0;
    int half;
    int full;
    int k;
};

std::vector<Pos2D>::const_iterator ProjectionPath::begin() const
{
    return this->position.cbegin();
}

std::vector<Pos2D>::const_iterator ProjectionPath::end() const
{
    return this->position.cend();
}

const Pos2D &ProjectionPath::front() const
{
    return this->position.front();
}

const Pos2D &ProjectionPath::back() const
{
    return this->position.back();
}

const Pos2D &ProjectionPath::operator[](int num) const
{
    return this->position[num];
}

int ProjectionPath::path_num() const
{
    return static_cast<int>(this->position.size());
}

static bool project_stop(PlayerType *player_ptr, projection_path_type *pp_ptr)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    if (none_bits(pp_ptr->flag, PROJECT_THRU) && (pp_ptr->pos == pp_ptr->pos_dst)) {
        return true;
    }

    if (any_bits(pp_ptr->flag, PROJECT_DISI)) {
        if (!pp_ptr->position->empty() && cave_stop_disintegration(floor_ptr, pp_ptr->pos.y, pp_ptr->pos.x)) {
            return true;
        }
    } else if (any_bits(pp_ptr->flag, PROJECT_LOS)) {
        if (!pp_ptr->position->empty() && !cave_los_bold(floor_ptr, pp_ptr->pos.y, pp_ptr->pos.x)) {
            return true;
        }
    } else if (none_bits(pp_ptr->flag, PROJECT_PATH)) {
        if (!pp_ptr->position->empty() && !cave_has_flag_bold(floor_ptr, pp_ptr->pos.y, pp_ptr->pos.x, TerrainCharacteristics::PROJECT)) {
            return true;
        }
    }

    const auto &grid = floor_ptr->get_grid(pp_ptr->pos);
    if (any_bits(pp_ptr->flag, PROJECT_MIRROR)) {
        if (!pp_ptr->position->empty() && grid.is_mirror()) {
            return true;
        }
    }

    if (any_bits(pp_ptr->flag, PROJECT_STOP) && !pp_ptr->position->empty() && (player_ptr->is_located_at(pp_ptr->pos) || grid.has_monster())) {
        return true;
    }

    if (!in_bounds(floor_ptr, pp_ptr->pos.y, pp_ptr->pos.x)) {
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
        pp_ptr->pos.x += pp_ptr->pos_s.x;
    } else {
        pp_ptr->pos.y += pp_ptr->pos_s.y;
    }

    pp_ptr->frac -= pp_ptr->full;
    pp_ptr->k++;
}

static void calc_projection_to_target(PlayerType *player_ptr, projection_path_type *pp_ptr, bool is_vertical)
{
    while (true) {
        pp_ptr->position->push_back(pp_ptr->pos);
        if (static_cast<int>(pp_ptr->position->size()) + pp_ptr->k / 2 >= pp_ptr->range) {
            break;
        }

        if (project_stop(player_ptr, pp_ptr)) {
            break;
        }

        calc_frac(pp_ptr, is_vertical);
        if (is_vertical) {
            pp_ptr->pos.y += pp_ptr->pos_s.y;
        } else {
            pp_ptr->pos.x += pp_ptr->pos_s.x;
        }
    }
}

static bool calc_vertical_projection(PlayerType *player_ptr, projection_path_type *pp_ptr)
{
    if (pp_ptr->pos_a.y <= pp_ptr->pos_a.x) {
        return false;
    }

    pp_ptr->m = pp_ptr->pos_a.x * pp_ptr->pos_a.x * 2;
    pp_ptr->pos.y = pp_ptr->pos_src.y + pp_ptr->pos_s.y;
    pp_ptr->pos.x = pp_ptr->pos_src.x;
    pp_ptr->frac = pp_ptr->m;
    if (pp_ptr->frac > pp_ptr->half) {
        pp_ptr->pos.x += pp_ptr->pos_s.x;
        pp_ptr->frac -= pp_ptr->full;
        pp_ptr->k++;
    }

    calc_projection_to_target(player_ptr, pp_ptr, true);
    return true;
}

static bool calc_horizontal_projection(PlayerType *player_ptr, projection_path_type *pp_ptr)
{
    if (pp_ptr->pos_a.x <= pp_ptr->pos_a.y) {
        return false;
    }

    pp_ptr->m = pp_ptr->pos_a.y * pp_ptr->pos_a.y * 2;
    pp_ptr->pos.y = pp_ptr->pos_src.y;
    pp_ptr->pos.x = pp_ptr->pos_src.x + pp_ptr->pos_s.x;
    pp_ptr->frac = pp_ptr->m;
    if (pp_ptr->frac > pp_ptr->half) {
        pp_ptr->pos.y += pp_ptr->pos_s.y;
        pp_ptr->frac -= pp_ptr->full;
        pp_ptr->k++;
    }

    calc_projection_to_target(player_ptr, pp_ptr, false);
    return true;
}

static void calc_projection_others(PlayerType *player_ptr, projection_path_type *pp_ptr)
{
    while (true) {
        pp_ptr->position->push_back(pp_ptr->pos);
        if (static_cast<int>(pp_ptr->position->size()) * 3 / 2 >= pp_ptr->range) {
            break;
        }

        if (project_stop(player_ptr, pp_ptr)) {
            break;
        }

        pp_ptr->pos.y += pp_ptr->pos_src.y;
        pp_ptr->pos.x += pp_ptr->pos_src.x;
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
ProjectionPath::ProjectionPath(PlayerType *player_ptr, int range, const Pos2D &pos_src, const Pos2D &pos_dst, uint32_t flag)
{
    this->position.clear();
    if (pos_src == pos_dst) {
        return;
    }

    projection_path_type pp(&this->position, range, flag, pos_src, pos_dst);
    if (calc_vertical_projection(player_ptr, &pp)) {
        return;
    }

    if (calc_horizontal_projection(player_ptr, &pp)) {
        return;
    }

    pp.pos.y = pos_src.y + pp.pos_s.y;
    pp.pos.x = pos_src.x + pp.pos_s.x;
    calc_projection_others(player_ptr, &pp);
}

/*
 * Determine if a bolt spell cast from (y1,x1) to (y2,x2) will arrive
 * at the final destination, assuming no monster gets in the way.
 *
 * This is slightly (but significantly) different from "los(y1,x1,y2,x2)".
 */
bool projectable(PlayerType *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2)
{
    ProjectionPath grid_g(player_ptr, (project_length ? project_length : AngbandSystem::get_instance().get_max_range()), { y1, x1 }, { y2, x2 }, 0);
    if (grid_g.path_num() == 0) {
        return true;
    }

    const auto &[y, x] = grid_g.back();
    if ((y != y2) || (x != x2)) {
        return false;
    }

    return true;
}
