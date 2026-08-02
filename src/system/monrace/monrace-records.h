#pragma once

/*!
 * @brief モンスター種族に関するプレイ中の動的な記録を集合論的に取り扱うリポジトリ
 * @author Hourier
 * @date 2026/05/26
 */

#include "system/enums/monrace/monrace-id.h"
#include <memory>
#include <vector>

class MonraceRecord;
class MonraceRecords {
public:
    MonraceRecords(MonraceRecords &&) = delete;
    MonraceRecords(const MonraceRecords &) = delete;
    MonraceRecords &operator=(const MonraceRecords &) = delete;
    MonraceRecords &operator=(MonraceRecords &&) = delete;
    static MonraceRecords &get_instance();

    void initialize(size_t size);
    std::shared_ptr<MonraceRecord> get_record(MonraceId monrace_id);
    std::shared_ptr<const MonraceRecord> get_record(MonraceId monrace_id) const;

    bool has_been_seen(MonraceId monrace_id) const;
    void increment_seen_count(MonraceId monrace_id);
    short get_seen_count(MonraceId monrace_id) const; //!< セーブ用.
    void set_seen_count(MonraceId monrace_id, short count); //!< ロード用.

private:
    MonraceRecords() = default;

    static MonraceRecords instance;

    std::vector<std::shared_ptr<MonraceRecord>> records;

    std::shared_ptr<MonraceRecord> &get_record_ref(MonraceId monrace_id);
    const std::shared_ptr<MonraceRecord> &get_record_ref(MonraceId monrace_id) const;
    void validate_monrace_id(MonraceId monrace_id) const;
};
