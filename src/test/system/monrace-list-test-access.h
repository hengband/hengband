#pragma once

#include "system/monrace/monrace-list.h"
#include "test/info-reader/scoped-reader-state.h"

namespace test {
class MonraceListTestAccess {
public:
    MonraceListTestAccess()
        : monraces(MonraceList::get_instance())
        , reader_state(0)
    {
        monraces.monraces.swap(saved_monraces);
    }

    MonraceListTestAccess(const MonraceListTestAccess &) = delete;
    MonraceListTestAccess &operator=(const MonraceListTestAccess &) = delete;

    ~MonraceListTestAccess()
    {
        monraces.monraces.swap(saved_monraces);
    }

private:
    MonraceList &monraces;
    MonraceList::Container saved_monraces;
    ScopedReaderState reader_state;
};
}
