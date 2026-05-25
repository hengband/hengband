#pragma once

/*!
 * @brief モンスター種族に関するプレイ中の動的な記録を集合論的に取り扱うクラス
 * @author Hourier
 * @date 2026/05/26
 */

#include "system/enums/monrace/monrace-id.h"
#include "util/abstract-map-wrapper.h"
#include <memory>

class MonraceRecord;
class MonraceRecords : public util::AbstractMapWrapper<MonraceId, std::shared_ptr<MonraceRecord>> {
public:
    MonraceRecords(MonraceRecords &&) = delete;
    MonraceRecords(const MonraceRecords &) = delete;
    MonraceRecords &operator=(const MonraceRecords &) = delete;
    MonraceRecords &operator=(MonraceRecords &&) = delete;
    static MonraceRecords &get_instance();

    void initialize(size_t size);
    std::shared_ptr<const MonraceRecord> get_record(MonraceId monrace_id) const;

    bool has_been_seen(MonraceId monrace_id) const;
    void increment_seen_count(MonraceId monrace_id);
    short get_seen_count(MonraceId monrace_id) const; //!< セーブ用.
    void set_seen_count(MonraceId monrace_id, short count); //!< ロード用.

private:
    MonraceRecords() = default;

    static MonraceRecords instance;
    std::map<MonraceId, std::shared_ptr<MonraceRecord>> records;

    std::map<MonraceId, std::shared_ptr<MonraceRecord>> &get_inner_container() override
    {
        return this->records;
    }
};
