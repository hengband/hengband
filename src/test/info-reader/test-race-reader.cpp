#include "info-reader/info-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/race-reader.h"
#include "locale/character-encoding.h"
#include "monster-attack/monster-attack-effect.h"
#include "monster-attack/monster-attack-table.h"
#include "player-ability/player-ability-types.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monrace/monrace-record.h"
#include "system/monrace/monrace-records.h"
#include "test/info-reader/parse-result-helpers.h"
#include "test/info-reader/scoped-reader-state.h"
#include "test/scoped-rng.h"
#include "test/system/artifact-list-test-access.h"
#include "test/system/monrace-list-test-access.h"
#include "test/system/monrace-message-list-test-access.h"
#include "test/system/monrace-records-test-access.h"
#include <cstdint>
#include <doctest/doctest.h>
#include <initializer_list>
#include <limits>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <tl/expected.hpp>
#include <tuple>
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

/*!
 * @brief ファイルの先頭の定義として読む
 * @details IDは1つのファイルの中で昇順・重複なしとするため、同じ定義を繰り返し読むテストでは
 * 読み込みのたびに独立した Session で始める。
 */
tl::expected<void, ParseErrorType> read_as_first_record(const nlohmann::json &data, RaceReader::Session &session)
{
    session = RaceReader::create_session();
    return test::read_and_commit(RaceReader(data, session), session);
}
}

TEST_CASE("RaceReader accepts arena odds correction ratio boundaries")
{
    auto session = RaceReader::create_session();
    test::MonraceListTestAccess guard;
    auto data = make_monrace();
    for (const auto ratio : { 1, 1000 }) {
        CAPTURE(ratio);
        data["odds_correction_ratio"] = ratio;
        REQUIRE(read_as_first_record(data, session).has_value());
        CHECK(test_monrace().arena_ratio == ratio);
    }
}

TEST_CASE("RaceReader rejects arena odds correction ratios outside the range without changing the destination")
{
    auto session = RaceReader::create_session();
    test::MonraceListTestAccess guard;
    auto data = make_monrace();
    data["odds_correction_ratio"] = 100;
    REQUIRE(read_as_first_record(data, session).has_value());
    const auto identity = MonraceList::get_instance().get_monrace_shared(static_cast<MonraceId>(0));
    for (const auto ratio : { 0, 1001 }) {
        CAPTURE(ratio);
        data["odds_correction_ratio"] = ratio;
        CHECK(test::to_error_code(read_as_first_record(data, session)) == ParseErrorType::INVALID_VALUE);
        CHECK(MonraceList::get_instance().get_monrace_shared(static_cast<MonraceId>(0)) == identity);
        CHECK(test_monrace().arena_ratio == 100);
    }
}

TEST_CASE("RaceReader accepts evolution experience boundaries")
{
    auto session = RaceReader::create_session();
    test::MonraceListTestAccess guard;
    auto data = make_monrace();
    for (const auto need_exp : { 1, 9999999 }) {
        CAPTURE(need_exp);
        data["evolve"] = { { "need_exp", need_exp }, { "to", 2 } };
        REQUIRE(read_as_first_record(data, session).has_value());
        CHECK(test_monrace().next_exp == need_exp);
        CHECK(test_monrace().next_r_idx == static_cast<MonraceId>(2));
    }
}

TEST_CASE("RaceReader rejects evolution experience outside the range without changing the destination")
{
    auto session = RaceReader::create_session();
    test::MonraceListTestAccess guard;
    auto data = make_monrace();
    data["evolve"] = { { "need_exp", 10 }, { "to", 2 } };
    REQUIRE(read_as_first_record(data, session).has_value());
    const auto identity = MonraceList::get_instance().get_monrace_shared(static_cast<MonraceId>(0));
    for (const auto need_exp : { 0, 10000000 }) {
        CAPTURE(need_exp);
        data["evolve"] = { { "need_exp", need_exp }, { "to", 3 } };
        CHECK(test::to_error_code(read_as_first_record(data, session)) == ParseErrorType::INVALID_VALUE);
        CHECK(MonraceList::get_instance().get_monrace_shared(static_cast<MonraceId>(0)) == identity);
        CHECK(test_monrace().next_exp == 10);
        CHECK(test_monrace().next_r_idx == static_cast<MonraceId>(2));
    }
}

TEST_CASE("RaceReader accepts schema boundary IDs")
{
    auto session = RaceReader::create_session();
    test::MonraceListTestAccess guard;
    auto data = make_monrace();
    for (const auto id : { 0, 32766 }) {
        CAPTURE(id);
        data["id"] = id;
        REQUIRE(read_as_first_record(data, session).has_value());
        CHECK(MonraceList::get_instance().get_monrace(static_cast<MonraceId>(id)).idx == static_cast<MonraceId>(id));
        CHECK(session.last_id().value_or(-1) == id);
    }
}

TEST_CASE("RaceReader rejects out-of-range IDs before conversion or registration")
{
    auto session = RaceReader::create_session();
    test::MonraceListTestAccess guard;
    auto data = make_monrace();
    for (const auto &id : {
             nlohmann::json(-1), nlohmann::json(32767), nlohmann::json(32768), nlohmann::json(65536),
             nlohmann::json(int64_t{ 1 } << 32), nlohmann::json((int64_t{ 1 } << 32) + 1),
             nlohmann::json(std::numeric_limits<int64_t>::min()), nlohmann::json(std::numeric_limits<uint64_t>::max()) }) {
        CAPTURE(id);
        data["id"] = id;
        CHECK(test::to_error_code(read_as_first_record(data, session)) == ParseErrorType::INVALID_VALUE);
        CHECK(MonraceList::get_instance().empty());
        CHECK(session.last_id().value_or(-1) == -1);
    }
}

