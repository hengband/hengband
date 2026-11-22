#include "definitions-reader/magic-reader.h"
#include "definitions-reader/parse-error-types.h"
#include "locale/language-switcher.h"
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
#include "view/display-messages.h"
#include <cstdint>
#include <doctest/doctest.h>
#include <limits>
#include <nlohmann/json.hpp>
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
//! 初期データの傾き (習得レベル 30、消費MP 15、難易度 3)
constexpr AptitudeSteps DEFAULT_STEPS{ 30, 15, 3 };

// 職業魔法の読み込みテストの準備をまとめ、終了時に既存の定義とreaderの状態を戻す。
class MagicReaderState {
public:
    MagicReaderState()
    {
        class_magics_info.assign(PLAYER_CLASS_TYPE_MAX, {});
    }

    MagicReaderState(const MagicReaderState &) = delete;
    MagicReaderState &operator=(const MagicReaderState &) = delete;

private:
    test::ScopedReaderState reader_state;
    decltype(test::scoped_restore(class_magics_info)) restore{ test::scoped_restore(class_magics_info) };
    test::SpellInfoListTestAccess spell_state;
};

/*!
 * @brief 生命領域に基準値つきの 3 呪文を登録する
 * @details 1 巻目は "first" (ID 0、基準 1/2/7/4)・"second" (ID 1、基準 3/5/8/6)、2 巻目は "ninth" (ID 8、基準 20/9/30/2)。
 */
void register_spells(SpellInfoList &spells)
{
    struct Base {
        short id;
        const char *tag;
        PLAYER_LEVEL level;
        MANA_POINT mana;
        PERCENTAGE difficulty;
        EXP exp;
    };
    for (const auto &base : { Base{ 0, "first", 1, 2, 7, 4 }, Base{ 1, "second", 3, 5, 8, 6 }, Base{ 8, "ninth", 20, 9, 30, 2 } }) {
        SpellInfo info;
        info.idx = base.id;
        info.tag = base.tag;
        info.base.set_learn_level(base.level);
        info.base.smana = base.mana;
        info.base.sfail = base.difficulty;
        info.base.sexp = base.exp;
        spells.set_spell_info(RealmType::LIFE, base.id, std::move(info));
    }
}

/*!
 * @brief メイジの入力を作る (呪文も登録する)
 * @details 領域は魔法の 10 領域をすべて段階 0 で書く。先頭が生命領域。
 */
nlohmann::json make_magic_data(SpellInfoList &spells)
{
    register_spells(spells);
    auto realms = nlohmann::json::array();
    for (const auto *name : { "LIFE", "SORCERY", "NATURE", "CHAOS", "DEATH", "TRUMP", "ARCANE", "CRAFT", "DEMON", "CRUSADE" }) {
        realms.push_back({ { "name", name } });
    }
    return {
        { "name", "MAGE" },
        { "spell_type", "SORCERY" },
        { "magic_status", "INT" },
        { "has_glove_mp_penalty", true },
        { "has_magic_fail_rate_cap", false },
        { "is_spell_trainable", true },
        { "first_spell_level", 1 },
        { "armour_weight_limit", 430 },
        { "realms", std::move(realms) },
    };
}

/*!
 * @brief 段階 1 つあたりの動きを指定して 1 職業を読み、成功したら確定する
 */
tl::expected<void, ParseErrorType> read_with_steps(const nlohmann::json &data, const AptitudeSteps &steps = DEFAULT_STEPS)
{
    auto session = MagicReader::create_session();
    return test::read_and_commit(MagicReader(data, session, steps), session);
}

/*!
 * @brief 魔法の値が期待どおりかを確かめる
 */
void check_spell(const magic_type &spell, PLAYER_LEVEL level, MANA_POINT mana, PERCENTAGE difficulty, EXP exp)
{
    REQUIRE(spell.is_learnable());
    CHECK(spell.get_learn_level() == level);
    CHECK(spell.smana == mana);
    CHECK(spell.sfail == difficulty);
    CHECK(spell.sexp == exp);
}

/*!
 * @brief 魔法が習得できない (値がすべて空) ことを確かめる
 */
void check_unlearnable(const magic_type &spell)
{
    CHECK_FALSE(spell.is_learnable());
    CHECK_FALSE(spell.can_learn_at(PY_MAX_LEVEL));
    CHECK(spell.smana == 0);
    CHECK(spell.sfail == 0);
    CHECK(spell.sexp == 0);
}

