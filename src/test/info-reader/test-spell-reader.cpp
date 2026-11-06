#include "info-reader/parse-error-types.h"
#include "info-reader/spell-reader.h"
#include "system/spell-info-list.h"
#include "test/info-reader/parse-result-helpers.h"
#include "test/info-reader/scoped-reader-state.h"
#include "test/system/spell-info-list-test-access.h"
#include "util/enum-converter.h"
#include <doctest/doctest.h>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>
#include <vector>

TEST_CASE("SpellReader requires arrays for books and spells")
{
    const test::ScopedReaderState reader_state;
    SpellReader::Session session;
    auto &spells = SpellInfoList::get_instance();
    nlohmann::json data = { { "name", "LIFE" }, { "books", nlohmann::json::array() } };

    CHECK(SpellReader(data, spells, session).read().has_value());

    data["books"] = nlohmann::json::array({ { { "spells", nlohmann::json::array() } } });
    CHECK(SpellReader(data, spells, session).read().has_value());

    // A JSON object is iterable too, so these used to be accepted as record arrays.
    data["books"] = { { "first", { { "spells", nlohmann::json::array() } } } };
    CHECK(test::to_error_code(SpellReader(data, spells, session).read()) == ParseErrorType::INVALID_TYPE);

    data["books"] = nlohmann::json::array({ { { "spells", nlohmann::json::object() } } });
    CHECK(test::to_error_code(SpellReader(data, spells, session).read()) == ParseErrorType::INVALID_TYPE);
}

namespace {
nlohmann::json make_spell(int id, const std::string &tag)
{
    return {
        { "spell_id", id },
        { "spell_tag", tag },
        { "name", { { "ja", tag }, { "en", tag } } },
        { "description", { { "ja", tag + " description" }, { "en", tag + " description" } } },
    };
}

nlohmann::json make_realm(const std::string &name, const nlohmann::json &first, const nlohmann::json &second)
{
    return { { "name", name }, { "books", nlohmann::json::array({
                                              { { "spells", first } },
                                              { { "spells", second } },
                                          }) } };
}

std::vector<SpellInfo> snapshot_spells(const SpellInfoList &spells)
{
    std::vector<SpellInfo> saved;
    for (int realm = 0; realm < enum2i(RealmType::MAX); ++realm) {
        for (int id = 0; id < SPELLS_IN_REALM; ++id) {
            saved.push_back(spells.get_spell_info(i2enum<RealmType>(realm), id));
        }
    }
    return saved;
}

void check_spells_unchanged(const SpellInfoList &spells, const std::vector<SpellInfo> &saved)
{
    const auto actual = snapshot_spells(spells);
    REQUIRE(actual.size() == saved.size());
    for (size_t i = 0; i < saved.size(); ++i) {
        CHECK(actual[i].idx == saved[i].idx);
        CHECK(actual[i].tag == saved[i].tag);
        CHECK(actual[i].name == saved[i].name);
        CHECK(actual[i].description == saved[i].description);
    }
}
}