TEST_CASE("RaceReader preserves ID type and missing-value errors")
{
    auto session = RaceReader::create_session();
    test::MonraceListTestAccess guard;
    auto data = make_monrace();
    data.erase("id");
    CHECK(test::to_error_code(read_as_first_record(data, session)) == ParseErrorType::TOO_FEW_ARGUMENTS);
    for (const auto &id : { nlohmann::json(), nlohmann::json(true), nlohmann::json("1"), nlohmann::json(1.0), nlohmann::json::array(), nlohmann::json::object() }) {
        data["id"] = id;
        CHECK(test::to_error_code(read_as_first_record(data, session)) == (id.is_null() ? ParseErrorType::TOO_FEW_ARGUMENTS : ParseErrorType::INVALID_TYPE));
        CHECK(MonraceList::get_instance().empty());
    }
}

TEST_CASE("RaceReader accepts single-byte symbol characters")
{
    auto session = RaceReader::create_session();
    test::MonraceListTestAccess guard;
    auto data = make_monrace();
    for (const auto *character : { "p", " ", "\\" }) {
        CAPTURE(character);
        data["symbol"]["character"] = character;
        REQUIRE(read_as_first_record(data, session).has_value());
        CHECK(test_monrace().symbol_definition.character == character[0]);
    }
}

TEST_CASE("RaceReader rejects empty and multibyte symbol characters")
{
    auto session = RaceReader::create_session();
    test::MonraceListTestAccess guard;
    auto data = make_monrace();
    data["symbol"]["character"] = "";
    CHECK(test::to_error_code(read_as_first_record(data, session)) == ParseErrorType::INVALID_VALUE);
    for (const auto *character : { "pp", "\xc3\xa9", "\xf0\x9f\x8c\xb2" }) {
        CAPTURE(character);
        data["symbol"]["character"] = character;
        CHECK(test::to_error_code(read_as_first_record(data, session)) == ParseErrorType::INVALID_VALUE);
    }
}

TEST_CASE("RaceReader accepts exactly the supported number of blows")
{
    auto session = RaceReader::create_session();
    test::MonraceListTestAccess guard;
    auto data = make_monrace();
    data["blows"] = nlohmann::json::array();
    for (int i = 0; i < MAX_NUM_BLOWS; ++i) {
        data["blows"].push_back({ { "method", "HIT" }, { "effect", "HURT" }, { "damage_dice", std::to_string(i + 1) + "d6" } });
    }
    REQUIRE(read_as_first_record(data, session).has_value());
    for (int i = 0; i < MAX_NUM_BLOWS; ++i) {
        CHECK(test_monrace().blows[i].damage_dice == Dice(i + 1, 6));
    }
}

TEST_CASE("RaceReader rejects excess blows before accessing beyond their storage")
{
    auto session = RaceReader::create_session();
    test::MonraceListTestAccess guard;
    auto data = make_monrace();
    for (const auto count : { MAX_NUM_BLOWS + 1, MAX_NUM_BLOWS + 2, MAX_NUM_BLOWS + 3 }) {
        CAPTURE(count);
        data["blows"] = nlohmann::json::array();
        for (int i = 0; i < count; ++i) {
            data["blows"].push_back({ { "method", "HIT" }, { "effect", "HURT" }, { "damage_dice", "1d6" } });
        }
        CHECK(test::to_error_code(read_as_first_record(data, session)) == ParseErrorType::INVALID_VALUE);
    }
}

TEST_CASE("RaceReader accepts every schema-valid skill probability")
{
    auto session = RaceReader::create_session();
    test::MonraceListTestAccess guard;
    auto data = make_monrace();
    for (int denominator = 1; denominator <= 100; ++denominator) {
        CAPTURE(denominator);
        data["skill"]["probability"] = "1_IN_" + std::to_string(denominator);
        REQUIRE(read_as_first_record(data, session).has_value());
        CHECK(test_monrace().freq_spell == 100 / denominator);
        CHECK(test_monrace().ability_flags.has(MonsterAbilityType::SHRIEK));
    }
}

TEST_CASE("RaceReader rejects invalid skill probability denominators without throwing")
{
    auto session = RaceReader::create_session();
    test::MonraceListTestAccess guard;
    auto data = make_monrace();
    for (const auto *probability : { "1_IN_0", "1_IN_101", "1_IN_255", "1_IN_256", "1_IN_-1", "1_IN_x", "1_IN_3x", "1_IN_99999999999999999999", "1_IN_01", "1_IN_+3", "1_IN_ 3", "1_IN_3 ", "1_IN_" }) {
        CAPTURE(probability);
        data["skill"]["probability"] = probability;
        CHECK(test::to_error_code(read_as_first_record(data, session)) == ParseErrorType::INVALID_VALUE);
    }
    for (const auto *probability : { "", "bad", "1_IN", "2_IN_3", "1_OF_3", "1_IN_3_extra" }) {
        CAPTURE(probability);
        data["skill"]["probability"] = probability;
        CHECK(test::to_error_code(read_as_first_record(data, session)) == ParseErrorType::INVALID_VALUE);
    }
}