const auto LIFE = enum2i(RealmType::LIFE);
const auto MAGE = enum2i(PlayerClassType::MAGE);
}

TEST_CASE("MagicReader rejects a repeated class without changing its definition")
{
    const MagicReaderState state;
    auto session = MagicReader::create_session();
    auto data = make_magic_data(SpellInfoList::get_instance());
    REQUIRE(test::read_and_commit(MagicReader(data, session, DEFAULT_STEPS), session).has_value());
    CHECK(session.last_id().value_or(-1) == MAGE);

    data["spell_type"] = "LIFE";
    data["magic_status"] = "WIS";
    data["first_spell_level"] = 5;
    data["armour_weight_limit"] = 450;
    data["realms"][0]["aptitude"] = 2;
    CHECK(test::to_error_code(test::read_and_commit(MagicReader(data, session, DEFAULT_STEPS), session)) == ParseErrorType::NON_SEQUENTIAL_RECORDS);
    CHECK(session.last_id().value_or(-1) == MAGE);
    const auto &actual = class_magics_info[MAGE];
    CHECK(actual.spell_book == ItemKindType::SORCERY_BOOK);
    CHECK(actual.spell_first == 1);
    CHECK(actual.spell_weight == 430);
    check_spell(actual.info[LIFE][1], 3, 5, 8, 6);
}

TEST_CASE("MagicReader accepts a gap between classes")
{
    const MagicReaderState state;
    auto session = MagicReader::create_session();
    auto data = make_magic_data(SpellInfoList::get_instance());
    REQUIRE(test::read_and_commit(MagicReader(data, session), session).has_value());
    CHECK(session.last_id().value_or(-1) == MAGE);

    // 戦士 (ID 0) の後のメイジ (ID 1) を読み、プリースト (ID 2) を飛ばして盗賊 (ID 3) を読む
    data["name"] = "ROGUE";
    data["first_spell_level"] = 5;
    data["realms"] = nlohmann::json::array();
    for (const auto *name : { "SORCERY", "DEATH", "TRUMP", "ARCANE", "CRAFT" }) {
        data["realms"].push_back({ { "name", name } });
    }
    REQUIRE(test::read_and_commit(MagicReader(data, session), session).has_value());
    const auto rogue = enum2i(PlayerClassType::ROGUE);
    CHECK(session.last_id().value_or(-1) == rogue);
    CHECK(class_magics_info[MAGE].spell_first == 1);
    CHECK(class_magics_info[rogue].spell_first == 5);
    check_spell(class_magics_info[MAGE].info[LIFE][0], 1, 2, 7, 4);
    CHECK_FALSE(class_magics_info[rogue].info[LIFE][0].is_learnable());
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
        const MagicReaderState state;
        make_magic_data(spells);
        CHECK_FALSE(spells.get_spell_id(RealmType::SORCERY, "existing-tag"));
    }
    CHECK(&spells.get_spell_info(RealmType::SORCERY, 7) == original_info);
    CHECK(spells.get_spell_info(RealmType::SORCERY, 7).name == "Existing name");
    CHECK(spells.get_spell_info(RealmType::SORCERY, 7).description == "Existing description");
    CHECK(spells.get_spell_id(RealmType::SORCERY, "existing-tag") == 7);
}

/*!
 * @brief 段階 0 の領域は、魔法の基準値がそのまま職業別の値になる
 */
TEST_CASE("MagicReader copies the base values at aptitude zero")
{
    const MagicReaderState state;
    const auto data = make_magic_data(SpellInfoList::get_instance());
    REQUIRE(read_with_steps(data).has_value());
    const auto &magic = class_magics_info[MAGE];
    for (const auto *table : { magic.info, magic.info_second }) {
        check_spell(table[LIFE][0], 1, 2, 7, 4);
        check_spell(table[LIFE][1], 3, 5, 8, 6);
        check_spell(table[LIFE][8], 20, 9, 30, 2);
        // 魔法の定義に無い枠は習得できない
        check_unlearnable(table[LIFE][2]);
    }
    CHECK_FALSE(magic.has_distinct_second(RealmType::LIFE));
}

/*!
 * @brief 領域の段階から、習得レベル・消費MP・難易度を求める (初回経験値倍率は基準のまま)
 */
