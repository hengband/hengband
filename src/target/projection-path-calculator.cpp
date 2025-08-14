#include "target/projection-path-calculator.h"
#include "effect/effect-characteristics.h"
#include "effect/spells-effect-util.h"
#include "system/enums/terrain/terrain-characteristics.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "util/finalizer.h"

class ProjectionPathHelper {
public:
    ProjectionPathHelper(int range, uint32_t flag, const Pos2D &pos_src, const Pos2D &pos_dst)
        : range(range)
        , flag(flag)
        , pos_src(pos_src)
        , pos_dst(pos_dst)
        , pos_diff(pos_dst - pos_src)
        , half(std::abs(pos_diff.y) * std::abs(pos_diff.x))
        , full(half * 2)
    {
    }

    std::vector<Pos2D> positions;
    int range;
    uint32_t flag;
    Pos2D pos_src;
    Pos2D pos_dst;
    Pos2DVec pos_diff;
    Pos2D pos{ 0, 0 };
    int frac = 0;
    int m = 0;
    int half;
    int full;
    int k = 0;
};

std::vector<Pos2D>::const_iterator ProjectionPath::begin() const
{
    return this->positions.cbegin();
}

std::vector<Pos2D>::const_iterator ProjectionPath::end() const
{
    return this->positions.cend();
}

const Pos2D &ProjectionPath::front() const
{
    return this->positions.front();
}

const Pos2D &ProjectionPath::back() const
{
    return this->positions.back();
}

const Pos2D &ProjectionPath::operator[](int num) const
{
    return this->positions[num];
}

int ProjectionPath::path_num() const
{
    return static_cast<int>(this->positions.size());
}

static int sign(int num)
{
    if (num > 0) {
        return 1;
    }
    if (num < 0) {
        return -1;
    }
    return 0;
}

static bool project_stop(const FloorType &floor, const Pos2D &p_pos, ProjectionPathHelper *pph_ptr)
{
    if (none_bits(pph_ptr->flag, PROJECT_THRU) && (pph_ptr->pos == pph_ptr->pos_dst)) {
        return true;
    }

    const auto &grid = floor.get_grid(pph_ptr->pos);
    if (any_bits(pph_ptr->flag, PROJECT_DISI)) {
        if (!pph_ptr->positions.empty() && grid.can_block_disintegration()) {
            return true;
        }
    } else if (any_bits(pph_ptr->flag, PROJECT_LOS)) {
        if (!pph_ptr->positions.empty() && !grid.has_los_terrain()) {
            return true;
        }
    } else if (none_bits(pph_ptr->flag, PROJECT_PATH)) {
        if (!pph_ptr->positions.empty() && !grid.has(TerrainCharacteristics::PROJECTION)) {
            return true;
        }
    }

    if (any_bits(pph_ptr->flag, PROJECT_MIRROR)) {
        if (!pph_ptr->positions.empty() && grid.is_mirror()) {
            return true;
        }
    }

    if (any_bits(pph_ptr->flag, PROJECT_STOP) && !pph_ptr->positions.empty() && ((p_pos == pph_ptr->pos) || grid.has_monster())) {
        return true;
    }

    return !floor.contains(pph_ptr->pos);
}

static void calc_frac(ProjectionPathHelper *pph_ptr, bool is_vertical)
{
    if (pph_ptr->m == 0) {
        return;
    }

    pph_ptr->frac += pph_ptr->m;
    if (pph_ptr->frac <= pph_ptr->half) {
        return;
    }

    if (is_vertical) {
        pph_ptr->pos.x += sign(pph_ptr->pos_diff.x);
    } else {
        pph_ptr->pos.y += sign(pph_ptr->pos_diff.y);
    }

    pph_ptr->frac -= pph_ptr->full;
    pph_ptr->k++;
}

static void calc_projection_to_target(const FloorType &floor, const Pos2D &p_pos, ProjectionPathHelper *pph_ptr, bool is_vertical)
{
    while (true) {
        pph_ptr->positions.push_back(pph_ptr->pos);
        if (static_cast<int>(pph_ptr->positions.size()) + pph_ptr->k / 2 >= pph_ptr->range) {
            break;
        }

        if (project_stop(floor, p_pos, pph_ptr)) {
            break;
        }

        calc_frac(pph_ptr, is_vertical);
        if (is_vertical) {
            pph_ptr->pos.y += sign(pph_ptr->pos_diff.y);
        } else {
            pph_ptr->pos.x += sign(pph_ptr->pos_diff.x);
        }
    }
}

