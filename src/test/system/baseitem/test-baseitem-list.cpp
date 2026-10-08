#include "info-reader/baseitem-reader.h"
#include "info-reader/parse-error-types.h"
#include "object/tval-types.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-key.h"
#include "system/baseitem/baseitem-list.h"
#include "test/info-reader/scoped-reader-state.h"
#include "test/scoped-rng.h"
#include "test/scoped-vector-wrapper.h"
#include <doctest/doctest.h>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {
void set_definition(short id, ItemKindType type, int subtype)
{
    BaseitemDefinition item;
    item.name = "test";
    item.bi_key = { type, subtype };
    BaseitemList::get_instance().replace_baseitem(id, std::move(item));
}

void check_lookup(const BaseitemList &items, short id, ItemKindType type, int subtype)
{
    const auto restore_rng = test::scoped_rng();
    CHECK(items.lookup_baseitem_id({ type, subtype }) == id);
    CHECK(items.lookup_baseitem_id({ type }) == id);
}
}

TEST_CASE("Baseitem lookup caches follow isolated definitions and their restoration")
{
    auto &items = BaseitemList::get_instance();
    test::ScopedVectorWrapper original_guard(items);
    items.resize(2);
    set_definition(1, ItemKindType::SWORD, 10);
    check_lookup(items, 1, ItemKindType::SWORD, 10);
    CHECK(items.collect_valid_bi_ids() == std::vector<short>{ 1 });
    {
        test::ScopedVectorWrapper temporary_guard(items);
        items.resize(32768);
        set_definition(32767, ItemKindType::POTION, 20);
        check_lookup(items, 32767, ItemKindType::POTION, 20);
        CHECK(items.collect_valid_bi_ids() == std::vector<short>{ 32767 });
        CHECK_THROWS_AS(items.lookup_baseitem_id({ ItemKindType::SWORD, 10 }), std::runtime_error);
    }
    CHECK(items.size() == 2);
    check_lookup(items, 1, ItemKindType::SWORD, 10);
    CHECK(items.collect_valid_bi_ids() == std::vector<short>{ 1 });
    CHECK_THROWS_AS(items.lookup_baseitem_id({ ItemKindType::POTION, 20 }), std::runtime_error);
}

TEST_CASE("Baseitem caches rebuild after resizing and definition replacement")
{
    auto &items = BaseitemList::get_instance();
    test::ScopedVectorWrapper guard(items);
    CHECK(items.collect_valid_bi_ids().empty());
    items.resize(2);
    set_definition(1, ItemKindType::SWORD, 10);
    check_lookup(items, 1, ItemKindType::SWORD, 10);
    items.resize(3);
    set_definition(2, ItemKindType::POTION, 20);
    CHECK((items.collect_valid_bi_ids() == std::vector<short>{ 1, 2 }));
    check_lookup(items, 2, ItemKindType::POTION, 20);
    set_definition(1, ItemKindType::RING, 30);
    check_lookup(items, 1, ItemKindType::RING, 30);
    CHECK_THROWS_AS(items.lookup_baseitem_id({ ItemKindType::SWORD, 10 }), std::runtime_error);
    CHECK_THROWS_AS(items.lookup_baseitem_id({ ItemKindType::SWORD }), std::logic_error);
    items.resize(1);
    CHECK(items.collect_valid_bi_ids().empty());
    CHECK_THROWS_AS(items.lookup_baseitem_id({ ItemKindType::RING, 30 }), std::runtime_error);
}

TEST_CASE("BaseitemReader publication refreshes cached keys without changing the count")
{
    auto &items = BaseitemList::get_instance();
    test::ScopedVectorWrapper guard(items);
    items.resize(2);
    set_definition(1, ItemKindType::SWORD, 10);
    check_lookup(items, 1, ItemKindType::SWORD, 10);
    test::ScopedReaderState reader_state;
    const nlohmann::json data = {
        { "id", 1 },
        { "name", { { "ja", "test" }, { "en", "test" } } },
        { "symbol", { { "character", "!" }, { "color", "White" } } },
        { "itemkind", { { "type_value", static_cast<int>(ItemKindType::RING) }, { "subtype_value", 30 } } },
        { "level", 2 },
        { "weight", 3 },
        { "cost", 4 },
    };
    REQUIRE(BaseitemReader(data).read() == PARSE_ERROR_NONE);
    CHECK(items.size() == 2);
    check_lookup(items, 1, ItemKindType::RING, 30);
    CHECK_THROWS_AS(items.lookup_baseitem_id({ ItemKindType::SWORD, 10 }), std::runtime_error);
}

TEST_CASE("Baseitem valid ID snapshots survive cache rebuilding during iteration")
{
    auto &items = BaseitemList::get_instance();
    test::ScopedVectorWrapper guard(items);
    items.resize(4);
    for (short id = 1; id <= 3; ++id) {
        set_definition(id, ItemKindType::SWORD, id);
    }
    auto count = 0;
    for (const auto id : items.collect_valid_bi_ids()) {
        set_definition(id, ItemKindType::SWORD, id);
        CHECK(items.lookup_baseitem_id({ ItemKindType::SWORD, id }) == id);
        CHECK(items.collect_valid_bi_ids().size() == 3);
        CHECK(items.lookup_baseitem_id(items.get_baseitem(id).bi_key) == id);
        ++count;
    }
    CHECK(count == 3);
    CHECK((items.collect_valid_bi_ids() == std::vector<short>{ 1, 2, 3 }));
}
