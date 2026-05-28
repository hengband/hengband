#include "system/monrace/monrace-record.h"
#include <limits>

void MonraceRecord::reset_all()
{
    this->seen_count = 0;
}

bool MonraceRecord::has_been_seen() const
{
    return this->seen_count > 0;
}

void MonraceRecord::increment_seen_count()
{
    if (this->seen_count < std::numeric_limits<short>::max()) {
        this->seen_count++;
    }
}

short MonraceRecord::get_seen_count() const
{
    return this->seen_count;
}

void MonraceRecord::set_seen_count(short count)
{
    this->seen_count = count;
}
