/*!
 * @brief 文字列処理の汎用ユーティリティのテスト
 *
 * util/string-processor.h で宣言されている関数を検証する。
 *
 * 日本語版では2バイト文字を含む文字列の扱いが加わるため、テストを3層に分けている。
 * - 文字コードに依存しないもの (全ビルドで実行する)
 * - 2バイト文字を分断しないことの検証 (#ifdef JP。EUC-JPでもShift_JISでも成立する)
 * - ダメ文字の検証 (#if defined(JP) && defined(SJIS)。Windows版でのみ実行する)
 *
 * ダメ文字とは、Shift_JISの2バイト文字のうち後半バイトがASCIIの範囲と重なるものを指す。
 * EUC-JPでは後半バイトがASCIIの範囲と重ならないため、Linux/macOSのビルドでは再現できない。
 * ダメ文字のテストはMSVCのCI (Debug構成は JP + SJIS) で実行される。
 *
 * 2バイト文字のテストデータは必ず16進エスケープで書くこと。
 * 日本語版のビルドはソースの文字列リテラルを変換する (autotoolsは gcc-wrap が nkf で
 * EUC-JPへ、MSVCは /execution-charset:shift-jis でShift_JISへ) が、英語版では変換されない。
 * ソースに日本語をそのまま書くと、ビルド構成によってバイト列が変わってしまう。
 */

#include "util/string-processor.h"

#include "system/h-basic.h"

#include <doctest/doctest.h>

#include <cstdint>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace {

/*!
 * @brief テストデータの文字列を連結する
 *
 * 16進エスケープは後続の文字まで貪欲に取り込むため ("\x83\x5c" の直後に "A" を
 * 隣接させると "\x5cA" と解釈される)、リテラルの隣接連結を使わずにこの関数で連結する。
 */
template <typename... Args>
std::string cat(const Args &...args)
{
    std::string result;
    (result.append(args), ...);
    return result;
}

#ifdef JP
#ifdef SJIS
constexpr std::string_view KANJI_KAN = "\x8a\xbf"; //!< 漢
constexpr std::string_view KANJI_JI = "\x8e\x9a"; //!< 字
#else
constexpr std::string_view KANJI_KAN = "\xb4\xc1"; //!< 漢
constexpr std::string_view KANJI_JI = "\xbb\xfa"; //!< 字
#endif
#endif

#if defined(JP) && defined(SJIS)
constexpr std::string_view DAME_SO = "\x83\x5c"; //!< ソ (後半バイトが 0x5c、ASCIIの '\')
constexpr std::string_view DAME_HYOU = "\x95\x5c"; //!< 表 (後半バイトが 0x5c、ASCIIの '\')
constexpr std::string_view DAME_KANA_A = "\x83\x41"; //!< ア (後半バイトが 0x41、ASCIIの 'A')
constexpr std::string_view DAME_CHOON = "\x81\x5b"; //!< ー (後半バイトが 0x5b、ASCIIの '[')
constexpr std::string_view DAME_SPACE = "\x81\x40"; //!< 全角スペース (後半バイトが 0x40、ASCIIの '@')
#endif

}

//
// 文字コードに依存しないテスト
//

TEST_CASE("str_to_num converts a decimal string")
{
    CHECK(str_to_num<int>("123") == 123);
    CHECK(str_to_num<int>("-123") == -123);
    CHECK(str_to_num<int>("0") == 0);
}

TEST_CASE("str_to_num converts a string in the specified base")
{
    CHECK(str_to_num<int>("ff", 16) == 255);
    CHECK(str_to_num<int>("1010", 2) == 10);
    CHECK(str_to_num<int>("z", 36) == 35);
}

TEST_CASE("str_to_num rejects a string which is not entirely a number")
{
    CHECK_FALSE(str_to_num<int>("").has_value());
    CHECK_FALSE(str_to_num<int>("12a").has_value());
    CHECK_FALSE(str_to_num<int>("12 ").has_value());
    CHECK_FALSE(str_to_num<int>(" 12").has_value());

    // 先頭の + や基数の接頭辞は解釈しない
    CHECK_FALSE(str_to_num<int>("+12").has_value());
    CHECK_FALSE(str_to_num<int>("0x10", 16).has_value());
}

TEST_CASE("str_to_num rejects a base out of range")
{
    CHECK_FALSE(str_to_num<int>("1", 1).has_value());
    CHECK_FALSE(str_to_num<int>("1", 37).has_value());
}

