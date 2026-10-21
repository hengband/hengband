#include "info-reader/info-reader-util.h"
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
    auto &spells = SpellInfoList::get_instance();
    nlohmann::json data = { { "name", "LIFE" }, { "books", nlohmann::json::array() } };

    CHECK(SpellReader(data, spells).read().has_value());

    data["books"] = nlohmann::json::array({ { { "spells", nlohmann::json::array() } } });
    CHECK(SpellReader(data, spells).read().has_value());

    // A JSON object is iterable too, so these used to be accepted as record arrays.
    data["books"] = { { "first", { { "spells", nlohmann::json::array() } } } };
    CHECK(test::to_error_code(SpellReader(data, spells).read()) == ParseErrorType::INVALID_TYPE);

    data["books"] = nlohmann::json::array({ { { "spells", nlohmann::json::object() } } });
    CHECK(test::to_error_code(SpellReader(data, spells).read()) == ParseErrorType::INVALID_TYPE);
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
}

TEST_CASE("SpellReader commits all books only after the whole realm succeeds")
{
    auto &spells = SpellInfoList::get_instance();
    const test::ScopedReaderState reader_state;
    const test::SpellInfoListTestAccess spell_state;

    nlohmann::json data = {
        { "name", "LIFE" },
        { "books", nlohmann::json::array({
                       { { "spells", nlohmann::json::array({ make_spell(0, "old-zero"), make_spell(1, "old-one") }) } },
                       { { "spells", nlohmann::json::array({ make_spell(8, "old-eight") }) } },
                   }) },
    };
    REQUIRE(SpellReader(data, spells).read().has_value());
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
        CHECK(test::to_error_code(SpellReader(data, spells).read()) == ParseErrorType::TOO_FEW_ARGUMENTS);
        check_unchanged();
    }
    SUBCASE("a spell in a later book is invalid")
    {
        data["books"][1]["spells"][0]["spell_id"] = 32;
        CHECK(test::to_error_code(SpellReader(data, spells).read()) == ParseErrorType::INVALID_FLAG);
        check_unchanged();
    }
    SUBCASE("a later book has an invalid spells container")
    {
        data["books"][1]["spells"] = nlohmann::json::object();
        CHECK(test::to_error_code(SpellReader(data, spells).read()) == ParseErrorType::INVALID_TYPE);
        check_unchanged();
    }
    SUBCASE("successful reload preserves unspecified spells and other realms")
    {
        data["books"][1]["spells"] = nlohmann::json::array({ make_spell(8, "new-eight"), make_spell(0, "last-zero") });
        REQUIRE(SpellReader(data, spells).read().has_value());
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
        CHECK(SpellReader(data, spells).read().has_value());
        CHECK(spells.get_spell_id(RealmType::LIFE, "last-zero") == 0);
    }
}

/*!
 * @brief 内部の各解析エラーを伝播し、先行する呪文書を含む既存定義の部分更新を防ぐ
 */
TEST_CASE("SpellReader propagates helper errors without publishing staged spells")
{
    const test::ScopedReaderState reader_state;
    const test::SpellInfoListTestAccess spell_state;
    auto &spells = SpellInfoList::get_instance();
    nlohmann::json original = {
        { "name", "LIFE" },
        { "books", nlohmann::json::array({
                       { { "spells", nlohmann::json::array({ make_spell(0, "old-zero") }) } },
                       { { "spells", nlohmann::json::array({ make_spell(8, "old-eight") }) } },
                   }) },
    };
    REQUIRE(SpellReader(original, spells).read().has_value());
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
        { "/name", "UNKNOWN_REALM", ParseErrorType::OUT_OF_BOUNDS },
        { "/books", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/books", nlohmann::json::object(), ParseErrorType::INVALID_TYPE },
        { "/books/1/spells", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/books/1/spells", nlohmann::json::object(), ParseErrorType::INVALID_TYPE },
        { "/books/1/spells/0", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/books/1/spells/0/spell_id", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/books/1/spells/0/spell_id", 32, ParseErrorType::INVALID_FLAG },
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
        error_idx = 4;
        CHECK(test::to_error_code(SpellReader(candidate, spells).read()) == invalid.error);
        CHECK(error_idx == 5);
        CHECK(spells.get_spell_id(RealmType::LIFE, "old-zero") == 0);
        CHECK(spells.get_spell_id(RealmType::LIFE, "old-eight") == 8);
        CHECK_FALSE(spells.get_spell_id(RealmType::LIFE, "new-zero"));
        CHECK_FALSE(spells.get_spell_id(RealmType::LIFE, "new-eight"));
        CHECK(spells.get_spell_info(RealmType::LIFE, 0).name == "old-zero");
        CHECK(spells.get_spell_info(RealmType::LIFE, 8).description == "old-eight description");
        CHECK(spells.get_spell_id(RealmType::SORCERY, "other-realm") == 3);
    }
    REQUIRE(SpellReader(replacement, spells).read().has_value());
    CHECK(error_idx == 6);
    CHECK(spells.get_spell_id(RealmType::LIFE, "new-zero") == 0);
    CHECK(spells.get_spell_id(RealmType::LIFE, "new-eight") == 8);
    CHECK(spells.get_spell_id(RealmType::SORCERY, "other-realm") == 3);
}