TEST_CASE("SpellReader commits all books only after the whole realm succeeds")
{
    auto &spells = SpellInfoList::get_instance();
    const test::ScopedReaderState reader_state;
    SpellReader::Session session;
    const test::SpellInfoListTestAccess spell_state;

    nlohmann::json data = {
        { "name", "LIFE" },
        { "books", nlohmann::json::array({
                       { { "spells", nlohmann::json::array({ make_spell(0, "old-zero"), make_spell(1, "old-one") }) } },
                       { { "spells", nlohmann::json::array({ make_spell(8, "old-eight") }) } },
                   }) },
    };
    REQUIRE(SpellReader(data, spells, session).read().has_value());
    auto other = SpellInfo();
    other.idx = 3;
    other.tag = "other-realm";
    spells.set_spell_info(RealmType::SORCERY, 3, std::move(other));

    std::vector<std::vector<SpellInfo>> saved;
    for (int realm = 0; realm < enum2i(RealmType::MAX); ++realm) {
        auto &snapshot = saved.emplace_back();
        for (int id = 0; id < SPELLS_IN_REALM; ++id) {
            snapshot.push_back(spells.get_spell_info(i2enum<RealmType>(realm), id));
        }
    }
    const auto check_unchanged = [&] {
        for (int realm = 0; realm < enum2i(RealmType::MAX); ++realm) {
            for (int id = 0; id < SPELLS_IN_REALM; ++id) {
                const auto &actual = spells.get_spell_info(i2enum<RealmType>(realm), id);
                const auto &expected = saved[realm][id];
                CHECK(actual.idx == expected.idx);
                CHECK(actual.tag == expected.tag);
                CHECK(actual.name == expected.name);
                CHECK(actual.description == expected.description);
            }
        }
        CHECK(spells.get_spell_id(RealmType::LIFE, "old-zero") == 0);
        CHECK(spells.get_spell_id(RealmType::LIFE, "old-one") == 1);
        CHECK(spells.get_spell_id(RealmType::LIFE, "old-eight") == 8);
        CHECK_FALSE(spells.get_spell_id(RealmType::LIFE, "new-zero"));
        CHECK_FALSE(spells.get_spell_id(RealmType::LIFE, "new-two"));
    };

    data["books"][0]["spells"] = nlohmann::json::array({ make_spell(0, "new-zero"), make_spell(2, "new-two") });
    SUBCASE("a later spell in the same book is invalid")
    {
        auto invalid = make_spell(4, "invalid");
        invalid.erase("description");
        data["books"][0]["spells"].push_back(invalid);
        CHECK(test::to_error_code(SpellReader(data, spells, session).read()) == ParseErrorType::TOO_FEW_ARGUMENTS);
        check_unchanged();
    }
    SUBCASE("a spell in a later book is invalid")
    {
        data["books"][1]["spells"][0]["spell_id"] = 32;
        CHECK(test::to_error_code(SpellReader(data, spells, session).read()) == ParseErrorType::INVALID_VALUE);
        check_unchanged();
    }
    SUBCASE("a later book has an invalid spells container")
    {
        data["books"][1]["spells"] = nlohmann::json::object();
        CHECK(test::to_error_code(SpellReader(data, spells, session).read()) == ParseErrorType::INVALID_TYPE);
        check_unchanged();
    }
    SUBCASE("successful reload preserves unspecified spells and other realms")
    {
        SpellReader::Session reload_session;
        data["books"][0]["spells"][0] = make_spell(0, "last-zero");
        data["books"][1]["spells"] = nlohmann::json::array({ make_spell(8, "new-eight") });
        REQUIRE(SpellReader(data, spells, reload_session).read().has_value());
        CHECK(spells.get_spell_id(RealmType::LIFE, "last-zero") == 0);
        CHECK_FALSE(spells.get_spell_id(RealmType::LIFE, "old-zero"));
        CHECK_FALSE(spells.get_spell_id(RealmType::LIFE, "new-zero"));
        CHECK(spells.get_spell_id(RealmType::LIFE, "new-two") == 2);
        CHECK(spells.get_spell_id(RealmType::LIFE, "new-eight") == 8);
        CHECK(spells.get_spell_id(RealmType::LIFE, "old-one") == 1);
        CHECK(spells.get_spell_id(RealmType::SORCERY, "other-realm") == 3);
        CHECK(spells.get_spell_info(RealmType::LIFE, 0).name == "last-zero");
        CHECK(spells.get_spell_info(RealmType::LIFE, 0).description == "last-zero description");
        CHECK(spells.get_spell_info(RealmType::LIFE, 2).idx == 2);
        CHECK(spells.get_spell_info(RealmType::LIFE, 8).name == "new-eight");
        data["books"] = nlohmann::json::array();
        CHECK(SpellReader(data, spells, reload_session).read().has_value());
        CHECK(spells.get_spell_id(RealmType::LIFE, "last-zero") == 0);
    }
}

/*!
 * @brief 内部の各解析エラーを伝播し、先行する呪文書を含む既存定義の部分更新を防ぐ
 */
