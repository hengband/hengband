#include "system/grid-type-definition.h"
#include "monster-race/race-flags7.h"
#include "system/monster-race-definition.h"
#include "system/terrain-type-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief 指定座標がFLOOR属性を持ったマスかどうかを返す
 * @param Y 指定Y座標
 * @param X 指定X座標
 * @return FLOOR属性を持っているならばTRUE
 */
bool grid_type::is_floor() const
{
    return any_bits(this->info, CAVE_FLOOR);
}

bool grid_type::is_room() const
{
    return any_bits(this->info, CAVE_ROOM);
}

bool grid_type::is_extra() const
{
    return any_bits(this->info, CAVE_EXTRA);
}

bool grid_type::is_inner() const
{
    return any_bits(this->info, CAVE_INNER);
}

bool grid_type::is_outer() const
{
    return any_bits(this->info, CAVE_OUTER);
}

bool grid_type::is_solid() const
{
    return any_bits(this->info, CAVE_SOLID);
}

bool grid_type::is_icky() const
{
    return any_bits(this->info, CAVE_ICKY);
}

bool grid_type::is_lite() const
{
    return any_bits(this->info, CAVE_LITE);
}

bool grid_type::is_redraw() const
{
    return any_bits(this->info, CAVE_REDRAW);
}

bool grid_type::is_view() const
{
    return any_bits(this->info, CAVE_VIEW);
}

bool grid_type::is_object() const
{
    return any_bits(this->info, CAVE_OBJECT);
}

bool grid_type::is_mark() const
{
    return any_bits(this->info, CAVE_MARK);
}

bool grid_type::is_mirror() const
{
    return this->is_object() && terrains_info[this->mimic].flags.has(TerrainCharacteristics::MIRROR);
}

/*
 *  @brief 守りのルーンで守られているかを返す
 */
bool grid_type::is_rune_protection() const
{
    return this->is_object() && terrains_info[this->mimic].flags.has(TerrainCharacteristics::RUNE_PROTECTION);
}

/*
 *  @brief 爆発のルーンが仕掛けられているかを返す
 */
bool grid_type::is_rune_explosion() const
{
    return this->is_object() && terrains_info[this->mimic].flags.has(TerrainCharacteristics::RUNE_EXPLOSION);
}

byte grid_type::get_cost(monster_race *r_ptr) const
{
    return this->costs[get_grid_flow_type(r_ptr)];
}

byte grid_type::get_distance(monster_race *r_ptr) const
{
    return this->dists[get_grid_flow_type(r_ptr)];
}

flow_type grid_type::get_grid_flow_type(monster_race *r_ptr) const
{
    return r_ptr->feature_flags.has(MonsterFeatureType::CAN_FLY) ? FLOW_CAN_FLY : FLOW_NORMAL;
}

/*
 * @brief グリッドのミミック特性地形を返す
 * @param g_ptr グリッドへの参照ポインタ
 * @return 地形情報
 */
FEAT_IDX grid_type::get_feat_mimic() const
{
    return terrains_info[this->mimic ? this->mimic : this->feat].mimic;
}

bool grid_type::cave_has_flag(TerrainCharacteristics feature_flags) const
{
    return terrains_info[this->feat].flags.has(feature_flags);
}

/*!
 * @brief グリッドのシンボルが指定した記号かどうかを調べる
 * @param ch 指定するシンボル文字
 * @return シンボルが指定した記号か否か
 */
bool grid_type::is_symbol(const int ch) const
{
    return terrains_info[this->feat].x_char[0] == ch;
}