TEST_CASE("str_to_num rejects a value which does not fit in the type")
{
    CHECK(str_to_num<uint8_t>("255") == 255);
    CHECK_FALSE(str_to_num<uint8_t>("256").has_value());
    CHECK_FALSE(str_to_num<uint8_t>("-1").has_value());
}

TEST_CASE("angband_strcpy copies the whole string when it fits")
{
    char buf[16] = {};
    CHECK(angband_strcpy(buf, "abc", sizeof(buf)) == 3);
    CHECK(std::string_view(buf) == "abc");
}

TEST_CASE("angband_strcpy truncates the string to fit the buffer")
{
    char buf[4] = {};

    // 戻り値は切り詰める前のバイト数なので、bufsize と比べて切り詰めの有無が分かる
    CHECK(angband_strcpy(buf, "abcdef", sizeof(buf)) == 6);
    CHECK(std::string_view(buf) == "abc");
}

TEST_CASE("angband_strcpy terminates the buffer even with an empty source")
{
    char buf[4] = "xyz";
    CHECK(angband_strcpy(buf, "", sizeof(buf)) == 0);
    CHECK(std::string_view(buf).empty());
}

TEST_CASE("angband_strcpy writes nothing when the buffer size is one")
{
    char buf[4] = "xyz";
    CHECK(angband_strcpy(buf, "abc", 1) == 3);
    CHECK(std::string_view(buf).empty());
}

TEST_CASE("angband_strcpy writes nothing when the buffer size is zero")
{
    char buf[4] = "xyz";
    CHECK(angband_strcpy(buf, "abc", 0) == 3);
    CHECK(std::string_view(buf) == "xyz");
}

TEST_CASE("angband_strcpy looks only at the range of the given view")
{
    // NUL終端されていない部分ビューを渡しても、ビューの範囲を越えて読まない
    constexpr std::string_view src = "abcdef";
    char buf[16] = {};
    CHECK(angband_strcpy(buf, src.substr(0, 3), sizeof(buf)) == 3);
    CHECK(std::string_view(buf) == "abc");
}

TEST_CASE("angband_strcat appends to the existing string")
{
    char buf[16] = "abc";
    CHECK(angband_strcat(buf, "def", sizeof(buf)) == 6);
    CHECK(std::string_view(buf) == "abcdef");
}

TEST_CASE("angband_strcat truncates the appended string")
{
    char buf[5] = "abc";
    CHECK(angband_strcat(buf, "def", sizeof(buf)) == 6);
    CHECK(std::string_view(buf) == "abcd");
}

TEST_CASE("angband_strcat leaves the buffer untouched when it is already full")
{
    char buf[4] = "abc";
    CHECK(angband_strcat(buf, "def", sizeof(buf)) == 6);
    CHECK(std::string_view(buf) == "abc");
}

TEST_CASE("angband_strcat writes nothing when the buffer size is zero")
{
    char buf[4] = "abc";
    CHECK(angband_strcat(buf, "def", 0) == 6);
    CHECK(std::string_view(buf) == "abc");
}

TEST_CASE("angband_strstr finds the first occurrence of the needle")
{
    constexpr const char *haystack = "abcabc";
    CHECK(angband_strstr(haystack, "abc") == haystack);
    CHECK(angband_strstr(haystack, "bc") == haystack + 1);
    CHECK(angband_strstr(haystack, "c") == haystack + 2);
}

TEST_CASE("angband_strstr returns nullptr when the needle is not found")
{
    constexpr const char *haystack = "abc";
    CHECK(angband_strstr(haystack, "d") == nullptr);
    CHECK(angband_strstr(haystack, "abcd") == nullptr);
    CHECK(angband_strstr("", "a") == nullptr);
}

TEST_CASE("angband_strstr returns the head of the haystack for an empty needle")
{
    constexpr const char *haystack = "abc";
    CHECK(angband_strstr(haystack, "") == haystack);
}

TEST_CASE("angband_strchr finds the first occurrence of the character")
{
    constexpr const char *str = "abcabc";
    CHECK(angband_strchr(str, 'a') == str);
    CHECK(angband_strchr(str, 'c') == str + 2);
    CHECK(angband_strchr(str, 'd') == nullptr);
    CHECK(angband_strchr("", 'a') == nullptr);
}

