#include "artifact/random-art-effects.h"
#include "info-reader/baseitem-reader.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "load/load-util.h"
#include "object-enchant/tr-types.h"
#include "save/item-writer.h"
#include "save/save-util.h"
#include "system/baseitem/baseitem-allocation.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/baseitem/baseitem-record.h"
#include "system/baseitem/baseitem-records.h"
#include "test/info-reader/scoped-reader-state.h"
#include "util/dice.h"
#include "util/finalizer.h"
#include <algorithm>
#include <cstdio>
#include <doctest/doctest.h>
#include <limits>
#include <memory>
#include <nlohmann/json.hpp>
#include <stdexcept>
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

class BaseitemRecordsGuard {
public:
    BaseitemRecordsGuard()
    {
        auto &records = BaseitemRecords::get_instance();
        for (auto &record : records) {
            saved.push_back(std::move(record));
        }
        records.initialize(0);
    }

    BaseitemRecordsGuard(const BaseitemRecordsGuard &) = delete;
    BaseitemRecordsGuard &operator=(const BaseitemRecordsGuard &) = delete;

    ~BaseitemRecordsGuard()
    {
        auto &records = BaseitemRecords::get_instance();
        records.initialize(saved.size());
        auto target = records.begin();
        for (auto &record : saved) {
            std::destroy_at(std::addressof(*target));
            std::construct_at(std::addressof(*target), std::move(record));
            ++target;
        }
    }

private:
    std::vector<BaseitemRecord> saved;
};

class BaseitemAllocationGuard {
public:
    BaseitemAllocationGuard()
    {
        auto &table = BaseitemAllocationTable::get_instance();
        saved.assign(table.begin(), table.end());
    }

    BaseitemAllocationGuard(const BaseitemAllocationGuard &) = delete;
    BaseitemAllocationGuard &operator=(const BaseitemAllocationGuard &) = delete;

