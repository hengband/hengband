#include "floor/floor-base-definitions.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/vault-reader.h"
#include "locale/language-switcher.h"
#include "room/rooms-vault.h"
#include "test/info-reader/parse-result-helpers.h"
#include "test/info-reader/scoped-reader-state.h"
#include "test/scoped-restore.h"
#include <doctest/doctest.h>
#include <limits>
#include <nlohmann/json.hpp>

namespace {
nlohmann::json make_vault()
{
    return { { "id", 0 }, { "name", "Test vault" }, { "type", 7 }, { "rating", 5 }, { "height", 2 }, { "width", 4 }, { "layout", { " %: ", "\\\"# " } } };
}
}

TEST_CASE("VaultReader preserves metadata and every layout byte")
{
    const auto restore = test::scoped_restore(vaults_info, error_idx);
    vaults_info.clear();
    error_idx = -1;
    auto data = make_vault();
    for (const auto type : { 7, 8, 17 }) {
        data["id"] = type; // Sparse IDs remain supported.
        data["type"] = type;
        REQUIRE(VaultReader(data).read().has_value());
        const auto &vault = vaults_info.at(type);
        CHECK(vault.idx == type);
        CHECK(vault.typ == type);
        CHECK(vault.rat == 5);
        CHECK(vault.hgt == 2);
        CHECK(vault.wid == 4);
        CHECK(vault.name == "Test vault");
        CHECK(vault.text == " %: \\\"# ");
        CHECK(error_idx == type);
    }
}

TEST_CASE("VaultReader rejects invalid records atomically with field diagnostics")
{
    const auto restore = test::scoped_restore(vaults_info, error_idx);
    vaults_info.assign(1, {});
    vaults_info[0].name = "unchanged";
    vaults_info[0].text = "sentinel";
    error_idx = -1;
    const auto original = make_vault();
    auto check_invalid = [&](const nlohmann::json &data, std::string_view path) {
        VaultReader reader(data);
        CHECK_FALSE(reader.read().has_value());
        REQUIRE(reader.error().has_value());
        CHECK(reader.error()->path == path);
        CHECK_FALSE(reader.error()->reason.empty());
        REQUIRE(vaults_info.size() == 1);
        CHECK(vaults_info[0].name == "unchanged");
        CHECK(vaults_info[0].text == "sentinel");
        CHECK(error_idx == -1);
    };
    check_invalid(nlohmann::json::array(), "$");
    for (const auto key : { "id", "name", "type", "rating", "height", "width", "layout" }) {
        auto data = original;
        data.erase(key);
        check_invalid(data, std::string("$.") + key);
    }
    for (const auto key : { "id", "type", "rating", "height", "width" }) {
        for (const auto &value : std::vector<nlohmann::json>{ nullptr, true, "1", 1.5, -1, std::numeric_limits<uint64_t>::max() }) {
            auto data = original;
            data[key] = value;
            check_invalid(data, std::string("$.") + key);
        }
    }
    for (const auto &value : std::vector<nlohmann::json>{ "", nullptr, 42 }) {
        auto data = original;
        data["name"] = value;
        check_invalid(data, "$.name");
    }
#ifdef JP
    {
        // 日本語版では名前をシステムの文字コードに変換するので、変換できない名前 (途中に '\0' を含むなど) も不正とする
        auto data = original;
        data["name"] = std::string("Test\0vault", 10);
        check_invalid(data, "$.name");
    }
#endif
    for (const auto &value : std::vector<nlohmann::json>{ nullptr, "abcd", nlohmann::json::array(), { "only" } }) {
        auto data = original;
        data["layout"] = value;
        check_invalid(data, "$.layout");
    }
    for (const auto &value : std::vector<nlohmann::json>{ nullptr, 1, "", "abc", "abcde", "ab\tc", std::string("a\0bc", 4), "\xc3\xa9"
                                                                                                                            "ab" }) {
        auto data = original;
        data["layout"][1] = value;
        check_invalid(data, "$.layout[1]");
    }
    for (const auto &[key, value] : std::vector<std::pair<std::string, int>>{ { "id", 32768 }, { "type", 9 }, { "height", 0 }, { "width", 0 }, { "height", 67 }, { "width", 199 } }) {
        auto data = original;
        auto &field = data.at(key);
        field = value;
        check_invalid(data, "$." + key);
    }
    auto data = original;
    data["typo"] = 0;
    check_invalid(data, "$.typo");
}