TEST_CASE("angband_strchr cannot find the terminating NUL")
{
    // strchr と異なり、終端のNUL文字は見つけられない
    CHECK(angband_strchr("abc", '\0') == nullptr);
}

TEST_CASE("ltrim skips the leading spaces")
{
    char buf[] = "  abc ";
    CHECK(std::string_view(ltrim(buf)) == "abc ");
}

TEST_CASE("ltrim does not skip a tab")
{
    char buf[] = "\tabc";
    CHECK(std::string_view(ltrim(buf)) == "\tabc");
}

TEST_CASE("ltrim accepts a string which consists of spaces or is empty")
{
    char spaces[] = "   ";
    CHECK(std::string_view(ltrim(spaces)).empty());

    char empty[] = "";
    CHECK(std::string_view(ltrim(empty)).empty());
}

TEST_CASE("rtrim removes the trailing spaces")
{
    char buf[] = " abc  ";
    CHECK(std::string_view(rtrim(buf)) == " abc");
}

TEST_CASE("rtrim does not remove a tab")
{
    char buf[] = "abc\t";
    CHECK(std::string_view(rtrim(buf)) == "abc\t");
}

TEST_CASE("rtrim accepts a string which consists of spaces or is empty")
{
    char spaces[] = "   ";
    CHECK(std::string_view(rtrim(spaces)).empty());

    char empty[] = "";
    CHECK(std::string_view(rtrim(empty)).empty());
}

TEST_CASE("str_find tells whether the string contains the substring")
{
    CHECK(str_find("abcdef", "cde"));
    CHECK(str_find("abcdef", "abcdef"));
    CHECK_FALSE(str_find("abcdef", "cdf"));
    CHECK_FALSE(str_find("", "a"));

    // 空文字列はどの文字列にも含まれる
    CHECK(str_find("abcdef", ""));
}

TEST_CASE("str_trim removes the spaces and tabs at both ends")
{
    CHECK(str_trim(" \t abc \t ") == "abc");
    CHECK(str_trim("abc") == "abc");

    // 内側の空白は残る
    CHECK(str_trim(" a b ") == "a b");
}

TEST_CASE("str_trim returns an empty string when the whole string is blank")
{
    CHECK(str_trim(" \t ").empty());
    CHECK(str_trim("").empty());
}

TEST_CASE("str_rtrim removes the spaces and tabs at the right end only")
{
    CHECK(str_rtrim(" \t abc \t ") == " \t abc");
    CHECK(str_rtrim(" \t ").empty());
    CHECK(str_rtrim("").empty());
}

TEST_CASE("str_ltrim removes the spaces and tabs at the left end only")
{
    CHECK(str_ltrim(" \t abc \t ") == "abc \t ");
    CHECK(str_ltrim(" \t ").empty());
    CHECK(str_ltrim("").empty());
}

TEST_CASE("str_split splits the string at the delimiter")
{
    const auto tokens = str_split("a:bb:ccc", ':');
    REQUIRE(tokens.size() == 3);
    CHECK(tokens[0] == "a");
    CHECK(tokens[1] == "bb");
    CHECK(tokens[2] == "ccc");
}

TEST_CASE("str_split always returns at least one element")
{
    const auto without_delim = str_split("abc", ':');
    REQUIRE(without_delim.size() == 1);
    CHECK(without_delim[0] == "abc");

    const auto empty = str_split("", ':');
    REQUIRE(empty.size() == 1);
    CHECK(empty[0].empty());
}

TEST_CASE("str_split makes an empty element for each extra delimiter")
{
    const auto tokens = str_split(":a::b:", ':');
    REQUIRE(tokens.size() == 5);
    CHECK(tokens[0].empty());
    CHECK(tokens[1] == "a");
    CHECK(tokens[2].empty());
    CHECK(tokens[3] == "b");
    CHECK(tokens[4].empty());
}

TEST_CASE("str_split trims each element when requested")
{
    const auto tokens = str_split(" a :\tb\t", ':', true);
    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0] == "a");
    CHECK(tokens[1] == "b");
}

TEST_CASE("str_split does not pad the result up to the reserve hint")
{
    // num は領域確保のヒントであり、要素数を揃えるものではない
    CHECK(str_split("a:b", ':', false, 10).size() == 2);
}