TEST_CASE("MagicReader derives the class values from the realm aptitude")
{
    const MagicReaderState state;
    auto data = make_magic_data(SpellInfoList::get_instance());
    data["realms"][0]["aptitude"] = -1;
    REQUIRE(read_with_steps(data).has_value());
    const auto &magic = class_magics_info[MAGE];
    // 基準 3: 3 × 47 × 30 / 5000 = 0.846 は 1 動く。消費MP 5 × 115% = 5.75 は 6。難易度 8 + 3
    check_spell(magic.info[LIFE][1], 4, 6, 11, 6);
    // 基準 20: 20 × 30 × 30 / 5000 = 3.6 は 4 動く。消費MP 9 × 115% = 10.35 は 10。難易度 30 + 3
    check_spell(magic.info[LIFE][8], 24, 10, 33, 2);

    data["realms"][0]["aptitude"] = 2;
    REQUIRE(read_with_steps(data).has_value());
    // 基準 20: 20 × 30 × 60 / 5000 = 7.2 は 7 動く。消費MP 9 × 70% = 6.3 は 6。難易度 30 − 6
    check_spell(magic.info[LIFE][8], 13, 6, 24, 2);
    check_spell(magic.info_second[LIFE][8], 13, 6, 24, 2);

    // 傾きがすべて 0 なら、段階によらず基準値のまま
    REQUIRE(read_with_steps(data, AptitudeSteps{}).has_value());
    check_spell(magic.info[LIFE][8], 20, 9, 30, 2);
}

/*!
 * @brief 第二領域として選んだときの段階で、第二領域用の表を求める
 */
TEST_CASE("MagicReader derives the second-realm table from aptitude_as_second")
{
    const MagicReaderState state;
    auto data = make_magic_data(SpellInfoList::get_instance());
    data["realms"][0]["aptitude"] = 2;
    data["realms"][0]["aptitude_as_second"] = -1;
    REQUIRE(read_with_steps(data).has_value());
    const auto &magic = class_magics_info[MAGE];
    check_spell(magic.info[LIFE][8], 13, 6, 24, 2);
    check_spell(magic.info_second[LIFE][8], 24, 10, 33, 2);
    CHECK(magic.has_distinct_second(RealmType::LIFE));
    CHECK(&magic.spell_info(RealmType::LIFE, 8, RealmPosition::FIRST) == &magic.info[LIFE][8]);
    CHECK(&magic.spell_info(RealmType::LIFE, 8, RealmPosition::SECOND) == &magic.info_second[LIFE][8]);
    CHECK(&PlayerRealm::get_spell_info(RealmType::LIFE, 8, RealmPosition::SECOND, PlayerClassType::MAGE) == &magic.info_second[LIFE][8]);

    // aptitude と同じ値は書かない
    data["realms"][0]["aptitude_as_second"] = 2;
    CHECK(test::to_error_code(read_with_steps(data)) == ParseErrorType::INVALID_VALUE);
    check_spell(magic.info_second[LIFE][8], 24, 10, 33, 2);

    // 省略すると第一領域と同じ段階になる
    data["realms"][0].erase("aptitude_as_second");
    REQUIRE(read_with_steps(data).has_value());
    check_spell(magic.info_second[LIFE][8], 13, 6, 24, 2);
    CHECK_FALSE(magic.has_distinct_second(RealmType::LIFE));
}

/*!
 * @brief 魔法ごとの上書きは、第一領域用・第二領域用の両方の表に効く
 */
TEST_CASE("MagicReader applies spell overrides to both tables")
{
    const MagicReaderState state;
    auto data = make_magic_data(SpellInfoList::get_instance());
    data["realms"][0]["aptitude"] = -1;
    data["realms"][0]["aptitude_as_second"] = 1;
    data["realms"][0]["overrides"] = nlohmann::json::array({ { { "spell_tag", "ninth" }, { "aptitude", 2 } } });
    REQUIRE(read_with_steps(data).has_value());
    const auto &magic = class_magics_info[MAGE];
    check_spell(magic.info[LIFE][8], 13, 6, 24, 2);
    check_spell(magic.info_second[LIFE][8], 13, 6, 24, 2);
    check_spell(magic.info[LIFE][1], 4, 6, 11, 6);

    // 第一・第二のどちらか一方と同じ段階なら意味があるので書ける
    data["realms"][0]["overrides"][0]["aptitude"] = -1;
    REQUIRE(read_with_steps(data).has_value());
    check_spell(magic.info[LIFE][8], 24, 10, 33, 2);
    check_spell(magic.info_second[LIFE][8], 24, 10, 33, 2);
}

/*!
 * @brief 上書きの誤りを拒否し、既存定義を保持する
 */
