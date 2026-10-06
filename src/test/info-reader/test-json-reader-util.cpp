/*!
 * @brief 定義ファイル(JSON)読込の共通ユーティリティのテスト
 *
 * 各リーダーがJSONから値を取り出すのに使う get_json_value / info_set_* を検証する。
 * ここが誤ると全ての定義ファイルの読込に影響するため、正常系だけでなく
 * キー欠落・型不一致・範囲外のときに返すエラーコードと、
 * そのときに格納先の変数を書き換えないことも確かめる。
 */

#include "info-reader/json-reader-util.h"

#include "info-reader/parse-error-types.h"
#include "util/dice.h"

#ifdef JP
#include "locale/character-encoding.h"
#endif

#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

TEST_CASE("JSON definition roots require an object containing the named array")
{
    const std::vector<nlohmann::json> invalid_roots = {
        nullptr,
        true,
        1,
        "vaults",
        nlohmann::json::array(),
        nlohmann::json::object(),
        { { "other", nlohmann::json::array() } },
        { { "vaults", nullptr } },
        { { "vaults", 1 } },
        { { "vaults", "bad" } },
        { { "vaults", nlohmann::json::object() } },
    };
    for (const auto &root : invalid_roots) {
        const auto saved = root;
        CHECK(info_validate_json_array(root, "vaults") != PARSE_ERROR_NONE);
        CHECK(root == saved);
    }
}

TEST_CASE("Empty definition arrays are rejected only when requested")
{
    const nlohmann::json empty = { { "vaults", nlohmann::json::array() } };
    CHECK(info_validate_json_array(empty, "vaults") == PARSE_ERROR_NONE);
    CHECK(info_validate_json_array(empty, "vaults", false) == PARSE_ERROR_INVALID_VALUE);
    const nlohmann::json nonempty = { { "vaults", { { { "id", 0 } } } } };
    CHECK(info_validate_json_array(nonempty, "vaults", false) == PARSE_ERROR_NONE);
    CHECK(info_validate_json_array(nonempty, "missing", false) == PARSE_ERROR_TOO_FEW_ARGUMENTS);
}

#if defined(JP) && defined(EUC)
// _LIBICONV_VERSION (GNU libiconv の判定に使う) を参照するため
#include <iconv.h>
#endif

namespace {

//! info_set_integer がenum型を扱えることを確かめるためのenum
enum class TestKind : int {
    NONE = 0,
    FIRST = 1,
    LAST = 10,
};

//! 実行中のビルドで info_set_string が読むキーと、読まない方のキー
#ifdef JP
constexpr auto LANG_KEY = "ja";
constexpr auto OTHER_LANG_KEY = "en";
#else
constexpr auto LANG_KEY = "en";
constexpr auto OTHER_LANG_KEY = "ja";
#endif

//! 値が格納されなかったことを確認するための、テスト内では現れない番兵値
constexpr auto SENTINEL = -12345;

#ifdef JP
//! 文字コード変換のテストに使うUTF-8のバイト列 ("テスト")。
//! ソースの文字コード変換の影響を受けないよう、エスケープでバイト列を直接書く。
//! const char* にすると失敗時にポインタ値が表示されるため std::string で持つ
const std::string UTF8_TEST_STRING = "\xe3\x83\x86\xe3\x82\xb9\xe3\x83\x88";
#endif

}

TEST_CASE("get_json_value returns the value of the key")
{
    const auto json = nlohmann::json::parse(R"({ "level": 5, "nested": { "inner": 1 } })");

    SUBCASE("existing key")
    {
        const auto &value = get_json_value(json, "level");
        REQUIRE(value.is_number_integer());
        CHECK(value.get<int>() == 5);
    }

    SUBCASE("nested object is returned as it is")
    {
        const auto &nested = get_json_value(json, "nested");
        REQUIRE(nested.is_object());
        const auto &inner = get_json_value(nested, "inner");
        CHECK(inner.get<int>() == 1);
    }

    SUBCASE("missing key")
    {
        const auto &value = get_json_value(json, "unknown");
        CHECK(value.is_null());
    }
}

