/*
 * @brief 地形特性定義
 * @author Hourier
 * @date 2022/10/15
 */

#include "system/terrain/terrain-definition.h"
#include "grid/lighting-colors-table.h"
#include "system/enums/terrain/terrain-characteristics.h"
#include "system/enums/terrain/terrain-tag.h"
#include "util/flag-group.h"
#include <algorithm>
#include <unordered_map>

namespace {
const std::unordered_map<TerrainCharacteristics, EnumClassFlagGroup<TerrainAction>> TERRAIN_ACTIONS_TABLE = {
    { TerrainCharacteristics::BASH, { TerrainAction::CRASH_GLASS } },
    { TerrainCharacteristics::DISARM, { TerrainAction::DESTROY } },
    { TerrainCharacteristics::TUNNEL, { TerrainAction::DESTROY, TerrainAction::CRASH_GLASS } },
    { TerrainCharacteristics::STONE, { TerrainAction::DESTROY, TerrainAction::CRASH_GLASS } },
    { TerrainCharacteristics::CAN_DISINTEGRATE, { TerrainAction::DESTROY, TerrainAction::NO_DROP, TerrainAction::CRASH_GLASS } },
};
}

bool TerrainType::has(TerrainCharacteristics tc, TerrainAction ta)
{
    static const auto begin = TERRAIN_ACTIONS_TABLE.begin();
    static const auto end = TERRAIN_ACTIONS_TABLE.end();
    const auto action = std::find_if(begin, end, [tc](const auto &x) { return x.first == tc; });
    if (action == end) {
        return false;
    }

    return action->second.has(ta);
}

bool TerrainType::is_permanent_wall() const
{
    return this->flags.has_all_of({ TerrainCharacteristics::WALL, TerrainCharacteristics::PERMANENT });
}

bool TerrainType::can_damage_player() const
{
    return this->flags.has_any_of({
               TerrainCharacteristics::LAVA,
               TerrainCharacteristics::COLD_PUDDLE,
               TerrainCharacteristics::ELEC_PUDDLE,
               TerrainCharacteristics::ACID_PUDDLE,
               TerrainCharacteristics::POISON_PUDDLE,
           }) ||
           this->flags.has_all_of({ TerrainCharacteristics::WATER, TerrainCharacteristics::DEEP });
}

/*!
 * @brief ドアやカーテンが開いているかを調べる
 * @return 閉じる機能の有無
 * @details 上流でダンジョン側の判定と併せて判定すること
 */
bool TerrainType::is_open() const
{
    return this->flags.has(TerrainCharacteristics::CLOSE);
}

/*!
 * @brief 閉じたドアであるかを調べる
 * @return 閉じたドアか否か
 */
bool TerrainType::is_closed_door() const
{
    return (this->flags.has(TerrainCharacteristics::OPEN) || this->flags.has(TerrainCharacteristics::BASH)) &&
           this->flags.has_not(TerrainCharacteristics::MOVE);
}

bool TerrainType::has(TerrainCharacteristics tc) const
{
    return this->flags.has(tc);
}

bool TerrainType::init_trap_type(TrapType type)
{
    if (this->flags.has_not(TerrainCharacteristics::TRAP)) {
        return false;
    }

    this->trap_type = type;
    return true;
}

bool TerrainType::init_pattern_tile_type(PatternTileType type)
{
    if (this->flags.has_not(TerrainCharacteristics::PATTERN)) {
        return false;
    }

    this->pattern_tile_type = type;
    return true;
}

bool TerrainType::init_store_sale_type(StoreSaleType type)
{
    if (this->flags.has_not(TerrainCharacteristics::STORE)) {
        return false;
    }

    this->store_sale_type = type;
    return true;
}

bool TerrainType::init_building_type(BuildingType type)
{
    if (this->flags.has_not(TerrainCharacteristics::BLDG)) {
        return false;
    }

    this->building_type = type;
    return true;
}

bool TerrainType::init_conversion_type(TerrainConversionType type, int index)
{
    if (this->flags.has_not(TerrainCharacteristics::CONVERT)) {
        return false;
    }

    if (type == TerrainConversionType::STREAM) {
        if (index < 0) {
            return false;
        }

        this->stream_index = index;
    }
    this->conversion_type = type;

    return true;
}

/*!
 * @brief 地形のライティング状況をリセットする
 * @param is_config 設定値ならばtrue、定義値ならばfalse (定義値が入るのは初期化時のみ)
 */
void TerrainType::reset_lighting(bool is_config)
{
    auto &symbols = is_config ? this->symbol_configs : this->symbol_definitions;
    if (symbols[F_LIT_STANDARD].is_ascii_graphics()) {
        this->reset_lighting_ascii(symbols);
        return;
    }

    this->reset_lighting_graphics(symbols);
}

void TerrainType::reset_lighting_ascii(TerrainSymbols &symbols)
{
    const auto color_standard = symbols[F_LIT_STANDARD].color;
    const auto character_standard = symbols[F_LIT_STANDARD].character;
    symbols[F_LIT_LITE].color = lighting_colours[color_standard & 0x0f][0];
    symbols[F_LIT_DARK].color = lighting_colours[color_standard & 0x0f][1];
    for (int i = F_LIT_NS_BEGIN; i < F_LIT_MAX; i++) {
        symbols[i].character = character_standard;
    }
}

void TerrainType::reset_lighting_graphics(TerrainSymbols &symbols)
{
    const auto color_standard = symbols[F_LIT_STANDARD].color;
    const auto character_standard = symbols[F_LIT_STANDARD].character;
    for (auto i = F_LIT_NS_BEGIN; i < F_LIT_MAX; i++) {
        symbols[i].color = color_standard;
    }

    symbols[F_LIT_LITE].character = character_standard + 2;
    symbols[F_LIT_DARK].character = character_standard + 1;
}
