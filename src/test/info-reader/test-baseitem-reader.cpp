#include "artifact/random-art-effects.h"
#include "info-reader/baseitem-reader.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "object-enchant/tr-types.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "test/info-reader/scoped-reader-state.h"
#include "util/dice.h"
#include <doctest/doctest.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <utility>
#include <vector>

namespace {
class BaseitemStateGuard {
public:
    BaseitemStateGuard()
    {
        auto &items = BaseitemList::get_instance();
        for (auto &item : items) {
            saved.push_back(std::move(item));
        }
        items.resize(0);
    }

    BaseitemStateGuard(const BaseitemStateGuard &) = delete;
    BaseitemStateGuard &operator=(const BaseitemStateGuard &) = delete;

    ~BaseitemStateGuard()
    {
        auto &items = BaseitemList::get_instance();
        items.resize(saved.size());
        auto target = items.begin();
        for (auto &item : saved) {
            std::destroy_at(std::addressof(*target));
            std::construct_at(std::addressof(*target), std::move(item));
            ++target;
        }
    }

private:
    std::vector<BaseitemDefinition> saved;
    test::ScopedReaderState reader_state;
};

nlohmann::json make_baseitem(int id = 1)
{
    return {
        { "id", id },
        { "name", { { "ja", "Test" }, { "en", "Test" } } },
        { "symbol", { { "character", "!" }, { "color", "White" } } },
        { "itemkind", { { "type_value", 1 }, { "subtype_value", 1 } } },
        { "level", 2 },
        { "weight", 3 },
        { "cost", 4 },
    };
}
}

TEST_CASE("BaseitemReader does not grow the list after a parse error")
{
    BaseitemStateGuard guard;
    auto &items = BaseitemList::get_instance();
    auto data = make_baseitem(7);
    SUBCASE("early name error")
    {
        data["name"] = 42;
        CHECK(BaseitemReader(data).read() == PARSE_ERROR_INVALID_TYPE);
    }
    SUBCASE("late activation error")
    {
        data["activate"] = "UNKNOWN_ACTIVATION";
        CHECK(BaseitemReader(data).read() == PARSE_ERROR_INVALID_FLAG);
    }
    SUBCASE("late flag error after a valid flag")
    {
        data["flags"] = { "STR", "UNKNOWN_FLAG" };
        CHECK(BaseitemReader(data).read() == PARSE_ERROR_INVALID_FLAG);
    }
    CHECK(items.empty());
    CHECK(error_idx == 7);
}

TEST_CASE("BaseitemReader preserves existing slots after a late parse error")
{
    BaseitemStateGuard guard;
    auto &items = BaseitemList::get_instance();
    const auto original = make_baseitem();
    REQUIRE(BaseitemReader(original).read() == PARSE_ERROR_NONE);
    auto &existing = items.get_baseitem(1);
    existing.name = "Existing";
    existing.cost = 91;
    existing.alloc_tables[0] = { 12, 13 };
    existing.act_idx = RandomArtActType::BA_FIRE_4;
    existing.flags.set(TR_ACTIVATE);
    auto data = make_baseitem();
    data["allocations"] = { { { "depth", 4 }, { "rarity", 5 } }, { { "depth", 6 }, { "rarity", "bad" } } };
    SUBCASE("partially parsed allocations")
    {
        error_idx = -1;
        CHECK(BaseitemReader(data).read() == PARSE_ERROR_INVALID_TYPE);
    }
    SUBCASE("flags after allocations and activation")
    {
        data["allocations"] = { { { "depth", 4 }, { "rarity", 5 } } };
        data["activate"] = "LIGHT";
        data["flags"] = { "STR", "UNKNOWN_FLAG" };
        error_idx = -1;
        CHECK(BaseitemReader(data).read() == PARSE_ERROR_INVALID_FLAG);
    }
    REQUIRE(items.size() == 2);
    CHECK(&items.get_baseitem(1) == &existing);
    CHECK(existing.name == "Existing");
    CHECK(existing.cost == 91);
    CHECK(existing.alloc_tables[0].level == 12);
    CHECK(existing.alloc_tables[0].chance == 13);
    CHECK(existing.act_idx == RandomArtActType::BA_FIRE_4);
    CHECK(existing.flags.has(TR_ACTIVATE));
    CHECK_FALSE(existing.flags.has(TR_STR));
    CHECK(error_idx == 1);
}

TEST_CASE("BaseitemReader publishes complete records and preserves omitted optional fields")
{
    BaseitemStateGuard guard;
    auto &items = BaseitemList::get_instance();
    auto data = make_baseitem(3);
    data["activate"] = "LIGHT";
    data["flags"] = { "STR" };
    data["allocations"] = { { { "depth", 4 }, { "rarity", 5 } } };
    REQUIRE(BaseitemReader(data).read() == PARSE_ERROR_NONE);
    REQUIRE(items.size() == 4);
    auto &existing = items.get_baseitem(3);
    CHECK(existing.name == "Test");
    CHECK(existing.cost == 4);
    CHECK(existing.level == 2);
    CHECK(existing.weight == 3);
    CHECK(existing.act_idx == RandomArtActType::LIGHT);
    CHECK(existing.flags.has(TR_STR));
    CHECK(existing.flags.has(TR_ACTIVATE));
    CHECK(existing.alloc_tables[0].level == 4);
    CHECK(existing.alloc_tables[0].chance == 5);
    existing.text = "Flavor";
    existing.flavor_name = "Unknown";
    existing.pval = 6;
    existing.ac = 7;
    existing.to_h = 8;
    existing.to_d = 9;
    existing.to_a = 10;
    existing.damage_dice = Dice(2, 3);
    existing.easy_know = true;
    data = make_baseitem(3);
    data["cost"] = 17;
    error_idx = -1;
    REQUIRE(BaseitemReader(data).read() == PARSE_ERROR_NONE);
    const auto &updated = items.get_baseitem(3);
    CHECK(updated.cost == 17);
    CHECK(updated.text == "Flavor");
    CHECK(updated.flavor_name == "Unknown");
    CHECK(updated.pval == 6);
    CHECK(updated.ac == 7);
    CHECK(updated.to_h == 8);
    CHECK(updated.to_d == 9);
    CHECK(updated.to_a == 10);
    CHECK(updated.damage_dice == Dice(2, 3));
    CHECK(updated.easy_know);
    CHECK(updated.act_idx == RandomArtActType::LIGHT);
    CHECK(updated.flags.has(TR_STR));
    CHECK(updated.flags.has(TR_ACTIVATE));
    CHECK(updated.alloc_tables[0].level == 4);
    CHECK(updated.alloc_tables[0].chance == 5);
    CHECK(error_idx == 3);
    CHECK(BaseitemReader(data).read() == PARSE_ERROR_NON_SEQUENTIAL_RECORDS);
}