TEST_CASE("get_json_value returns null for a key whose value is null")
{
    // 定義ファイルにnullと書かれたキーは、書かれていないキーと同じ扱いになる。
    // info_set_* はいずれもこの性質に依存している
    const auto json = nlohmann::json::parse(R"({ "key": null })");

    const auto &value = get_json_value(json, "key");
    CHECK(value.is_null());
}

TEST_CASE("get_json_value returns null for a non-object JSON")
{
    // 定義ファイル側の記述が誤っていても、キー取得は例外ではなくnullになる
    for (const auto *const json_str : { "[ 1, 2, 3 ]", R"("string")", "42", "1.5", "true", "null" }) {
        CAPTURE(json_str);

        const auto json = nlohmann::json::parse(json_str);
        const auto &value = get_json_value(json, "key");
        CHECK(value.is_null());
    }
}

TEST_CASE("info_set_integer stores the value")
{
    auto data = SENTINEL;

    SUBCASE("positive value")
    {
        CHECK(info_set_integer(nlohmann::json(100), data, true) == PARSE_ERROR_NONE);
        CHECK(data == 100);
    }

    SUBCASE("negative value")
    {
        CHECK(info_set_integer(nlohmann::json(-100), data, true) == PARSE_ERROR_NONE);
        CHECK(data == -100);
    }

    SUBCASE("optional key is stored as well")
    {
        // 必須でないキーでも、値が書かれていれば格納する
        CHECK(info_set_integer(nlohmann::json(100), data, false) == PARSE_ERROR_NONE);
        CHECK(data == 100);
    }

    SUBCASE("value stored into a short")
    {
        short short_data = 0;
        CHECK(info_set_integer(nlohmann::json(30000), short_data, true) == PARSE_ERROR_NONE);
        CHECK(short_data == 30000);
    }

    SUBCASE("enum value")
    {
        auto kind = TestKind::NONE;
        CHECK(info_set_integer(nlohmann::json(1), kind, true) == PARSE_ERROR_NONE);
        CHECK(kind == TestKind::FIRST);
    }

    SUBCASE("no range preserves conversion to the destination type")
    {
        // Rangeを省略した場合は、格納先の表現範囲も検証しない。
        std::uint8_t narrow_data = 123;
        CHECK(info_set_integer(nlohmann::json(300), narrow_data, true) == PARSE_ERROR_NONE);
        CHECK(narrow_data == 44);
        CHECK(info_set_integer(nlohmann::json(-1), narrow_data, false) == PARSE_ERROR_NONE);
        CHECK(narrow_data == 255);
    }
}

TEST_CASE("info_set_integer treats a null JSON as an unwritten key")
{
    auto data = SENTINEL;
    const nlohmann::json null_json;

    SUBCASE("required key is an error")
    {
        CHECK(info_set_integer(null_json, data, true) == PARSE_ERROR_TOO_FEW_ARGUMENTS);
    }

    SUBCASE("optional key is not an error")
    {
        CHECK(info_set_integer(null_json, data, false) == PARSE_ERROR_NONE);
    }

    // どちらの場合も格納先は元の値のままにしておく必要がある
    CHECK(data == SENTINEL);
}