TEST_CASE("VaultReader rejects duplicate and descending IDs and permits retry")
{
    const auto restore = test::scoped_restore(vaults_info, error_idx);
    vaults_info.clear();
    error_idx = -1;
    auto data = make_vault();
    data["id"] = 2;
    REQUIRE(VaultReader(data).read().has_value());
    CHECK(test::to_error_code(VaultReader(data).read()) == PARSE_ERROR_NON_SEQUENTIAL_RECORDS);
    data["id"] = 1;
    CHECK(test::to_error_code(VaultReader(data).read()) == PARSE_ERROR_NON_SEQUENTIAL_RECORDS);
    data["id"] = 3;
    data["layout"][0] = "bad";
    VaultReader reader(data);
    CHECK_FALSE(reader.read().has_value());
    CHECK(error_idx == 2);
    data["layout"][0] = "good";
    CHECK(reader.read().has_value());
    CHECK_FALSE(reader.error().has_value());
    CHECK(error_idx == 3);
    data["id"] = 4;
    data["rating"] = "bad";
    CHECK(test::to_error_code(reader.read()) == PARSE_ERROR_INVALID_TYPE);
    REQUIRE(reader.error().has_value());
    CHECK(reader.error()->id == "4");
    CHECK(reader.error()->path == "$.rating");
    REQUIRE(vaults_info.size() == 4);
    CHECK(vaults_info[3].text == "good\\\"# ");
    CHECK(error_idx == 3);
    data["rating"] = 8;
    CHECK(reader.read().has_value());
    CHECK_FALSE(reader.error().has_value());
    REQUIRE(vaults_info.size() == 5);
    CHECK(vaults_info[4].rat == 8);
    CHECK(error_idx == 4);
}

TEST_CASE("VaultReader preserves exact error precedence and every existing record field")
{
    const auto restore = test::scoped_restore(vaults_info);
    const test::ScopedReaderState reader_state;
    vaults_info.clear();
    auto data = make_vault();
    data["id"] = 2;
    REQUIRE(VaultReader(data).read().has_value());
    const auto previous = vaults_info;
    data["id"] = 5;
    data["name"] = "Changed vault";
    auto expected = PARSE_ERROR_TOO_FEW_ARGUMENTS;
    std::string id = "5";
    std::string path;
    std::string reason;
    SUBCASE("required root keys before unknown keys")
    {
        data.erase("rating");
        data["typo"] = 42;
        path = "$.rating";
        reason = _("必須項目がありません", "missing required field");
    }
    SUBCASE("unknown root keys before id conversion")
    {
        data["typo"] = 42;
        data["id"] = "bad";
        expected = PARSE_ERROR_UNDEFINED_DIRECTIVE;
        id = "\"bad\"";
        path = "$.typo";
        reason = _("未知の項目です", "unknown field");
    }
    SUBCASE("id conversion before name validation")
    {
        data["id"] = "bad";
        data["name"] = 42;
        expected = PARSE_ERROR_INVALID_TYPE;
        id = "\"bad\"";
        path = "$.id";
        reason = _("0以上32767以下の整数が必要です", "expected an integer in [0, 32767]");
    }
    SUBCASE("id order before name validation")
    {
        data["id"] = 2;
        data["name"] = 42;
        expected = PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
        id = "2";
        path = "$.id";
        reason = _("IDは重複せず昇順に並べてください", "IDs must be unique and in increasing order");
    }
    SUBCASE("vault kind before rating")
    {
        data["type"] = 9;
        data["rating"] = nullptr;
        expected = PARSE_ERROR_INVALID_VALUE;
        path = "$.type";
        reason = _("Vault種別は7、8、17のいずれかです", "vault type must be 7, 8 or 17");
    }
    SUBCASE("rating before dimensions")
    {
        data["rating"] = nullptr;
        data["height"] = 0;
        path = "$.rating";
        reason = _("0以上2147483647以下の整数が必要です", "expected an integer in [0, 2147483647]");
    }
    SUBCASE("height before width")
    {
        data["height"] = 0;
        data["width"] = 0;
        expected = PARSE_ERROR_INVALID_FLAG;
        path = "$.height";
        reason = _("1以上66以下の整数が必要です", "expected an integer in [1, 66]");
    }
    SUBCASE("width before layout")
    {
        data["width"] = MAX_WID + 1;
        data["layout"] = nullptr;
        expected = PARSE_ERROR_INVALID_FLAG;
        path = "$.width";
        reason = _("1以上198以下の整数が必要です", "expected an integer in [1, 198]");
    }
    SUBCASE("second layout row after successfully parsed metadata and first row")
    {
        data["layout"][1] = 42;
        expected = PARSE_ERROR_INVALID_TYPE;
        path = "$.layout[1]";
        reason = _("文字列が必要です", "expected a string");
    }
    VaultReader reader(data);
    CHECK(test::to_error_code(reader.read()) == expected);
    REQUIRE(reader.error().has_value());
    CHECK(reader.error()->id == id);
    CHECK(reader.error()->path == path);
    CHECK(reader.error()->reason == reason);
    CHECK(error_idx == 2);
    REQUIRE(vaults_info.size() == previous.size());
    for (size_t i = 0; i < previous.size(); ++i) {
        CAPTURE(i);
        CHECK(vaults_info[i].idx == previous[i].idx);
        CHECK(vaults_info[i].typ == previous[i].typ);
        CHECK(vaults_info[i].rat == previous[i].rat);
        CHECK(vaults_info[i].hgt == previous[i].hgt);
        CHECK(vaults_info[i].wid == previous[i].wid);
        CHECK(vaults_info[i].name == previous[i].name);
        CHECK(vaults_info[i].text == previous[i].text);
    }
}
