#include "system/grid-type-definition.h"
#include "monster/monster-util.h"
#include "room/door-definition.h"
#include "system/angband-system.h"
#include "system/enums/grid-flow.h"
#include "system/terrain-type-definition.h"
#include "util/bit-flags-calculator.h"

Grid::Grid()
{
    for (const auto gf : GRID_FLOW_RANGE) {
        this->costs[gf] = 0;
        this->dists[gf] = 0;
    }
}

/*!
 * @brief 指定座標がFLOOR属性を持ったマスかどうかを返す
 * @param Y 指定Y座標
 * @param X 指定X座標
 * @return FLOOR属性を持っているならばTRUE
 */
bool Grid::is_floor() const
{
    return any_bits(this->info, CAVE_FLOOR);
}

bool Grid::is_room() const
{
    return any_bits(this->info, CAVE_ROOM);
}

bool Grid::is_extra() const
{
    return any_bits(this->info, CAVE_EXTRA);
}

bool Grid::is_inner() const
{
    return any_bits(this->info, CAVE_INNER);
}

bool Grid::is_outer() const
{
    return any_bits(this->info, CAVE_OUTER);
}

bool Grid::is_solid() const
{
    return any_bits(this->info, CAVE_SOLID);
}

bool Grid::is_icky() const
{
    return any_bits(this->info, CAVE_ICKY);
}

bool Grid::is_lite() const
{
    return any_bits(this->info, CAVE_LITE);
}

bool Grid::is_redraw() const
{
    return any_bits(this->info, CAVE_REDRAW);
}

bool Grid::is_view() const
{
    return any_bits(this->info, CAVE_VIEW);
}

bool Grid::is_object() const
{
    return any_bits(this->info, CAVE_OBJECT);
}

bool Grid::is_mark() const
{
    return any_bits(this->info, CAVE_MARK);
}

bool Grid::is_mirror() const
{
    return this->is_object() && TerrainList::get_instance().get_terrain(this->mimic).flags.has(TerrainCharacteristics::MIRROR);
}

/*
 *  @brief 守りのルーンで守られているかを返す
 */
bool Grid::is_rune_protection() const
{
    return this->is_object() && TerrainList::get_instance().get_terrain(this->mimic).flags.has(TerrainCharacteristics::RUNE_PROTECTION);
}

/*
 *  @brief 爆発のルーンが仕掛けられているかを返す
 */
bool Grid::is_rune_explosion() const
{
    return this->is_object() && TerrainList::get_instance().get_terrain(this->mimic).flags.has(TerrainCharacteristics::RUNE_EXPLOSION);
}

/*!
 * @brief マスに隠されたドアがあるかの判定
 * @return 隠されたドアがあるか否か
 */
bool Grid::is_hidden_door() const
{
    const auto is_secret = (this->mimic > 0) || this->cave_has_flag(TerrainCharacteristics::SECRET);
    return is_secret && this->get_terrain().is_closed_door();
}

bool Grid::has_monster() const
{
    return is_monster(this->m_idx);
}

uint8_t Grid::get_cost(GridFlow gf) const
{
    return this->costs.at(gf);
}

uint8_t Grid::get_distance(GridFlow gf) const
{
    return this->dists.at(gf);
}

/*
 * @brief グリッドのミミック特性地形を返す
 * @param g_ptr グリッドへの参照ポインタ
 * @return 地形情報
 */
FEAT_IDX Grid::get_feat_mimic() const
{
    return TerrainList::get_instance().get_terrain(this->mimic ? this->mimic : this->feat).mimic;
}

bool Grid::cave_has_flag(TerrainCharacteristics feature_flags) const
{
    return this->get_terrain().flags.has(feature_flags);
}

/*!
 * @brief グリッドのシンボルが指定した記号かどうかを調べる
 * @param ch 指定するシンボル文字
 * @return シンボルが指定した記号か否か
 */
bool Grid::is_symbol(const int ch) const
{
    return this->get_terrain().symbol_configs.at(F_LIT_STANDARD).character == ch;
}

void Grid::reset_costs()
{
    for (const auto gf : GRID_FLOW_RANGE) {
        this->costs[gf] = 0;
    }
}

void Grid::reset_dists()
{
    for (const auto gf : GRID_FLOW_RANGE) {
        this->dists[gf] = 0;
    }
}

bool Grid::has_los() const
{
    return any_bits(this->info, CAVE_VIEW) || AngbandSystem::get_instance().is_phase_out();
}

TerrainType &Grid::get_terrain()
{
    return TerrainList::get_instance().get_terrain(this->feat);
}

const TerrainType &Grid::get_terrain() const
{
    return TerrainList::get_instance().get_terrain(this->feat);
}

TerrainType &Grid::get_terrain_mimic()
{
    return TerrainList::get_instance().get_terrain(this->get_feat_mimic());
}

const TerrainType &Grid::get_terrain_mimic() const
{
    return TerrainList::get_instance().get_terrain(this->get_feat_mimic());
}

TerrainType &Grid::get_terrain_mimic_raw()
{
    return TerrainList::get_instance().get_terrain(this->mimic);
}

const TerrainType &Grid::get_terrain_mimic_raw() const
{
    return TerrainList::get_instance().get_terrain(this->mimic);
}

void Grid::place_closed_curtain()
{
    this->feat = feat_door[DOOR_CURTAIN].closed;
    this->info &= ~(CAVE_MASK);
}

/*!
 * @brief グリッドに状態フラグを付与する
 * @param grid_info フラグ群
 * @todo intをenumに変更する
 */
void Grid::add_info(int grid_info)
{
    this->info |= grid_info;
}