TEST_CASE("info_set_integer rejects a non-integer JSON")
{
    auto data = SENTINEL;

    SUBCASE("string")
    {
        CHECK(info_set_integer(nlohmann::json("100"), data, true) == PARSE_ERROR_INVALID_TYPE);
    }

    SUBCASE("floating point number")
    {
        CHECK(info_set_integer(nlohmann::json(1.5), data, true) == PARSE_ERROR_INVALID_TYPE);
    }

    SUBCASE("boolean")
    {
        CHECK(info_set_integer(nlohmann::json(true), data, true) == PARSE_ERROR_INVALID_TYPE);
    }

    SUBCASE("optional key is an error too")
    {
        // 必須でなくても、書かれている値の型が誤っていれば見逃さない
        CHECK(info_set_integer(nlohmann::json("100"), data, false) == PARSE_ERROR_INVALID_TYPE);
    }

    SUBCASE("containers are not integers")
    {
        for (const auto &json : { nlohmann::json::array(), nlohmann::json::object() }) {
            for (const auto is_required : { false, true }) {
                CHECK(info_set_integer(json, data, is_required) == PARSE_ERROR_INVALID_TYPE);
                CHECK(data == SENTINEL);
            }
        }
    }

    CHECK(data == SENTINEL);
}

TEST_CASE("info_set_integer checks the value range")
{
    auto data = SENTINEL;

    SUBCASE("both ends of the range are valid")
    {
        CHECK(info_set_integer(nlohmann::json(0), data, true, Range(0, 128)) == PARSE_ERROR_NONE);
        CHECK(data == 0);

        CHECK(info_set_integer(nlohmann::json(128), data, true, Range(0, 128)) == PARSE_ERROR_NONE);
        CHECK(data == 128);
    }

    SUBCASE("below the range")
    {
        CHECK(info_set_integer(nlohmann::json(-1), data, true, Range(0, 128)) == PARSE_ERROR_INVALID_FLAG);
        CHECK(data == SENTINEL);
    }

    SUBCASE("above the range")
    {
        CHECK(info_set_integer(nlohmann::json(129), data, true, Range(0, 128)) == PARSE_ERROR_INVALID_FLAG);
        CHECK(data == SENTINEL);
    }

    SUBCASE("range containing negative numbers")
    {
        CHECK(info_set_integer(nlohmann::json(-99), data, true, Range(-99, 99)) == PARSE_ERROR_NONE);
        CHECK(data == -99);

        CHECK(info_set_integer(nlohmann::json(-100), data, true, Range(-99, 99)) == PARSE_ERROR_INVALID_FLAG);
        CHECK(data == -99);
    }

    SUBCASE("enum value out of the range")
    {
        auto kind = TestKind::NONE;
        CHECK(info_set_integer(nlohmann::json(11), kind, true, Range(0, 10)) == PARSE_ERROR_INVALID_FLAG);
        CHECK(kind == TestKind::NONE);

        CHECK(info_set_integer(nlohmann::json(10), kind, true, Range(0, 10)) == PARSE_ERROR_NONE);
        CHECK(kind == TestKind::LAST);
    }

    SUBCASE("value which does not fit in the destination type")
    {
        // 範囲外の値が格納先の型で表現できない場合も範囲チェックで弾く。
        // 格納先の型へ変換してから範囲を見ると、uint8_t への 300 は 44 に切り詰められて
        // Range(0, 255) を通ってしまう (terrain の "power" がこの形で読まれている)。
        // 番兵値は切り詰め後の値 (44, 255) と重ならないものにする
        constexpr uint8_t uint8_sentinel = 123;
        auto uint8_data = uint8_sentinel;

        CHECK(info_set_integer(nlohmann::json(300), uint8_data, true, Range(0, 255)) == PARSE_ERROR_INVALID_FLAG);
        CHECK(uint8_data == uint8_sentinel);

        CHECK(info_set_integer(nlohmann::json(-1), uint8_data, true, Range(0, 255)) == PARSE_ERROR_INVALID_FLAG);
        CHECK(uint8_data == uint8_sentinel);

        CHECK(info_set_integer(nlohmann::json(255), uint8_data, true, Range(0, 255)) == PARSE_ERROR_NONE);
        CHECK(uint8_data == 255);
    }
}