TEST_CASE("MagicReader rejects invalid spell overrides")
{
    const MagicReaderState state;
    auto data = make_magic_data(SpellInfoList::get_instance());
    data["realms"][0]["aptitude"] = -1;
    REQUIRE(read_with_steps(data).has_value());

    struct Case {
        const char *name;
        nlohmann::json overrides;
        nlohmann::json unlearnable;
        tl::optional<int> max_book;
        ParseErrorType error;
    };
    const auto override_of = [](nlohmann::json tag, nlohmann::json aptitude) {
        nlohmann::json override_obj = nlohmann::json::object();
        if (!tag.is_null()) {
            override_obj["spell_tag"] = std::move(tag);
        }
        if (!aptitude.is_null()) {
            override_obj["aptitude"] = std::move(aptitude);
        }
        return override_obj;
    };
    const std::vector<Case> cases = {
        { "same as the realm", nlohmann::json::array({ override_of("ninth", -1) }), nullptr, tl::nullopt, ParseErrorType::INVALID_VALUE },
        { "listed twice", nlohmann::json::array({ override_of("ninth", 1), override_of("ninth", 2) }), nullptr, tl::nullopt, ParseErrorType::INVALID_VALUE },
        { "unlearnable spell", nlohmann::json::array({ override_of("second", 1) }), nlohmann::json::array({ "second" }), tl::nullopt, ParseErrorType::INVALID_VALUE },
        { "after max_book", nlohmann::json::array({ override_of("ninth", 1) }), nullptr, 1, ParseErrorType::INVALID_VALUE },
        { "unknown tag", nlohmann::json::array({ override_of("unknown-tag", 1) }), nullptr, tl::nullopt, ParseErrorType::INVALID_FLAG },
        { "missing tag", nlohmann::json::array({ override_of(nullptr, 1) }), nullptr, tl::nullopt, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "missing aptitude", nlohmann::json::array({ override_of("ninth", nullptr) }), nullptr, tl::nullopt, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "aptitude out of range", nlohmann::json::array({ override_of("ninth", 10) }), nullptr, tl::nullopt, ParseErrorType::INVALID_VALUE },
        { "non-integer aptitude", nlohmann::json::array({ override_of("ninth", 1.0) }), nullptr, tl::nullopt, ParseErrorType::INVALID_TYPE },
        { "not an array", nlohmann::json::object(), nullptr, tl::nullopt, ParseErrorType::INVALID_TYPE },
        { "non-object element", nlohmann::json::array({ 1 }), nullptr, tl::nullopt, ParseErrorType::INVALID_TYPE },
    };
    for (const auto &c : cases) {
        CAPTURE(c.name);
        auto candidate = data;
        candidate["realms"][0]["overrides"] = c.overrides;
        if (!c.unlearnable.is_null()) {
            candidate["realms"][0]["unlearnable"] = c.unlearnable;
        }
        if (c.max_book) {
            candidate["realms"][0]["max_book"] = *c.max_book;
        }
        CHECK(test::to_error_code(read_with_steps(candidate)) == c.error);
        check_spell(class_magics_info[MAGE].info[LIFE][8], 24, 10, 33, 2);
    }
}

/*!
 * @brief 職業魔法の定義の誤りの診断に、領域名と魔法の名前を出す
 */
TEST_CASE("MagicReader names the realm and the spell in its diagnostics")
{
    const MagicReaderState state;
    auto data = make_magic_data(SpellInfoList::get_instance());

    SUBCASE("an unknown spell tag names the tag and the realm")
    {
        data["realms"][0]["unlearnable"] = { "no-such-spell" };
        ScopedMessageCapture diagnostics;
        CHECK(test::to_error_code(read_with_steps(data)) == ParseErrorType::INVALID_FLAG);
        REQUIRE(diagnostics.messages().size() >= 3);
        CHECK(diagnostics.messages()[0] == _("魔法 'no-such-spell' は領域の魔法の定義にありません。", "Spell 'no-such-spell' is not defined in the realm."));
        CHECK(diagnostics.messages()[1] == _("習得できない魔法の一覧読込失敗。", "Failed to load unlearnable spells."));
        CHECK(diagnostics.messages()[2] == _("領域 'LIFE' の読込失敗。", "Failed to load realm 'LIFE'."));
    }
    SUBCASE("an invalid aptitude names the realm")
    {
        data["realms"][1]["aptitude"] = 100;
        ScopedMessageCapture diagnostics;
        CHECK(test::to_error_code(read_with_steps(data)) == ParseErrorType::INVALID_VALUE);
        REQUIRE(diagnostics.messages().size() >= 2);
        CHECK(diagnostics.messages()[1] == _("領域 'SORCERY' の読込失敗。", "Failed to load realm 'SORCERY'."));
    }
    SUBCASE("an unknown realm name is echoed")
    {
        data["realms"][2]["name"] = "NO_SUCH_REALM";
        ScopedMessageCapture diagnostics;
        CHECK(test::to_error_code(read_with_steps(data)) == ParseErrorType::INVALID_FLAG);
        REQUIRE_FALSE(diagnostics.messages().empty());
        CHECK(diagnostics.messages()[0] == _("領域 'NO_SUCH_REALM' を知りません。", "Unknown realm 'NO_SUCH_REALM'."));
    }
}

