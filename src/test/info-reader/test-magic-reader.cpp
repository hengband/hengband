#include "info-reader/magic-reader.h"
#include "info-reader/parse-error-types.h"
#include "player-info/class-info.h"
#include "system/spell-info-list.h"
#include "test/info-reader/parse-result-helpers.h"
#include "test/info-reader/scoped-reader-state.h"
#include "test/scoped-restore.h"
#include "test/system/spell-info-list-test-access.h"
#include "util/enum-converter.h"
#include <cstdint>
#include <doctest/doctest.h>
#include <limits>
#include <nlohmann/json.hpp>
#include <string_view>
#include <utility>
#include <vector>

TEST_CASE("MagicReader preserves an existing class on a later parse error")
{
    const test::ScopedReaderState reader_state;
    const auto restore = test::scoped_restore(class_magics_info);
    class_magics_info.assign(1, {});
    auto session = MagicReader::create_session();

    nlohmann::json data = {
        { "name", "WARRIOR" },
        { "spell_type", "NONE" },
        { "magic_status", "STR" },
        { "has_glove_mp_penalty", false },
        { "has_magic_fail_rate_cap", false },
        { "is_spell_trainable", false },
        { "first_spell_level", 99 },
        { "armour_weight_limit", 0 },
        { "realms", nlohmann::json::array() },
    };
    REQUIRE(test::read_and_commit(MagicReader(data, session), session).has_value());
    CHECK(session.last_id().value_or(-1) == 0);
    CHECK(class_magics_info[0].spell_book == ItemKindType::NONE);
    CHECK(class_magics_info[0].spell_first == 99);

    data["spell_type"] = "LIFE";
    data["magic_status"] = "WIS";
    data["has_glove_mp_penalty"] = true;
    data["first_spell_level"] = 1;
    data["realms"] = nlohmann::json::object();
    session = MagicReader::create_session();
    CHECK(test::to_error_code(test::read_and_commit(MagicReader(data, session), session)) == ParseErrorType::INVALID_TYPE);
    CHECK(class_magics_info[0].spell_book == ItemKindType::NONE);
    CHECK(class_magics_info[0].has_glove_mp_penalty == false);
    CHECK(class_magics_info[0].spell_first == 99);
    CHECK_FALSE(session.last_id());
}

namespace {
// 数値境界テストの準備をまとめ、終了時に既存の定義とreaderの状態を戻す。
class MagicNumericState {
public:
    MagicNumericState()
    {
        class_magics_info.assign(1, {});
    }

    MagicNumericState(const MagicNumericState &) = delete;
    MagicNumericState &operator=(const MagicNumericState &) = delete;

private:
    test::ScopedReaderState reader_state;
    decltype(test::scoped_restore(class_magics_info)) restore{ test::scoped_restore(class_magics_info) };
    test::SpellInfoListTestAccess spell_state;
};

/*!
 * @brief 数値の境界検証で参照する2呪文を登録し、職業の入力を作る
 */
nlohmann::json make_magic_numeric_data(SpellInfoList &spells)
{
    for (short id = 0; id < 2; ++id) {
        SpellInfo info;
        info.idx = id;
        info.tag = id == 0 ? "first" : "second";
        spells.set_spell_info(RealmType::LIFE, id, std::move(info));
    }
    return {
        { "name", "WARRIOR" },
        { "spell_type", "SORCERY" },
        { "magic_status", "INT" },
        { "has_glove_mp_penalty", true },
        { "has_magic_fail_rate_cap", false },
        { "is_spell_trainable", true },
        { "first_spell_level", 1 },
        { "armour_weight_limit", 430 },
        { "realms", nlohmann::json::array({ {
                        { "name", "LIFE" },
                        { "spells_info", nlohmann::json::array({
                                             { { "spell_tag", "first" }, { "learn_level", 1 }, { "mana_cost", 2 }, { "difficulty", 7 }, { "first_cast_exp_rate", 4 } },
                                             { { "spell_tag", "second" }, { "learn_level", 3 }, { "mana_cost", 5 }, { "difficulty", 8 }, { "first_cast_exp_rate", 6 } },
                                         }) },
                    } }) },
    };
}
}