TEST_CASE("str_separate splits the string into chunks of the given size")
{
    const auto parts = str_separate("abcdef", 2);
    REQUIRE(parts.size() == 3);
    CHECK(parts[0] == "ab");
    CHECK(parts[1] == "cd");
    CHECK(parts[2] == "ef");
}

TEST_CASE("str_separate puts the remainder into the last chunk")
{
    const auto parts = str_separate("abcde", 2);
    REQUIRE(parts.size() == 3);
    CHECK(parts[2] == "e");
}

TEST_CASE("str_separate returns an empty vector for an empty string")
{
    CHECK(str_separate("", 2).empty());
}

TEST_CASE("str_separate gives up when it cannot take even one byte")
{
    // 幅0では1バイトも取り出せない。無限ループせずに打ち切る
    CHECK(str_separate("abc", 0).empty());
}

TEST_CASE("str_erase removes every specified character")
{
    CHECK(str_erase("abcabc", "a") == "bcbc");
    CHECK(str_erase("abcabc", "ac") == "bb");
    CHECK(str_erase("abc", "abc").empty());
}

TEST_CASE("str_erase leaves the string as it is when nothing matches")
{
    CHECK(str_erase("abc", "xyz") == "abc");
    CHECK(str_erase("abc", "") == "abc");
    CHECK(str_erase("", "abc").empty());
}

TEST_CASE("str_replace replaces every occurrence")
{
    CHECK(str_replace("abcabc", "a", "X") == "XbcXbc");
    CHECK(str_replace("abc", "abc", "X") == "X");
    CHECK(str_replace("abc", "b", "") == "ac");
}

TEST_CASE("str_replace prefers the earlier match when the matches overlap")
{
    CHECK(str_replace("aaa", "aa", "X") == "Xa");
}

TEST_CASE("str_replace leaves the string as it is when nothing matches")
{
    CHECK(str_replace("abc", "x", "y") == "abc");
    CHECK(str_replace("", "x", "y").empty());

    // 置き換え元が空文字列なら何もしない
    CHECK(str_replace("abc", "", "X") == "abc");
}

TEST_CASE("str_substr takes the substring at the given position")
{
    constexpr std::string_view str = "abcdef";
    CHECK(str_substr(str, 2, 3) == "cde");
    CHECK(str_substr(str, 2) == "cdef");
    CHECK(str_substr(str) == "abcdef");
}

TEST_CASE("str_substr clamps the range to the length of the string")
{
    constexpr std::string_view str = "abc";
    CHECK(str_substr(str, 1, 100) == "bc");
    CHECK(str_substr(str, 3).empty());
    CHECK(str_substr(str, 100).empty());
    CHECK(str_substr(str, 1, 0).empty());
}

TEST_CASE("str_substr accepts a rvalue string and a pointer to a string")
{
    CHECK(str_substr(std::string("abcdef"), 1, 2) == "bc");
    CHECK(str_substr(std::string("abc"), 100).empty());
    CHECK(str_substr("abcdef", 1, 2) == "bc");
}

TEST_CASE("str_toupper converts the alphabets to upper case")
{
    CHECK(str_toupper("abcXYZ 123!") == "ABCXYZ 123!");
    CHECK(str_toupper("").empty());
}

TEST_CASE("str_tolower converts the alphabets to lower case")
{
    CHECK(str_tolower("abcXYZ 123!") == "abcxyz 123!");
    CHECK(str_tolower("").empty());
}

TEST_CASE("str_upcase_first converts only the first character")
{
    CHECK(str_upcase_first("abc def") == "Abc def");
    CHECK(str_upcase_first("Abc") == "Abc");
}

TEST_CASE("str_upcase_first leaves a string which does not start with an alphabet")
{
    CHECK(str_upcase_first("1abc") == "1abc");
    CHECK(str_upcase_first(" abc") == " abc");
    CHECK(str_upcase_first("").empty());
}

TEST_CASE("extract_suffix extracts the substring from the found character")
{
    const auto found = extract_suffix("abc@def", '@');
    REQUIRE(found.has_value());
    CHECK(*found == "@def");

    const auto at_head = extract_suffix("@abc", '@');
    REQUIRE(at_head.has_value());
    CHECK(*at_head == "@abc");

    CHECK_FALSE(extract_suffix("abc", '@').has_value());
    CHECK_FALSE(extract_suffix("", '@').has_value());
}