/*!
 * @brief 範囲外・非整数の段階と、範囲外の値になる段階を拒否する
 */
TEST_CASE("MagicReader rejects invalid realm aptitudes")
{
    const MagicReaderState state;
    auto data = make_magic_data(SpellInfoList::get_instance());
    REQUIRE(read_with_steps(data).has_value());
    for (const auto *field : { "aptitude", "aptitude_as_second" }) {
        for (const auto &value : std::vector<nlohmann::json>{ 10, -10, 1.0, "1", true }) {
            CAPTURE(field);
            CAPTURE(value.dump());
            auto candidate = data;
            candidate["realms"][0][field] = value;
            const auto expected = value.is_number_integer() ? ParseErrorType::INVALID_VALUE : ParseErrorType::INVALID_TYPE;
            CHECK(test::to_error_code(read_with_steps(candidate)) == expected);
            check_spell(class_magics_info[MAGE].info[LIFE][8], 20, 9, 30, 2);
        }
    }

    // 段階 7 は消費MP の倍率が 100 − 15 × 7 = −5% になり、値を求められない
    data["realms"][0]["aptitude"] = 7;
    CHECK(test::to_error_code(read_with_steps(data)) == ParseErrorType::INVALID_VALUE);
    data["realms"][0]["aptitude"] = 6;
    REQUIRE(read_with_steps(data).has_value());
    CHECK(class_magics_info[MAGE].info[LIFE][8].smana == 1);
}

/*!
 * @brief 同じ職業に同じ領域を 2 度書くのはエラー
 */
TEST_CASE("MagicReader rejects a realm listed twice")
{
    const MagicReaderState state;
    auto data = make_magic_data(SpellInfoList::get_instance());
    data["realms"].push_back({ { "name", "LIFE" }, { "aptitude", 1 } });
    CHECK(test::to_error_code(read_with_steps(data)) == ParseErrorType::INVALID_VALUE);
    CHECK_FALSE(class_magics_info[MAGE].info[LIFE][0].is_learnable());
}

/*!
 * @brief 職業が選べる魔法領域は、過不足なく書く
 */
TEST_CASE("MagicReader requires exactly the realms that the class can choose")
{
    const MagicReaderState state;
    const auto mage = make_magic_data(SpellInfoList::get_instance());

    // 選べる領域が 1 つでも欠けるとエラー
    auto missing = mage;
    missing["realms"].erase(9);
    CHECK(test::to_error_code(read_with_steps(missing)) == ParseErrorType::TOO_FEW_ARGUMENTS);

    // 混沌の戦士はカオスと悪魔だけを選べる
    auto chaos_warrior = mage;
    chaos_warrior["name"] = "CHAOS_WARRIOR";
    chaos_warrior["realms"] = nlohmann::json::array({ { { "name", "CHAOS" } }, { { "name", "DEMON" } } });
    REQUIRE(read_with_steps(chaos_warrior).has_value());
    auto extra = chaos_warrior;
    extra["realms"].push_back({ { "name", "LIFE" } });
    CHECK(test::to_error_code(read_with_steps(extra)) == ParseErrorType::INVALID_VALUE);

    // 全領域を使う職業 (ソーサラー) は 10 領域すべて
    auto sorcerer = mage;
    sorcerer["name"] = "SORCERER";
    REQUIRE(read_with_steps(sorcerer).has_value());
    sorcerer["realms"].erase(0);
    CHECK(test::to_error_code(read_with_steps(sorcerer)) == ParseErrorType::TOO_FEW_ARGUMENTS);

    // 魔法を使わない職業は書かない
    auto warrior = mage;
    warrior["name"] = "WARRIOR";
    warrior["spell_type"] = "NONE";
    CHECK(test::to_error_code(read_with_steps(warrior)) == ParseErrorType::INVALID_VALUE);
    warrior["realms"] = nlohmann::json::array();
    REQUIRE(read_with_steps(warrior).has_value());
}

