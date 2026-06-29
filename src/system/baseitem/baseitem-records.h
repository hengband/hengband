#pragma once

#include "util/abstract-vector-wrapper.h"
#include <cstddef>

class BaseitemRecord;
class BaseitemRecords : public util::AbstractVectorWrapper<BaseitemRecord> {
public:
    BaseitemRecords(BaseitemRecords &&) = delete;
    BaseitemRecords(const BaseitemRecords &) = delete;
    BaseitemRecords &operator=(const BaseitemRecords &) = delete;
    BaseitemRecords &operator=(BaseitemRecords &&) = delete;
    ~BaseitemRecords() override;

    static BaseitemRecords &get_instance();
    void initialize(size_t size);

    BaseitemRecord &get_record(short bi_id);
    const BaseitemRecord &get_record(short bi_id) const;

    void reset_identification_flags();

private:
    BaseitemRecords() = default;

    static BaseitemRecords instance;

    std::vector<BaseitemRecord> records;

    void validate(short bi_id) const;

    std::vector<BaseitemRecord> &get_inner_container() override
    {
        return this->records;
    }
};
