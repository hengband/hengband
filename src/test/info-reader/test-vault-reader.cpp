#include "info-reader/info-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/vault-reader.h"
#include "room/rooms-vault.h"
#include "util/finalizer.h"
#include <doctest/doctest.h>
#include <limits>
#include <nlohmann/json.hpp>

namespace {
nlohmann::json make_vault()
{
    return { { "id", 0 }, { "name", "Test vault" }, { "type", 7 }, { "rating", 5 }, { "height", 2 }, { "width", 4 }, { "layout", { " %: ", "\\\"# " } } };
}

auto preserve_vaults()
{
    return util::make_finalizer([saved = vaults_info, index = error_idx] {
        vaults_info = saved;
        error_idx = index;
    });
}
}

TEST_CASE("VaultReader preserves metadata and every layout byte")
{
    const auto restore = preserve_vaults();
    vaults_info.clear();
    error_idx = -1;
    auto data = make_vault();
    for (const auto type : { 7, 8, 17 }) {
        data["id"] = type; // Sparse IDs remain supported.
        data["type"] = type;
        REQUIRE(VaultReader(data).read() == PARSE_ERROR_NONE);
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
    const auto restore = preserve_vaults();
    vaults_info.assign(1, {});
    vaults_info[0].name = "unchanged";
    vaults_info[0].text = "sentinel";
    error_idx = -1;
    const auto original = make_vault();
    auto check_invalid = [&](const nlohmann::json &data, std::string_view path) {
        VaultReader reader(data);
        CHECK(reader.read() != PARSE_ERROR_NONE);
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
    const auto restore = preserve_vaults();
    vaults_info.clear();
    error_idx = -1;
    auto data = make_vault();
    data["id"] = 2;
    REQUIRE(VaultReader(data).read() == PARSE_ERROR_NONE);
    CHECK(VaultReader(data).read() == PARSE_ERROR_NON_SEQUENTIAL_RECORDS);
    data["id"] = 1;
    CHECK(VaultReader(data).read() == PARSE_ERROR_NON_SEQUENTIAL_RECORDS);
    data["id"] = 3;
    data["layout"][0] = "bad";
    VaultReader reader(data);
    CHECK(reader.read() != PARSE_ERROR_NONE);
    CHECK(error_idx == 2);
    data["layout"][0] = "good";
    CHECK(reader.read() == PARSE_ERROR_NONE);
    CHECK_FALSE(reader.error().has_value());
    CHECK(error_idx == 3);
}
