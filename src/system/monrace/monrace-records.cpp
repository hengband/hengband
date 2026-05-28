#include "system/monrace/monrace-records.h"
#include "system/angband-exceptions.h"
#include "system/monrace/monrace-record.h"
#include "util/enum-converter.h"
#include <fmt/format.h>

MonraceRecords MonraceRecords::instance{};

/*!
 * @brief レコードのmapを初期化する
 *
 * 起動時に一度だけ実行されるが、ドメイン仕様とは無関係に初期化処理は冪等性を担保すべきである.
 * そのため、呼ばれる度に全てをリセットする設計とする.
 */
void MonraceRecords::initialize(size_t size)
{
    if (!this->records.empty()) {
        for (auto &[_, record] : this->records) {
            record->reset_all();
        }

        return;
    }

    for (size_t i = 0; i < size; i++) {
        this->records.emplace(static_cast<MonraceId>(i), std::make_shared<MonraceRecord>());
    }
}

MonraceRecords &MonraceRecords::get_instance()
{
    return instance;
}

std::shared_ptr<const MonraceRecord> MonraceRecords::get_record(MonraceId monrace_id) const
{
    this->validate_monrace_id(monrace_id);
    return this->records.at(monrace_id);
}

bool MonraceRecords::has_been_seen(MonraceId monrace_id) const
{
    this->validate_monrace_id(monrace_id);
    if (monrace_id == MonraceId::PLAYER) {
        return false;
    }

    return this->records.at(monrace_id)->has_been_seen();
}

void MonraceRecords::increment_seen_count(MonraceId monrace_id)
{
    this->validate_monrace_id(monrace_id);
    if (monrace_id == MonraceId::PLAYER) {
        return;
    }

    this->records[monrace_id]->increment_seen_count();
}

short MonraceRecords::get_seen_count(MonraceId monrace_id) const
{
    this->validate_monrace_id(monrace_id);
    if (monrace_id == MonraceId::PLAYER) {
        return 0;
    }

    return this->records.at(monrace_id)->get_seen_count();
}

void MonraceRecords::set_seen_count(MonraceId monrace_id, short count)
{
    this->validate_monrace_id(monrace_id);
    if (monrace_id == MonraceId::PLAYER) {
        return;
    }

    this->records[monrace_id]->set_seen_count(count);
}

void MonraceRecords::validate_monrace_id(MonraceId monrace_id) const
{
    if ((monrace_id < MonraceId::PLAYER) || monrace_id >= i2enum<MonraceId>(this->records.size())) {
        THROW_EXCEPTION(std::out_of_range, fmt::format("Invalid Monrace ID: {}", enum2i(monrace_id)));
    }
}
