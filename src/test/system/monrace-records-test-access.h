#pragma once

#include "system/monrace/monrace-records.h"

namespace test {
class MonraceRecordsTestAccess {
public:
    MonraceRecordsTestAccess()
        : records(MonraceRecords::get_instance())
    {
        records.records.swap(saved_records);
        records.initialize(3);
    }

    MonraceRecordsTestAccess(const MonraceRecordsTestAccess &) = delete;
    MonraceRecordsTestAccess &operator=(const MonraceRecordsTestAccess &) = delete;

    ~MonraceRecordsTestAccess()
    {
        records.records.swap(saved_records);
    }

private:
    MonraceRecords &records;
    std::vector<std::shared_ptr<MonraceRecord>> saved_records;
};
}