TEST_CASE("RaceReader reports missing and nonstring skill probabilities")
{
    auto session = RaceReader::create_session();
    test::MonraceListTestAccess guard;
    auto data = make_monrace();
    data["skill"].erase("probability");
    CHECK(test::to_error_code(read_as_first_record(data, session)) == ParseErrorType::TOO_FEW_ARGUMENTS);
    for (const auto &value : { nlohmann::json(), nlohmann::json(42), nlohmann::json(true), nlohmann::json::array(), nlohmann::json::object() }) {
        data["skill"]["probability"] = value;
        CHECK(test::to_error_code(read_as_first_record(data, session)) == ParseErrorType::TOO_FEW_ARGUMENTS);
    }
}

TEST_CASE("RaceReader reads shoot dice with and without a skill list")
{
    auto session = RaceReader::create_session();
    test::MonraceListTestAccess guard;
    auto data = make_monrace();
    data["skill"]["shoot"] = "2d6";
    REQUIRE(read_as_first_record(data, session).has_value());
    CHECK(test_monrace().shoot_damage_dice == Dice(2, 6));
    CHECK(test_monrace().ability_flags.has(MonsterAbilityType::SHOOT));
    CHECK(test_monrace().ability_flags.has(MonsterAbilityType::SHRIEK));
    data["skill"].erase("list");
    REQUIRE(read_as_first_record(data, session).has_value());
    CHECK(test_monrace().shoot_damage_dice == Dice(2, 6));
    data["skill"].erase("shoot");
    CHECK(test::to_error_code(read_as_first_record(data, session)) == ParseErrorType::TOO_FEW_ARGUMENTS);
}

TEST_CASE("RaceReader treats a null skill list like an omitted one")
{
    auto session = RaceReader::create_session();
    test::MonraceListTestAccess guard;
    auto data = make_monrace();
    data["skill"]["list"] = nullptr;
    CHECK(test::to_error_code(read_as_first_record(data, session)) == ParseErrorType::TOO_FEW_ARGUMENTS);
    data["skill"]["shoot"] = "2d6";
    REQUIRE(read_as_first_record(data, session).has_value());
    CHECK(test_monrace().ability_flags.has(MonsterAbilityType::SHOOT));
    CHECK_FALSE(test_monrace().ability_flags.has(MonsterAbilityType::SHRIEK));
}

TEST_CASE("RaceReader rejects nonstring shoot dice and skill list entries")
{
    auto session = RaceReader::create_session();
    test::MonraceListTestAccess guard;
    auto data = make_monrace();
    for (const auto &value : { nlohmann::json(42), nlohmann::json(true), nlohmann::json::array(), nlohmann::json::object() }) {
        data["skill"]["shoot"] = value;
        CHECK(test::to_error_code(read_as_first_record(data, session)) == ParseErrorType::INVALID_TYPE);
    }
    data["skill"]["shoot"] = nullptr;
    CHECK(test::to_error_code(read_as_first_record(data, session)) == ParseErrorType::TOO_FEW_ARGUMENTS);
    data["skill"]["shoot"] = "bad";
    CHECK(test::to_error_code(read_as_first_record(data, session)) == ParseErrorType::INVALID_VALUE);
    data["skill"].erase("shoot");
    for (const auto &value : { nlohmann::json(), nlohmann::json(42), nlohmann::json(true), nlohmann::json::array(), nlohmann::json::object() }) {
        data["skill"]["list"] = nlohmann::json::array({ "SHRIEK", value });
        CHECK(test::to_error_code(read_as_first_record(data, session)) == ParseErrorType::INVALID_TYPE);
    }
    data["skill"]["list"] = { "UNKNOWN" };
    CHECK(test::to_error_code(read_as_first_record(data, session)) == ParseErrorType::INVALID_FLAG);
}

TEST_CASE("RaceReader rejects nonstring message actions before message insertion")
{
    auto session = RaceReader::create_session();
    test::MonraceListTestAccess guard;
    auto data = make_monrace();
    data["message"] = nlohmann::json::array({ { { "chance", 100 } } });
    CHECK(test::to_error_code(read_as_first_record(data, session)) == ParseErrorType::TOO_FEW_ARGUMENTS);
    data["message"][0]["action"] = nullptr;
    CHECK(test::to_error_code(read_as_first_record(data, session)) == ParseErrorType::TOO_FEW_ARGUMENTS);
    for (const auto &value : { nlohmann::json(42), nlohmann::json(true), nlohmann::json::array(), nlohmann::json::object() }) {
        data["message"][0]["action"] = value;
        CHECK(test::to_error_code(read_as_first_record(data, session)) == ParseErrorType::INVALID_TYPE);
    }
    data["message"][0]["action"] = "UNKNOWN";
    CHECK(test::to_error_code(read_as_first_record(data, session)) == ParseErrorType::INVALID_FLAG);
}

