/*!
 * @brief ダンジョンの定義と記録に関するサービス処理実装
 * @author Hourier
 * @date 2024/12/28
 */

#include "system/services/dungeon-service.h"
#include "system/angband-exceptions.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/dungeon/dungeon-record.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "term/z-rand.h"
#include "util/enum-converter.h"

namespace {
std::string get_dungeon_format(DungeonMessageFormat dmf)
{
    switch (dmf) {
    case DungeonMessageFormat::DUMP:
        return _("   %c%-12s: %3d 階\n", "   %c%-16s: level %3d\n");
    case DungeonMessageFormat::KNOWLEDGE:
        return _("%c%-12s :  %3d 階\n", "%c%-16s :  level %3d\n");
    case DungeonMessageFormat::RECALL:
        return _("      %c) %c%-12s : 最大 %d 階", "      %c) %c%-16s : Max level %d");
    default:
        THROW_EXCEPTION(std::logic_error, format("Invalid dungeon message format is specified! %d", enum2i(dmf)));
    }
}
}

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

std::vector<std::string> DungeonService::build_known_dungeons(DungeonMessageFormat dmf)
{
    const auto fmt = get_dungeon_format(dmf);
    const auto &dungeons = DungeonList::get_instance();
    std::vector<std::string> known_dungeons;
    auto num_entered = 0;
    for (const auto &[dungeon_id, record] : DungeonRecords::get_instance()) {
        if (!record->has_entered()) {
            continue;
        }

        const auto max_level = record->get_max_level();
        const auto &dungeon = dungeons.get_dungeon(dungeon_id);
        const auto is_dungeon_conquered = dungeon.is_conquered();
        const auto is_conquered = is_dungeon_conquered || (max_level == dungeon.maxdepth);
        std::string known_dungeon;
        switch (dmf) {
        case DungeonMessageFormat::DUMP:
        case DungeonMessageFormat::KNOWLEDGE:
            known_dungeon = format(fmt.data(), is_conquered ? '!' : ' ', dungeon.name.data(), max_level);
            break;
        case DungeonMessageFormat::RECALL:
            known_dungeon = format(fmt.data(), static_cast<char>('a' + num_entered), is_conquered ? '!' : ' ', dungeon.name.data(), max_level);
            num_entered++;
            break;
        default:
            THROW_EXCEPTION(std::logic_error, format("Invalid dungeon message format is specified! %d", enum2i(dmf)));
        }

        known_dungeons.push_back(known_dungeon);
    }

    return known_dungeons;
}
