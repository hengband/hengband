/*!
 * @brief 職業技能JSON Readerの境界値・原子性テスト
 */
#include "info-reader/info-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/skill-reader.h"
#include "locale/language-switcher.h"
#include "object/tval-types.h"
#include "player/player-skill.h"
#include "test/info-reader/parse-result-helpers.h"
#include "test/info-reader/scoped-reader-state.h"
#include "test/scoped-restore.h"
#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

namespace {
nlohmann::json make_class()
{
    nlohmann::json data = { { "id", 0 }, { "weapons", nlohmann::json::object() }, { "skills", nlohmann::json::object() } };
    for (const auto *name : { "BOW", "DIGGING", "HAFTED", "POLEARM", "SWORD" }) {
        data["weapons"][name] = { { "start_ranks", std::vector<int>(64, 0) }, { "max_ranks", std::vector<int>(64, 4) } };
    }
    for (const auto *name : { "MARTIAL_ARTS", "TWO_WEAPON", "RIDING", "SHIELD" }) {
        data["skills"][name] = { { "start_exp", 0 }, { "max_exp", 8000 } };
    }
    return data;
}
}

TEST_CASE("SkillReader converts all weapon ranks and preserves raw skill experience")
{
    const auto restore = test::scoped_restore(class_skills_info, error_idx);
    class_skills_info.assign(29, {});
    error_idx = -1;
    auto data = make_class();
    for (int rank = 0; rank < 5; ++rank) {
        data["weapons"]["BOW"]["start_ranks"][rank] = rank;
        data["weapons"]["BOW"]["max_ranks"][rank] = rank;
    }
    data["skills"]["RIDING"] = { { "start_exp", 500 }, { "max_exp", 5000 } };
    const auto result = SkillReader(data).read();
    REQUIRE(result.has_value());
    CHECK(error_idx == 0);
    constexpr int expected[] = { 0, 4000, 6000, 7000, 8000 };
    for (size_t rank = 0; rank < 5; ++rank) {
        CHECK(class_skills_info[0].w_start.at(ItemKindType::BOW)[rank] == expected[rank]);
        CHECK(class_skills_info[0].w_max.at(ItemKindType::BOW)[rank] == expected[rank]);
    }
    for (const auto tval : { ItemKindType::BOW, ItemKindType::DIGGING, ItemKindType::HAFTED, ItemKindType::POLEARM, ItemKindType::SWORD }) {
        CHECK(class_skills_info[0].w_start.at(tval)[63] == 0);
        CHECK(class_skills_info[0].w_max.at(tval)[63] == 8000);
    }
    CHECK(class_skills_info[0].s_start.at(PlayerSkillKindType::RIDING) == 500);
    CHECK(class_skills_info[0].s_max.at(PlayerSkillKindType::RIDING) == 5000);
}