/*!
 * @brief aptitude_as_second は、第一領域にも第二領域にも選べる領域にだけ書ける
 */
TEST_CASE("MagicReader allows aptitude_as_second only for realms of both positions")
{
    const MagicReaderState state;
    auto data = make_magic_data(SpellInfoList::get_instance());
    data["name"] = "CHAOS_WARRIOR";
    data["realms"] = nlohmann::json::array({ { { "name", "CHAOS" }, { "aptitude_as_second", 1 } }, { { "name", "DEMON" } } });
    CHECK(test::to_error_code(read_with_steps(data)) == ParseErrorType::INVALID_VALUE);

    // プリーストは暗黒を第一・第二の両方に選べるが、仙術は第二領域だけ
    data["name"] = "PRIEST";
    data["realms"] = make_magic_data(SpellInfoList::get_instance())["realms"];
    data["realms"][4]["aptitude_as_second"] = 1;
    REQUIRE(data["realms"][4]["name"] == "DEATH");
    REQUIRE(read_with_steps(data).has_value());
    data["realms"][1]["aptitude_as_second"] = 1;
    REQUIRE(data["realms"][1]["name"] == "SORCERY");
    CHECK(test::to_error_code(read_with_steps(data)) == ParseErrorType::INVALID_VALUE);
}

/*!
 * @brief ルートの版と段階 1 つあたりの動きを読む
 */
TEST_CASE("MagicReader reads aptitude_steps from the root")
{
    const test::ScopedReaderState reader_state;
    nlohmann::json root = {
        { "version", 2 },
        { "aptitude_steps", { { "learn_level_percent", 30 }, { "mana_cost_percent", 15 }, { "difficulty", 3 } } },
        { "classes", nlohmann::json::array() },
    };
    const auto steps = MagicReader::read_aptitude_steps(root);
    REQUIRE(steps.has_value());
    CHECK(steps->learn_level_percent == 30);
    CHECK(steps->mana_cost_percent == 15);
    CHECK(steps->difficulty == 3);

    struct Case {
        const char *path;
        nlohmann::json value;
        ParseErrorType error;
    };
    const std::vector<Case> cases = {
        { "/version", 1, ParseErrorType::INVALID_VALUE },
        { "/version", 2.0, ParseErrorType::INVALID_TYPE },
        { "/version", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/aptitude_steps", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/aptitude_steps", 1, ParseErrorType::INVALID_TYPE },
        { "/aptitude_steps/learn_level_percent", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/aptitude_steps/learn_level_percent", 101, ParseErrorType::INVALID_VALUE },
        { "/aptitude_steps/learn_level_percent", -1, ParseErrorType::INVALID_VALUE },
        { "/aptitude_steps/mana_cost_percent", 51, ParseErrorType::INVALID_VALUE },
        { "/aptitude_steps/mana_cost_percent", 15.0, ParseErrorType::INVALID_TYPE },
        { "/aptitude_steps/difficulty", 21, ParseErrorType::INVALID_VALUE },
        { "/aptitude_steps/difficulty", "3", ParseErrorType::INVALID_TYPE },
    };
    for (const auto &c : cases) {
        CAPTURE(c.path);
        CAPTURE(c.value.dump());
        auto candidate = root;
        if (c.value.is_null()) {
            const nlohmann::json::json_pointer pointer(c.path);
            candidate[pointer.parent_pointer()].erase(pointer.back());
        } else {
            candidate[nlohmann::json::json_pointer(c.path)] = c.value;
        }
        const auto result = MagicReader::read_aptitude_steps(candidate);
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == c.error);
    }
}

/*!
 * @brief 内部解析の各エラーを公開APIへ伝播し、後続解析で変更した既存定義を保持する
 */
TEST_CASE("MagicReader propagates helper errors without publishing partial data")
{
    const MagicReaderState state;
    auto session = MagicReader::create_session();
    auto data = make_magic_data(SpellInfoList::get_instance());
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
        candidate["realms"][0]["aptitude"] = 1;
        candidate[nlohmann::json::json_pointer(invalid.path)] = invalid.value;
        session = MagicReader::create_session();
        CHECK(test::to_error_code(test::read_and_commit(MagicReader(candidate, session, DEFAULT_STEPS), session)) == invalid.error);
        CHECK_FALSE(session.last_id());
        CHECK(class_magics_info[MAGE].spell_book == ItemKindType::SORCERY_BOOK);
        CHECK(class_magics_info[MAGE].spell_first == 1);
        check_spell(class_magics_info[MAGE].info[LIFE][8], 20, 9, 30, 2);
    }
}

