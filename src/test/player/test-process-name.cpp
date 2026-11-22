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
 * 2バイト文字のテストデータは src/test/string-helpers.h の定数を使う。
 */

#include "player/process-name.h"

#include "system/h-basic.h"
#include "test/string-helpers.h"
#include "test/temporary-json-files.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>

using namespace test;

TEST_CASE("make_player_base_name keeps a printable name")
{
    CHECK(make_player_base_name("Frodo") == "Frodo");
}

TEST_CASE("make_player_base_name drops non-printable characters")
{
    // 16進エスケープが後続の文字を取り込まないよう、制御文字の後ろは別の文字列として連結する
    CHECK(make_player_base_name(cat("Fro\x01", "do")) == "Frodo");
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
    // 基本名の上限の 31 バイトに収まる文字までを残す (EUC-JP・Shift_JIS では 16 文字のうち 15 文字)
    const auto fit_count = static_cast<int>(31 / KANJI_KAN.length());
    CHECK(make_player_base_name(repeat(KANJI_KAN, fit_count + 1)) == repeat(KANJI_KAN, fit_count));
}

TEST_CASE("make_player_base_name drops a lone lead byte at the end")
{
    const auto name = cat("ab", KANJI_KAN.substr(0, 1));
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
    const auto name = cat(KANJI_KAN, " a");
    CHECK(make_player_name_for_expression(name) == cat(KANJI_KAN, "_a"));
}

TEST_CASE("make_player_name_for_expression keeps a lone lead byte at the end")
{
    // 名前の範囲の外に文字を置いておき、末尾の1バイト目の後ろを読まないことを確かめる
    const auto buffer = cat("a ", KANJI_KAN.substr(0, 1), "XYZ");
    const std::string_view name(buffer.data(), 3);
    CHECK(make_player_name_for_expression(name) == cat("a_", KANJI_KAN.substr(0, 1)));
}
#endif

#if defined(JP) && defined(SJIS)
TEST_CASE("make_player_name_for_expression does not replace a trail byte that equals a bracket")
{
    CHECK(make_player_name_for_expression(DAME_CHOON) == DAME_CHOON);
}
#endif

#if defined(JP) && defined(UTF8)
// テストのファイルは、同時に走るほかのテストと重ならない一時ディレクトリ (TemporaryJsonFiles) に作る
namespace {
constexpr std::string_view NIHON_UTF8 = "\xe6\x97\xa5\xe6\x9c\xac"; //!< 日本 (UTF-8)
constexpr std::string_view NIHON_EUC = "\xc6\xfc\xcb\xdc"; //!< 日本 (EUC-JP)

std::string read_file(const std::filesystem::path &path)
{
    std::ifstream ifs(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
}

/*!
 * @brief 旧名 (EUC-JP) のファイルを作る
 * @return 作れたら true
 * @details UTF-8 として正しくない名前のファイルを作れないファイルシステム (macOS の APFS など) では、
 * 旧名のセーブファイルもできないので、作れなければテストを省く。
 */
bool write_legacy_file(const TemporaryJsonFiles &dir, const std::filesystem::path &name, std::string_view data)
{
    try {
        dir.write_raw(name, data);
        return true;
    } catch (const std::ios_base::failure &) {
        MESSAGE("Skipped: the file system cannot create a file whose name is not valid UTF-8.");
        return false;
    }
}
}

TEST_CASE("rename_legacy_named_file renames a file named in EUC-JP")
{
    const TemporaryJsonFiles dir;
    const auto path = dir.directory / cat("1000.", NIHON_UTF8);
    const auto legacy_path = dir.directory / cat("1000.", NIHON_EUC);
    if (!write_legacy_file(dir, legacy_path.filename(), "legacy")) {
        return;
    }

    // 付け替えたら、今の名前のパスを返す
    CHECK(rename_legacy_named_file(path) == path);
    CHECK(read_file(path) == "legacy");
    CHECK_FALSE(std::filesystem::exists(legacy_path));
}

TEST_CASE("rename_legacy_named_file keeps both files when the file already exists")
{
    const TemporaryJsonFiles dir;
    const auto path = dir.directory / cat("1000.", NIHON_UTF8);
    const auto legacy_path = dir.directory / cat("1000.", NIHON_EUC);
    dir.write_raw(path.filename(), "current");
    if (!write_legacy_file(dir, legacy_path.filename(), "legacy")) {
        return;
    }

    CHECK_FALSE(rename_legacy_named_file(path).has_value());
    CHECK(read_file(path) == "current");
    CHECK(read_file(legacy_path) == "legacy");
}

TEST_CASE("rename_legacy_named_file does nothing without a file named in EUC-JP")
{
    const TemporaryJsonFiles dir;
    const auto path = dir.directory / cat("1000.", NIHON_UTF8);
    CHECK_FALSE(rename_legacy_named_file(path).has_value());
    CHECK_FALSE(std::filesystem::exists(path));

    // ASCII だけの名前は旧名も同じなので、引き継がない
    const auto ascii_path = dir.directory / "1000.Frodo";
    dir.write_raw(ascii_path.filename(), "ascii");
    CHECK_FALSE(rename_legacy_named_file(ascii_path).has_value());
    CHECK(read_file(ascii_path) == "ascii");
}
#endif
