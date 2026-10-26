#include "system/baseitem/baseitem-allocation.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "test/scoped-vector-wrapper.h"
#include <doctest/doctest.h>

TEST_CASE("Baseitem allocation counts remain in range beyond 32767 entries")
{
    test::ScopedVectorWrapper baseitems_guard(BaseitemList::get_instance());
    test::ScopedVectorWrapper allocation_guard(BaseitemAllocationTable::get_instance());
    auto &baseitems = BaseitemList::get_instance();
    baseitems.resize(32768);
    for (int index = 1; index <= 8192; index++) {
        auto &baseitem = baseitems.get_baseitem(static_cast<short>(index));
        baseitem.name = "test";
        for (int level = 0; level < 4; level++) {
            baseitem.alloc_tables[level] = { level, 1 };
        }
    }
    auto &last = baseitems.get_baseitem(32767);
    last.name = "test";
    last.alloc_tables[0] = { 4, 1 };

    auto &table = BaseitemAllocationTable::get_instance();
    table.initialize();
    REQUIRE(table.size() == 32769);
    CHECK(table.get_entry(32768).index == 32767);
}