TEST_CASE("RaceReader preserves existing definition messages and record after helper failures")
{
    auto session = RaceReader::create_session();
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
    auto expected = ParseErrorType::INVALID_FLAG;
    SUBCASE("late message failure") {}
    SUBCASE("late message boolean failure")
    {
        data["message"][1] = data["message"][0];
        data["message"][1]["use_name"] = "false";
        expected = ParseErrorType::INVALID_TYPE;
    }
    SUBCASE("late message string failure")
    {
        data["message"][1] = data["message"][0];
        data["message"][1]["message"]["ja"] = 7;
        data["message"][1]["message"]["en"] = 7;
        expected = ParseErrorType::INVALID_TYPE;
    }
    SUBCASE("late race failure")
    {
        data["flags"] = { "RES_FIRE", "UNKNOWN" };
    }
    SUBCASE("name before symbol")
    {
        data["name"] = 42;
        data["symbol"]["character"] = "pp";
        expected = ParseErrorType::TOO_FEW_ARGUMENTS;
    }
    SUBCASE("symbol before speed")
    {
        data["symbol"]["character"] = "pp";
        // 速度を先に検証すると INVALID_TYPE になり、シンボルの INVALID_VALUE と区別できる。
        data["speed"] = "bad";
        expected = ParseErrorType::INVALID_VALUE;
    }
    SUBCASE("speed before hit dice")
    {
        data["speed"] = "bad";
        data["hit_point"] = "0d1";
        expected = ParseErrorType::INVALID_TYPE;
    }
    SUBCASE("hit dice before vision")
    {
        data["hit_point"] = "1d32768";
        data["vision"] = "bad";
        expected = ParseErrorType::INVALID_VALUE;
    }
    SUBCASE("evolution before sex")
    {
        data["evolve"] = { { "need_exp", "bad" }, { "to", 2 } };
        data["sex"] = "UNKNOWN";
        expected = ParseErrorType::INVALID_TYPE;
    }
    SUBCASE("sex before artifact drops")
    {
        data["sex"] = 42;
        data["artifacts"] = nlohmann::json::array({ { { "drop_artifact_id", 0 }, { "drop_probability", 50 } } });
        expected = ParseErrorType::INVALID_TYPE;
    }
    SUBCASE("artifact probability before escort dice")
    {
        data["artifacts"] = nlohmann::json::array({ { { "drop_artifact_id", 3 }, { "drop_probability", "bad" } } });
        data["escorts"][0]["escort_num"] = "1d32768";
        expected = ParseErrorType::INVALID_TYPE;
    }
    SUBCASE("escort dice before blows")
    {
        data["escorts"][0]["escort_num"] = "01d2";
        data["blows"] = "bad";
        expected = ParseErrorType::INVALID_VALUE;
    }
    SUBCASE("blow dice before flags")
    {
        data["blows"] = nlohmann::json::array({ { { "method", "HIT" }, { "effect", "HURT" }, { "damage_dice", "0d2" } } });
        data["flags"] = { "UNKNOWN" };
        expected = ParseErrorType::INVALID_VALUE;
    }
    SUBCASE("shoot dice before final summons")
    {
        data["skill"]["shoot"] = "01d2";
        data["final_summon"] = "bad";
        expected = ParseErrorType::INVALID_VALUE;
    }
    SUBCASE("final summon count relation before messages")
    {
        data["final_summon"] = nlohmann::json::array({ { { "id", 2 }, { "probability", 100 }, { "min_num", 2 }, { "max_num", 1 }, { "radius", 1 } } });
        expected = ParseErrorType::INVALID_VALUE;
    }
    CHECK(test::to_error_code(read_as_first_record(data, session)) == expected);
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
    CHECK_FALSE(session.last_id());
}

TEST_CASE("RaceReader publishes complete nested helper values and messages on success")
{
    auto session = RaceReader::create_session();
    test::MonraceListTestAccess races;
    test::MonraceMessageListTestAccess messages;
    test::ArtifactListTestAccess artifacts;
    const auto rng = test::scoped_rng();
    MonraceList::get_instance().emplace(static_cast<MonraceId>(2));
    ArtifactList::get_instance().emplace(static_cast<FixedArtifactId>(3), {});
    auto data = make_monrace();
    data["id"] = 1;
    data["speed"] = 5;
    data["hit_point"] = "2d5";
    data["evolve"] = { { "need_exp", 10 }, { "to", 2 } };
    data["sex"] = "MALE";
    data["artifacts"] = nlohmann::json::array({ { { "drop_artifact_id", 3 }, { "drop_probability", 50 } } });
    data["escorts"] = nlohmann::json::array({ { { "escorts_id", 2 }, { "escort_num", "3d2" } } });
    data["blows"] = nlohmann::json::array({ { { "method", "HIT" }, { "effect", "HURT" }, { "damage_dice", "1d4" } } });
    data["flags"] = { "RES_FIRE" };
    data["skill"] = { { "probability", "1_IN_2" }, { "shoot", "2d3" }, { "list", { "SHRIEK" } } };
    data["final_summon"] = nlohmann::json::array({ { { "id", 2 }, { "probability", 100 }, { "min_num", 1 }, { "max_num", 2 }, { "radius", 3 } } });
    data["message"] = nlohmann::json::array({ { { "action", "SPEAK_ALL" }, { "chance", 1 }, { "use_name", false }, { "message", { { "ja", "hello" }, { "en", "hello" } } } } });
    REQUIRE(read_as_first_record(data, session).has_value());
    const auto &monrace = MonraceList::get_instance().get_monrace(static_cast<MonraceId>(1));
    CHECK(monrace.name.en_string() == "test");
    CHECK(monrace.symbol_definition.character == 'p');
    CHECK(monrace.speed == STANDARD_SPEED + 5);
    CHECK(monrace.hit_dice == Dice(2, 5));
    CHECK(monrace.next_exp == 10);
    CHECK(monrace.next_r_idx == static_cast<MonraceId>(2));
    CHECK(monrace.is_male());
    CHECK_FALSE(monrace.is_female());
    REQUIRE(monrace.get_drop_artifacts().size() == 1);
    CHECK(monrace.get_drop_artifacts()[0].fa_id == static_cast<FixedArtifactId>(3));
    CHECK(monrace.get_drop_artifacts()[0].chance == 50);
    REQUIRE(monrace.get_reinforces().size() == 1);
    CHECK(monrace.get_reinforces()[0].get_monrace_id() == static_cast<MonraceId>(2));
    CHECK(monrace.get_reinforces()[0].get_dice_as_string() == "3d2");
    CHECK(monrace.blows[0].method == RaceBlowMethodType::HIT);
    CHECK(monrace.blows[0].effect == RaceBlowEffectType::HURT);
    CHECK(monrace.blows[0].damage_dice == Dice(1, 4));
    CHECK(monrace.resistance_flags.has(MonsterResistanceType::RESIST_FIRE));
    CHECK(monrace.freq_spell == 50);
    CHECK(monrace.shoot_damage_dice == Dice(2, 3));
    CHECK(monrace.ability_flags.has(MonsterAbilityType::SHOOT));
    CHECK(monrace.ability_flags.has(MonsterAbilityType::SHRIEK));
    REQUIRE(monrace.get_final_summons().size() == 1);
    CHECK(monrace.get_final_summons()[0].id == static_cast<MonraceId>(2));
    CHECK(monrace.get_final_summons()[0].min_num == 1);
    CHECK(monrace.get_final_summons()[0].max_num == 2);
    CHECK(monrace.get_final_summons()[0].radius == 3);
    CHECK(messages.message_count(1, MonsterMessageType::SPEAK_ALL) == 1);
    CHECK(MonraceMessageList::get_instance().get_message(1, "", MonsterMessageType::SPEAK_ALL) == "hello");
    CHECK_FALSE(RaceReader::validate_references().has_value());
    CHECK(session.last_id().value_or(-1) == 1);
}