TEST_CASE("info_set_integer checks signed int boundaries before conversion")
{
    constexpr auto minimum = std::numeric_limits<int>::min();
    constexpr auto maximum = std::numeric_limits<int>::max();
    auto data = SENTINEL;

    CHECK(info_set_integer(nlohmann::json(minimum), data, true, Range(minimum, maximum)) == PARSE_ERROR_NONE);
    CHECK(data == minimum);
    CHECK(info_set_integer(nlohmann::json(maximum), data, true, Range(minimum, maximum)) == PARSE_ERROR_NONE);
    CHECK(data == maximum);

    const auto below = static_cast<std::int64_t>(minimum) - 1;
    const auto above = static_cast<std::int64_t>(maximum) + 1;
    CHECK(info_set_integer(nlohmann::json(below), data, true, Range(minimum, maximum)) == PARSE_ERROR_INVALID_FLAG);
    CHECK(data == maximum);
    CHECK(info_set_integer(nlohmann::json(above), data, true, Range(minimum, maximum)) == PARSE_ERROR_INVALID_FLAG);
    CHECK(data == maximum);
}

TEST_CASE("info_set_integer checks unsigned JSON values and numeric types")
{
    auto data = SENTINEL;
    constexpr auto maximum = std::numeric_limits<int>::max();
    const auto unsigned_maximum = nlohmann::json(static_cast<std::uint64_t>(maximum));
    const auto unsigned_above = nlohmann::json(static_cast<std::uint64_t>(maximum) + 1);

    CHECK(info_set_integer(unsigned_maximum, data, true, Range(0, maximum)) == PARSE_ERROR_NONE);
    CHECK(data == maximum);
    CHECK(info_set_integer(unsigned_above, data, true, Range(0, maximum)) == PARSE_ERROR_INVALID_FLAG);
    CHECK(data == maximum);
    CHECK(info_set_integer(nlohmann::json(1.0), data, true, Range(0, maximum)) == PARSE_ERROR_INVALID_TYPE);
    CHECK(data == maximum);
}

TEST_CASE("info_set_integer checks large unsigned JSON range and storage")
{
    const auto largest = std::numeric_limits<std::uint64_t>::max();
    const auto json = nlohmann::json::parse("18446744073709551615");
    REQUIRE(json.is_number_unsigned());
    auto data = SENTINEL;

    CHECK(info_set_integer(json, data, true, Range(-1, 1)) == PARSE_ERROR_INVALID_FLAG);
    CHECK(data == SENTINEL);
    CHECK(info_set_integer(json, data, false, Range(std::numeric_limits<int>::min(), std::numeric_limits<int>::max())) == PARSE_ERROR_INVALID_FLAG);
    CHECK(data == SENTINEL);

    std::uint64_t unsigned_data = 0;
    CHECK(info_set_integer(json, unsigned_data, true) == PARSE_ERROR_NONE);
    CHECK(unsigned_data == largest);
}

TEST_CASE("info_set_integer compares unsigned JSON values with signed range bounds")
{
    for (const auto is_required : { false, true }) {
        const nlohmann::json zero = std::uint64_t(0);
        REQUIRE(zero.is_number_unsigned());
        auto data = SENTINEL;

        CHECK(info_set_integer(zero, data, is_required, Range(-1, 1)) == PARSE_ERROR_NONE);
        CHECK(data == 0);
        CHECK(info_set_integer(zero, data, is_required, Range(-3, -1)) == PARSE_ERROR_INVALID_FLAG);
        CHECK(data == 0);
        CHECK(info_set_integer(zero, data, is_required, Range(1, 3)) == PARSE_ERROR_INVALID_FLAG);
        CHECK(data == 0);
    }
}

TEST_CASE("info_set_string picks the string of the language of the build")
{
    nlohmann::json json;
    json[LANG_KEY] = "name-of-this-build";
    json[OTHER_LANG_KEY] = "name-of-the-other-build";

    std::string data;
    CHECK(info_set_string(json, data, true) == PARSE_ERROR_NONE);
    CHECK(data == "name-of-this-build");
}

