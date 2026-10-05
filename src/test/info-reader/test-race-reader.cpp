#include "info-reader/info-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/race-reader.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monrace/monrace-record.h"
#include "system/monrace/monrace-records.h"
#include "test/scoped-rng.h"
#include "test/system/monrace-list-test-access.h"
#include "test/system/monrace-message-list-test-access.h"
#include "test/system/monrace-records-test-access.h"
#include <cstdint>
#include <doctest/doctest.h>
#include <limits>
#include <nlohmann/json.hpp>
#include <string>
#include <type_traits>

static_assert(!std::is_copy_constructible_v<MonraceDefinition>);
static_assert(!std::is_move_assignable_v<MonraceDefinition>);

namespace {
nlohmann::json make_monrace()
{
    return {
        { "id", 0 },
        { "name", { { "ja", "test" }, { "en", "test" } } },
        { "symbol", { { "character", "p" }, { "color", "White" } } },
        { "speed", 0 },
        { "hit_point", "1d1" },
        { "vision", 1 },
        { "armor_class", 0 },
        { "alertness", 0 },
        { "level", 0 },
        { "rarity", 1 },
        { "exp", 0 },
        { "skill", { { "probability", "1_IN_3" }, { "list", { "SHRIEK" } } } },
    };
}

MonraceDefinition &test_monrace()
{
    return MonraceList::get_instance().get_monrace(static_cast<MonraceId>(0));
}
}

TEST_CASE("RaceReader accepts schema boundary IDs")
{
    test::MonraceListTestAccess guard;
    auto data = make_monrace();
    for (const auto id : { 0, 9999 }) {
        CAPTURE(id);
        data["id"] = id;
        REQUIRE(RaceReader(data).read() == PARSE_ERROR_NONE);
        CHECK(MonraceList::get_instance().get_monrace(static_cast<MonraceId>(id)).idx == static_cast<MonraceId>(id));
        CHECK(error_idx == id);
    }
}

TEST_CASE("RaceReader rejects out-of-range IDs before conversion or registration")
{
    test::MonraceListTestAccess guard;
    auto data = make_monrace();
    for (const auto &id : {
             nlohmann::json(-1), nlohmann::json(10000), nlohmann::json(65536),
             nlohmann::json(int64_t{ 1 } << 32), nlohmann::json((int64_t{ 1 } << 32) + 1),
             nlohmann::json(std::numeric_limits<int64_t>::min()), nlohmann::json(std::numeric_limits<uint64_t>::max()) }) {
        CAPTURE(id);
        data["id"] = id;
        CHECK(RaceReader(data).read() == PARSE_ERROR_INVALID_FLAG);
        CHECK(MonraceList::get_instance().empty());
        CHECK(error_idx == 0);
    }
}

TEST_CASE("RaceReader preserves ID type and missing-value errors")
{
    test::MonraceListTestAccess guard;
    auto data = make_monrace();
    data.erase("id");
    CHECK(RaceReader(data).read() == PARSE_ERROR_TOO_FEW_ARGUMENTS);
    for (const auto &id : { nlohmann::json(), nlohmann::json(true), nlohmann::json("1"), nlohmann::json(1.0), nlohmann::json::array(), nlohmann::json::object() }) {
        data["id"] = id;
        CHECK(RaceReader(data).read() == PARSE_ERROR_TOO_FEW_ARGUMENTS);
        CHECK(MonraceList::get_instance().empty());
    }
}

TEST_CASE("RaceReader accepts single-byte symbol characters")
{
    test::MonraceListTestAccess guard;
    auto data = make_monrace();
    for (const auto *character : { "p", " ", "\\" }) {
        CAPTURE(character);
        data["symbol"]["character"] = character;
        REQUIRE(RaceReader(data).read() == PARSE_ERROR_NONE);
        CHECK(test_monrace().symbol_definition.character == character[0]);
    }
}

TEST_CASE("RaceReader rejects empty and multibyte symbol characters")
{
    test::MonraceListTestAccess guard;
    auto data = make_monrace();
    data["symbol"]["character"] = "";
    CHECK(RaceReader(data).read() == PARSE_ERROR_GENERIC);
    for (const auto *character : { "pp", "\xc3\xa9", "\xf0\x9f\x8c\xb2" }) {
        CAPTURE(character);
        data["symbol"]["character"] = character;
        CHECK(RaceReader(data).read() == PARSE_ERROR_INVALID_VALUE);
    }
}