TEST_CASE("SkillReader rejects malformed records without modifying the destination")
{
    const auto restore = test::scoped_restore(class_skills_info, error_idx);
    class_skills_info.assign(29, {});
    class_skills_info[0].s_start[PlayerSkillKindType::SHIELD] = 123;
    error_idx = -1;
    auto data = make_class();
    SUBCASE("non object")
    {
        data = nlohmann::json::array();
    }
    SUBCASE("missing id")
    {
        data.erase("id");
    }
    SUBCASE("negative id")
    {
        data["id"] = -1;
    }
    SUBCASE("id beyond table")
    {
        data["id"] = 29;
    }
    SUBCASE("string id")
    {
        data["id"] = "0";
    }
    SUBCASE("huge unsigned id")
    {
        data["id"] = UINT64_MAX;
    }
    SUBCASE("fractional id")
    {
        data["id"] = 0.5;
    }
    SUBCASE("missing weapon")
    {
        data["weapons"].erase("SWORD");
    }
    SUBCASE("unknown weapon")
    {
        data["weapons"]["UNKNOWN"] = data["weapons"]["BOW"];
    }
    SUBCASE("missing rank field")
    {
        data["weapons"]["BOW"].erase("start_ranks");
    }
    SUBCASE("short rank array")
    {
        data["weapons"]["BOW"]["start_ranks"].erase(63);
    }
    SUBCASE("long rank array")
    {
        data["weapons"]["BOW"]["max_ranks"].push_back(4);
    }
    SUBCASE("non array ranks")
    {
        data["weapons"]["BOW"]["start_ranks"] = 0;
    }
    SUBCASE("negative rank")
    {
        data["weapons"]["BOW"]["start_ranks"][0] = -1;
    }
    SUBCASE("rank above master")
    {
        data["weapons"]["BOW"]["max_ranks"][63] = 5;
    }
    SUBCASE("null rank")
    {
        data["weapons"]["BOW"]["start_ranks"][0] = nullptr;
    }
    SUBCASE("boolean rank")
    {
        data["weapons"]["BOW"]["start_ranks"][0] = false;
    }
    SUBCASE("fractional rank")
    {
        data["weapons"]["BOW"]["start_ranks"][0] = 1.5;
    }
    SUBCASE("start rank above maximum")
    {
        data["weapons"]["SWORD"]["start_ranks"][63] = 4;
        data["weapons"]["SWORD"]["max_ranks"][63] = 3;
    }
    SUBCASE("missing skill")
    {
        data["skills"].erase("SHIELD");
    }
    SUBCASE("unknown skill")
    {
        data["skills"]["OTHER"] = data["skills"]["SHIELD"];
    }
    SUBCASE("missing experience")
    {
        data["skills"]["SHIELD"].erase("max_exp");
    }
    SUBCASE("negative experience")
    {
        data["skills"]["SHIELD"]["start_exp"] = -1;
    }
    SUBCASE("excess experience")
    {
        data["skills"]["SHIELD"]["max_exp"] = 8001;
    }
    SUBCASE("fractional experience")
    {
        data["skills"]["SHIELD"]["start_exp"] = 0.5;
    }
    SUBCASE("start experience above maximum")
    {
        data["skills"]["SHIELD"] = { { "start_exp", 500 }, { "max_exp", 499 } };
    }
    SkillReader reader(data);
    const auto result = reader.read();
    CHECK_FALSE(result.has_value());
    REQUIRE(reader.error().has_value());
    CHECK_FALSE(reader.error()->path.empty());
    CHECK_FALSE(reader.error()->reason.empty());
    CHECK(error_idx == -1);
    CHECK(class_skills_info[0].w_start.empty());
    CHECK(class_skills_info[0].w_max.empty());
    CHECK(class_skills_info[0].s_start.size() == 1);
    CHECK(class_skills_info[0].s_start.at(PlayerSkillKindType::SHIELD) == 123);
    CHECK(class_skills_info[0].s_max.empty());
}

TEST_CASE("SkillReader rejects duplicate and missing class ids and can be reused after reset")
{
    const auto restore = test::scoped_restore(class_skills_info, error_idx);
    class_skills_info.assign(29, {});
    error_idx = -1;
    auto data = make_class();
    auto result = SkillReader(data).read();
    REQUIRE(result.has_value());
    result = SkillReader(data).read();
    CHECK(test::to_error_code(result) == ParseErrorType::NON_SEQUENTIAL_RECORDS);
    data["id"] = 2;
    result = SkillReader(data).read();
    CHECK(test::to_error_code(result) == ParseErrorType::NON_SEQUENTIAL_RECORDS);
    CHECK(error_idx == 0);
    for (int id = 1; id < 29; ++id) {
        data["id"] = id;
        result = SkillReader(data).read();
        CHECK(result.has_value());
    }
    CHECK(error_idx == 28);
    class_skills_info.assign(29, {});
    error_idx = -1;
    data["id"] = 0;
    result = SkillReader(data).read();
    CHECK(result.has_value());
}