TEST_CASE("extract_suffix extracts the substring from the found string")
{
    const auto found = extract_suffix("abcdef", "cd");
    REQUIRE(found.has_value());
    CHECK(*found == "cdef");

    CHECK_FALSE(extract_suffix("abcdef", "xy").has_value());

    // 空文字列は先頭で見つかる
    const auto empty_needle = extract_suffix("abc", "");
    REQUIRE(empty_needle.has_value());
    CHECK(*empty_needle == "abc");
}

TEST_CASE("count_digits counts the digits of the value")
{
    CHECK(count_digits(0) == 1);
    CHECK(count_digits(7) == 1);
    CHECK(count_digits(10) == 2);
    CHECK(count_digits(999) == 3);
}

TEST_CASE("count_digits does not count the sign")
{
    CHECK(count_digits(-1) == 1);
    CHECK(count_digits(-123) == 3);
}

TEST_CASE("count_digits counts in the specified base")
{
    CHECK(count_digits(255, 16) == 2);
    CHECK(count_digits(256, 16) == 3);
    CHECK(count_digits(8, 2) == 4);
}

TEST_CASE("count_digits returns zero for an invalid base")
{
    CHECK(count_digits(123, 1) == 0);
    CHECK(count_digits(123, 0) == 0);
    CHECK(count_digits(123, -1) == 0);
}

TEST_CASE("hexify_upper returns the upper digit of the hexadecimal notation")
{
    CHECK(hexify_upper(0x00) == '0');
    CHECK(hexify_upper(0x5a) == '5');
    CHECK(hexify_upper(0xa0) == 'A');
    CHECK(hexify_upper(0xff) == 'F');
}

TEST_CASE("hexify_lower returns the lower digit of the hexadecimal notation")
{
    CHECK(hexify_lower(0x00) == '0');
    CHECK(hexify_lower(0x5a) == 'A');
    CHECK(hexify_lower(0xa9) == '9');
    CHECK(hexify_lower(0xff) == 'F');
}

TEST_CASE("octify returns the octal digit")
{
    CHECK(octify(0) == '0');
    CHECK(octify(7) == '7');

    // 8以上の値は8で割った余りを変換する
    CHECK(octify(8) == '0');
    CHECK(octify(255) == '7');
}

TEST_CASE("is_numeric accepts only the ASCII digits")
{
    CHECK(is_numeric('0'));
    CHECK(is_numeric('5'));
    CHECK(is_numeric('9'));

    CHECK_FALSE(is_numeric('/'));
    CHECK_FALSE(is_numeric(':'));
    CHECK_FALSE(is_numeric('a'));
    CHECK_FALSE(is_numeric('\0'));
}

#ifdef JP

//
// 2バイト文字を分断しないことのテスト (EUC-JP / Shift_JIS 共通)
//

TEST_CASE("angband_strcpy does not truncate in the middle of a two byte character")
{
    // 「漢」しか入らない大きさのバッファに「漢字」を書き込む
    char buf[4] = {};
    CHECK(angband_strcpy(buf, cat(KANJI_KAN, KANJI_JI), sizeof(buf)) == 4);
    CHECK(std::string_view(buf) == KANJI_KAN);
}

TEST_CASE("angband_strcpy drops a two byte character which lacks its trailing byte")
{
    char buf[16] = {};
    CHECK(angband_strcpy(buf, cat("a", KANJI_KAN.substr(0, 1)), sizeof(buf)) == 2);
    CHECK(std::string_view(buf) == "a");
}

TEST_CASE("str_substr does not split a two byte character")
{
    const auto str = cat(KANJI_KAN, KANJI_JI);

    // 終了位置が2バイト文字の前半バイトなので手前で区切る
    CHECK(str_substr(std::string_view(str), 0, 3) == KANJI_KAN);

    // 開始位置が2バイト文字の後半バイトなので次のバイトから始める
    CHECK(str_substr(std::string_view(str), 1, 3) == KANJI_JI);
}

TEST_CASE("str_separate does not split a two byte character")
{
    const auto parts = str_separate(cat(KANJI_KAN, KANJI_JI, "ab"), 3);
    REQUIRE(parts.size() == 3);
    CHECK(parts[0] == KANJI_KAN);
    CHECK(parts[1] == cat(KANJI_JI, "a"));
    CHECK(parts[2] == "b");
}