TEST_CASE("info_set_string ignores malformed values of the other language")
{
    const std::vector<nlohmann::json> ignored_values = {
        nullptr,
        true,
        100,
        nlohmann::json::array(),
        nlohmann::json::object(),
        std::string("\xff"),
        std::string("a\0b", 3),
    };
    for (const auto &ignored : ignored_values) {
        for (const auto is_required : { false, true }) {
            nlohmann::json json = { { LANG_KEY, "selected" }, { OTHER_LANG_KEY, ignored } };
            std::string data = "unchanged";
            CHECK(info_set_string(json, data, is_required) == PARSE_ERROR_NONE);
            CHECK(data == "selected");

            json.erase(LANG_KEY);
            data = "unchanged";
            CHECK(info_set_string(json, data, is_required) == (is_required ? PARSE_ERROR_TOO_FEW_ARGUMENTS : PARSE_ERROR_NONE));
            CHECK(data == "unchanged");
        }
    }
}

TEST_CASE("info_set_string stores an empty selected string")
{
    for (const auto is_required : { false, true }) {
        const nlohmann::json json = { { LANG_KEY, "" }, { OTHER_LANG_KEY, nullptr } };
        std::string data = "unchanged";
        CHECK(info_set_string(json, data, is_required) == PARSE_ERROR_NONE);
        CHECK(data.empty());
    }
}

TEST_CASE("info_set_string preserves the language-specific embedded NUL behavior")
{
    const std::string value("a\0b", 3);
    const nlohmann::json json = { { LANG_KEY, value } };
    for (const auto is_required : { false, true }) {
        std::string data = "unchanged";
#ifdef JP
        CHECK(info_set_string(json, data, is_required) == PARSE_ERROR_INVALID_FLAG);
        CHECK(data == "unchanged");
#else
        CHECK(info_set_string(json, data, is_required) == PARSE_ERROR_NONE);
        CHECK(data == value);
#endif
    }
}

TEST_CASE("info_set_string leaves malformed UTF-8 handling to the existing conversion")
{
    const std::string value("\xff");
    const nlohmann::json json = { { LANG_KEY, value } };
    for (const auto is_required : { false, true }) {
        std::string data = "unchanged";
#ifdef JP
        // Windows may replace invalid UTF-8, while iconv can reject it. Do not add a stricter policy here.
        const auto converted = utf8_to_sys(value);
        CHECK(info_set_string(json, data, is_required) == (converted ? PARSE_ERROR_NONE : PARSE_ERROR_INVALID_FLAG));
        CHECK(data == (converted ? *converted : "unchanged"));
#else
        CHECK(info_set_string(json, data, is_required) == PARSE_ERROR_NONE);
        CHECK(data == value);
#endif
    }
}

TEST_CASE("info_set_string rejects all non-string selected values without changing output")
{
    const std::vector<nlohmann::json> invalid_values = {
        nullptr,
        true,
        100,
        1.5,
        nlohmann::json::array(),
        nlohmann::json::object(),
    };
    for (const auto &value : invalid_values) {
        for (const auto is_required : { false, true }) {
            const nlohmann::json json = { { LANG_KEY, value }, { OTHER_LANG_KEY, "other" } };
            std::string data = "unchanged";
            CHECK(info_set_string(json, data, is_required) == PARSE_ERROR_INVALID_TYPE);
            CHECK(data == "unchanged");
        }
    }
}

#ifdef JP
TEST_CASE("info_set_string converts the UTF-8 string into the system encoding")
{
    nlohmann::json json;
    json["ja"] = UTF8_TEST_STRING;

    std::string data;
    REQUIRE(info_set_string(json, data, true) == PARSE_ERROR_NONE);

    // 日本語版のシステム文字コード(EUC-JP/Shift_JIS)はUTF-8と異なるので、
    // 変換されていれば元のバイト列とは一致しない
    CHECK(data != UTF8_TEST_STRING);

    // 変換結果の内容まで確かめる。期待するバイト列はEUC-JPとShift_JISで異なるため、
    // UTF-8へ戻して元の文字列と一致することを見る
    const auto restored = sys_to_utf8(data);
    REQUIRE(restored.has_value());
    CHECK(*restored == UTF8_TEST_STRING);
}
#endif