TEST_CASE("SkillReader diagnostics identify the class field and cause")
{
    const auto restore = test::scoped_restore(class_skills_info);
    const test::ScopedReaderState reader_state(6);
    class_skills_info.assign(29, {});
    auto &existing = class_skills_info[7];
    existing.w_start[ItemKindType::BOW][0] = 111;
    existing.w_max[ItemKindType::SWORD][63] = 222;
    existing.s_start[PlayerSkillKindType::RIDING] = 333;
    existing.s_max[PlayerSkillKindType::SHIELD] = 444;
    class_skills_info[6].s_start[PlayerSkillKindType::SHIELD] = 555;
    class_skills_info[8].s_start[PlayerSkillKindType::SHIELD] = 666;
    const auto previous = class_skills_info;
    auto data = make_class();
    data["id"] = 7;
    auto expected = ParseErrorType::TOO_FEW_ARGUMENTS;
    std::string path;
    std::string reason;
    std::string class_id = "7";
    SUBCASE("missing field")
    {
        data["skills"]["SHIELD"].erase("max_exp");
        path = "$.skills.SHIELD.max_exp";
        reason = _("必須項目がありません", "missing required field");
    }
    SUBCASE("unknown field")
    {
        expected = ParseErrorType::UNDEFINED_DIRECTIVE;
        data["weapons"]["SWORD"]["typo"] = 1;
        path = "$.weapons.SWORD.typo";
        reason = _("未知の項目です", "unknown field");
    }
    SUBCASE("wrong object type")
    {
        expected = ParseErrorType::INVALID_TYPE;
        data["skills"] = nlohmann::json::array();
        path = "$.skills";
        reason = _("オブジェクトが必要です", "expected an object");
    }
    SUBCASE("wrong integer type")
    {
        expected = ParseErrorType::INVALID_TYPE;
        data["weapons"]["SWORD"]["start_ranks"][63] = "4";
        path = "$.weapons.SWORD.start_ranks[63]";
        reason = _("整数が必要です", "expected an integer");
    }
    SUBCASE("out of range rank")
    {
        expected = ParseErrorType::INVALID_VALUE;
        data["weapons"]["SWORD"]["max_ranks"][63] = 5;
        path = "$.weapons.SWORD.max_ranks[63]";
        reason = _("0以上4以下の整数が必要です", "expected an integer in [0, 4]");
    }
    SUBCASE("invalid array type")
    {
        expected = ParseErrorType::INVALID_TYPE;
        data["weapons"]["SWORD"]["max_ranks"] = 4;
        path = "$.weapons.SWORD.max_ranks";
        reason = _("配列が必要です", "expected an array");
    }
    SUBCASE("wrong array length")
    {
        expected = ParseErrorType::INVALID_VALUE;
        data["weapons"]["SWORD"]["start_ranks"].erase(63);
        path = "$.weapons.SWORD.start_ranks";
        reason = _("要素数は64個が必要ですが、63個あります", "expected 64 entries, got 63");
    }
    SUBCASE("rank above maximum")
    {
        expected = ParseErrorType::INVALID_VALUE;
        data["weapons"]["SWORD"]["start_ranks"][63] = 4;
        data["weapons"]["SWORD"]["max_ranks"][63] = 3;
        path = "$.weapons.SWORD.start_ranks[63]";
        reason = _("初期ランク4が上限3を超えています", "start rank 4 exceeds maximum 3");
    }
    SUBCASE("experience above maximum")
    {
        expected = ParseErrorType::INVALID_VALUE;
        data["skills"]["SHIELD"]["start_exp"] = 7000;
        data["skills"]["SHIELD"]["max_exp"] = 6000;
        path = "$.skills.SHIELD.start_exp";
        reason = _("初期経験値7000が上限6000を超えています", "start experience 7000 exceeds maximum 6000");
    }
    SUBCASE("out of range experience")
    {
        expected = ParseErrorType::INVALID_VALUE;
        data["skills"]["SHIELD"]["max_exp"] = 8001;
        path = "$.skills.SHIELD.max_exp";
        reason = _("0以上8000以下の整数が必要です", "expected an integer in [0, 8000]");
    }
    SUBCASE("missing id")
    {
        data.erase("id");
        class_id = "<unknown>";
        path = "$.id";
        reason = _("必須項目がありません", "missing required field");
    }
    SUBCASE("invalid id")
    {
        expected = ParseErrorType::INVALID_VALUE;
        data["id"] = 29;
        class_id = "29";
        path = "$.id";
        reason = _("0以上28以下の整数が必要です", "expected an integer in [0, 28]");
    }
    SUBCASE("duplicate class before weapon validation")
    {
        expected = ParseErrorType::NON_SEQUENTIAL_RECORDS;
        data["id"] = 6;
        data["weapons"] = nullptr;
        class_id = "6";
        path = "$.id";
        reason = _("職業ID 7が必要です（職業の重複または欠落）", "expected class id 7 (duplicate or missing class)");
    }
    SUBCASE("non object record")
    {
        expected = ParseErrorType::INVALID_TYPE;
        data = nullptr;
        class_id = "<unknown>";
        path = "$";
        reason = _("オブジェクトが必要です", "expected an object");
    }
    SUBCASE("required root keys before unknown keys")
    {
        data.erase("weapons");
        data["typo"] = 42;
        path = "$.weapons";
        reason = _("必須項目がありません", "missing required field");
    }
    SUBCASE("root keys before id conversion")
    {
        data["typo"] = 42;
        data["id"] = "bad";
        expected = ParseErrorType::UNDEFINED_DIRECTIVE;
        class_id = "\"bad\"";
        path = "$.typo";
        reason = _("未知の項目です", "unknown field");
    }
    SUBCASE("id conversion before weapon validation")
    {
        data["id"] = "bad";
        data["weapons"] = nullptr;
        expected = ParseErrorType::INVALID_TYPE;
        class_id = "\"bad\"";
        path = "$.id";
        reason = _("整数が必要です", "expected an integer");
    }
    SUBCASE("late weapon failure before skills validation")
    {
        data["weapons"]["SWORD"]["start_ranks"][63] = 4;
        data["weapons"]["SWORD"]["max_ranks"][63] = 3;
        data["skills"] = nullptr;
        expected = ParseErrorType::INVALID_VALUE;
        path = "$.weapons.SWORD.start_ranks[63]";
        reason = _("初期ランク4が上限3を超えています", "start rank 4 exceeds maximum 3");
    }
    SkillReader reader(data);
    const auto result = reader.read();
    CHECK(test::to_error_code(result) == expected);
    REQUIRE(reader.error().has_value());
    CHECK(reader.error()->class_id == class_id);
    CHECK(reader.error()->path == path);
    CHECK(reader.error()->reason == reason);
    CHECK(error_idx == 6);
    REQUIRE(class_skills_info.size() == previous.size());
    for (size_t i = 0; i < previous.size(); ++i) {
        CAPTURE(i);
        CHECK(class_skills_info[i].w_start == previous[i].w_start);
        CHECK(class_skills_info[i].w_max == previous[i].w_max);
        CHECK(class_skills_info[i].s_start == previous[i].s_start);
        CHECK(class_skills_info[i].s_max == previous[i].s_max);
    }
}

TEST_CASE("SkillReader clears stale diagnostics before another read")
{
    const auto restore = test::scoped_restore(class_skills_info, error_idx);
    class_skills_info.assign(29, {});
    error_idx = -1;
    auto data = make_class();
    SkillReader reader(data);
    CHECK_FALSE(reader.error().has_value());
    data["skills"]["RIDING"]["max_exp"] = 8001;
    auto result = reader.read();
    REQUIRE(!result.has_value());
    REQUIRE(reader.error().has_value());
    CHECK(reader.error()->path == "$.skills.RIDING.max_exp");
    data["skills"]["RIDING"]["max_exp"] = 8000;
    result = reader.read();
    CHECK(result.has_value());
    CHECK_FALSE(reader.error().has_value());
    // 再度失敗した際も前回の技能エラーを残さない。
    result = reader.read();
    REQUIRE(!result.has_value());
    REQUIRE(reader.error().has_value());
    CHECK(reader.error()->path == "$.id");
}