TEST_CASE("MagicReader reports non-string tokens as invalid types")
{
    const MagicReaderState state;
    const auto original = make_magic_data(SpellInfoList::get_instance());
    for (const auto *path : { "/name", "/spell_type", "/magic_status", "/realms/0/name" }) {
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
 * @brief 装備重量閾値の SHRT_MAX/16 境界を受理し、範囲外・非整数を拒否する
 */
TEST_CASE("MagicReader checks the bounds of armour_weight_limit")
{
    const MagicReaderState state;
    auto data = make_magic_data(SpellInfoList::get_instance());
    for (const auto &value : std::vector<nlohmann::json>{ 0, 999, 2047, 2047u }) {
        CAPTURE(value.dump());
        data["armour_weight_limit"] = value;
        REQUIRE(test::read_record<MagicReader>(data).has_value());
        CHECK(class_magics_info[MAGE].spell_weight == value.get<int>());
    }
    for (const auto &value : std::vector<nlohmann::json>{ -1, 2048, std::numeric_limits<uint64_t>::max(), 1.0, true }) {
        CAPTURE(value.dump());
        data["armour_weight_limit"] = value;
        const auto expected = value.is_number_integer() ? ParseErrorType::INVALID_VALUE : ParseErrorType::INVALID_TYPE;
        CHECK(test::to_error_code(test::read_record<MagicReader>(data)) == expected);
        CHECK(class_magics_info[MAGE].spell_weight == 2047);
    }
}

/*!
 * @brief 領域の習得できる巻数を読み、省略時は4にする
 */
TEST_CASE("MagicReader reads max_book and defaults it to four")
{
    const MagicReaderState state;
    auto data = make_magic_data(SpellInfoList::get_instance());
    REQUIRE(test::read_record<MagicReader>(data).has_value());
    CHECK(class_magics_info[MAGE].max_book[LIFE] == MAX_BOOKS_IN_REALM);
    CHECK(class_magics_info[MAGE].max_book[enum2i(RealmType::SORCERY)] == MAX_BOOKS_IN_REALM);

    for (const auto max_book : { 1, 2, 3 }) {
        CAPTURE(max_book);
        data["realms"][0]["max_book"] = max_book;
        REQUIRE(test::read_record<MagicReader>(data).has_value());
        CHECK(class_magics_info[MAGE].max_book[LIFE] == max_book);
        CHECK(PlayerRealm::get_max_book(RealmType::LIFE, PlayerClassType::MAGE) == max_book);
    }

    // 読み直しで省略すると4に戻る
    data["realms"][0].erase("max_book");
    REQUIRE(test::read_record<MagicReader>(data).has_value());
    CHECK(class_magics_info[MAGE].max_book[LIFE] == MAX_BOOKS_IN_REALM);
}

/*!
 * @brief 範囲外・非整数・省略時と同じ値の習得できる巻数を拒否し、既存定義を保持する
 */
TEST_CASE("MagicReader rejects invalid max_book without publishing partial data")
{
    const MagicReaderState state;
    auto session = MagicReader::create_session();
    auto data = make_magic_data(SpellInfoList::get_instance());
    data["realms"][0]["max_book"] = 2;
    REQUIRE(test::read_and_commit(MagicReader(data, session), session).has_value());
    // 4 は範囲内だが、省略時と同じ値なので書かない
    for (const auto &value : std::vector<nlohmann::json>{ 0, 4, 5, -1, 2.0, "2", true }) {
        CAPTURE(value.dump());
        data["realms"][0]["max_book"] = value;
        const auto expected = value.is_number_integer() ? ParseErrorType::INVALID_VALUE : ParseErrorType::INVALID_TYPE;
        session = MagicReader::create_session();
        CHECK(test::to_error_code(test::read_and_commit(MagicReader(data, session), session)) == expected);
        CHECK(class_magics_info[MAGE].max_book[LIFE] == 2);
    }

    // 省略時と同じ 4 は、その理由を出す
    data["realms"][0]["max_book"] = MAX_BOOKS_IN_REALM;
    ScopedMessageCapture diagnostics;
    session = MagicReader::create_session();
    CHECK(test::to_error_code(test::read_and_commit(MagicReader(data, session), session)) == ParseErrorType::INVALID_VALUE);
    REQUIRE_FALSE(diagnostics.messages().empty());
    CHECK(diagnostics.messages()[0] == _("習得できる巻数が省略時と同じ 4 です (書かない)。", "max_book is 4, the same as the default (omit it)."));
}

/*!
 * @brief 技能領域の習得できる巻数は職業によらず4
 */
TEST_CASE("PlayerRealm returns four books for technic realms")
{
    const MagicReaderState state;
    for (const auto realm : { RealmType::MUSIC, RealmType::HISSATSU, RealmType::HEX }) {
        CHECK(PlayerRealm::get_max_book(realm, PlayerClassType::MAGE) == MAX_BOOKS_IN_REALM);
    }
}

/*!
 * @brief 魔法を使わない職業は最初の魔法のレベルを省略でき、魔法書を使う職業は省略できない
 */
TEST_CASE("MagicReader requires first_spell_level only for classes with spellbooks")
{
    const MagicReaderState state;
    auto data = make_magic_data(SpellInfoList::get_instance());
    REQUIRE(test::read_record<MagicReader>(data).has_value());
    CHECK(class_magics_info[MAGE].spell_first == 1);

    data.erase("first_spell_level");
    auto session = MagicReader::create_session();
    CHECK(test::to_error_code(test::read_and_commit(MagicReader(data, session), session)) == ParseErrorType::TOO_FEW_ARGUMENTS);
    CHECK(class_magics_info[MAGE].spell_first == 1);

    // 魔法書を使わない職業は省略でき、読み直すと前の値は残らない
    data["spell_type"] = "NONE";
    REQUIRE(test::read_record<MagicReader>(data).has_value());
    CHECK_FALSE(class_magics_info[MAGE].spell_first);
}

/*!
 * @brief unlearnable に書いた魔法は習得レベルが値なしになり、習得できない
 */
TEST_CASE("MagicReader marks spells listed in unlearnable as not learnable")
{
    const MagicReaderState state;
    auto data = make_magic_data(SpellInfoList::get_instance());
    REQUIRE(test::read_record<MagicReader>(data).has_value());
    CHECK(class_magics_info[MAGE].info[LIFE][1].can_learn_at(3));
    CHECK_FALSE(class_magics_info[MAGE].info[LIFE][1].can_learn_at(2));

    data["realms"][0]["unlearnable"] = nlohmann::json::array({ "second" });
    REQUIRE(test::read_record<MagicReader>(data).has_value());
    check_unlearnable(class_magics_info[MAGE].info[LIFE][1]);
    check_unlearnable(class_magics_info[MAGE].info_second[LIFE][1]);
    check_spell(class_magics_info[MAGE].info[LIFE][0], 1, 2, 7, 4);
    check_spell(class_magics_info[MAGE].info[LIFE][8], 20, 9, 30, 2);

    // 読み直しで一覧から外すと、また習得できる
    data["realms"][0].erase("unlearnable");
    REQUIRE(test::read_record<MagicReader>(data).has_value());
    check_spell(class_magics_info[MAGE].info[LIFE][1], 3, 5, 8, 6);

    // 同じ魔法を 2 度書くのはエラー
    data["realms"][0]["unlearnable"] = nlohmann::json::array({ "second", "second" });
    CHECK(test::to_error_code(test::read_record<MagicReader>(data)) == ParseErrorType::INVALID_VALUE);
    check_spell(class_magics_info[MAGE].info[LIFE][1], 3, 5, 8, 6);
}

/*!
 * @brief 習得できる巻数より後ろの巻の魔法は、書かなくても習得できず、unlearnable に書くとエラーになる
 */
TEST_CASE("MagicReader makes spells after max_book unlearnable")
{
    const MagicReaderState state;
    auto data = make_magic_data(SpellInfoList::get_instance());
    data["realms"][0]["max_book"] = 1;
    REQUIRE(test::read_record<MagicReader>(data).has_value());
    check_unlearnable(class_magics_info[MAGE].info[LIFE][8]);
    check_unlearnable(class_magics_info[MAGE].info_second[LIFE][8]);
    check_spell(class_magics_info[MAGE].info[LIFE][0], 1, 2, 7, 4);

    data["realms"][0]["unlearnable"] = nlohmann::json::array({ "ninth" });
    CHECK(test::to_error_code(test::read_record<MagicReader>(data)) == ParseErrorType::INVALID_VALUE);
}
