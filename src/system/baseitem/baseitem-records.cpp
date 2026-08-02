#include "system/baseitem/baseitem-records.h"
#include "system/angband-exceptions.h"
#include "system/baseitem/baseitem-record.h"
#include <fmt/format.h>

BaseitemRecords BaseitemRecords::instance{};

BaseitemRecords::~BaseitemRecords() = default;

BaseitemRecords &BaseitemRecords::get_instance()
{
    return instance;
}

void BaseitemRecords::initialize(size_t size)
{
    this->records.clear();
    this->records.resize(size);
    this->records.shrink_to_fit();
}

BaseitemRecord &BaseitemRecords::get_record(short bi_id)
{
    this->validate(bi_id);
    return this->records[bi_id];
}

const BaseitemRecord &BaseitemRecords::get_record(short bi_id) const
{
    this->validate(bi_id);
    return this->records.at(bi_id);
}

/*!
 * @brief ベースアイテムの鑑定済みフラグをリセットする
 * @details 不具合対策で0からリセットする(セーブは0から)
 */
void BaseitemRecords::reset_identification_flags()
{
    for (auto &record : this->records) {
        record.mark_trial(false);
        record.mark_awareness(false);
    }
}

void BaseitemRecords::validate(short bi_id) const
{
    if ((bi_id < 0) || (bi_id >= static_cast<short>(this->size()))) {
        THROW_EXCEPTION(std::logic_error, fmt::format("Invalid Baseitem ID: {}", bi_id));
    }
}