#if defined(JP) && defined(EUC) && (defined(__GLIBC__) || defined(_LIBICONV_VERSION))
TEST_CASE("info_set_string cannot convert a character missing from EUC-JP")
{
    // U+1F600 (GRINNING FACE)。EUC-JPに対応する文字がないため変換に失敗する。
    // Windows版(Shift_JIS)は MultiByteToWideChar/WideCharToMultiByte をエラー指定なしで
    // 呼ぶので置換文字になって成功してしまう。そのためEUC版限定のテストとする。
    // さらに、変換不能文字でiconvがエラーを返すのは glibc/GNU libiconv での挙動で、
    // musl のiconvは '*' に置換して成功扱いにする。そのため glibc (__GLIBC__) か
    // GNU libiconv (_LIBICONV_VERSION) のときに限定し、musl 系ではこのテストごと除外する
    nlohmann::json json;
    json["ja"] = "\xf0\x9f\x98\x80";

    std::string data = "unchanged";

    // 変換失敗は必須かどうかによらずエラーになり、格納先は書き換えない
    CHECK(info_set_string(json, data, true) == PARSE_ERROR_INVALID_FLAG);
    CHECK(info_set_string(json, data, false) == PARSE_ERROR_INVALID_FLAG);

    CHECK(data == "unchanged");

    // 変換に失敗しても後続の変換に影響しないことを確かめる。
    // utf8_to_euc の iconv_t は関数内staticで、変換失敗後にリセットされないため
    nlohmann::json valid_json;
    valid_json["ja"] = UTF8_TEST_STRING;

    REQUIRE(info_set_string(valid_json, data, true) == PARSE_ERROR_NONE);
    const auto restored = sys_to_utf8(data);
    REQUIRE(restored.has_value());
    CHECK(*restored == UTF8_TEST_STRING);
}
#endif

TEST_CASE("info_set_string treats a missing string as an unwritten key")
{
    std::string data = "unchanged";

    SUBCASE("null JSON")
    {
        const nlohmann::json null_json;

        CHECK(info_set_string(null_json, data, true) == PARSE_ERROR_TOO_FEW_ARGUMENTS);
        CHECK(info_set_string(null_json, data, false) == PARSE_ERROR_NONE);
    }

    SUBCASE("object which has only the key of the other language")
    {
        nlohmann::json json;
        json[OTHER_LANG_KEY] = "name-of-the-other-build";

        CHECK(info_set_string(json, data, true) == PARSE_ERROR_TOO_FEW_ARGUMENTS);
        CHECK(info_set_string(json, data, false) == PARSE_ERROR_NONE);
    }

    CHECK(data == "unchanged");
}

TEST_CASE("info_set_string rejects a JSON of an invalid type")
{
    std::string data = "unchanged";

    SUBCASE("string written directly instead of an object")
    {
        // 言語別のオブジェクトではなく文字列が直接書かれている場合
        CHECK(info_set_string(nlohmann::json("name"), data, true) == PARSE_ERROR_INVALID_TYPE);
        CHECK(info_set_string(nlohmann::json("name"), data, false) == PARSE_ERROR_INVALID_TYPE);
    }

    SUBCASE("the value of the language key is not a string")
    {
        nlohmann::json json;
        json[LANG_KEY] = 100;

        CHECK(info_set_string(json, data, true) == PARSE_ERROR_INVALID_TYPE);
        CHECK(info_set_string(json, data, false) == PARSE_ERROR_INVALID_TYPE);
    }

    CHECK(data == "unchanged");
}

