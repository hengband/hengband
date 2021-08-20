#include "system/grid-type-definition.h"
#include "grid/feature.h" // @todo 相互依存している. 後で何とかする.
#include "monster-race/race-flags7.h"
#include "system/monster-race-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief 指定座標がFLOOR属性を持ったマスかどうかを返す
 * @param Y 指定Y座標
 * @param X 指定X座標
 * @return FLOOR属性を持っているならばTRUE
 */
bool grid_type::is_floor()
{
    return any_bits(this->info, CAVE_FLOOR);
}

bool grid_type::is_room()
{
    return any_bits(this->info, CAVE_ROOM);
}

bool grid_type::is_extra()
{
    return any_bits(this->info, CAVE_EXTRA);
}

bool grid_type::is_inner()
{
    return any_bits(this->info, CAVE_INNER);
}

bool grid_type::is_outer()
{
    return any_bits(this->info, CAVE_OUTER);
}

bool grid_type::is_solid()
{
    return any_bits(this->info, CAVE_SOLID);
}

bool grid_type::is_icky()
{
    return any_bits(this->info, CAVE_ICKY);
}

bool grid_type::is_lite()
{
    return any_bits(this->info, CAVE_LITE);
}

bool grid_type::is_redraw()
{
    return any_bits(this->info, CAVE_REDRAW);
}

bool grid_type::is_view()
{
    return any_bits(this->info, CAVE_VIEW);
}

bool grid_type::is_object()
{
    return any_bits(this->info, CAVE_OBJECT);
}

bool grid_type::is_mark()
{
    return any_bits(this->info, CAVE_MARK);
}

bool grid_type::is_mirror()
{
    return this->is_object() && f_info[this->mimic].flags.has(FF::MIRROR);
}

/*
 *  @brief 守りのルーンで守られているかを返す
 */
bool grid_type::is_rune_protection()
{
    return this->is_object() && f_info[this->mimic].flags.has(FF::RUNE_PROTECTION);
}

/*
 *  @brief 爆発のルーンが仕掛けられているかを返す
 */
bool grid_type::is_rune_explosion()
{
    return this->is_object() && f_info[this->mimic].flags.has(FF::RUNE_EXPLOSION);
}

byte grid_type::get_cost(monster_race *r_ptr)
{
    return this->costs[get_grid_flow_type(r_ptr)];
}

byte grid_type::get_distance(monster_race *r_ptr)
{
    return this->dists[get_grid_flow_type(r_ptr)];
}

flow_type grid_type::get_grid_flow_type(monster_race *r_ptr)
{
    return any_bits(r_ptr->flags7, RF7_CAN_FLY) ? FLOW_CAN_FLY : FLOW_NORMAL;
}

/*
 * @brief Get feature mimic from f_info[] (applying "mimic" field)
 * @param g_ptr グリッドへの参照ポインタ
 * @return 地形情報
 */
FEAT_IDX grid_type::get_feat_mimic()
{
    return f_info[this->mimic ? this->mimic : this->feat].mimic;
}

bool grid_type::cave_has_flag(FF feature_flags)
{
    return f_info[this->feat].flags.has(feature_flags);
}