static bool calc_vertical_projection(const FloorType &floor, const Pos2D &p_pos, ProjectionPathHelper *pph_ptr)
{
    if (std::abs(pph_ptr->pos_diff.y) <= std::abs(pph_ptr->pos_diff.x)) {
        return false;
    }

    pph_ptr->m = pph_ptr->pos_diff.x * pph_ptr->pos_diff.x * 2;
    pph_ptr->pos.y = pph_ptr->pos_src.y + sign(pph_ptr->pos_diff.y);
    pph_ptr->pos.x = pph_ptr->pos_src.x;
    pph_ptr->frac = pph_ptr->m;
    if (pph_ptr->frac > pph_ptr->half) {
        pph_ptr->pos.x += sign(pph_ptr->pos_diff.x);
        pph_ptr->frac -= pph_ptr->full;
        pph_ptr->k++;
    }

    calc_projection_to_target(floor, p_pos, pph_ptr, true);
    return true;
}

static bool calc_horizontal_projection(const FloorType &floor, const Pos2D &p_pos, ProjectionPathHelper *pph_ptr)
{
    if (std::abs(pph_ptr->pos_diff.x) <= std::abs(pph_ptr->pos_diff.y)) {
        return false;
    }

    pph_ptr->m = pph_ptr->pos_diff.y * pph_ptr->pos_diff.y * 2;
    pph_ptr->pos.y = pph_ptr->pos_src.y;
    pph_ptr->pos.x = pph_ptr->pos_src.x + sign(pph_ptr->pos_diff.x);
    pph_ptr->frac = pph_ptr->m;
    if (pph_ptr->frac > pph_ptr->half) {
        pph_ptr->pos.y += sign(pph_ptr->pos_diff.y);
        pph_ptr->frac -= pph_ptr->full;
        pph_ptr->k++;
    }

    calc_projection_to_target(floor, p_pos, pph_ptr, false);
    return true;
}

static void calc_diagonal_projection(const FloorType &floor, const Pos2D &p_pos, ProjectionPathHelper *pph_ptr)
{
    pph_ptr->pos.y = pph_ptr->pos_src.y + sign(pph_ptr->pos_diff.y);
    pph_ptr->pos.x = pph_ptr->pos_src.x + sign(pph_ptr->pos_diff.x);

    while (true) {
        pph_ptr->positions.push_back(pph_ptr->pos);
        if (static_cast<int>(pph_ptr->positions.size()) * 3 / 2 >= pph_ptr->range) {
            break;
        }

        if (project_stop(floor, p_pos, pph_ptr)) {
            break;
        }

        pph_ptr->pos.y += sign(pph_ptr->pos_diff.y);
        pph_ptr->pos.x += sign(pph_ptr->pos_diff.x);
    }
}

/*!
 * @brief 始点から終点への直線経路を返す
 * @param floor フロアへの参照
 * @param range 距離
 * @param p_pos プレイヤーの座標
 * @param pos_src 始点座標
 * @param pos_dst 終点座標
 * @param flag フラグ群
 */
ProjectionPath::ProjectionPath(const FloorType &floor, int range, const Pos2D &p_pos, const Pos2D &pos_src, const Pos2D &pos_dst, uint32_t flag)
{
    if (pos_src == pos_dst) {
        return;
    }

    ProjectionPathHelper pph(range, flag, pos_src, pos_dst);
    const auto finalizer = util::make_finalizer([this, &pph] { this->positions = std::move(pph.positions); });
    if (calc_vertical_projection(floor, p_pos, &pph)) {
        return;
    }

    if (calc_horizontal_projection(floor, p_pos, &pph)) {
        return;
    }

    calc_diagonal_projection(floor, p_pos, &pph);
}

/*!
 * @brief 始点から終点への直線経路を返す(フラグ指定なし)
 * @param floor フロアへの参照
 * @param range 距離
 * @param pos_src 始点座標
 * @param pos_dst 終点座標
 * @details プレイヤーの座標はPROJECT_STOPフラグが設定されている時のみ使用されるため、フラグ指定無しの場合は不要。
 * したがってダミー座標を渡しておく。
 */
ProjectionPath::ProjectionPath(const FloorType &floor, int range, const Pos2D &pos_src, const Pos2D &pos_dst)
    : ProjectionPath(floor, range, { 0, 0 } /* dummy */, pos_src, pos_dst, 0)
{
}

/*
 * Determine if a bolt spell cast from pos_src to pos_dst will arrive
 * at the final destination, assuming no monster gets in the way.
 *
 * This is slightly (but significantly) different from "los(floor, pos_src, pos_dst)".
 */
bool projectable(const FloorType &floor, const Pos2D &pos_src, const Pos2D &pos_dst)
{
    const auto range = project_length ? project_length : AngbandSystem::get_instance().get_max_range();
    ProjectionPath grid_g(floor, range, pos_src, pos_dst);
    if (grid_g.path_num() == 0) {
        return true;
    }

    const auto &pos = grid_g.back();
    return pos == pos_dst;
}