TEST_CASE("info_set_dice parses the dice string")
{
    Dice dice;

    SUBCASE("single digit")
    {
        CHECK(info_set_dice(nlohmann::json("3d5"), dice, true) == PARSE_ERROR_NONE);
        CHECK(dice == Dice(3, 5));
    }

    SUBCASE("multiple digits")
    {
        CHECK(info_set_dice(nlohmann::json("10d100"), dice, true) == PARSE_ERROR_NONE);
        CHECK(dice == Dice(10, 100));
    }

    SUBCASE("optional string is parsed as well")
    {
        CHECK(info_set_dice(nlohmann::json("3d5"), dice, false) == PARSE_ERROR_NONE);
        CHECK(dice == Dice(3, 5));
    }
}

TEST_CASE("info_set_dice treats a null JSON as an unwritten key")
{
    Dice dice(1, 1);
    const nlohmann::json null_json;

    CHECK(info_set_dice(null_json, dice, true) == PARSE_ERROR_TOO_FEW_ARGUMENTS);
    CHECK(info_set_dice(null_json, dice, false) == PARSE_ERROR_NONE);

    CHECK(dice == Dice(1, 1));
}

TEST_CASE("info_set_dice rejects an invalid dice notation")
{
    Dice dice(1, 1);

    SUBCASE("non-string JSON is a type error")
    {
        const std::vector<nlohmann::json> invalid_values = {
            3,
            1.5,
            true,
            nlohmann::json::array(),
            nlohmann::json::object(),
        };
        for (const auto &json : invalid_values) {
            for (const auto is_required : { false, true }) {
                CHECK(info_set_dice(json, dice, is_required) == PARSE_ERROR_INVALID_TYPE);
                CHECK(dice == Dice(1, 1));
            }
        }
    }

    SUBCASE("malformed dice string")
    {
        // Dice::parse が投げる例外は捕捉され、必須かどうかによらずエラーになる
        for (const auto *const dice_str : { "3", "3d", "dice", "" }) {
            CAPTURE(dice_str);

            CHECK(info_set_dice(nlohmann::json(dice_str), dice, true) == PARSE_ERROR_TOO_FEW_ARGUMENTS);
            CHECK(info_set_dice(nlohmann::json(dice_str), dice, false) == PARSE_ERROR_TOO_FEW_ARGUMENTS);
        }
    }

    CHECK(dice == Dice(1, 1));
}

TEST_CASE("info_set_bool stores the value")
{
    for (const auto is_required : { false, true }) {
        auto data = false;

        CHECK(info_set_bool(nlohmann::json(true), data, is_required) == PARSE_ERROR_NONE);
        CHECK(data);

        CHECK(info_set_bool(nlohmann::json(false), data, is_required) == PARSE_ERROR_NONE);
        CHECK_FALSE(data);
    }
}

TEST_CASE("info_set_bool treats a non-boolean JSON as an unwritten key")
{
    // 格納先を書き換えないことを確かめるため、既定値の false ではなく true から始める。
    // false で始めると、誤って false を書き込む実装でもテストが通ってしまう
    auto data = true;

    SUBCASE("null JSON")
    {
        const nlohmann::json null_json;

        CHECK(info_set_bool(null_json, data, true) == PARSE_ERROR_TOO_FEW_ARGUMENTS);
        CHECK(info_set_bool(null_json, data, false) == PARSE_ERROR_NONE);
    }

    SUBCASE("JSON of another type")
    {
        // 他の info_set_* と異なり、型が違っても PARSE_ERROR_INVALID_TYPE ではなく
        // キーが書かれていない場合と同じ扱いになる
        const std::vector<nlohmann::json> invalid_values = {
            "true",
            1,
            1.5,
            nlohmann::json::array(),
            nlohmann::json::object(),
        };
        for (const auto &json : invalid_values) {
            CAPTURE(json.type_name());

            CHECK(info_set_bool(json, data, true) == PARSE_ERROR_TOO_FEW_ARGUMENTS);
            CHECK(data);
            CHECK(info_set_bool(json, data, false) == PARSE_ERROR_NONE);
            CHECK(data);
        }
    }

    CHECK(data);
}
