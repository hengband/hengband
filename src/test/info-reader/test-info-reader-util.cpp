/*!
 * @brief 定義ファイル読込の共通ユーティリティのテスト
 *
 * 定義ファイル中の文字列を定数へ変換する info_get_const / info_grab_one_const を検証する。
 * JSON から値を取り出す info_set_* 群は test-json-reader-util.cpp で扱う。
 *
 * 同じヘッダで宣言されている grab_one_activation_flag は test-activation-reader.cpp で扱う。
 */

#include "info-reader/info-reader-util.h"

#include <doctest/doctest.h>

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>

namespace {

//! 変換対象の定数。実際の定義ファイルと同じく、値が連番でない場合も含める
enum class TestFlag {
    FIRST = 1,
    SECOND = 2,
    LARGE = 0x40000000,
};

//! 実際のトークン辞書 (f_info_flags など) と同じ形の辞書
const std::unordered_map<std::string_view, TestFlag> TEST_FLAGS = {
    { "FIRST", TestFlag::FIRST },
    { "SECOND", TestFlag::SECOND },
    { "LARGE", TestFlag::LARGE },
};

}

TEST_CASE("info_get_const returns the constant of the token")
{
    CHECK(info_get_const(TEST_FLAGS, "FIRST") == TestFlag::FIRST);
    CHECK(info_get_const(TEST_FLAGS, "SECOND") == TestFlag::SECOND);
}

TEST_CASE("info_get_const returns nullopt for an unknown token")
{
    CHECK_FALSE(info_get_const(TEST_FLAGS, "UNKNOWN").has_value());

    // 大文字小文字は区別され、空文字列も見つからない扱いになる
    CHECK_FALSE(info_get_const(TEST_FLAGS, "first").has_value());
    CHECK_FALSE(info_get_const(TEST_FLAGS, "").has_value());
}

TEST_CASE("info_get_const accepts any dictionary type indexed by the key")
{
    // std::string をキーとする std::map でも同じように引ける
    const std::map<std::string, int> dict = { { "ONE", 1 }, { "TWO", 2 } };

    CHECK(info_get_const(dict, "ONE") == 1);
    CHECK(info_get_const(dict, std::string("TWO")) == 2);
    CHECK_FALSE(info_get_const(dict, "THREE").has_value());
}

TEST_CASE("info_grab_one_const stores the constant only when the token is found")
{
    constexpr uint32_t initial_value = 0xdeadbeef;
    auto buf = initial_value;

    SUBCASE("known token")
    {
        CHECK(info_grab_one_const(buf, TEST_FLAGS, "SECOND"));
        CHECK(buf == static_cast<uint32_t>(TestFlag::SECOND));
    }

    SUBCASE("value which uses the upper bits")
    {
        CHECK(info_grab_one_const(buf, TEST_FLAGS, "LARGE"));
        CHECK(buf == static_cast<uint32_t>(TestFlag::LARGE));
    }

    SUBCASE("unknown token leaves the buffer as it is")
    {
        CHECK_FALSE(info_grab_one_const(buf, TEST_FLAGS, "UNKNOWN"));
        CHECK(buf == initial_value);
    }
}
