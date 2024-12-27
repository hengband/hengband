/*!
 * @brief ダンジョンの定義と記録に関するサービス処理実装
 * @author Hourier
 * @date 2024/12/28
 */

#include "system/services/dungeon-service.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/dungeon/dungeon-record.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "term/z-rand.h"

int DungeonService::decide_gradiator_level()
{
    const auto max_dungeon_level = find_max_level();
    auto gradiator_level = randint1(std::min(max_dungeon_level, 122)) + 5;
    if (evaluate_percent(60)) {
        const auto random_level = randint1(std::min(max_dungeon_level, 122)) + 5;
        gradiator_level = std::max(random_level, gradiator_level);
    }

    if (evaluate_percent(30)) {
        const auto random_level = randint1(std::min(max_dungeon_level, 122)) + 5;
        gradiator_level = std::max(random_level, gradiator_level);
    }

    return gradiator_level;
}

int DungeonService::find_max_level()
{
    const auto &records = DungeonRecords::get_instance();
    const auto &dungeons = DungeonList::get_instance();
    auto max_level = 0;
    for (auto dungeon_id : DUNGEON_IDS) {
        const auto &record = records.get_record(dungeon_id);
        const auto &dungeon = dungeons.get_dungeon(dungeon_id);
        const auto max_level_each = record.get_max_level();
        if (max_level_each < dungeon.mindepth) {
            continue;
        }

        if (max_level < max_level_each) {
            max_level = max_level_each;
        }
    }

    return max_level;
}

std::optional<std::string> DungeonService::check_first_entrance(DungeonId dungeon_id)
{
    const auto &record = DungeonRecords::get_instance().get_record(dungeon_id);
    if (!record.has_entered()) {
        return std::nullopt;
    }

    return DungeonList::get_instance().get_dungeon(dungeon_id).build_entrance_message();
}
