#include "definitions-reader/magic-reader.h"
#include "definitions-reader/parse-error-types.h"
#include "player-info/class-info.h"
#include "player-info/class-types.h"
#include "player/player-realm.h"
#include "realm/realm-types.h"
#include "system/spell-info-list.h"
#include "system/system-variables.h"
#include "test/definitions-reader/parse-result-helpers.h"
#include "test/definitions-reader/scoped-reader-state.h"
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
        { "armour_weight_limit", 0 },
        { "realms", nlohmann::json::array() },
    };
    REQUIRE(test::read_and_commit(MagicReader(data, session), session).has_value());
    CHECK(session.last_id().value_or(-1) == 0);
    CHECK(class_magics_info[0].spell_book == ItemKindType::NONE);
    CHECK_FALSE(class_magics_info[0].spell_first);

    data["spell_type"] = "LIFE";
    data["magic_status"] = "WIS";
    data["has_glove_mp_penalty"] = true;
    data["first_spell_level"] = 1;
    data["realms"] = nlohmann::json::object();
    session = MagicReader::create_session();
    CHECK(test::to_error_code(test::read_and_commit(MagicReader(data, session), session)) == ParseErrorType::INVALID_TYPE);
    CHECK(class_magics_info[0].spell_book == ItemKindType::NONE);
    CHECK(class_magics_info[0].has_glove_mp_penalty == false);
    CHECK_FALSE(class_magics_info[0].spell_first);
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
            CHECK(actual.info[realm][spell].is_learnable() == original.info[realm][spell].is_learnable());
            if (actual.info[realm][spell].is_learnable() && original.info[realm][spell].is_learnable()) {
                CHECK(actual.info[realm][spell].get_learn_level() == original.info[realm][spell].get_learn_level());
            }
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
        { "/realms/0", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/realms/0", 1, ParseErrorType::INVALID_TYPE },
        { "/realms/0/name", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/realms/0/name", "UNKNOWN_REALM", ParseErrorType::INVALID_FLAG },
        { "/realms/0/spells_info", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/realms/0/spells_info", nlohmann::json::object(), ParseErrorType::INVALID_TYPE },
        { "/realms/0/spells_info/1", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/realms/0/spells_info/1", 1, ParseErrorType::INVALID_TYPE },
        { "/realms/0/spells_info/1/spell_tag", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/realms/0/spells_info/1/spell_tag", "unknown-tag", ParseErrorType::INVALID_FLAG },
        { "/realms/0/spells_info/1/learn_level", 0, ParseErrorType::INVALID_VALUE },
        { "/realms/0/spells_info/1/learn_level", 51, ParseErrorType::INVALID_VALUE },
        { "/realms/0/spells_info/1/learn_level", 99, ParseErrorType::INVALID_VALUE },
        { "/first_spell_level", -1, ParseErrorType::INVALID_VALUE },
        { "/first_spell_level", 51, ParseErrorType::INVALID_VALUE },
        { "/first_spell_level", 99, ParseErrorType::INVALID_VALUE },
        { "/realms/0/unlearnable", nlohmann::json::object(), ParseErrorType::INVALID_TYPE },
        { "/realms/0/unlearnable", nlohmann::json::array({ 1 }), ParseErrorType::INVALID_TYPE },
        { "/realms/0/unlearnable", nlohmann::json::array({ "unknown-tag" }), ParseErrorType::INVALID_FLAG },
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
        CHECK(class_magics_info[0].info[enum2i(RealmType::LIFE)][1].get_learn_level() == 3);
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

/*!
 * @brief 第一領域用と第二領域用の表に同じ値が入り、位置で引き分けられる
 */
TEST_CASE("MagicReader fills the second-realm table with the same values")
{
    const MagicNumericState state;
    auto session = MagicReader::create_session();
    auto data = make_magic_numeric_data(SpellInfoList::get_instance());
    REQUIRE(test::read_and_commit(MagicReader(data, session), session).has_value());

    const auto &magic = class_magics_info[0];
    const auto realm = enum2i(RealmType::LIFE);
    for (auto spell_id = 0; spell_id < 2; spell_id++) {
        CHECK(magic.info_second[realm][spell_id].is_learnable() == magic.info[realm][spell_id].is_learnable());
        if (magic.info_second[realm][spell_id].is_learnable() && magic.info[realm][spell_id].is_learnable()) {
            CHECK(magic.info_second[realm][spell_id].get_learn_level() == magic.info[realm][spell_id].get_learn_level());
        }
        CHECK(magic.info_second[realm][spell_id].smana == magic.info[realm][spell_id].smana);
        CHECK(magic.info_second[realm][spell_id].sfail == magic.info[realm][spell_id].sfail);
        CHECK(magic.info_second[realm][spell_id].sexp == magic.info[realm][spell_id].sexp);
    }
    CHECK(magic.info[realm][1].get_learn_level() == 3);
    CHECK_FALSE(magic.has_distinct_second(RealmType::LIFE));
    CHECK(&magic.spell_info(RealmType::LIFE, 1, RealmPosition::FIRST) == &magic.info[realm][1]);
    CHECK(&magic.spell_info(RealmType::LIFE, 1, RealmPosition::SECOND) == &magic.info_second[realm][1]);
    CHECK(&PlayerRealm::get_spell_info(RealmType::LIFE, 1, RealmPosition::SECOND, PlayerClassType::WARRIOR) == &magic.info_second[realm][1]);

    // 第二領域用の値を変えると「違う」と判定される
    class_magics_info[0].info_second[realm][1].set_learn_level(10);
    CHECK(class_magics_info[0].has_distinct_second(RealmType::LIFE));
}

/*!
 * @brief 領域の習得できる巻数を読み、省略時は4にする
 */
TEST_CASE("MagicReader reads max_book and defaults it to four")
{
    const MagicNumericState state;
    auto data = make_magic_numeric_data(SpellInfoList::get_instance());
    REQUIRE(test::read_record<MagicReader>(data).has_value());
    CHECK(class_magics_info[0].max_book[enum2i(RealmType::LIFE)] == MAX_BOOKS_IN_REALM);
    CHECK(class_magics_info[0].max_book[enum2i(RealmType::SORCERY)] == MAX_BOOKS_IN_REALM);

    for (const auto max_book : { 1, 2, 4 }) {
        CAPTURE(max_book);
        data["realms"][0]["max_book"] = max_book;
        REQUIRE(test::read_record<MagicReader>(data).has_value());
        CHECK(class_magics_info[0].max_book[enum2i(RealmType::LIFE)] == max_book);
        CHECK(PlayerRealm::get_max_book(RealmType::LIFE, PlayerClassType::WARRIOR) == max_book);
    }

    // 読み直しで省略すると4に戻る
    data["realms"][0].erase("max_book");
    REQUIRE(test::read_record<MagicReader>(data).has_value());
    CHECK(class_magics_info[0].max_book[enum2i(RealmType::LIFE)] == MAX_BOOKS_IN_REALM);
}

/*!
 * @brief 範囲外・非整数の習得できる巻数を拒否し、既存定義を保持する
 */
TEST_CASE("MagicReader rejects invalid max_book without publishing partial data")
{
    const MagicNumericState state;
    auto session = MagicReader::create_session();
    auto data = make_magic_numeric_data(SpellInfoList::get_instance());
    data["realms"][0]["max_book"] = 2;
    REQUIRE(test::read_and_commit(MagicReader(data, session), session).has_value());
    for (const auto &value : std::vector<nlohmann::json>{ 0, 5, -1, 2.0, "2", true }) {
        CAPTURE(value.dump());
        data["realms"][0]["max_book"] = value;
        const auto expected = value.is_number_integer() ? ParseErrorType::INVALID_VALUE : ParseErrorType::INVALID_TYPE;
        session = MagicReader::create_session();
        CHECK(test::to_error_code(test::read_and_commit(MagicReader(data, session), session)) == expected);
        CHECK(class_magics_info[0].max_book[enum2i(RealmType::LIFE)] == 2);
    }
}

/*!
 * @brief 技能領域の習得できる巻数は職業によらず4
 */
TEST_CASE("PlayerRealm returns four books for technic realms")
{
    const MagicNumericState state;
    for (const auto realm : { RealmType::MUSIC, RealmType::HISSATSU, RealmType::HEX }) {
        CHECK(PlayerRealm::get_max_book(realm, PlayerClassType::WARRIOR) == MAX_BOOKS_IN_REALM);
    }
}

/*!
 * @brief 魔法を使わない職業は最初の魔法のレベルを省略でき、魔法書を使う職業は省略できない
 */
TEST_CASE("MagicReader requires first_spell_level only for classes with spellbooks")
{
    const MagicNumericState state;
    auto data = make_magic_numeric_data(SpellInfoList::get_instance());
    REQUIRE(test::read_record<MagicReader>(data).has_value());
    CHECK(class_magics_info[0].spell_first == 1);

    data.erase("first_spell_level");
    auto session = MagicReader::create_session();
    CHECK(test::to_error_code(test::read_and_commit(MagicReader(data, session), session)) == ParseErrorType::TOO_FEW_ARGUMENTS);
    CHECK(class_magics_info[0].spell_first == 1);

    // 魔法書を使わない職業は省略でき、読み直すと前の値は残らない
    data["spell_type"] = "NONE";
    REQUIRE(test::read_record<MagicReader>(data).has_value());
    CHECK_FALSE(class_magics_info[0].spell_first);
}

namespace {
/*!
 * @brief 習得できない魔法の検証用に、1 巻目の 2 呪文と 2 巻目の 1 呪文を基準値つきで登録した職業の入力を作る
 * @details 1 巻目は "first" (ID 0)・"second" (ID 1)、2 巻目は "ninth" (ID 8)。
 */
nlohmann::json make_unlearnable_data(SpellInfoList &spells)
{
    auto data = make_magic_numeric_data(spells);
    for (const auto id : { short{ 0 }, short{ 1 }, short{ 8 } }) {
        SpellInfo info;
        info.idx = id;
        info.tag = (id == 0) ? "first" : (id == 1) ? "second"
                                                   : "ninth";
        info.base.set_learn_level(1);
        info.base.smana = 1;
        spells.set_spell_info(RealmType::LIFE, id, std::move(info));
    }
    data["realms"][0]["spells_info"].push_back({ { "spell_tag", "ninth" }, { "learn_level", 20 }, { "mana_cost", 9 }, { "difficulty", 30 }, { "first_cast_exp_rate", 2 } });
    return data;
}
}

/*!
 * @brief unlearnable に書いた魔法は習得レベルが値なしになり、習得できない
 */
TEST_CASE("MagicReader marks spells listed in unlearnable as not learnable")
{
    const MagicNumericState state;
    auto data = make_unlearnable_data(SpellInfoList::get_instance());
    REQUIRE(test::read_record<MagicReader>(data).has_value());
    const auto realm = enum2i(RealmType::LIFE);
    CHECK(class_magics_info[0].info[realm][1].get_learn_level() == 3);
    CHECK(class_magics_info[0].info[realm][1].can_learn_at(3));
    CHECK_FALSE(class_magics_info[0].info[realm][1].can_learn_at(2));

    data["realms"][0]["spells_info"].erase(1);
    data["realms"][0]["unlearnable"] = nlohmann::json::array({ "second" });
    REQUIRE(test::read_record<MagicReader>(data).has_value());
    for (const auto *table : { class_magics_info[0].info, class_magics_info[0].info_second }) {
        const auto &spell = table[realm][1];
        CHECK_FALSE(spell.is_learnable());
        CHECK_FALSE(spell.can_learn_at(PY_MAX_LEVEL));
        CHECK(spell.smana == 0);
        CHECK(spell.sfail == 0);
        CHECK(spell.sexp == 0);
    }
    CHECK(class_magics_info[0].info[realm][0].is_learnable());
    CHECK(class_magics_info[0].info[realm][8].get_learn_level() == 20);
}

/*!
 * @brief 習得できる巻数より後ろの巻の魔法は、書かなくても習得できず、書くとエラーになる
 */
TEST_CASE("MagicReader makes spells after max_book unlearnable")
{
    const MagicNumericState state;
    auto data = make_unlearnable_data(SpellInfoList::get_instance());
    REQUIRE(test::read_record<MagicReader>(data).has_value());
    const auto realm = enum2i(RealmType::LIFE);
    REQUIRE(class_magics_info[0].info[realm][8].is_learnable());

    // 2 巻目の魔法を spells_info に書いたまま max_book を 1 にするとエラー
    data["realms"][0]["max_book"] = 1;
    auto session = MagicReader::create_session();
    CHECK(test::to_error_code(test::read_and_commit(MagicReader(data, session), session)) == ParseErrorType::INVALID_VALUE);
    CHECK(class_magics_info[0].info[realm][8].get_learn_level() == 20);

    // unlearnable に書いてもエラー (書かなくても習得できない)
    auto redundant = data;
    redundant["realms"][0]["spells_info"].erase(2);
    redundant["realms"][0]["unlearnable"] = nlohmann::json::array({ "ninth" });
    session = MagicReader::create_session();
    CHECK(test::to_error_code(test::read_and_commit(MagicReader(redundant, session), session)) == ParseErrorType::INVALID_VALUE);

    // どちらにも書かなければ習得できない
    data["realms"][0]["spells_info"].erase(2);
    REQUIRE(test::read_record<MagicReader>(data).has_value());
    CHECK_FALSE(class_magics_info[0].info[realm][8].is_learnable());
    CHECK_FALSE(class_magics_info[0].info_second[realm][8].is_learnable());
    CHECK(class_magics_info[0].info[realm][0].get_learn_level() == 1);
}

/*!
 * @brief 同じ魔法を 2 度書く・どちらにも書かない魔法があるのはエラー
 */
TEST_CASE("MagicReader rejects spells listed twice or not listed")
{
    const MagicNumericState state;
    auto session = MagicReader::create_session();
    const auto original = make_unlearnable_data(SpellInfoList::get_instance());
    REQUIRE(test::read_and_commit(MagicReader(original, session), session).has_value());
    const auto realm = enum2i(RealmType::LIFE);

    struct Case {
        const char *name;
        nlohmann::json unlearnable;
        bool erase_second;
        ParseErrorType error;
    };
    const std::vector<Case> cases = {
        { "both spells_info and unlearnable", nlohmann::json::array({ "second" }), false, ParseErrorType::INVALID_VALUE },
        { "twice in unlearnable", nlohmann::json::array({ "second", "second" }), true, ParseErrorType::INVALID_VALUE },
        { "not listed", nullptr, true, ParseErrorType::TOO_FEW_ARGUMENTS },
    };
    for (const auto &c : cases) {
        CAPTURE(c.name);
        auto data = original;
        if (c.erase_second) {
            data["realms"][0]["spells_info"].erase(1);
        }
        if (!c.unlearnable.is_null()) {
            data["realms"][0]["unlearnable"] = c.unlearnable;
        }
        session = MagicReader::create_session();
        CHECK(test::to_error_code(test::read_and_commit(MagicReader(data, session), session)) == c.error);
        CHECK(class_magics_info[0].info[realm][1].get_learn_level() == 3);
    }

    // spells_info に同じ魔法を 2 度書くのもエラー
    auto twice = original;
    twice["realms"][0]["spells_info"].push_back(original["realms"][0]["spells_info"][0]);
    session = MagicReader::create_session();
    CHECK(test::to_error_code(test::read_and_commit(MagicReader(twice, session), session)) == ParseErrorType::INVALID_VALUE);
}