    ~BaseitemAllocationGuard()
    {
        auto &table = BaseitemAllocationTable::get_instance();
        table.resize(saved.size());
        std::copy(saved.begin(), saved.end(), table.begin());
    }

private:
    std::vector<BaseitemAllocationEntry> saved;
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

TEST_CASE("BaseitemReader validates full-width integer IDs before narrowing or publishing")
{
    BaseitemStateGuard guard;
    auto &items = BaseitemList::get_instance();
    const auto original = make_baseitem();
    REQUIRE(BaseitemReader(original).read() == PARSE_ERROR_NONE);
    const auto *existing = std::addressof(items.get_baseitem(1));
    const auto invalid_ids = std::vector<nlohmann::json>{
        32768,
        65537,
        std::numeric_limits<int>::max(),
        std::numeric_limits<nlohmann::json::number_integer_t>::max(),
        nlohmann::json::number_integer_t{ 4294967297LL },
        nlohmann::json::number_integer_t{ -4294967295LL },
        std::numeric_limits<nlohmann::json::number_integer_t>::min(),
        -1,
        nlohmann::json::number_unsigned_t{ 32768 },
        nlohmann::json::number_unsigned_t{ 4294967297ULL },
        std::numeric_limits<nlohmann::json::number_unsigned_t>::max(),
    };
    for (const auto &id : invalid_ids) {
        CAPTURE(id);
        auto data = make_baseitem();
        data["id"] = id;
        data["cost"] = 91;
        error_idx = -1;
        const auto is_negative = !id.is_number_unsigned() && id.get<nlohmann::json::number_integer_t>() < 0;
        const auto expected = is_negative ? PARSE_ERROR_NON_SEQUENTIAL_RECORDS : PARSE_ERROR_OUT_OF_BOUNDS;
        CHECK(BaseitemReader(data).read() == expected);
        REQUIRE(items.size() == 2);
        CHECK(std::addressof(items.get_baseitem(1)) == existing);
        CHECK(existing->name == "Test");
        CHECK(existing->cost == 4);
        CHECK(error_idx == -1);
    }
}

TEST_CASE("BaseitemReader preserves missing and wrong-type ID errors without publishing")
{
    BaseitemStateGuard guard;
    auto &items = BaseitemList::get_instance();
    for (const auto &id : { nlohmann::json(), nlohmann::json("1"), nlohmann::json(1.0), nlohmann::json(true), nlohmann::json::array(), nlohmann::json::object() }) {
        CAPTURE(id);
        auto data = make_baseitem();
        data["id"] = id;
        const auto expected = id.is_null() ? PARSE_ERROR_TOO_FEW_ARGUMENTS : PARSE_ERROR_INVALID_TYPE;
        CHECK(BaseitemReader(data).read() == expected);
        CHECK(items.empty());
        CHECK(error_idx == -1);
    }
    auto data = make_baseitem();
    data.erase("id");
    CHECK(BaseitemReader(data).read() == PARSE_ERROR_TOO_FEW_ARGUMENTS);
    CHECK(items.empty());
    CHECK(error_idx == -1);
}

TEST_CASE("BaseitemReader accepts signed and unsigned zero IDs")
{
    for (const auto &id : { nlohmann::json(0), nlohmann::json(nlohmann::json::number_unsigned_t{ 0 }) }) {
        CAPTURE(id);
        BaseitemStateGuard guard;
        auto data = make_baseitem();
        data["id"] = id;
        REQUIRE(BaseitemReader(data).read() == PARSE_ERROR_NONE);
        auto &items = BaseitemList::get_instance();
        REQUIRE(items.size() == 1);
        CHECK(items.get_baseitem(0).name == "Test");
        CHECK(error_idx == 0);
    }
}

TEST_CASE("BaseitemReader preserves the short ID upper bound independently of the schema cap")
{
    const auto ids = std::vector<nlohmann::json>{
        9999,
        10000,
        32767,
        nlohmann::json::number_unsigned_t{ 32767 },
    };
    for (const auto &id : ids) {
        CAPTURE(id);
        BaseitemStateGuard guard;
        auto data = make_baseitem();
        data["id"] = id;
        // Prove the ID passed validation without allocating 32768 definitions.
        data["name"] = 42;
        CHECK(BaseitemReader(data).read() == PARSE_ERROR_INVALID_TYPE);
        CHECK(error_idx == id.get<int>());
        CHECK(BaseitemList::get_instance().empty());
    }
}

TEST_CASE("Baseitem ID 32767 remains accessible and saveable with 32768 entries")
{
    BaseitemStateGuard baseitems_guard;
    BaseitemRecordsGuard records_guard;
    error_idx = -1;
    const auto data = make_baseitem(32767);
    REQUIRE(BaseitemReader(data).read() == PARSE_ERROR_NONE);

    auto &baseitems = BaseitemList::get_instance();
    REQUIRE(baseitems.size() == 32768);
    CHECK(baseitems.is_valid(32767));
    CHECK_FALSE(baseitems.is_valid(32766));
    CHECK(baseitems.get_baseitem(32767).cost == 4);
    CHECK_THROWS_AS(baseitems.get_baseitem(-1), std::logic_error);

    auto &records = BaseitemRecords::get_instance();
    records.initialize(baseitems.size());
    REQUIRE(records.size() == 32768);
    records.get_record(32767).mark_awareness(true);
    records.get_record(32767).mark_trial(true);
    CHECK(records.get_record(32767).is_aware());
    CHECK_THROWS_AS(records.get_record(-1), std::logic_error);

    auto *file = std::tmpfile();
    REQUIRE(file != nullptr);
    const auto close_file = util::make_finalizer([file] { std::fclose(file); });
    const auto restore_io = util::make_finalizer([writer = saving_savefile, reader = loading_savefile,
                                                     write_xor = save_xor_byte, read_xor = load_xor_byte,
                                                     write_v = v_stamp, write_x = x_stamp, read_v = v_check, read_x = x_check] {
        saving_savefile = writer;
        loading_savefile = reader;
        save_xor_byte = write_xor;
        load_xor_byte = read_xor;
        v_stamp = write_v;
        x_stamp = write_x;
        v_check = read_v;
        x_check = read_x;
    });
    saving_savefile = file;
    save_xor_byte = 0;
    v_stamp = x_stamp = 0;
    wr_baseitem_records();
    wr_u16b(0x1234);

    std::rewind(file);
    loading_savefile = file;
    load_xor_byte = 0;
    v_check = x_check = 0;
    CHECK(rd_u16b() == 32768);
    CHECK(rd_byte() == 0);
    strip_bytes(32766);
    CHECK(rd_byte() == 0x03);
    CHECK(rd_u16b() == 0x1234);
}

TEST_CASE("Baseitem allocation counts remain in range beyond 32767 entries")
{
    BaseitemStateGuard baseitems_guard;
    BaseitemAllocationGuard allocation_guard;
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

TEST_CASE("BaseitemReader preserves ordering precedence and rejects negative indices")
{
    BaseitemStateGuard guard;
    auto data = make_baseitem();
    auto expected = PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
    auto previous = 1;
    SUBCASE("duplicate")
    {
        data["id"] = 1;
    }
    SUBCASE("descending unsigned zero")
    {
        data["id"] = nlohmann::json::number_unsigned_t{ 0 };
    }
    SUBCASE("out-of-range ID below the previous ID still fails ordering first")
    {
        data["id"] = nlohmann::json::number_unsigned_t{ 32768 };
        previous = 40000;
    }
    SUBCASE("negative ID above an abnormal negative previous ID cannot index the list")
    {
        data["id"] = -1;
        previous = -100;
        expected = PARSE_ERROR_OUT_OF_BOUNDS;
    }
    error_idx = previous;
    CHECK(BaseitemReader(data).read() == expected);
    CHECK(error_idx == previous);
    CHECK(BaseitemList::get_instance().empty());
}