TEST_CASE("SpellReader propagates helper errors without publishing staged spells")
{
    const test::ScopedReaderState reader_state;
    SpellReader::Session session;
    const test::SpellInfoListTestAccess spell_state;
    auto &spells = SpellInfoList::get_instance();
    nlohmann::json original = {
        { "name", "LIFE" },
        { "books", nlohmann::json::array({
                       { { "spells", nlohmann::json::array({ make_spell(0, "old-zero") }) } },
                       { { "spells", nlohmann::json::array({ make_spell(8, "old-eight") }) } },
                   }) },
    };
    REQUIRE(SpellReader(original, spells, session).read().has_value());
    auto other = SpellInfo();
    other.idx = 3;
    other.tag = "other-realm";
    spells.set_spell_info(RealmType::SORCERY, 3, std::move(other));
    auto replacement = original;
    replacement["books"][0]["spells"][0] = make_spell(0, "new-zero");
    replacement["books"][1]["spells"][0] = make_spell(8, "new-eight");

    struct InvalidField {
        const char *path;
        nlohmann::json value;
        ParseErrorType error;
    };
    const std::vector<InvalidField> invalid_fields = {
        { "", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/name", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/name", "UNKNOWN_REALM", ParseErrorType::INVALID_FLAG },
        { "/books", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/books", nlohmann::json::object(), ParseErrorType::INVALID_TYPE },
        { "/books/1/spells", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/books/1/spells", nlohmann::json::object(), ParseErrorType::INVALID_TYPE },
        { "/books/1/spells/0", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/books/1/spells/0/spell_id", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/books/1/spells/0/spell_id", 32, ParseErrorType::INVALID_VALUE },
        { "/books/1/spells/0/spell_id", true, ParseErrorType::INVALID_TYPE },
        { "/books/1/spells/0/spell_tag", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/books/1/spells/0/name", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/books/1/spells/0/name", 7, ParseErrorType::INVALID_TYPE },
        { "/books/1/spells/0/description", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/books/1/spells/0/description", 7, ParseErrorType::INVALID_TYPE },
    };
    for (const auto &invalid : invalid_fields) {
        CAPTURE(invalid.path);
        CAPTURE(invalid.value.dump());
        auto candidate = replacement;
        candidate[nlohmann::json::json_pointer(invalid.path)] = invalid.value;
        CHECK(test::to_error_code(SpellReader(candidate, spells, session).read()) == invalid.error);
        CHECK(spells.get_spell_id(RealmType::LIFE, "old-zero") == 0);
        CHECK(spells.get_spell_id(RealmType::LIFE, "old-eight") == 8);
        CHECK_FALSE(spells.get_spell_id(RealmType::LIFE, "new-zero"));
        CHECK_FALSE(spells.get_spell_id(RealmType::LIFE, "new-eight"));
        CHECK(spells.get_spell_info(RealmType::LIFE, 0).name == "old-zero");
        CHECK(spells.get_spell_info(RealmType::LIFE, 8).description == "old-eight description");
        CHECK(spells.get_spell_id(RealmType::SORCERY, "other-realm") == 3);
    }
    SpellReader::Session reload_session;
    REQUIRE(SpellReader(replacement, spells, reload_session).read().has_value());
    CHECK(spells.get_spell_id(RealmType::LIFE, "new-zero") == 0);
    CHECK(spells.get_spell_id(RealmType::LIFE, "new-eight") == 8);
    CHECK(spells.get_spell_id(RealmType::SORCERY, "other-realm") == 3);
}

TEST_CASE("SpellReader rejects duplicate IDs within a realm record")
{
    const test::ScopedReaderState reader_state;
    const test::SpellInfoListTestAccess spell_state;
    auto &spells = SpellInfoList::get_instance();
    SpellReader::Session seed_session;
    auto seed = make_realm("LIFE", nlohmann::json::array({ make_spell(0, "old-zero") }), nlohmann::json::array());
    REQUIRE(SpellReader(seed, spells, seed_session).read().has_value());
    const auto saved = snapshot_spells(spells);
    SpellReader::Session session;
    auto data = make_realm("LIFE", nlohmann::json::array({ make_spell(0, "new-zero"), make_spell(2, "new-two") }), nlohmann::json::array());
    SUBCASE("same book")
    {
        data["books"][0]["spells"].push_back(make_spell(0, "duplicate-id"));
    }
    SUBCASE("different books")
    {
        data["books"][1]["spells"].push_back(make_spell(0, "duplicate-id"));
    }
    CHECK(test::to_error_code(SpellReader(data, spells, session).read()) == ParseErrorType::NON_SEQUENTIAL_RECORDS);
    check_spells_unchanged(spells, saved);
    // 失敗したレコードで見たID・タグを、セッションに残さない。
    data["books"][0]["spells"] = nlohmann::json::array({ make_spell(0, "new-zero"), make_spell(2, "new-two") });
    data["books"][1]["spells"] = nlohmann::json::array();
    CHECK(SpellReader(data, spells, session).read().has_value());
}

TEST_CASE("SpellReader rejects duplicate tags within a realm record")
{
    const test::ScopedReaderState reader_state;
    const test::SpellInfoListTestAccess spell_state;
    auto &spells = SpellInfoList::get_instance();
    const auto saved = snapshot_spells(spells);
    SpellReader::Session session;
    auto data = make_realm("LIFE", nlohmann::json::array({ make_spell(0, "shared-tag") }), nlohmann::json::array());
    SUBCASE("same book")
    {
        data["books"][0]["spells"].push_back(make_spell(1, "shared-tag"));
    }
    SUBCASE("different books")
    {
        data["books"][1]["spells"].push_back(make_spell(1, "shared-tag"));
    }
    CHECK(test::to_error_code(SpellReader(data, spells, session).read()) == ParseErrorType::INVALID_VALUE);
    check_spells_unchanged(spells, saved);
    data["books"][0]["spells"] = nlohmann::json::array({ make_spell(0, "shared-tag") });
    data["books"][1]["spells"] = nlohmann::json::array({ make_spell(1, "unique-tag") });
    CHECK(SpellReader(data, spells, session).read().has_value());
}

TEST_CASE("SpellReader rejects duplicate IDs and tags across realm records in one session")
{
    const test::ScopedReaderState reader_state;
    const test::SpellInfoListTestAccess spell_state;
    auto &spells = SpellInfoList::get_instance();
    SpellReader::Session session;
    auto first = make_realm("LIFE", nlohmann::json::array({ make_spell(0, "first") }), nlohmann::json::array());
    REQUIRE(SpellReader(first, spells, session).read().has_value());
    const auto saved = snapshot_spells(spells);
    auto next = make_realm("LIFE", nlohmann::json::array({ make_spell(2, "staged") }), nlohmann::json::array());
    SUBCASE("duplicate ID")
    {
        next["books"][1]["spells"].push_back(make_spell(0, "different-tag"));
        CHECK(test::to_error_code(SpellReader(next, spells, session).read()) == ParseErrorType::NON_SEQUENTIAL_RECORDS);
    }
    SUBCASE("duplicate tag")
    {
        next["books"][1]["spells"].push_back(make_spell(1, "first"));
        CHECK(test::to_error_code(SpellReader(next, spells, session).read()) == ParseErrorType::INVALID_VALUE);
    }
    check_spells_unchanged(spells, saved);
    next["books"][1]["spells"] = nlohmann::json::array({ make_spell(1, "unique") });
    REQUIRE(SpellReader(next, spells, session).read().has_value());
    CHECK(spells.get_spell_id(RealmType::LIFE, "first") == 0);
    CHECK(spells.get_spell_id(RealmType::LIFE, "staged") == 2);
    CHECK(spells.get_spell_id(RealmType::LIFE, "unique") == 1);
}

TEST_CASE("SpellReader allows the same ID and tag in different realms and a new session")
{
    const test::ScopedReaderState reader_state;
    const test::SpellInfoListTestAccess spell_state;
    auto &spells = SpellInfoList::get_instance();
    SpellReader::Session session;
    auto data = make_realm("LIFE", nlohmann::json::array({ make_spell(0, "shared") }), nlohmann::json::array());
    REQUIRE(SpellReader(data, spells, session).read().has_value());
    data["name"] = "SORCERY";
    REQUIRE(SpellReader(data, spells, session).read().has_value());
    CHECK(spells.get_spell_id(RealmType::LIFE, "shared") == 0);
    CHECK(spells.get_spell_id(RealmType::SORCERY, "shared") == 0);

    SpellReader::Session reload_session;
    data["name"] = "LIFE";
    data["books"][0]["spells"][0]["description"]["en"] = "replacement";
    data["books"][0]["spells"][0]["description"]["ja"] = "replacement";
    REQUIRE(SpellReader(data, spells, reload_session).read().has_value());
    CHECK(spells.get_spell_info(RealmType::LIFE, 0).description == "replacement");
    CHECK(spells.get_spell_info(RealmType::SORCERY, 0).description == "shared description");
}

TEST_CASE("SpellReader reports non-string realm names and tags as invalid types")
{
    const test::ScopedReaderState reader_state;
    const test::SpellInfoListTestAccess spell_state;
    auto &spells = SpellInfoList::get_instance();
    for (const auto *path : { "/name", "/books/0/spells/0/spell_tag" }) {
        for (const auto &value : { nlohmann::json(1), nlohmann::json(true), nlohmann::json::array(), nlohmann::json::object() }) {
            CAPTURE(path);
            auto data = make_realm("LIFE", nlohmann::json::array({ make_spell(0, "first") }), nlohmann::json::array());
            data[nlohmann::json::json_pointer(path)] = value;
            SpellReader::Session session;
            CHECK(test::to_error_code(SpellReader(data, spells, session).read()) == ParseErrorType::INVALID_TYPE);
            CHECK_FALSE(spells.get_spell_id(RealmType::LIFE, "first"));
        }
    }
}
