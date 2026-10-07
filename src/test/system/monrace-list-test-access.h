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
        monraces.monraces_by_id.swap(saved_monraces_by_id);
    }

    MonraceListTestAccess(const MonraceListTestAccess &) = delete;
    MonraceListTestAccess &operator=(const MonraceListTestAccess &) = delete;

    ~MonraceListTestAccess()
    {
        monraces.monraces.swap(saved_monraces);
        monraces.monraces_by_id.swap(saved_monraces_by_id);
    }

private:
    MonraceList &monraces;
    MonraceList::Container saved_monraces;
    decltype(MonraceList::monraces_by_id) saved_monraces_by_id;
    ScopedReaderState reader_state;
};
}
