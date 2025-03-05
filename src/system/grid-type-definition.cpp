#include "system/grid-type-definition.h"
#include "monster/monster-util.h"
#include "room/door-definition.h"
#include "system/angband-system.h"
#include "system/enums/grid-flow.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"

Grid::Grid()
{
    for (const auto gf : GRID_FLOW_RANGE) {
        this->costs[gf] = 0;
        this->dists[gf] = 0;
    }
}

/*!
 * @brief 2点間の距離をニュートン・ラプソン法で算出する / Distance between two points via Newton-Raphson technique
 * @param pos1 1点目の座標
 * @param pos2 2点目の座標
 * @return 2点間の距離
 */
int Grid::calc_distance(const Pos2D &pos1, const Pos2D &pos2)
{
    const auto dy = std::abs(pos1.y - pos2.y);
    const auto dx = std::abs(pos1.x - pos2.x);
    auto approximate_distance = std::max(dy, dx) + std::min(dy, dx) / 2;

    if (dy == 0 || dx == 0) {
        return approximate_distance;
    }

    const auto squared_distance = (dy * dy) + (dx * dx);
    while (true) {
        const auto approximate_error = (squared_distance - approximate_distance * approximate_distance) / (2 * approximate_distance);
        if (approximate_error == 0) {
            return approximate_distance;
        }

        approximate_distance += approximate_error;
    }
}

short Grid::get_terrain_id(TerrainKind tk) const
{
    switch (tk) {
    case TerrainKind::NORMAL:
        return this->feat;
    case TerrainKind::MIMIC:
        return TerrainList::get_instance().get_terrain(this->mimic ? this->mimic : this->feat).mimic;
    case TerrainKind::MIMIC_RAW:
        return this->mimic;
    default:
        THROW_EXCEPTION(std::logic_error, format("Invalid terrain kind is specified! %d", enum2i(tk)));
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

bool Grid::is_open() const
{
    return this->get_terrain(TerrainKind::MIMIC).is_open();
}

bool Grid::is_closed_door(bool is_mimic) const
{
    const auto tk = is_mimic ? TerrainKind::MIMIC : TerrainKind::NORMAL;
    return this->get_terrain(tk).is_closed_door();
}

/*!
 * @brief マスに隠されたドアがあるかの判定
 * @return 隠されたドアがあるか否か
 */
bool Grid::is_hidden_door() const
{
    const auto is_secret = (this->mimic > 0) || this->has(TerrainCharacteristics::SECRET);
    return is_secret && this->get_terrain().is_closed_door();
}

bool Grid::is_acceptable_target() const
{
    auto is_acceptable = this->has(TerrainCharacteristics::LESS);
    is_acceptable |= this->has(TerrainCharacteristics::MORE);
    is_acceptable |= this->has(TerrainCharacteristics::QUEST_ENTER);
    is_acceptable |= this->has(TerrainCharacteristics::QUEST_EXIT);
    is_acceptable |= this->has(TerrainCharacteristics::STORE);
    is_acceptable |= this->has(TerrainCharacteristics::BLDG);
    return is_acceptable;
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

bool Grid::has(TerrainCharacteristics tc) const
{
    return this->get_terrain().has(tc);
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

/*!
 * @brief モンスターにより照明が消されている地形か否かを判定する
 * @return 照明が消されているか否か
 */
bool Grid::is_darkened() const
{
    return match_bits(this->info, CAVE_VIEW | CAVE_LITE | CAVE_MNLT | CAVE_MNDK, CAVE_VIEW | CAVE_MNDK);
}

bool Grid::is_clean() const
{
    return this->has(TerrainCharacteristics::FLOOR) && !this->is_object() && this->o_idx_list.empty();
}

bool Grid::has_special_terrain() const
{
    return this->get_terrain().flags.has(TerrainCharacteristics::SPECIAL);
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

bool Grid::has_los_terrain(TerrainKind tk) const
{
    return this->get_terrain(tk).flags.has(TerrainCharacteristics::LOS);
}

TerrainType &Grid::get_terrain(TerrainKind tk)
{
    auto &terrains = TerrainList::get_instance();
    switch (tk) {
    case TerrainKind::NORMAL:
        return terrains.get_terrain(this->feat);
    case TerrainKind::MIMIC:
        return terrains.get_terrain(this->get_terrain_id(TerrainKind::MIMIC));
    case TerrainKind::MIMIC_RAW:
        return terrains.get_terrain(this->mimic);
    default:
        THROW_EXCEPTION(std::logic_error, format("Invalid terrain kind is specified! %d", enum2i(tk)));
    }
}

const TerrainType &Grid::get_terrain(TerrainKind tk) const
{
    const auto &terrains = TerrainList::get_instance();
    switch (tk) {
    case TerrainKind::NORMAL:
        return terrains.get_terrain(this->feat);
    case TerrainKind::MIMIC:
        return terrains.get_terrain(this->get_terrain_id(TerrainKind::MIMIC));
    case TerrainKind::MIMIC_RAW:
        return terrains.get_terrain(this->mimic);
    default:
        THROW_EXCEPTION(std::logic_error, format("Invalid terrain kind is specified! %d", enum2i(tk)));
    }
}

void Grid::place_closed_curtain()
{
    this->set_terrain_id(Doors::get_instance().get_door(DoorKind::CURTAIN).closed);
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

//!< @details MIMIC_RAW は入ってこない想定.
void Grid::set_terrain_id(short terrain_id, TerrainKind tk)
{
    switch (tk) {
    case TerrainKind::NORMAL:
        this->feat = terrain_id;
        break;
    case TerrainKind::MIMIC:
        this->mimic = terrain_id;
        break;
    default:
        THROW_EXCEPTION(std::logic_error, format("Invalid terrain kind is specified! %d", enum2i(tk)));
    }
}

void Grid::set_terrain_id(TerrainTag tag, TerrainKind tk)
{
    this->set_terrain_id(TerrainList::get_instance().get_terrain_id(tag), tk);
}

void Grid::set_door_id(short terrain_id_random)
{
    if (!this->has_los_terrain(TerrainKind::MIMIC_RAW) || this->has_los_terrain()) {
        return;
    }

    const auto &terrain_mimic = this->get_terrain(TerrainKind::MIMIC_RAW);
    if (terrain_mimic.flags.has(TerrainCharacteristics::MOVE) || terrain_mimic.flags.has(TerrainCharacteristics::CAN_FLY)) {
        const auto terrain_id = one_in_(2) ? this->mimic : terrain_id_random;
        this->set_terrain_id(terrain_id);
    }

    this->set_terrain_id(TerrainTag::NONE, TerrainKind::MIMIC);
}
