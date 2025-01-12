/*!
 * @brief ダンジョンとモンスター種族定義の両方に依存するサービス実装
 * @author Hourier
 * @date 2025/01/11
 */

#include "system/services/dungeon-monrace-service.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"

/*!
 * @brief モンスターがダンジョンに出現するかどうかを返す
 * @param dungeon_id 出現させようとするダンジョンID
 * @param monrace_id モンスター種族ID
 * @return ダンジョンに出現するならばTRUEを返す
 * @details
 * 荒野限定(WILD_ONLY)の場合、荒野の山(WILD_MOUNTAIN)に出るモンスターにのみ、ダンジョンの山にも出現を許可する。
 * その他の場合、山と火山以外のダンジョンでは全てのモンスターに出現を許可する。
 * ダンジョンが山の場合は、荒野の山(WILD_MOUNTAIN)に出ない水棲動物(AQUATIC)は許可しない。
 * ダンジョンが火山の場合は、荒野の火山(WILD_VOLCANO)に出ない水棲動物(AQUATIC)は許可しない。
 */
bool DungeonMonraceService::is_suitable_for_dungeon(DungeonId dungeon_id, MonraceId monrace_id)
{
    const auto &dungeon = DungeonList::get_instance().get_dungeon(dungeon_id);
    const auto &monrace = MonraceList::get_instance().get_monrace(monrace_id);
    if (monrace.wilderness_flags.has(MonsterWildernessType::WILD_ONLY)) {
        return dungeon.mon_wilderness_flags.has(MonsterWildernessType::WILD_MOUNTAIN) && monrace.wilderness_flags.has(MonsterWildernessType::WILD_MOUNTAIN);
    }

    const auto land = monrace.feature_flags.has_not(MonsterFeatureType::AQUATIC);
    auto is_mountain_monster = dungeon.mon_wilderness_flags.has_none_of({ MonsterWildernessType::WILD_MOUNTAIN, MonsterWildernessType::WILD_VOLCANO });
    is_mountain_monster |= dungeon.mon_wilderness_flags.has(MonsterWildernessType::WILD_MOUNTAIN) && (land || monrace.wilderness_flags.has(MonsterWildernessType::WILD_MOUNTAIN));
    is_mountain_monster |= dungeon.mon_wilderness_flags.has(MonsterWildernessType::WILD_VOLCANO) && (land || monrace.wilderness_flags.has(MonsterWildernessType::WILD_VOLCANO));
    return is_mountain_monster;
}