TEST_CASE("str_separate gives up when the width is too small for a two byte character")
{
    CHECK(str_separate(KANJI_KAN, 1).empty());
}

TEST_CASE("str_toupper and str_tolower leave a two byte character as it is")
{
    CHECK(str_tolower(cat(KANJI_KAN, "ABC")) == cat(KANJI_KAN, "abc"));
    CHECK(str_toupper(cat(KANJI_KAN, "abc")) == cat(KANJI_KAN, "ABC"));
}

TEST_CASE("str_upcase_first leaves a leading two byte character as it is")
{
    const auto str = cat(KANJI_KAN, "abc");
    CHECK(str_upcase_first(str) == str);
}

TEST_CASE("str_find_all_multibyte_chars returns the index of each leading byte")
{
    const std::set<int> expected = { 1, 4 };
    CHECK(str_find_all_multibyte_chars(cat("a", KANJI_KAN, "b", KANJI_JI)) == expected);
    CHECK(str_find_all_multibyte_chars("abc").empty());
}

TEST_CASE("string-processor accepts a two byte character which lacks its trailing byte")
{
    // 前半バイトだけで終わる壊れた文字列を渡しても、範囲外を読まずに処理を終える
    const auto str = cat("a", KANJI_KAN.substr(0, 1));

    // 欠けた前半バイトも2バイト文字の一部として扱い、変換の対象にしない
    CHECK(str_toupper(str) == cat("A", KANJI_KAN.substr(0, 1)));
    CHECK(str_tolower(str) == str);

    CHECK(str_erase(str, "x") == str);
    CHECK(str_substr(std::string_view(str), 2).empty());

    const std::set<int> expected = { 1 };
    CHECK(str_find_all_multibyte_chars(str) == expected);
}

#endif

#if defined(JP) && defined(SJIS)

//
// ダメ文字のテスト (Shift_JISのみ)
//

TEST_CASE("angband_strchr does not match the trailing byte of a two byte character")
{
    const auto str = cat(DAME_SO, "a");
    CHECK(angband_strchr(str.data(), '\\') == nullptr);
    CHECK(angband_strchr(str.data(), 'a') == str.data() + 2);
}

TEST_CASE("angband_strstr does not match from the trailing byte of a two byte character")
{
    // 単純なバイト列としては "\\a" が一致するが、2バイト文字の途中なのでマッチしない
    const auto str = cat(DAME_SO, "a");
    CHECK(angband_strstr(str.data(), "\\a") == nullptr);
}

TEST_CASE("str_find does not match from the trailing byte of a two byte character")
{
    CHECK_FALSE(str_find(cat(DAME_SO, "a"), "\\a"));
    CHECK(str_find(cat(DAME_SO, "a"), "a"));
}

TEST_CASE("str_split does not split at the trailing byte of a two byte character")
{
    // 全角スペースの後半バイトは '@' と同じ 0x40
    const auto str = cat("a", DAME_SPACE, "b");
    const auto tokens = str_split(str, '@');
    REQUIRE(tokens.size() == 1);
    CHECK(tokens[0] == str);
}

TEST_CASE("str_replace does not replace the trailing byte of a two byte character")
{
    const auto str = cat("a", DAME_SO, "b");
    CHECK(str_replace(str, "\\", "X") == str);

    // 2バイト文字の一部でない '\' は置き換えられる
    CHECK(str_replace(cat("\\", DAME_SO), "\\", "X") == cat("X", DAME_SO));
}

TEST_CASE("str_erase does not break a two byte character")
{
    const auto str = cat("a", DAME_SO, "b");
    CHECK(str_erase(str, "\\") == str);
    CHECK(str_erase(cat(DAME_KANA_A, "A"), "A") == DAME_KANA_A);
}

TEST_CASE("str_substr does not split a two byte character whose trailing byte is ASCII")
{
    const auto str = cat(DAME_SO, DAME_HYOU);
    CHECK(str_substr(std::string_view(str), 0, 3) == DAME_SO);
    CHECK(str_substr(std::string_view(str), 1, 3) == DAME_HYOU);
}

TEST_CASE("str_find_all_multibyte_chars finds a character whose trailing byte is ASCII")
{
    const std::set<int> expected = { 0, 2, 4 };
    CHECK(str_find_all_multibyte_chars(cat(DAME_SO, DAME_CHOON, DAME_KANA_A)) == expected);
}

#endif