TEST_CASE("RaceReader accepts exactly the supported number of blows")
{
    test::MonraceListTestAccess guard;
    auto data = make_monrace();
    data["blows"] = nlohmann::json::array();
    for (int i = 0; i < MAX_NUM_BLOWS; ++i) {
        data["blows"].push_back({ { "method", "HIT" }, { "effect", "HURT" }, { "damage_dice", std::to_string(i + 1) + "d6" } });
    }
    REQUIRE(RaceReader(data).read() == PARSE_ERROR_NONE);
    for (int i = 0; i < MAX_NUM_BLOWS; ++i) {
        CHECK(test_monrace().blows[i].damage_dice == Dice(i + 1, 6));
    }
}

TEST_CASE("RaceReader rejects excess blows before accessing beyond their storage")
{
    test::MonraceListTestAccess guard;
    auto data = make_monrace();
    for (const auto count : { MAX_NUM_BLOWS + 1, MAX_NUM_BLOWS + 2, MAX_NUM_BLOWS + 3 }) {
        CAPTURE(count);
        data["blows"] = nlohmann::json::array();
        for (int i = 0; i < count; ++i) {
            data["blows"].push_back({ { "method", "HIT" }, { "effect", "HURT" }, { "damage_dice", "1d6" } });
        }
        CHECK(RaceReader(data).read() == PARSE_ERROR_GENERIC);
    }
}

TEST_CASE("RaceReader accepts every schema-valid skill probability")
{
    test::MonraceListTestAccess guard;
    auto data = make_monrace();
    for (int denominator = 1; denominator <= 100; ++denominator) {
        CAPTURE(denominator);
        data["skill"]["probability"] = "1_IN_" + std::to_string(denominator);
        REQUIRE(RaceReader(data).read() == PARSE_ERROR_NONE);
        CHECK(test_monrace().freq_spell == 100 / denominator);
        CHECK(test_monrace().ability_flags.has(MonsterAbilityType::SHRIEK));
    }
}

TEST_CASE("RaceReader rejects invalid skill probability denominators without throwing")
{
    test::MonraceListTestAccess guard;
    auto data = make_monrace();
    for (const auto *probability : { "1_IN_0", "1_IN_101", "1_IN_255", "1_IN_256", "1_IN_-1", "1_IN_x", "1_IN_3x", "1_IN_99999999999999999999", "1_IN_01", "1_IN_+3", "1_IN_ 3", "1_IN_3 ", "1_IN_" }) {
        CAPTURE(probability);
        data["skill"]["probability"] = probability;
        CHECK(RaceReader(data).read() == PARSE_ERROR_INVALID_VALUE);
    }
    for (const auto *probability : { "", "bad", "1_IN", "2_IN_3", "1_OF_3", "1_IN_3_extra" }) {
        CAPTURE(probability);
        data["skill"]["probability"] = probability;
        CHECK(RaceReader(data).read() == PARSE_ERROR_GENERIC);
    }
}

TEST_CASE("RaceReader reports missing and nonstring skill probabilities")
{
    test::MonraceListTestAccess guard;
    auto data = make_monrace();
    data["skill"].erase("probability");
    CHECK(RaceReader(data).read() == PARSE_ERROR_TOO_FEW_ARGUMENTS);
    for (const auto &value : { nlohmann::json(), nlohmann::json(42), nlohmann::json(true), nlohmann::json::array(), nlohmann::json::object() }) {
        data["skill"]["probability"] = value;
        CHECK(RaceReader(data).read() == PARSE_ERROR_TOO_FEW_ARGUMENTS);
    }
}

TEST_CASE("RaceReader reads shoot dice with and without a skill list")
{
    test::MonraceListTestAccess guard;
    auto data = make_monrace();
    data["skill"]["shoot"] = "2d6";
    REQUIRE(RaceReader(data).read() == PARSE_ERROR_NONE);
    CHECK(test_monrace().shoot_damage_dice == Dice(2, 6));
    CHECK(test_monrace().ability_flags.has(MonsterAbilityType::SHOOT));
    CHECK(test_monrace().ability_flags.has(MonsterAbilityType::SHRIEK));
    data["skill"].erase("list");
    REQUIRE(RaceReader(data).read() == PARSE_ERROR_NONE);
    CHECK(test_monrace().shoot_damage_dice == Dice(2, 6));
    data["skill"].erase("shoot");
    CHECK(RaceReader(data).read() == PARSE_ERROR_TOO_FEW_ARGUMENTS);
}

