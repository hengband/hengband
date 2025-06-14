/*!
 * @brief ダンジョンに関するプレイ記録実装
 * @author Hourier
 * @date 2024/12/01
 */

#include "system/dungeon/dungeon-record.h"
#include "locale/language-switcher.h"
#include "system/angband-exceptions.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "term/z-form.h"
#include "term/z-rand.h"
#include "util/enum-converter.h"
#include "util/enum-range.h"

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
    if (level <= 0) {
        THROW_EXCEPTION(std::logic_error, format("Invalid dungeon level: %d", level));
    }

    this->max_level = level;
    if (!this->max_max_level || (level > this->max_max_level)) {
        this->max_max_level = level;
    }
}

void DungeonRecord::reset()
{
    this->max_level = tl::nullopt;
    this->max_max_level = tl::nullopt;
}

DungeonRecords DungeonRecords::instance{};

DungeonRecords::DungeonRecords()
{
    for (auto dungeon_id : DUNGEON_IDS) {
        this->records.emplace(dungeon_id, std::make_shared<DungeonRecord>());
    }
}

DungeonRecords &DungeonRecords::get_instance()
{
    return instance;
}

DungeonRecord &DungeonRecords::get_record(DungeonId dungeon_id)
{
    return *this->records.at(dungeon_id);
}

const DungeonRecord &DungeonRecords::get_record(DungeonId dungeon_id) const
{
    return *this->records.at(dungeon_id);
}

std::shared_ptr<DungeonRecord> DungeonRecords::get_record_shared(DungeonId dungeon_id)
{
    return this->records.at(dungeon_id);
}

std::shared_ptr<const DungeonRecord> DungeonRecords::get_record_shared(DungeonId dungeon_id) const
{
    return this->records.at(dungeon_id);
}

void DungeonRecords::reset_all()
{
    for (auto &[_, record] : this->records) {
        record->reset();
    }
}

std::vector<DungeonId> DungeonRecords::collect_entered_dungeon_ids() const
{
    std::vector<DungeonId> ids;
    for (const auto &[dungeon_id, record] : this->records) {
        if (record->has_entered()) {
            ids.push_back(dungeon_id);
        }
    }

    return ids;
}
