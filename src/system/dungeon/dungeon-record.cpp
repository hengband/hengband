/*!
 * @brief ダンジョンに関するプレイ記録実装
 * @author Hourier
 * @date 2024/12/01
 */

#include "system/dungeon/dungeon-record.h"
#include "locale/language-switcher.h"
#include "system/angband-exceptions.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
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

bool DungeonRecord::has_entered() const
{
    return this->max_level.has_value();
}

int DungeonRecord::get_max_level() const
{
    return this->max_level.value_or(0);
}

int DungeonRecord::get_max_max_level() const
{
    return this->max_max_level.value_or(0);
}

void DungeonRecord::set_max_level(int level)
{
    if (!this->max_max_level || (level > this->max_max_level)) {
        this->max_max_level = level;
    }

    this->max_level = level;
}

void DungeonRecord::reset()
{
    this->max_level = std::nullopt;
    this->max_max_level = std::nullopt;
}

DungeonRecords DungeonRecords::instance{};

DungeonRecords::DungeonRecords()
{
    //!< @todo 後でenum class に変える.
    for (auto i = 0; i < 21; i++) {
        this->records.emplace(i, DungeonRecord());
    }
}

DungeonRecords &DungeonRecords::get_instance()
{
    return instance;
}

DungeonRecord &DungeonRecords::get_record(int dungeon_id)
{
    return this->records.at(dungeon_id);
}

const DungeonRecord &DungeonRecords::get_record(int dungeon_id) const
{
    return this->records.at(dungeon_id);
}

std::map<int, DungeonRecord>::iterator DungeonRecords::begin()
{
    return this->records.begin();
}

std::map<int, DungeonRecord>::const_iterator DungeonRecords::begin() const
{
    return this->records.cbegin();
}

std::map<int, DungeonRecord>::iterator DungeonRecords::end()
{
    return this->records.end();
}

std::map<int, DungeonRecord>::const_iterator DungeonRecords::end() const
{
    return this->records.cend();
}

std::map<int, DungeonRecord>::reverse_iterator DungeonRecords::rbegin()
{
    return this->records.rbegin();
}

std::map<int, DungeonRecord>::const_reverse_iterator DungeonRecords::rbegin() const
{
    return this->records.crbegin();
}

std::map<int, DungeonRecord>::reverse_iterator DungeonRecords::rend()
{
    return this->records.rend();
}

std::map<int, DungeonRecord>::const_reverse_iterator DungeonRecords::rend() const
{
    return this->records.crend();
}

size_t DungeonRecords::size() const
{
    return this->records.size();
}

bool DungeonRecords::empty() const
{
    return this->records.empty();
}

void DungeonRecords::reset_all()
{
    for (auto &[_, record] : this->records) {
        record.reset();
    }
}

int DungeonRecords::find_max_level() const
{
    const auto &dungeons = DungeonList::get_instance();
    auto max_level = 0;
    for (const auto &[dungeon_id, record] : this->records) {
        const auto max_level_each = record.get_max_level();
        if (max_level_each < dungeons.get_dungeon(dungeon_id).mindepth) {
            continue;
        }

        if (max_level < max_level_each) {
            max_level = max_level_each;
        }
    }

    return max_level;
}

std::vector<std::string> DungeonRecords::build_known_dungeons(DungeonMessageFormat dmf) const
{
    const auto fmt = get_dungeon_format(dmf);
    const auto &dungeons = DungeonList::get_instance();
    std::vector<std::string> recall_dungeons;
    auto num_entered = 0;
    for (const auto &[dungeon_id, record] : this->records) {
        if (!record.has_entered()) {
            continue;
        }

        const auto &dungeon = dungeons.get_dungeon(dungeon_id);
        if (!dungeon.is_dungeon() || !dungeon.maxdepth) {
            continue;
        }

        const auto max_level = record.get_max_level();
        const auto is_dungeon_conquered = dungeon.is_conquered();
        const auto is_conquered = is_dungeon_conquered || (max_level == dungeon.maxdepth);
        std::string message;
        switch (dmf) {
        case DungeonMessageFormat::DUMP:
        case DungeonMessageFormat::KNOWLEDGE:
            message = format(fmt.data(), is_conquered ? '!' : ' ', dungeon.name.data(), max_level);
            break;
        case DungeonMessageFormat::RECALL:
            message = format(fmt.data(), static_cast<char>('a' + num_entered), is_conquered ? '!' : ' ', dungeon.name.data(), max_level);
            num_entered++;
            break;
        default:
            THROW_EXCEPTION(std::logic_error, format("Invalid dungeon message format is specified! %d", enum2i(dmf)));
        }

        recall_dungeons.push_back(message);
    }

    return recall_dungeons;
}