TEST_CASE("RaceReader rejects nonstring shoot dice and skill list entries")
{
    test::MonraceListTestAccess guard;
    auto data = make_monrace();
    for (const auto &value : { nlohmann::json(42), nlohmann::json(true), nlohmann::json::array(), nlohmann::json::object() }) {
        data["skill"]["shoot"] = value;
        CHECK(RaceReader(data).read() == PARSE_ERROR_INVALID_TYPE);
    }
    data["skill"]["shoot"] = nullptr;
    CHECK(RaceReader(data).read() == PARSE_ERROR_TOO_FEW_ARGUMENTS);
    data["skill"]["shoot"] = "bad";
    CHECK(RaceReader(data).read() == PARSE_ERROR_TOO_FEW_ARGUMENTS);
    data["skill"].erase("shoot");
    for (const auto &value : { nlohmann::json(), nlohmann::json(42), nlohmann::json(true), nlohmann::json::array(), nlohmann::json::object() }) {
        data["skill"]["list"] = nlohmann::json::array({ "SHRIEK", value });
        CHECK(RaceReader(data).read() == PARSE_ERROR_INVALID_TYPE);
    }
    data["skill"]["list"] = { "UNKNOWN" };
    CHECK(RaceReader(data).read() == PARSE_ERROR_INVALID_FLAG);
}

TEST_CASE("RaceReader rejects nonstring message actions before message insertion")
{
    test::MonraceListTestAccess guard;
    auto data = make_monrace();
    data["message"] = nlohmann::json::array({ { { "chance", 100 } } });
    CHECK(RaceReader(data).read() == PARSE_ERROR_TOO_FEW_ARGUMENTS);
    data["message"][0]["action"] = nullptr;
    CHECK(RaceReader(data).read() == PARSE_ERROR_TOO_FEW_ARGUMENTS);
    for (const auto &value : { nlohmann::json(42), nlohmann::json(true), nlohmann::json::array(), nlohmann::json::object() }) {
        data["message"][0]["action"] = value;
        CHECK(RaceReader(data).read() == PARSE_ERROR_INVALID_TYPE);
    }
    data["message"][0]["action"] = "UNKNOWN";
    CHECK(RaceReader(data).read() == PARSE_ERROR_INVALID_FLAG);
}

TEST_CASE("RaceReader preserves existing definition messages and record after late failure")
{
    test::MonraceListTestAccess races;
    test::MonraceMessageListTestAccess messages;
    test::MonraceRecordsTestAccess records;
    const auto rng = test::scoped_rng();
    auto data = make_monrace();
    data["id"] = 1;
    auto &list = MonraceList::get_instance();
    auto &original = list.emplace(static_cast<MonraceId>(1));
    original.name = { "original", "original" };
    original.level = 42;
    original.r_pkills = 17;
    original.emplace_reinforce(static_cast<MonraceId>(2), Dice(3, 4));
    const auto identity = list.get_monrace_shared(static_cast<MonraceId>(1));
    const auto record = MonraceRecords::get_instance().get_record(static_cast<MonraceId>(1));
    record->set_seen_count(23);
    data["escorts"] = nlohmann::json::array({ { { "escorts_id", 2 }, { "escort_num", "1d2" } } });
    auto &message_list = MonraceMessageList::get_instance();
    message_list.emplace(1, MonsterMessageType::SPEAK_ALL, 1, false, "old");
    data["message"] = nlohmann::json::array({
        { { "action", "SPEAK_ALL" }, { "chance", 1 }, { "use_name", false }, { "message", { { "ja", "new" }, { "en", "new" } } } },
        { { "action", "UNKNOWN" }, { "chance", 1 } },
    });
    auto expected = PARSE_ERROR_INVALID_FLAG;
    SUBCASE("late message failure") {}
    SUBCASE("late message string failure")
    {
        data["message"][1] = data["message"][0];
        data["message"][1]["message"]["ja"] = 7;
        data["message"][1]["message"]["en"] = 7;
        expected = PARSE_ERROR_INVALID_TYPE;
    }
    SUBCASE("late race failure")
    {
        data["flags"] = { "RES_FIRE", "UNKNOWN" };
    }
    CHECK(RaceReader(data).read() == expected);
    CHECK(list.get_monrace_shared(static_cast<MonraceId>(1)) == identity);
    CHECK(original.name.en_string() == "original");
    CHECK(original.level == 42);
    CHECK(original.r_pkills == 17);
    CHECK(original.resistance_flags.none());
    REQUIRE(original.get_reinforces().size() == 1);
    CHECK(original.get_reinforces()[0].get_dice_as_string() == "3d4");
    CHECK(message_list.get_message(1, "", MonsterMessageType::SPEAK_ALL) == "old");
    CHECK(messages.message_count(1, MonsterMessageType::SPEAK_ALL) == 1);
    CHECK(MonraceRecords::get_instance().get_record(static_cast<MonraceId>(1)) == record);
    CHECK(record->get_seen_count() == 23);
    CHECK(error_idx == 1);
}