TEST_CASE("RaceReader does not publish a new ID or messages after a late failure")
{
    auto session = RaceReader::create_session();
    test::MonraceListTestAccess races;
    test::MonraceMessageListTestAccess messages;
    auto data = make_monrace();
    data["id"] = 2;
    data["message"] = nlohmann::json::array({
        { { "action", "SPEAK_ALL" }, { "chance", 1 }, { "message", { { "ja", "new" }, { "en", "new" } } } },
        { { "action", "SPEAK_BATTLE" }, { "chance", 101 } },
    });
    auto expected = ParseErrorType::INVALID_VALUE;
    SUBCASE("late message failure") {}
    SUBCASE("late race failure")
    {
        data["flags"] = { "RES_FIRE", "UNKNOWN" };
        expected = ParseErrorType::INVALID_FLAG;
    }
    CHECK(test::to_error_code(read_as_first_record(data, session)) == expected);
    CHECK(MonraceList::get_instance().empty());
    CHECK(messages.message_count(2, MonsterMessageType::SPEAK_ALL) == 0);
    CHECK(messages.message_count(2, MonsterMessageType::SPEAK_BATTLE) == 0);
    CHECK_FALSE(session.last_id());
}

TEST_CASE("RaceReader rereading a file keeps identity and appends optional collections")
{
    auto session = RaceReader::create_session();
    test::MonraceListTestAccess races;
    test::MonraceMessageListTestAccess messages;
    test::MonraceRecordsTestAccess records;
    auto data = make_monrace();
    data["id"] = 1;
    data["escorts"] = nlohmann::json::array({ { { "escorts_id", 2 }, { "escort_num", "1d2" } } });
    data["message"] = nlohmann::json::array({ { { "action", "SPEAK_ALL" }, { "chance", 1 }, { "message", { { "ja", "text" }, { "en", "text" } } } } });
    REQUIRE(read_as_first_record(data, session).has_value());
    auto &list = MonraceList::get_instance();
    const auto identity = list.get_monrace_shared(static_cast<MonraceId>(1));
    identity->r_pkills = 19;
    const auto record = MonraceRecords::get_instance().get_record(static_cast<MonraceId>(1));
    record->set_seen_count(31);
    data["level"] = 50;
    REQUIRE(read_as_first_record(data, session).has_value());
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
    REQUIRE(read_as_first_record(data, session).has_value());
    CHECK(identity->get_reinforces().size() == 2);
    CHECK(messages.message_count(1, MonsterMessageType::SPEAK_ALL) == 2);
    CHECK(MonraceRecords::get_instance().get_record(static_cast<MonraceId>(1)) == record);
    CHECK(record->get_seen_count() == 31);
}

TEST_CASE("RaceReader keeps absent message groups absent for missing null and empty messages")
{
    auto session = RaceReader::create_session();
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
    REQUIRE(read_as_first_record(data, session).has_value());
    CHECK(list.get_message(0, "", MonsterMessageType::SPEAK_BATTLE) == "battle");
    CHECK(messages.message_count(0, MonsterMessageType::SPEAK_ALL) == 0);
}

TEST_CASE("RaceReader rejects duplicate and descending IDs within a file")
{
    auto session = RaceReader::create_session();
    test::MonraceListTestAccess races;
    auto data = make_monrace();
    data["id"] = 2;
    data["level"] = 10;
    REQUIRE(read_as_first_record(data, session).has_value());
    for (const auto id : { 2, 1 }) {
        CAPTURE(id);
        data["id"] = id;
        data["level"] = 20;
        data["flags"] = { "RES_FIRE" };
        CHECK(test::to_error_code(test::read_and_commit(RaceReader(data, session), session)) == ParseErrorType::NON_SEQUENTIAL_RECORDS);
        CHECK(session.last_id().value_or(-1) == 2);
    }
    auto &list = MonraceList::get_instance();
    CHECK_FALSE(list.contains(static_cast<MonraceId>(1)));
    CHECK(list.get_monrace(static_cast<MonraceId>(2)).level == 10);
    CHECK(list.get_monrace(static_cast<MonraceId>(2)).resistance_flags.none());
    data["id"] = 3;
    CHECK(test::read_and_commit(RaceReader(data, session), session).has_value());
}

