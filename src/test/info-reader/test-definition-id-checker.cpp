#include "info-reader/definition-id-checker.h"
#include "locale/language-switcher.h"
#include "view/display-messages.h"
#include <cstdint>
#include <doctest/doctest.h>
#include <fmt/format.h>
#include <limits>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <vector>

TEST_CASE("DefinitionIdChecker sequential IDs and commit")
{
    DefinitionIdChecker checker(DefinitionIdOrder::SEQUENTIAL, { 1, 3 }, "artifact");
    CHECK_THROWS_AS(checker.commit(), std::logic_error);
    CHECK(checker.check(2).error().code == ParseErrorType::NON_SEQUENTIAL_RECORDS);
    CHECK(checker.check(2).error().reason == fmt::format(_("{}の最初の ID は {} です", "The first {} ID must be {}"), "artifact", 1));
    for (int id = 1; id <= 3; ++id) {
        REQUIRE(checker.read(id));
        CHECK(checker.last_id() == (id == 1 ? tl::nullopt : tl::optional<int>(id - 1)));
        REQUIRE(checker.check(id));
        checker.commit();
        CHECK(checker.last_id() == id);
        CHECK(checker.check(id).error().code == ParseErrorType::NON_SEQUENTIAL_RECORDS);
    }
    DefinitionIdChecker seeded(DefinitionIdOrder::SEQUENTIAL, { 1, 4 }, "artifact", 2);
    CHECK(seeded.check(4).error().reason == fmt::format(_("{}の ID '{}' は直前の ID '{}' の次（{}）ではありません", "{} ID '{}' does not follow previous ID '{}' (expected {})"), "artifact", 4, 2, 3));
    REQUIRE(seeded.check(3));
    seeded.commit();
    CHECK(seeded.last_id() == 3);
}

TEST_CASE("DefinitionIdChecker increasing and unordered IDs")
{
    DefinitionIdChecker increasing(DefinitionIdOrder::STRICTLY_INCREASING, { 0, 10 }, "race");
    REQUIRE(increasing.check(2));
    increasing.commit();
    CHECK(increasing.check(2).error().code == ParseErrorType::NON_SEQUENTIAL_RECORDS);
    CHECK(increasing.check(1).error().reason == fmt::format(_("{}の ID '{}' は直前の ID '{}' より大きくありません", "{} ID '{}' is not greater than previous ID '{}'"), "race", 1, 2));
    REQUIRE(increasing.check(10));
    increasing.commit();
    DefinitionIdChecker unique(DefinitionIdOrder::UNIQUE, { 0, 10 }, "terrain");
    REQUIRE(unique.check(10));
    REQUIRE(unique.check(10));
    CHECK_FALSE(unique.last_id());
    unique.commit();
    REQUIRE(unique.check(0));
    unique.commit();
    CHECK(unique.check(10).error().reason == fmt::format(_("{}の ID '{}' は重複しています", "{} ID '{}' is duplicated"), "terrain", 10));
}

TEST_CASE("DefinitionIdChecker rejects invalid IDs and clears pending")
{
    DefinitionIdChecker checker(DefinitionIdOrder::STRICTLY_INCREASING, { 0, 32766 }, "item");
    CHECK(checker.read(nullptr).error().code == ParseErrorType::TOO_FEW_ARGUMENTS);
    CHECK(checker.read(nullptr).error().reason == fmt::format(_("{}の ID がありません", "Missing {} ID"), "item"));
    std::vector<nlohmann::json> invalid;
    invalid.emplace_back(1.5);
    invalid.emplace_back("1");
    invalid.emplace_back(true);
    for (const auto &value : invalid) {
        CHECK(checker.read(value).error().code == ParseErrorType::INVALID_TYPE);
        CHECK(checker.read(value).error().reason == fmt::format(_("{}の ID は整数ではありません", "{} ID is not an integer"), "item"));
    }
    CHECK(checker.read(std::numeric_limits<std::uint64_t>::max()).error().code == ParseErrorType::INVALID_VALUE);
    CHECK(checker.read(70000).error().reason == fmt::format(_("{}の ID '{}' は {}～{} の範囲外です", "{} ID '{}' is outside the range {} to {}"), "item", "70000", 0, 32766));
    CHECK(checker.check(-1).error().code == ParseErrorType::INVALID_VALUE);
    CHECK(checker.check(32767).error().code == ParseErrorType::INVALID_VALUE);
    REQUIRE(checker.check(0));
    CHECK_FALSE(checker.check(-1));
    CHECK_THROWS_AS(checker.commit(), std::logic_error);
    REQUIRE(checker.read(0));
    CHECK_FALSE(checker.read(nullptr));
    CHECK_THROWS_AS(checker.commit(), std::logic_error);
    REQUIRE(checker.check(0));
    checker.commit();
    REQUIRE(checker.read(32766));
    checker.commit();
    CHECK(checker.last_id() == 32766);
}

TEST_CASE("DefinitionIdChecker read_and_report preserves IDs and diagnostics")
{
    DefinitionIdChecker checker(DefinitionIdOrder::SEQUENTIAL, { 1, 3 }, "item");
    ScopedMessageCapture capture;
    REQUIRE(checker.read_and_report(1) == 1);
    CHECK(capture.messages().empty());
    CHECK_FALSE(checker.last_id());
    checker.commit();
    CHECK(checker.last_id() == 1);

    const std::vector<nlohmann::json> invalid{
        nullptr,
        "1",
        1.5,
        true,
        nlohmann::json::array(),
        nlohmann::json::object(),
        -1,
        4,
        std::numeric_limits<std::uint64_t>::max(),
        1,
        3,
    };
    for (const auto &value : invalid) {
        CAPTURE(value);
        DefinitionIdChecker reference(DefinitionIdOrder::SEQUENTIAL, { 1, 3 }, "item", 1);
        const auto expected = reference.read(value);
        REQUIRE_FALSE(expected);
        const auto count = capture.messages().size();
        const auto result = checker.read_and_report(value);
        REQUIRE_FALSE(result);
        CHECK(result.error() == expected.error().code);
        REQUIRE(capture.messages().size() == count + 1);
        CHECK(capture.messages().back() == expected.error().reason);
        CHECK(checker.last_id() == 1);
        CHECK_THROWS_AS(checker.commit(), std::logic_error);
    }
    REQUIRE(checker.read_and_report(2) == 2);
    checker.commit();
    CHECK(checker.last_id() == 2);
}

TEST_CASE("DefinitionIdChecker read_and_report prints each ordering failure")
{
    for (const auto ordering : { DefinitionIdOrder::SEQUENTIAL, DefinitionIdOrder::STRICTLY_INCREASING, DefinitionIdOrder::UNIQUE }) {
        DefinitionIdChecker checker(ordering, { 1, 3 }, "item");
        DefinitionIdChecker reference(ordering, { 1, 3 }, "item");
        ScopedMessageCapture capture;
        if (ordering == DefinitionIdOrder::SEQUENTIAL) {
            const auto expected = reference.read(2);
            REQUIRE_FALSE(expected);
            CHECK(checker.read_and_report(2).error() == expected.error().code);
            REQUIRE(capture.messages().size() == 1);
            CHECK(capture.messages().back() == expected.error().reason);
        }
        REQUIRE(checker.read_and_report(1));
        checker.commit();
        REQUIRE(reference.read(1));
        reference.commit();
        const auto expected = reference.read(1);
        const auto count = capture.messages().size();
        CHECK(checker.read_and_report(1).error() == expected.error().code);
        REQUIRE(capture.messages().size() == count + 1);
        CHECK(capture.messages().back() == expected.error().reason);
    }
}