TEST_CASE("MagicReader rejects a repeated class without changing its definition")
{
    const MagicNumericState state;
    auto session = MagicReader::create_session();
    auto data = make_magic_numeric_data(SpellInfoList::get_instance());
    REQUIRE(test::read_and_commit(MagicReader(data, session), session).has_value());
    const auto original = class_magics_info[0];

    data["spell_type"] = "LIFE";
    data["magic_status"] = "WIS";
    data["has_glove_mp_penalty"] = false;
    data["has_magic_fail_rate_cap"] = true;
    data["is_spell_trainable"] = false;
    data["first_spell_level"] = 5;
    data["armour_weight_limit"] = 450;
    data["realms"][0]["spells_info"][0]["mana_cost"] = 100;
    CHECK(test::to_error_code(test::read_and_commit(MagicReader(data, session), session)) == ParseErrorType::NON_SEQUENTIAL_RECORDS);
    CHECK(session.last_id().value_or(-1) == 0);
    REQUIRE(class_magics_info.size() == 1);
    const auto &actual = class_magics_info[0];
    CHECK(actual.spell_book == original.spell_book);
    CHECK(actual.spell_stat == original.spell_stat);
    CHECK(actual.has_glove_mp_penalty == original.has_glove_mp_penalty);
    CHECK(actual.has_magic_fail_rate_cap == original.has_magic_fail_rate_cap);
    CHECK(actual.is_spell_trainable == original.is_spell_trainable);
    CHECK(actual.spell_first == original.spell_first);
    CHECK(actual.spell_weight == original.spell_weight);
    for (int realm = 0; realm <= MAX_MAGIC; ++realm) {
        for (int spell = 0; spell < 32; ++spell) {
            CAPTURE(realm);
            CAPTURE(spell);
            CHECK(actual.info[realm][spell].slevel == original.info[realm][spell].slevel);
            CHECK(actual.info[realm][spell].smana == original.info[realm][spell].smana);
            CHECK(actual.info[realm][spell].sfail == original.info[realm][spell].sfail);
            CHECK(actual.info[realm][spell].sexp == original.info[realm][spell].sexp);
        }
    }
}

TEST_CASE("MagicReader accepts a gap between classes")
{
    const MagicNumericState state;
    auto session = MagicReader::create_session();
    class_magics_info.assign(3, {});
    session = MagicReader::create_session();
    auto data = make_magic_numeric_data(SpellInfoList::get_instance());
    REQUIRE(test::read_and_commit(MagicReader(data, session), session).has_value());
    CHECK(session.last_id().value_or(-1) == 0);

    data["name"] = "PRIEST";
    data["first_spell_level"] = 5;
    REQUIRE(test::read_and_commit(MagicReader(data, session), session).has_value());
    CHECK(session.last_id().value_or(-1) == 2);
    CHECK(class_magics_info[0].spell_first == 1);
    CHECK(class_magics_info[2].spell_first == 5);
    CHECK(class_magics_info[2].info[enum2i(RealmType::LIFE)][0].smana == 2);
}

/*!
 * @brief 補正前の難易度の0・100を受理し、符号なしの境界値も維持する
 */
TEST_CASE("MagicReader accepts difficulty from zero through one hundred")
{
    const MagicNumericState state;
    auto &spells = SpellInfoList::get_instance();
    auto data = make_magic_numeric_data(spells);
    for (const auto &difficulty : std::vector<nlohmann::json>{ 0, 95, 100, 100u }) {
        CAPTURE(difficulty.dump());
        data["realms"][0]["spells_info"][1]["difficulty"] = difficulty;
        REQUIRE(test::read_record<MagicReader>(data).has_value());
        CHECK(class_magics_info[0].info[enum2i(RealmType::LIFE)][0].sfail == 7);
        CHECK(class_magics_info[0].info[enum2i(RealmType::LIFE)][1].sfail == difficulty.get<int>());
    }
}

/*!
 * @brief 後続呪文の範囲外・非整数難易度を拒否し、職業の既存定義を保持する
 */
