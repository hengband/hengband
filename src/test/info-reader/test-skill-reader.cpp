/*!
 * @brief 職業技能JSON Readerの境界値・原子性テスト
 */
#include "info-reader/info-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/skill-reader.h"
#include "locale/language-switcher.h"
#include "object/tval-types.h"
#include "player/player-skill.h"
#include "util/finalizer.h"
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

auto preserve_skills()
{
    return util::make_finalizer([saved = class_skills_info, index = error_idx] {
        class_skills_info = saved;
        error_idx = index;
    });
}
}

TEST_CASE("SkillReader converts all weapon ranks and preserves raw skill experience")
{
    const auto restore = preserve_skills();
    class_skills_info.assign(29, {});
    error_idx = -1;
    auto data = make_class();
    for (int rank = 0; rank < 5; ++rank) {
        data["weapons"]["BOW"]["start_ranks"][rank] = rank;
        data["weapons"]["BOW"]["max_ranks"][rank] = rank;
    }
    data["skills"]["RIDING"] = { { "start_exp", 500 }, { "max_exp", 5000 } };
    const auto err = SkillReader(data).read();
    REQUIRE(err == PARSE_ERROR_NONE);
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
    const auto restore = preserve_skills();
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
    const auto err = reader.read();
    CHECK(err != PARSE_ERROR_NONE);
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
    const auto restore = preserve_skills();
    class_skills_info.assign(29, {});
    error_idx = -1;
    auto data = make_class();
    auto err = SkillReader(data).read();
    REQUIRE(err == PARSE_ERROR_NONE);
    err = SkillReader(data).read();
    CHECK(err == PARSE_ERROR_NON_SEQUENTIAL_RECORDS);
    data["id"] = 2;
    err = SkillReader(data).read();
    CHECK(err == PARSE_ERROR_NON_SEQUENTIAL_RECORDS);
    CHECK(error_idx == 0);
    for (int id = 1; id < 29; ++id) {
        data["id"] = id;
        err = SkillReader(data).read();
        CHECK(err == PARSE_ERROR_NONE);
    }
    CHECK(error_idx == 28);
    class_skills_info.assign(29, {});
    error_idx = -1;
    data["id"] = 0;
    err = SkillReader(data).read();
    CHECK(err == PARSE_ERROR_NONE);
}

TEST_CASE("SkillReader diagnostics identify the class field and cause")
{
    const auto restore = preserve_skills();
    class_skills_info.assign(29, {});
    error_idx = 6;
    auto data = make_class();
    data["id"] = 7;
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
        data["weapons"]["SWORD"]["typo"] = 1;
        path = "$.weapons.SWORD.typo";
        reason = _("未知の項目です", "unknown field");
    }
    SUBCASE("wrong object type")
    {
        data["skills"] = nlohmann::json::array();
        path = "$.skills";
        reason = _("オブジェクトが必要です", "expected an object");
    }
    SUBCASE("wrong integer type")
    {
        data["weapons"]["SWORD"]["start_ranks"][63] = "4";
        path = "$.weapons.SWORD.start_ranks[63]";
        reason = _("整数が必要です", "expected an integer");
    }
    SUBCASE("out of range rank")
    {
        data["weapons"]["SWORD"]["max_ranks"][63] = 5;
        path = "$.weapons.SWORD.max_ranks[63]";
        reason = _("0以上4以下の整数が必要です", "expected an integer in [0, 4]");
    }
    SUBCASE("invalid array type")
    {
        data["weapons"]["SWORD"]["max_ranks"] = 4;
        path = "$.weapons.SWORD.max_ranks";
        reason = _("配列が必要です", "expected an array");
    }
    SUBCASE("wrong array length")
    {
        data["weapons"]["SWORD"]["start_ranks"].erase(63);
        path = "$.weapons.SWORD.start_ranks";
        reason = _("要素数は64個が必要ですが、63個あります", "expected 64 entries, got 63");
    }
    SUBCASE("rank above maximum")
    {
        data["weapons"]["SWORD"]["start_ranks"][63] = 4;
        data["weapons"]["SWORD"]["max_ranks"][63] = 3;
        path = "$.weapons.SWORD.start_ranks[63]";
        reason = _("初期ランク4が上限3を超えています", "start rank 4 exceeds maximum 3");
    }
    SUBCASE("experience above maximum")
    {
        data["skills"]["SHIELD"]["start_exp"] = 7000;
        data["skills"]["SHIELD"]["max_exp"] = 6000;
        path = "$.skills.SHIELD.start_exp";
        reason = _("初期経験値7000が上限6000を超えています", "start experience 7000 exceeds maximum 6000");
    }
    SUBCASE("out of range experience")
    {
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
        data["id"] = 29;
        class_id = "29";
        path = "$.id";
        reason = _("0以上28以下の整数が必要です", "expected an integer in [0, 28]");
    }
    SUBCASE("duplicate class")
    {
        data["id"] = 6;
        class_id = "6";
        path = "$.id";
        reason = _("職業ID 7が必要です（職業の重複または欠落）", "expected class id 7 (duplicate or missing class)");
    }
    SUBCASE("non object record")
    {
        data = nullptr;
        class_id = "<unknown>";
        path = "$";
        reason = _("オブジェクトが必要です", "expected an object");
    }
    SkillReader reader(data);
    const auto err = reader.read();
    CHECK(err != PARSE_ERROR_NONE);
    REQUIRE(reader.error().has_value());
    CHECK(reader.error()->class_id == class_id);
    CHECK(reader.error()->path == path);
    CHECK(reader.error()->reason == reason);
    CHECK(error_idx == 6);
    CHECK(class_skills_info[7].w_start.empty());
}

TEST_CASE("SkillReader clears stale diagnostics before another read")
{
    const auto restore = preserve_skills();
    class_skills_info.assign(29, {});
    error_idx = -1;
    auto data = make_class();
    SkillReader reader(data);
    CHECK_FALSE(reader.error().has_value());
    data["skills"]["RIDING"]["max_exp"] = 8001;
    auto err = reader.read();
    REQUIRE(err != PARSE_ERROR_NONE);
    REQUIRE(reader.error().has_value());
    CHECK(reader.error()->path == "$.skills.RIDING.max_exp");
    data["skills"]["RIDING"]["max_exp"] = 8000;
    err = reader.read();
    CHECK(err == PARSE_ERROR_NONE);
    CHECK_FALSE(reader.error().has_value());
    // 再度失敗した際も前回の技能エラーを残さない。
    err = reader.read();
    REQUIRE(err != PARSE_ERROR_NONE);
    REQUIRE(reader.error().has_value());
    CHECK(reader.error()->path == "$.id");
}
