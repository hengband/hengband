/*!
 * @brief 定義ファイル(JSON)読込の共通ユーティリティのテスト
 *
 * 各リーダーがJSONから値を取り出すのに使う get_json_value / info_set_* を検証する。
 * ここが誤ると全ての定義ファイルの読込に影響するため、正常系だけでなく
 * キー欠落・型不一致・範囲外のときに返すエラーコードと、
 * そのときに格納先の変数を書き換えないことも確かめる。
 *
 * @details 検証対象の関数呼び出しを `== 期待値` と組み合わせて CHECK に直接書くと、
 * MSVCが評価順序に関する警告 (C4866) を出すことがある。この警告はエラーとして
 * 扱われるため、呼び出しの結果は一旦変数で受けてから比較する。
 * (関数呼び出しを CHECK に直接書くこと自体が警告になるわけではない。
 *  test-info-reader-util.cpp では直接書いて警告なくビルドできている)
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
#include <string>

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
        const auto level = value.get<int>();
        CHECK(level == 5);
    }

    SUBCASE("nested object is returned as it is")
    {
        const auto &nested = get_json_value(json, "nested");
        REQUIRE(nested.is_object());
        const auto &inner = get_json_value(nested, "inner");
        const auto inner_value = inner.get<int>();
        CHECK(inner_value == 1);
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
    for (const auto *const json_str : { "[ 1, 2, 3 ]", R"("string")", "42", "null" }) {
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
        const auto result = info_set_integer(nlohmann::json(100), data, true);
        CHECK(result == PARSE_ERROR_NONE);
        CHECK(data == 100);
    }

    SUBCASE("negative value")
    {
        const auto result = info_set_integer(nlohmann::json(-100), data, true);
        CHECK(result == PARSE_ERROR_NONE);
        CHECK(data == -100);
    }

    SUBCASE("optional key is stored as well")
    {
        // 必須でないキーでも、値が書かれていれば格納する
        const auto result = info_set_integer(nlohmann::json(100), data, false);
        CHECK(result == PARSE_ERROR_NONE);
        CHECK(data == 100);
    }

    SUBCASE("value stored into a short")
    {
        short short_data = 0;
        const auto result = info_set_integer(nlohmann::json(30000), short_data, true);
        CHECK(result == PARSE_ERROR_NONE);
        CHECK(short_data == 30000);
    }

    SUBCASE("enum value")
    {
        auto kind = TestKind::NONE;
        const auto result = info_set_integer(nlohmann::json(1), kind, true);
        CHECK(result == PARSE_ERROR_NONE);
        CHECK(kind == TestKind::FIRST);
    }
}

TEST_CASE("info_set_integer treats a null JSON as an unwritten key")
{
    auto data = SENTINEL;
    const nlohmann::json null_json;

    SUBCASE("required key is an error")
    {
        const auto result = info_set_integer(null_json, data, true);
        CHECK(result == PARSE_ERROR_TOO_FEW_ARGUMENTS);
    }

    SUBCASE("optional key is not an error")
    {
        const auto result = info_set_integer(null_json, data, false);
        CHECK(result == PARSE_ERROR_NONE);
    }

    // どちらの場合も格納先は元の値のままにしておく必要がある
    CHECK(data == SENTINEL);
}

TEST_CASE("info_set_integer rejects a non-integer JSON")
{
    auto data = SENTINEL;

    SUBCASE("string")
    {
        const auto result = info_set_integer(nlohmann::json("100"), data, true);
        CHECK(result == PARSE_ERROR_INVALID_TYPE);
    }

    SUBCASE("floating point number")
    {
        const auto result = info_set_integer(nlohmann::json(1.5), data, true);
        CHECK(result == PARSE_ERROR_INVALID_TYPE);
    }

    SUBCASE("boolean")
    {
        const auto result = info_set_integer(nlohmann::json(true), data, true);
        CHECK(result == PARSE_ERROR_INVALID_TYPE);
    }

    SUBCASE("optional key is an error too")
    {
        // 必須でなくても、書かれている値の型が誤っていれば見逃さない
        const auto result = info_set_integer(nlohmann::json("100"), data, false);
        CHECK(result == PARSE_ERROR_INVALID_TYPE);
    }

    CHECK(data == SENTINEL);
}

TEST_CASE("info_set_integer checks the value range")
{
    auto data = SENTINEL;

    SUBCASE("both ends of the range are valid")
    {
        const auto lower_result = info_set_integer(nlohmann::json(0), data, true, Range(0, 128));
        CHECK(lower_result == PARSE_ERROR_NONE);
        CHECK(data == 0);

        const auto upper_result = info_set_integer(nlohmann::json(128), data, true, Range(0, 128));
        CHECK(upper_result == PARSE_ERROR_NONE);
        CHECK(data == 128);
    }

    SUBCASE("below the range")
    {
        const auto result = info_set_integer(nlohmann::json(-1), data, true, Range(0, 128));
        CHECK(result == PARSE_ERROR_INVALID_FLAG);
        CHECK(data == SENTINEL);
    }

    SUBCASE("above the range")
    {
        const auto result = info_set_integer(nlohmann::json(129), data, true, Range(0, 128));
        CHECK(result == PARSE_ERROR_INVALID_FLAG);
        CHECK(data == SENTINEL);
    }

    SUBCASE("range containing negative numbers")
    {
        const auto valid_result = info_set_integer(nlohmann::json(-99), data, true, Range(-99, 99));
        CHECK(valid_result == PARSE_ERROR_NONE);
        CHECK(data == -99);

        const auto invalid_result = info_set_integer(nlohmann::json(-100), data, true, Range(-99, 99));
        CHECK(invalid_result == PARSE_ERROR_INVALID_FLAG);
        CHECK(data == -99);
    }

    SUBCASE("enum value out of the range")
    {
        auto kind = TestKind::NONE;
        const auto invalid_result = info_set_integer(nlohmann::json(11), kind, true, Range(0, 10));
        CHECK(invalid_result == PARSE_ERROR_INVALID_FLAG);
        CHECK(kind == TestKind::NONE);

        const auto valid_result = info_set_integer(nlohmann::json(10), kind, true, Range(0, 10));
        CHECK(valid_result == PARSE_ERROR_NONE);
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

        const auto above_result = info_set_integer(nlohmann::json(300), uint8_data, true, Range(0, 255));
        CHECK(above_result == PARSE_ERROR_INVALID_FLAG);
        CHECK(uint8_data == uint8_sentinel);

        const auto below_result = info_set_integer(nlohmann::json(-1), uint8_data, true, Range(0, 255));
        CHECK(below_result == PARSE_ERROR_INVALID_FLAG);
        CHECK(uint8_data == uint8_sentinel);

        const auto valid_result = info_set_integer(nlohmann::json(255), uint8_data, true, Range(0, 255));
        CHECK(valid_result == PARSE_ERROR_NONE);
        CHECK(uint8_data == 255);
    }
}

TEST_CASE("info_set_string picks the string of the language of the build")
{
    nlohmann::json json;
    json[LANG_KEY] = "name-of-this-build";
    json[OTHER_LANG_KEY] = "name-of-the-other-build";

    std::string data;
    const auto result = info_set_string(json, data, true);
    CHECK(result == PARSE_ERROR_NONE);
    CHECK(data == "name-of-this-build");
}

#ifdef JP
TEST_CASE("info_set_string converts the UTF-8 string into the system encoding")
{
    nlohmann::json json;
    json["ja"] = UTF8_TEST_STRING;

    std::string data;
    const auto result = info_set_string(json, data, true);
    REQUIRE(result == PARSE_ERROR_NONE);

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
    const auto required_result = info_set_string(json, data, true);
    CHECK(required_result == PARSE_ERROR_INVALID_FLAG);

    const auto optional_result = info_set_string(json, data, false);
    CHECK(optional_result == PARSE_ERROR_INVALID_FLAG);

    CHECK(data == "unchanged");

    // 変換に失敗しても後続の変換に影響しないことを確かめる。
    // utf8_to_euc の iconv_t は関数内staticで、変換失敗後にリセットされないため
    nlohmann::json valid_json;
    valid_json["ja"] = UTF8_TEST_STRING;

    const auto result_after_failure = info_set_string(valid_json, data, true);
    REQUIRE(result_after_failure == PARSE_ERROR_NONE);
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

        const auto required_result = info_set_string(null_json, data, true);
        CHECK(required_result == PARSE_ERROR_TOO_FEW_ARGUMENTS);

        const auto optional_result = info_set_string(null_json, data, false);
        CHECK(optional_result == PARSE_ERROR_NONE);
    }

    SUBCASE("object which has only the key of the other language")
    {
        nlohmann::json json;
        json[OTHER_LANG_KEY] = "name-of-the-other-build";

        const auto required_result = info_set_string(json, data, true);
        CHECK(required_result == PARSE_ERROR_TOO_FEW_ARGUMENTS);

        const auto optional_result = info_set_string(json, data, false);
        CHECK(optional_result == PARSE_ERROR_NONE);
    }

    CHECK(data == "unchanged");
}

TEST_CASE("info_set_string rejects a JSON of an invalid type")
{
    std::string data = "unchanged";

    SUBCASE("string written directly instead of an object")
    {
        // 言語別のオブジェクトではなく文字列が直接書かれている場合
        const auto required_result = info_set_string(nlohmann::json("name"), data, true);
        CHECK(required_result == PARSE_ERROR_INVALID_TYPE);

        const auto optional_result = info_set_string(nlohmann::json("name"), data, false);
        CHECK(optional_result == PARSE_ERROR_INVALID_TYPE);
    }

    SUBCASE("the value of the language key is not a string")
    {
        nlohmann::json json;
        json[LANG_KEY] = 100;

        const auto required_result = info_set_string(json, data, true);
        CHECK(required_result == PARSE_ERROR_INVALID_TYPE);

        const auto optional_result = info_set_string(json, data, false);
        CHECK(optional_result == PARSE_ERROR_INVALID_TYPE);
    }

    CHECK(data == "unchanged");
}

TEST_CASE("info_set_dice parses the dice string")
{
    Dice dice;

    SUBCASE("single digit")
    {
        const auto result = info_set_dice(nlohmann::json("3d5"), dice, true);
        CHECK(result == PARSE_ERROR_NONE);
        CHECK(dice == Dice(3, 5));
    }

    SUBCASE("multiple digits")
    {
        const auto result = info_set_dice(nlohmann::json("10d100"), dice, true);
        CHECK(result == PARSE_ERROR_NONE);
        CHECK(dice == Dice(10, 100));
    }
}

TEST_CASE("info_set_dice treats a null JSON as an unwritten key")
{
    Dice dice(1, 1);
    const nlohmann::json null_json;

    const auto required_result = info_set_dice(null_json, dice, true);
    CHECK(required_result == PARSE_ERROR_TOO_FEW_ARGUMENTS);

    const auto optional_result = info_set_dice(null_json, dice, false);
    CHECK(optional_result == PARSE_ERROR_NONE);

    CHECK(dice == Dice(1, 1));
}

TEST_CASE("info_set_dice rejects an invalid dice notation")
{
    Dice dice(1, 1);

    SUBCASE("non-string JSON is a type error")
    {
        const auto required_result = info_set_dice(nlohmann::json(3), dice, true);
        CHECK(required_result == PARSE_ERROR_INVALID_TYPE);

        const auto optional_result = info_set_dice(nlohmann::json(3), dice, false);
        CHECK(optional_result == PARSE_ERROR_INVALID_TYPE);
    }

    SUBCASE("malformed dice string")
    {
        // Dice::parse が投げる例外は捕捉され、必須かどうかによらずエラーになる
        for (const auto *const dice_str : { "3", "3d", "dice", "" }) {
            CAPTURE(dice_str);

            const auto required_result = info_set_dice(nlohmann::json(dice_str), dice, true);
            CHECK(required_result == PARSE_ERROR_TOO_FEW_ARGUMENTS);

            const auto optional_result = info_set_dice(nlohmann::json(dice_str), dice, false);
            CHECK(optional_result == PARSE_ERROR_TOO_FEW_ARGUMENTS);
        }
    }

    CHECK(dice == Dice(1, 1));
}

TEST_CASE("info_set_bool stores the value")
{
    auto data = false;

    const auto true_result = info_set_bool(nlohmann::json(true), data, true);
    CHECK(true_result == PARSE_ERROR_NONE);
    CHECK(data);

    const auto false_result = info_set_bool(nlohmann::json(false), data, true);
    CHECK(false_result == PARSE_ERROR_NONE);
    CHECK_FALSE(data);
}

TEST_CASE("info_set_bool treats a non-boolean JSON as an unwritten key")
{
    // 格納先を書き換えないことを確かめるため、既定値の false ではなく true から始める。
    // false で始めると、誤って false を書き込む実装でもテストが通ってしまう
    auto data = true;

    SUBCASE("null JSON")
    {
        const nlohmann::json null_json;

        const auto required_result = info_set_bool(null_json, data, true);
        CHECK(required_result == PARSE_ERROR_TOO_FEW_ARGUMENTS);

        const auto optional_result = info_set_bool(null_json, data, false);
        CHECK(optional_result == PARSE_ERROR_NONE);
    }

    SUBCASE("JSON of another type")
    {
        // 他の info_set_* と異なり、型が違っても PARSE_ERROR_INVALID_TYPE ではなく
        // キーが書かれていない場合と同じ扱いになる
        for (const auto &json : { nlohmann::json("true"), nlohmann::json(1) }) {
            CAPTURE(json.type_name());

            const auto required_result = info_set_bool(json, data, true);
            CHECK(required_result == PARSE_ERROR_TOO_FEW_ARGUMENTS);

            const auto optional_result = info_set_bool(json, data, false);
            CHECK(optional_result == PARSE_ERROR_NONE);
        }
    }

    CHECK(data);
}