TEST_CASE("MagicReader rejects invalid difficulty without publishing partial data")
{
    const MagicNumericState state;
    auto session = MagicReader::create_session();
    auto &spells = SpellInfoList::get_instance();
    auto data = make_magic_numeric_data(spells);
    REQUIRE(test::read_and_commit(MagicReader(data, session), session).has_value());
    data["first_spell_level"] = 5;
    data["armour_weight_limit"] = 450;
    data["realms"][0]["spells_info"][0]["difficulty"] = 20;
    for (const auto &difficulty : std::vector<nlohmann::json>{ -1, 101, 999, std::numeric_limits<uint64_t>::max(), 1.0, true }) {
        CAPTURE(difficulty.dump());
        data["realms"][0]["spells_info"][1]["difficulty"] = difficulty;
        const auto expected = difficulty.is_number_integer() ? ParseErrorType::INVALID_VALUE : ParseErrorType::INVALID_TYPE;
        session = MagicReader::create_session();
        CHECK(test::to_error_code(test::read_and_commit(MagicReader(data, session), session)) == expected);
        CHECK(class_magics_info[0].spell_first == 1);
        CHECK(class_magics_info[0].spell_weight == 430);
        CHECK(class_magics_info[0].info[enum2i(RealmType::LIFE)][0].sfail == 7);
        CHECK(class_magics_info[0].info[enum2i(RealmType::LIFE)][1].sfail == 8);
        CHECK_FALSE(session.last_id());
    }
}

/*!
 * @brief 消費MP・初回経験値倍率・重量閾値のSHRT_MAX/16境界を受理する
 */
TEST_CASE("MagicReader accepts configured maximum for mana experience and armour weight")
{
    const MagicNumericState state;
    auto &spells = SpellInfoList::get_instance();
    auto data = make_magic_numeric_data(spells);
    for (const auto &value : std::vector<nlohmann::json>{ 0, 999, 1000, 2047, 2047u }) {
        CAPTURE(value.dump());
        data["armour_weight_limit"] = value;
        data["realms"][0]["spells_info"][1]["mana_cost"] = value;
        data["realms"][0]["spells_info"][1]["first_cast_exp_rate"] = value;
        REQUIRE(test::read_record<MagicReader>(data).has_value());
        CHECK(class_magics_info[0].spell_weight == value.get<int>());
        CHECK(class_magics_info[0].info[enum2i(RealmType::LIFE)][1].smana == value.get<int>());
        CHECK(class_magics_info[0].info[enum2i(RealmType::LIFE)][1].sexp == value.get<int>());
    }
}

/*!
 * @brief 3項目の範囲外・非整数を拒否し、既存定義を保持する
 */
TEST_CASE("MagicReader rejects invalid bounded fields without publishing partial data")
{
    const MagicNumericState state;
    auto session = MagicReader::create_session();
    auto &spells = SpellInfoList::get_instance();
    const auto original = make_magic_numeric_data(spells);
    REQUIRE(test::read_and_commit(MagicReader(original, session), session).has_value());
    for (const auto *field : { "armour_weight_limit", "mana_cost", "first_cast_exp_rate" }) {
        for (const auto &value : std::vector<nlohmann::json>{ -1, 2048, std::numeric_limits<uint64_t>::max(), 1.0, true }) {
            CAPTURE(field);
            CAPTURE(value.dump());
            auto data = original;
            data["first_spell_level"] = 5;
            data["realms"][0]["spells_info"][0]["mana_cost"] = 1000;
            if (std::string_view(field) == "armour_weight_limit") {
                data[field] = value;
            } else {
                data["realms"][0]["spells_info"][1][field] = value;
            }
            const auto expected = value.is_number_integer() ? ParseErrorType::INVALID_VALUE : ParseErrorType::INVALID_TYPE;
            session = MagicReader::create_session();
            CHECK(test::to_error_code(test::read_and_commit(MagicReader(data, session), session)) == expected);
            CHECK(class_magics_info[0].spell_first == 1);
            CHECK(class_magics_info[0].spell_weight == 430);
            CHECK(class_magics_info[0].info[enum2i(RealmType::LIFE)][0].smana == 2);
            CHECK(class_magics_info[0].info[enum2i(RealmType::LIFE)][1].smana == 5);
            CHECK(class_magics_info[0].info[enum2i(RealmType::LIFE)][1].sexp == 6);
        }
    }
}