TEST_CASE("RaceReader validates level rarity starting HP and armor class ranges")
{
    auto session = RaceReader::create_session();
    test::MonraceListTestAccess races;
    auto data = make_monrace();
    for (const auto &[field, value, expected] : std::initializer_list<std::tuple<const char *, int, ParseErrorType>>{
             { "level", 0, ParseErrorType::NONE },
             { "level", 127, ParseErrorType::NONE },
             { "level", 128, ParseErrorType::INVALID_VALUE },
             { "rarity", 1, ParseErrorType::NONE },
             { "rarity", 255, ParseErrorType::NONE },
             { "rarity", 0, ParseErrorType::INVALID_VALUE },
             { "start_hp_percentage", 1, ParseErrorType::NONE },
             { "start_hp_percentage", 99, ParseErrorType::NONE },
             { "start_hp_percentage", 0, ParseErrorType::INVALID_VALUE },
             { "armor_class", 0, ParseErrorType::NONE },
             { "armor_class", 10000, ParseErrorType::NONE },
             { "armor_class", 10001, ParseErrorType::INVALID_VALUE },
             { "armor_class", -1, ParseErrorType::INVALID_VALUE },
         }) {
        CAPTURE(field);
        CAPTURE(value);
        auto monrace = data;
        monrace[field] = value;
        CHECK(test::to_error_code(read_as_first_record(monrace, session)) == expected);
    }
}

TEST_CASE("RaceReader validates referenced ID ranges")
{
    auto session = RaceReader::create_session();
    test::MonraceListTestAccess races;
    auto data = make_monrace();
    const auto with_evolve = [&data](int to) {
        auto monrace = data;
        monrace["evolve"] = { { "need_exp", 10 }, { "to", to } };
        return monrace;
    };
    const auto with_escort = [&data](int id) {
        auto monrace = data;
        monrace["escorts"] = nlohmann::json::array({ { { "escorts_id", id }, { "escort_num", "1d2" } } });
        return monrace;
    };
    const auto with_final_summon = [&data](int id) {
        auto monrace = data;
        monrace["final_summon"] = nlohmann::json::array({ { { "id", id }, { "probability", 100 }, { "min_num", 1 }, { "max_num", 1 }, { "radius", 1 } } });
        return monrace;
    };
    const auto with_artifact = [&data](int id) {
        auto monrace = data;
        monrace["artifacts"] = nlohmann::json::array({ { { "drop_artifact_id", id }, { "drop_probability", 50 } } });
        return monrace;
    };
    for (const auto id : { 1, 32766 }) {
        CAPTURE(id);
        CHECK(read_as_first_record(with_evolve(id), session).has_value());
        CHECK(read_as_first_record(with_escort(id), session).has_value());
        CHECK(read_as_first_record(with_final_summon(id), session).has_value());
    }
    for (const auto id : { 0, 32767 }) {
        CAPTURE(id);
        CHECK(test::to_error_code(read_as_first_record(with_evolve(id), session)) == ParseErrorType::INVALID_VALUE);
        CHECK(test::to_error_code(read_as_first_record(with_escort(id), session)) == ParseErrorType::INVALID_VALUE);
        CHECK(test::to_error_code(read_as_first_record(with_final_summon(id), session)) == ParseErrorType::INVALID_VALUE);
    }
    CHECK(read_as_first_record(with_artifact(32766), session).has_value());
    for (const auto id : { 0, 32767 }) {
        CAPTURE(id);
        CHECK(test::to_error_code(read_as_first_record(with_artifact(id), session)) == ParseErrorType::INVALID_VALUE);
    }
}

TEST_CASE("RaceReader limits every monster dice to positive values and a short maximum roll")
{
    auto session = RaceReader::create_session();
    test::MonraceListTestAccess races;
    const auto base = make_monrace();
    const auto set_dice = [&base](std::string_view kind, std::string_view dice) {
        auto monrace = base;
        if (kind == "hit_point") {
            monrace["hit_point"] = dice;
        } else if (kind == "escort_num") {
            monrace["escorts"] = nlohmann::json::array({ { { "escorts_id", 1 }, { "escort_num", dice } } });
        } else if (kind == "damage_dice") {
            monrace["blows"] = nlohmann::json::array({ { { "method", "HIT" }, { "effect", "HURT" }, { "damage_dice", dice } } });
        } else {
            monrace["skill"] = { { "probability", "1_IN_2" }, { "shoot", dice } };
        }
        return monrace;
    };
    for (const auto *kind : { "hit_point", "escort_num", "damage_dice", "shoot" }) {
        CAPTURE(kind);
        for (const auto *dice : { "1d1", "1d32767", "32767d1", "181d181" }) {
            CAPTURE(dice);
            CHECK(read_as_first_record(set_dice(kind, dice), session).has_value());
        }
        for (const auto *dice : { "1d32768", "32768d1", "182d181", "0d5", "5d0", "-1d5", "1d-5", "2147483647d2147483647" }) {
            CAPTURE(dice);
            CHECK(test::to_error_code(read_as_first_record(set_dice(kind, dice), session)) == ParseErrorType::INVALID_VALUE);
        }
        for (const auto *dice : { "01d5", "1d05", "00001d1" }) {
            CAPTURE(dice);
            CHECK(test::to_error_code(read_as_first_record(set_dice(kind, dice), session)) == ParseErrorType::INVALID_VALUE);
        }
        CHECK(test::to_error_code(read_as_first_record(set_dice(kind, "1d5\n"), session)) == ParseErrorType::INVALID_VALUE);
    }
}

