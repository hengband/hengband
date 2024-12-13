/*!
 * @brief ダンジョンに関するプレイ記録実装
 * @author Hourier
 * @date 2024/12/01
 */

#include "system/dungeon/dungeon-record.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"

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