TEST_CASE("MagicReader numeric fixture restores existing spell definitions")
{
    const test::SpellInfoListTestAccess original_state;
    auto &spells = SpellInfoList::get_instance();
    SpellInfo original;
    original.idx = 7;
    original.name = "Existing name";
    original.description = "Existing description";
    original.tag = "existing-tag";
    spells.set_spell_info(RealmType::SORCERY, 7, std::move(original));
    const auto *original_info = &spells.get_spell_info(RealmType::SORCERY, 7);
    {
        const MagicNumericState state;
        auto session = MagicReader::create_session();
        make_magic_numeric_data(spells);
        CHECK_FALSE(spells.get_spell_id(RealmType::SORCERY, "existing-tag"));
    }
    CHECK(&spells.get_spell_info(RealmType::SORCERY, 7) == original_info);
    CHECK(spells.get_spell_info(RealmType::SORCERY, 7).name == "Existing name");
    CHECK(spells.get_spell_info(RealmType::SORCERY, 7).description == "Existing description");
    CHECK(spells.get_spell_id(RealmType::SORCERY, "existing-tag") == 7);
}

/*!
 * @brief 内部解析の各エラーを公開APIへ伝播し、後続解析で変更した既存定義を保持する
 */
TEST_CASE("MagicReader propagates helper errors without publishing partial data")
{
    const MagicNumericState state;
    auto session = MagicReader::create_session();
    auto data = make_magic_numeric_data(SpellInfoList::get_instance());
    REQUIRE(test::read_and_commit(MagicReader(data, session), session).has_value());

    struct InvalidField {
        const char *path;
        nlohmann::json value;
        ParseErrorType error;
    };
    const std::vector<InvalidField> invalid_fields = {
        { "/name", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/name", "UNKNOWN_CLASS", ParseErrorType::INVALID_FLAG },
        { "/spell_type", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/spell_type", "UNKNOWN_BOOK", ParseErrorType::INVALID_FLAG },
        { "/magic_status", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/magic_status", "UNKNOWN_STAT", ParseErrorType::INVALID_FLAG },
        { "/realms", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/realms", nlohmann::json::object(), ParseErrorType::INVALID_TYPE },
        { "/realms/0", 1, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/realms/0/name", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/realms/0/name", "UNKNOWN_REALM", ParseErrorType::INVALID_FLAG },
        { "/realms/0/spells_info", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/realms/0/spells_info", nlohmann::json::object(), ParseErrorType::INVALID_TYPE },
        { "/realms/0/spells_info/1", 1, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/realms/0/spells_info/1/spell_tag", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/realms/0/spells_info/1/spell_tag", "unknown-tag", ParseErrorType::INVALID_FLAG },
        { "/realms/0/spells_info/1/learn_level", 100, ParseErrorType::INVALID_VALUE },
    };
    for (const auto &invalid : invalid_fields) {
        CAPTURE(invalid.path);
        CAPTURE(invalid.value.dump());
        auto candidate = data;
        candidate["spell_type"] = "LIFE";
        candidate["first_spell_level"] = 5;
        candidate["realms"][0]["spells_info"][0]["mana_cost"] = 100;
        candidate[nlohmann::json::json_pointer(invalid.path)] = invalid.value;
        session = MagicReader::create_session();
        CHECK(test::to_error_code(test::read_and_commit(MagicReader(candidate, session), session)) == invalid.error);
        CHECK_FALSE(session.last_id());
        CHECK(class_magics_info[0].spell_book == ItemKindType::SORCERY_BOOK);
        CHECK(class_magics_info[0].spell_first == 1);
        CHECK(class_magics_info[0].info[enum2i(RealmType::LIFE)][0].smana == 2);
        CHECK(class_magics_info[0].info[enum2i(RealmType::LIFE)][1].slevel == 3);
    }
}

TEST_CASE("MagicReader reports non-string tokens as invalid types")
{
    const MagicNumericState state;
    const auto original = make_magic_numeric_data(SpellInfoList::get_instance());
    for (const auto *path : { "/name", "/spell_type", "/magic_status", "/realms/0/name", "/realms/0/spells_info/0/spell_tag" }) {
        for (const auto &value : { nlohmann::json(1), nlohmann::json(true), nlohmann::json::array(), nlohmann::json::object() }) {
            CAPTURE(path);
            CAPTURE(value.dump());
            auto data = original;
            data[nlohmann::json::json_pointer(path)] = value;
            auto session = MagicReader::create_session();
            CHECK(test::to_error_code(MagicReader(data, session).read()) == ParseErrorType::INVALID_TYPE);
            CHECK_FALSE(session.last_id());
        }
    }
}