TEST_CASE("RaceReader validates references after every definition is read")
{
    auto session = RaceReader::create_session();
    test::MonraceListTestAccess races;
    test::ArtifactListTestAccess artifacts;
    auto &list = MonraceList::get_instance();
    auto &monrace = list.emplace(static_cast<MonraceId>(1));
    list.emplace(static_cast<MonraceId>(2));
    ArtifactList::get_instance().emplace(static_cast<FixedArtifactId>(3), {});
    monrace.next_r_idx = static_cast<MonraceId>(2);
    monrace.emplace_reinforce(static_cast<MonraceId>(2), Dice(1, 2));
    monrace.emplace_final_summon(static_cast<MonraceId>(2), 100, 1, 1, 1);
    monrace.emplace_drop_artifact(static_cast<FixedArtifactId>(3), 50);
    CHECK_FALSE(RaceReader::validate_references().has_value());

    // 定義済みの参照は2と3だけなので、報告された参照のIDで原因を確かめる。
    std::string reported_id = " 4 ";
    SUBCASE("undefined evolution target")
    {
        monrace.next_r_idx = static_cast<MonraceId>(4);
    }
    SUBCASE("undefined escort")
    {
        monrace.emplace_reinforce(static_cast<MonraceId>(4), Dice(1, 2));
    }
    SUBCASE("player escort")
    {
        monrace.emplace_reinforce(MonraceId::PLAYER, Dice(1, 2));
        reported_id = " 0 ";
    }
    SUBCASE("undefined final summon")
    {
        monrace.emplace_final_summon(static_cast<MonraceId>(4), 100, 1, 1, 1);
    }
    SUBCASE("undefined artifact")
    {
        monrace.emplace_drop_artifact(static_cast<FixedArtifactId>(4), 50);
    }
    const auto error = RaceReader::validate_references();
    REQUIRE(error.has_value());
    CHECK(error->find(reported_id) != std::string::npos);
}

TEST_CASE("RaceReader rejects malformed blow flags and skill list types without publishing")
{
    auto session = RaceReader::create_session();
    test::MonraceListTestAccess races;
    test::MonraceMessageListTestAccess messages;
    test::MonraceRecordsTestAccess records;
    const auto rng = test::scoped_rng();
    auto &list = MonraceList::get_instance();
    auto &original = list.emplace(static_cast<MonraceId>(1));
    original.level = 42;
    const auto identity = list.get_monrace_shared(static_cast<MonraceId>(1));
    const auto record = MonraceRecords::get_instance().get_record(static_cast<MonraceId>(1));
    record->set_seen_count(23);
    auto &message_list = MonraceMessageList::get_instance();
    message_list.emplace(1, MonsterMessageType::SPEAK_ALL, 1, false, "old");
    const auto base = [] {
        auto data = make_monrace();
        data["id"] = 1;
        data["blows"] = nlohmann::json::array({ { { "method", "HIT" }, { "effect", "HURT" } } });
        data["flags"] = { "RES_FIRE" };
        return data;
    }();
    for (const std::string path : { "/blows/0/method", "/blows/0/effect", "/flags/1", "/skill/list" }) {
        CAPTURE(path);
        for (const auto &value : { nlohmann::json(7), nlohmann::json(true), nlohmann::json::array(), nlohmann::json{ { "first", "SHRIEK" } } }) {
            // 空の配列は技能の一覧として正しい型なので対象外とする
            if ((path == "/skill/list") && value.is_array()) {
                continue;
            }
            CAPTURE(value);
            auto data = base;
            data[nlohmann::json::json_pointer(path)] = value;
            auto error = ParseErrorType::NONE;
            CHECK_NOTHROW(error = test::to_error_code(read_as_first_record(data, session)));
            CHECK(error == ParseErrorType::INVALID_TYPE);
            CHECK(list.get_monrace_shared(static_cast<MonraceId>(1)) == identity);
            CHECK(original.level == 42);
            CHECK(record->get_seen_count() == 23);
            CHECK(message_list.get_message(1, "", MonsterMessageType::SPEAK_ALL) == "old");
            CHECK_FALSE(session.last_id());
        }
    }
    REQUIRE(read_as_first_record(base, session).has_value());
    CHECK(list.get_monrace(static_cast<MonraceId>(1)).level == 0);
}

TEST_CASE("RaceReader rejects non-boolean message use_name without publishing")
{
    auto session = RaceReader::create_session();
    test::MonraceListTestAccess races;
    test::MonraceMessageListTestAccess messages;
    auto data = make_monrace();
    data["message"] = nlohmann::json::array({ { { "action", "SPEAK_ALL" }, { "chance", 1 }, { "use_name", "false" }, { "message", { { "ja", "new" }, { "en", "new" } } } } });
    CHECK(test::to_error_code(read_as_first_record(data, session)) == ParseErrorType::INVALID_TYPE);
    CHECK(MonraceList::get_instance().empty());
    CHECK(messages.message_count(0, MonsterMessageType::SPEAK_ALL) == 0);
}

