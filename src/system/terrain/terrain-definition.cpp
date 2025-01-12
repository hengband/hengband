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
    { TerrainCharacteristics::HURT_ROCK, { TerrainAction::DESTROY, TerrainAction::CRASH_GLASS } },
    { TerrainCharacteristics::HURT_DISI, { TerrainAction::DESTROY, TerrainAction::NO_DROP, TerrainAction::CRASH_GLASS } },
};
}

TerrainType::TerrainType()
    : symbol_definitions(DEFAULT_SYMBOLS)
    , symbol_configs(DEFAULT_SYMBOLS)
{
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

/*!
 * @brief ドアやカーテンが開いているかを調べる
 * @return 開いているか否か
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

void TerrainType::reset_lighting_ascii(std::map<int, DisplaySymbol> &symbols)
{
    const auto color_standard = symbols[F_LIT_STANDARD].color;
    const auto character_standard = symbols[F_LIT_STANDARD].character;
    symbols[F_LIT_LITE].color = lighting_colours[color_standard & 0x0f][0];
    symbols[F_LIT_DARK].color = lighting_colours[color_standard & 0x0f][1];
    for (int i = F_LIT_NS_BEGIN; i < F_LIT_MAX; i++) {
        symbols[i].character = character_standard;
    }
}

void TerrainType::reset_lighting_graphics(std::map<int, DisplaySymbol> &symbols)
{
    const auto color_standard = symbols[F_LIT_STANDARD].color;
    const auto character_standard = symbols[F_LIT_STANDARD].character;
    for (auto i = F_LIT_NS_BEGIN; i < F_LIT_MAX; i++) {
        symbols[i].color = color_standard;
    }

    symbols[F_LIT_LITE].character = character_standard + 2;
    symbols[F_LIT_DARK].character = character_standard + 1;
}