TEST_CASE("RaceReader does not publish a new ID or messages after a late failure")
{
    test::MonraceListTestAccess races;
    test::MonraceMessageListTestAccess messages;
    auto data = make_monrace();
    data["id"] = 2;
    data["message"] = nlohmann::json::array({
        { { "action", "SPEAK_ALL" }, { "chance", 1 }, { "message", { { "ja", "new" }, { "en", "new" } } } },
        { { "action", "SPEAK_BATTLE" }, { "chance", 101 } },
    });
    SUBCASE("late message failure") {}
    SUBCASE("late race failure")
    {
        data["flags"] = { "RES_FIRE", "UNKNOWN" };
    }
    CHECK(RaceReader(data).read() == PARSE_ERROR_INVALID_FLAG);
    CHECK(MonraceList::get_instance().empty());
    CHECK(messages.message_count(2, MonsterMessageType::SPEAK_ALL) == 0);
    CHECK(messages.message_count(2, MonsterMessageType::SPEAK_BATTLE) == 0);
    CHECK(error_idx == 2);
}

TEST_CASE("RaceReader successful repeated ID keeps identity and appends optional collections")
{
    test::MonraceListTestAccess races;
    test::MonraceMessageListTestAccess messages;
    test::MonraceRecordsTestAccess records;
    auto data = make_monrace();
    data["id"] = 1;
    data["escorts"] = nlohmann::json::array({ { { "escorts_id", 2 }, { "escort_num", "1d2" } } });
    data["message"] = nlohmann::json::array({ { { "action", "SPEAK_ALL" }, { "chance", 1 }, { "message", { { "ja", "text" }, { "en", "text" } } } } });
    REQUIRE(RaceReader(data).read() == PARSE_ERROR_NONE);
    auto &list = MonraceList::get_instance();
    const auto identity = list.get_monrace_shared(static_cast<MonraceId>(1));
    identity->r_pkills = 19;
    const auto record = MonraceRecords::get_instance().get_record(static_cast<MonraceId>(1));
    record->set_seen_count(31);
    data["level"] = 50;
    REQUIRE(RaceReader(data).read() == PARSE_ERROR_NONE);
    CHECK(list.get_monrace_shared(static_cast<MonraceId>(1)) == identity);
    CHECK(identity->level == 50);
    CHECK(identity->r_pkills == 19);
    CHECK(identity->get_reinforces().size() == 2);
    CHECK(messages.message_count(1, MonsterMessageType::SPEAK_ALL) == 2);
    data.erase("escorts");
    SUBCASE("missing messages")
    {
        data.erase("message");
    }
    SUBCASE("null messages")
    {
        data["message"] = nullptr;
    }
    SUBCASE("empty messages")
    {
        data["message"] = nlohmann::json::array();
    }
    REQUIRE(RaceReader(data).read() == PARSE_ERROR_NONE);
    CHECK(identity->get_reinforces().size() == 2);
    CHECK(messages.message_count(1, MonsterMessageType::SPEAK_ALL) == 2);
    CHECK(MonraceRecords::get_instance().get_record(static_cast<MonraceId>(1)) == record);
    CHECK(record->get_seen_count() == 31);
}

TEST_CASE("RaceReader keeps absent message groups absent for missing null and empty messages")
{
    test::MonraceListTestAccess races;
    test::MonraceMessageListTestAccess messages;
    const auto rng = test::scoped_rng();
    auto &list = MonraceMessageList::get_instance();
    list.emplace_default(MonsterMessageType::SPEAK_BATTLE, 1, false, "battle");
    list.emplace_default(MonsterMessageType::SPEAK_ALL, 1, false, "all");
    auto data = make_monrace();
    SUBCASE("missing messages") {}
    SUBCASE("null messages")
    {
        data["message"] = nullptr;
    }
    SUBCASE("empty messages")
    {
        data["message"] = nlohmann::json::array();
    }
    REQUIRE(RaceReader(data).read() == PARSE_ERROR_NONE);
    CHECK(list.get_message(0, "", MonsterMessageType::SPEAK_BATTLE) == "battle");
    CHECK(messages.message_count(0, MonsterMessageType::SPEAK_ALL) == 0);
}