#ifdef JP
TEST_CASE("RaceReader reports a name which cannot be converted to the system encoding as an invalid value")
{
    auto session = RaceReader::create_session();
    test::MonraceListTestAccess guard;
    // 途中に '\0' を含む文字列は、どの環境でもシステムの文字コードに変換しない
    const std::string name("a\0b", 3);
    REQUIRE_FALSE(utf8_to_sys(name).has_value());
    auto data = make_monrace();
    data["name"]["ja"] = name;
    CHECK(test::to_error_code(read_as_first_record(data, session)) == ParseErrorType::INVALID_VALUE);
}
#endif

TEST_CASE("RaceReader pins flag array type errors and missing values")
{
    for (const auto *path : { "/flags", "/skill/list" }) {
        for (const auto *input : { "42", "[1]", "null", "missing", "[]" }) {
            CAPTURE(path);
            CAPTURE(input);
            test::MonraceListTestAccess guard;
            auto session = RaceReader::create_session();
            auto data = make_monrace();
            const auto pointer = nlohmann::json::json_pointer(path);
            const std::string input_text = input;
            if (input_text == "missing") {
                data[pointer.parent_pointer()].erase(pointer.back());
            } else {
                data[pointer] = nlohmann::json::parse(input);
            }
            auto expected = ParseErrorType::NONE;
            if (input_text == "42" || input_text == "[1]") {
                expected = ParseErrorType::INVALID_TYPE;
            }
            if (std::string(path) == "/skill/list" && (input_text == "null" || input_text == "missing")) {
                expected = ParseErrorType::TOO_FEW_ARGUMENTS;
            }
            CHECK(test::to_error_code(test::read_and_commit(RaceReader(data, session), session)) == expected);
        }
    }
}

TEST_CASE("RaceReader preserves symbol error codes and validation precedence")
{
    test::MonraceListTestAccess guard;
    auto session = RaceReader::create_session();
    struct InvalidSymbol {
        const char *json;
        ParseErrorType error;
    };
    const InvalidSymbol cases[] = {
        { R"(null)", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"(42)", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"([])", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"(true)", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"("bad")", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"character":null,"color":"White"})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"character":42,"color":"White"})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"character":true,"color":"White"})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"character":[],"color":"White"})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"character":{},"color":"White"})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"color":null,"character":"!"})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"color":42,"character":"!"})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"color":true,"character":"!"})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"color":[],"character":"!"})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"color":{},"character":"!"})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"character":"!"})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"color":"White"})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"character":"","color":"White"})", ParseErrorType::INVALID_VALUE },
        { R"({"character":"!!","color":"White"})", ParseErrorType::INVALID_VALUE },
        { R"({"character":"\u3042","color":"White"})", ParseErrorType::INVALID_VALUE },
        { R"({"character":"!","color":"UNKNOWN"})", ParseErrorType::INVALID_FLAG },
        { R"({"character":"","color":"UNKNOWN"})", ParseErrorType::INVALID_FLAG },
        { R"({"character":"","color":42})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"character":""})", ParseErrorType::TOO_FEW_ARGUMENTS },
    };
    for (const auto &invalid : cases) {
        CAPTURE(invalid.json);
        auto data = make_monrace();
        data["symbol"] = nlohmann::json::parse(invalid.json);
        CHECK(test::to_error_code(read_as_first_record(data, session)) == invalid.error);
    }
    auto data = make_monrace();
    data.erase("symbol");
    CHECK(test::to_error_code(read_as_first_record(data, session)) == ParseErrorType::TOO_FEW_ARGUMENTS);
}

TEST_CASE("RaceReader reports non-string tokens as invalid types")
{
    const test::ScopedReaderState reader_state;
    test::MonraceListTestAccess guard;
    for (const auto *path : { "/sex", "/blows/0/method", "/blows/0/effect", "/message/0/action" }) {
        for (const auto &value : { nlohmann::json(1), nlohmann::json(true), nlohmann::json::array(), nlohmann::json::object() }) {
            CAPTURE(path);
            CAPTURE(value.dump());
            auto data = make_monrace();
            data["blows"] = nlohmann::json::array({ { { "method", "HIT" }, { "effect", "HURT" } } });
            data["message"] = nlohmann::json::array({ { { "action", "SPEAK_ALL" }, { "chance", 1 }, { "message", { { "ja", "test" }, { "en", "test" } } } } });
            data[nlohmann::json::json_pointer(path)] = value;
            auto session = RaceReader::create_session();
            CHECK(test::to_error_code(RaceReader(data, session).read()) == ParseErrorType::INVALID_TYPE);
            CHECK(MonraceList::get_instance().empty());
        }
    }
}

TEST_CASE("RaceReader limits experience to the same cap as evolution experience")
{
    auto session = RaceReader::create_session();
    test::MonraceListTestAccess guard;
    auto data = make_monrace();
    for (const auto exp : { 0, 9999999 }) {
        CAPTURE(exp);
        data["exp"] = exp;
        REQUIRE(read_as_first_record(data, session).has_value());
        CHECK(test_monrace().mexp == exp);
    }
    for (const auto exp : { -1, 10000000 }) {
        CAPTURE(exp);
        data["exp"] = exp;
        CHECK(test::to_error_code(read_as_first_record(data, session)) == ParseErrorType::INVALID_VALUE);
        CHECK(test_monrace().mexp == 9999999);
    }
}
