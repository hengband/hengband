#include "system/terrain/terrain-actions-table.h"
#include "system/enums/terrain/terrain-characteristics.h"
#include "util/flag-group.h"
#include <algorithm>
#include <unordered_map>

namespace {
/*!
 * @brief 地形状態フラグテーブル
 */
const std::unordered_map<TerrainCharacteristics, EnumClassFlagGroup<TerrainAction>> TERRAIN_ACTIONS_TABLE = {
    { TerrainCharacteristics::BASH, { TerrainAction::CRASH_GLASS } },
    { TerrainCharacteristics::DISARM, { TerrainAction::DESTROY } },
    { TerrainCharacteristics::TUNNEL, { TerrainAction::DESTROY, TerrainAction::CRASH_GLASS } },
    { TerrainCharacteristics::HURT_ROCK, { TerrainAction::DESTROY, TerrainAction::CRASH_GLASS } },
    { TerrainCharacteristics::HURT_DISI, { TerrainAction::DESTROY, TerrainAction::NO_DROP, TerrainAction::CRASH_GLASS } },
};
}

bool TerrainActionFlagChecker::has(TerrainCharacteristics tc, TerrainAction taf)
{
    static const auto begin = TERRAIN_ACTIONS_TABLE.begin();
    static const auto end = TERRAIN_ACTIONS_TABLE.end();
    const auto action = std::find_if(begin, end, [tc](const auto &x) { return x.first == tc; });
    if (action == end) {
        return false;
    }

    return action->second.has(taf);
}
