/*!
 * @brief プレイヤー名の処理のテスト
 *
 * player/process-name.h の make_player_base_name() と make_player_name_for_expression() を検証する。
 * 基本名はセーブファイルや設定ファイルの名前に使われるため、従来の変換結果を変えないことと、
 * 名前の範囲外を読まずに PlayerType::base_name に収まる長さまでで打ち切ることを確かめる。
 * 条件式の $PLAYER で使う名前は、区切りに使われる文字を置き換えつつ、2バイト文字を壊さず、
 * 名前の範囲外を読まないことを確かめる。
 *
 * 区切り文字 (PATH_SEP) は Unix 版と Windows 版で異なるため、テストデータでも PATH_SEP を使う。
 * 2バイト文字のテストデータは、src/test/util/test-string-processor.cpp と同じく16進エスケープで書く。
 */

#include "player/process-name.h"

#include "system/h-basic.h"

#include <doctest/doctest.h>

#include <string>
#include <string_view>

namespace {

#if defined(JP) && defined(SJIS)
constexpr std::string_view KANJI_KAN = "\x8a\xbf"; //!< 漢
constexpr std::string_view DAME_CHOON = "\x81\x5b"; //!< ー (後半バイトが 0x5b、ASCIIの '[')
#elif defined(JP)
constexpr std::string_view KANJI_KAN = "\xb4\xc1"; //!< 漢
#endif

#ifdef JP
/*!
 * @brief 文字列を指定した回数だけ繰り返した文字列を作る
 * @param str 繰り返す文字列
 * @param count 繰り返す回数
 * @return 繰り返した文字列
 */
std::string repeat(std::string_view str, int count)
{
    std::string result;
    for (auto i = 0; i < count; ++i) {
        result.append(str);
    }

    return result;
}
#endif

}

TEST_CASE("make_player_base_name keeps a printable name")
{
    CHECK(make_player_base_name("Frodo") == "Frodo");
}

TEST_CASE("make_player_base_name drops non-printable characters")
{
    // 16進エスケープが後続の文字を取り込まないよう、制御文字の後ろは別の文字列として連結する
    CHECK(make_player_base_name(std::string("Fro\x01") + "do") == "Frodo");
}

TEST_CASE("make_player_base_name falls back to PLAYER for an empty result")
{
    CHECK(make_player_base_name("") == "PLAYER");
    CHECK(make_player_base_name("\x01") == "PLAYER");
}

TEST_CASE("make_player_base_name replaces a path separator and skips the next character")
{
    // 区切り文字の直後の1文字が落ちるのは従来の挙動。既存のセーブファイル名と対応させるため変えない
    const auto name = std::string("ab") + PATH_SEP + "cd";
    CHECK(make_player_base_name(name) == "ab_d");
}

TEST_CASE("make_player_base_name does not read past a trailing path separator")
{
    // 名前の範囲の外に文字を置いておき、読み飛ばしで範囲外を読まないことを確かめる
    const auto buffer = std::string("ab") + PATH_SEP + "XYZ";
    const std::string_view name(buffer.data(), 2 + std::string_view(PATH_SEP).length());
    CHECK(make_player_base_name(name) == "ab_");
}

TEST_CASE("make_player_base_name truncates a long name to fit the base name")
{
    const std::string name(40, 'a');
    CHECK(make_player_base_name(name) == std::string(31, 'a'));
}

#ifdef JP
TEST_CASE("make_player_base_name does not split a double-byte character at the length limit")
{
    // 16文字 (32バイト) のうち、31バイトに収まる15文字 (30バイト) までを残す
    CHECK(make_player_base_name(repeat(KANJI_KAN, 16)) == repeat(KANJI_KAN, 15));
}

TEST_CASE("make_player_base_name drops a lone lead byte at the end")
{
    const auto name = std::string("ab") + std::string(KANJI_KAN.substr(0, 1));
    CHECK(make_player_base_name(name) == "ab");
}
#endif

TEST_CASE("make_player_name_for_expression replaces spaces and brackets with underscores")
{
    CHECK(make_player_name_for_expression("[ Temp ]") == "__Temp__");
}

TEST_CASE("make_player_name_for_expression keeps other characters")
{
    CHECK(make_player_name_for_expression("Frodo-2") == "Frodo-2");
}

#ifdef JP
TEST_CASE("make_player_name_for_expression keeps a double-byte character")
{
    const auto name = std::string(KANJI_KAN) + " a";
    CHECK(make_player_name_for_expression(name) == std::string(KANJI_KAN) + "_a");
}

TEST_CASE("make_player_name_for_expression keeps a lone lead byte at the end")
{
    // 名前の範囲の外に文字を置いておき、末尾の1バイト目の後ろを読まないことを確かめる
    const auto buffer = std::string("a ") + std::string(KANJI_KAN.substr(0, 1)) + "XYZ";
    const std::string_view name(buffer.data(), 3);
    CHECK(make_player_name_for_expression(name) == std::string("a_") + std::string(KANJI_KAN.substr(0, 1)));
}
#endif

#if defined(JP) && defined(SJIS)
TEST_CASE("make_player_name_for_expression does not replace a trail byte that equals a bracket")
{
    CHECK(make_player_name_for_expression(DAME_CHOON) == DAME_CHOON);
}
#endif
