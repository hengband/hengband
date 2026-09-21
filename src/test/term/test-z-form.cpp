/*!
 * @brief 書式付き文字列生成のテスト
 *
 * term/z-form.h で宣言されている format() を検証する。
 *
 * 日本語版では2バイト文字の扱いが加わるため、テストを3層に分けている。
 * - 文字コードに依存しないもの (全ビルドで実行する)
 * - 2バイト文字の扱いの検証 (#ifdef JP。EUC-JPでもShift_JISでも成立する)
 * - ダメ文字の検証 (#if defined(JP) && defined(SJIS)。Windows版でのみ実行する)
 *
 * 2バイト文字のテストデータは必ず16進エスケープで書くこと。
 * 日本語版のビルドはソースの文字列リテラルを変換する (autotoolsは gcc-wrap が nkf で
 * EUC-JPへ、MSVCは /execution-charset:shift-jis でShift_JISへ) が、英語版では変換されない。
 * ソースに日本語をそのまま書くと、ビルド構成によってバイト列が変わってしまう。
 *
 * format() は内部で1024バイトのバッファに書き込み、溢れたらバッファを広げて最初から
 * やり直す。1023バイト以上を出力するテストは、このやり直しの経路を通る。
 */

#include "term/z-form.h"

#include <doctest/doctest.h>

#include <climits>
#include <cstdarg>
#include <cstdio>
#include <limits>
#include <string>
#include <string_view>

namespace {

/*!
 * @brief 書式のコンパイル時検査を受けずに vformat() を呼び出す
 *
 * format() には printf 形式の format 属性が付いているため、不正な書式や nullptr を直接渡すと
 * -Wformat の警告が出る (CIでは -Werror でビルドが失敗する)。そうした入力の検証にはこちらを使う。
 * 実行時に組み立てた書式文字列を渡すときにも使う。
 */
std::string format_unchecked(const char *fmt, ...)
{
    va_list vp;
    va_start(vp, fmt);
    auto res = vformat(fmt, vp);
    va_end(vp);
    return res;
}

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
/*!
 * @brief 文字列を指定回数繰り返す
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

#if defined(JP) && defined(SJIS)
constexpr std::string_view KANJI_KAN = "\x8a\xbf"; //!< 漢
constexpr std::string_view KANJI_JI = "\x8e\x9a"; //!< 字
#elif defined(JP)
constexpr std::string_view KANJI_KAN = "\xb4\xc1"; //!< 漢
constexpr std::string_view KANJI_JI = "\xbb\xfa"; //!< 字
#endif

#if defined(JP) && defined(SJIS)
constexpr std::string_view DAME_SO = "\x83\x5c"; //!< ソ (後半バイトが 0x5c、ASCIIの '\')
constexpr std::string_view DAME_KANA_A = "\x83\x41"; //!< ア (後半バイトが 0x41、ASCIIの 'A')
constexpr std::string_view DAME_KANA_DI = "\x83\x61"; //!< ヂ (後半バイトが 0x61、ASCIIの 'a')
constexpr std::string_view DAME_KANA_TA = "\x83\x5e"; //!< タ (後半バイトが 0x5e、ASCIIの '^')
#endif

}

//
// 文字コードに依存しないテスト
//

TEST_CASE("format outputs a string without conversion specifications as is")
{
    CHECK(format_unchecked("") == "");
    CHECK(format("hello, world") == "hello, world");
}

TEST_CASE("format outputs a percent sign for %%")
{
    CHECK(format("%%") == "%");
    CHECK(format("100%%") == "100%");
    CHECK(format("%%d") == "%d");
}

TEST_CASE("format formats signed integers")
{
    CHECK(format("%d", 0) == "0");
    CHECK(format("%d", 123) == "123");
    CHECK(format("%d", -123) == "-123");
    CHECK(format("%i", 456) == "456");
    CHECK(format("%d", INT_MAX) == std::to_string(INT_MAX));
    CHECK(format("%d", INT_MIN) == std::to_string(INT_MIN));

    SUBCASE("with flags and width")
    {
        CHECK(format("%5d|", 42) == "   42|");
        CHECK(format("%-5d|", 42) == "42   |");
        CHECK(format("%05d", 42) == "00042");
        CHECK(format("%+d", 42) == "+42");
        CHECK(format("%+d", -42) == "-42");
    }

    SUBCASE("with long and long long")
    {
        constexpr auto long_max = std::numeric_limits<long>::max();
        constexpr auto long_min = std::numeric_limits<long>::min();
        constexpr auto llong_max = std::numeric_limits<long long>::max();
        constexpr auto llong_min = std::numeric_limits<long long>::min();
        CHECK(format("%ld", long_max) == std::to_string(long_max));
        CHECK(format("%ld", long_min) == std::to_string(long_min));
        CHECK(format("%lld", llong_max) == std::to_string(llong_max));
        CHECK(format("%lld", llong_min) == std::to_string(llong_min));
        CHECK(format("%li", -1L) == "-1");
    }
}

TEST_CASE("format formats unsigned integers")
{
    CHECK(format("%u", 123U) == "123");
    CHECK(format("%u", UINT_MAX) == std::to_string(UINT_MAX));
    CHECK(format("%o", 8U) == "10");
    CHECK(format("%x", 255U) == "ff");
    CHECK(format("%X", 255U) == "FF");
    CHECK(format("%#x", 255U) == "0xff");
    CHECK(format("%08X", 0xBEEFU) == "0000BEEF");

    SUBCASE("with long and long long")
    {
        constexpr auto ulong_max = std::numeric_limits<unsigned long>::max();
        constexpr auto ullong_max = std::numeric_limits<unsigned long long>::max();
        CHECK(format("%lu", ulong_max) == std::to_string(ulong_max));
        CHECK(format("%llu", ullong_max) == std::to_string(ullong_max));
        CHECK(format("%lx", 0xabcdefUL) == "abcdef");
        CHECK(format("%llX", 0x123456789abcdefULL) == "123456789ABCDEF");
        CHECK(format("%lo", 64UL) == "100");
    }
}

TEST_CASE("format formats floating point numbers")
{
    CHECK(format("%f", 1.5) == "1.500000");
    CHECK(format("%.2f", 3.14159) == "3.14");
    CHECK(format("%6.1f|", -2.5) == "  -2.5|");
    CHECK(format("%e", 123.45) == "1.234500e+02");
    CHECK(format("%E", 123.45) == "1.234500E+02");
    CHECK(format("%g", 0.0001) == "0.0001");
    CHECK(format("%F", 1.5) == "1.500000");
    CHECK(format("%G", 1e-10) == "1E-10");
    CHECK(format("%Lf", 1.5L) == "1.500000");
    CHECK(format("%.3Le", 1234.0L) == "1.234e+03");
}

TEST_CASE("format formats characters")
{
    CHECK(format("%c", 'A') == "A");
    CHECK(format("[%c%c]", 'x', 'y') == "[xy]");
}

TEST_CASE("format formats strings")
{
    CHECK(format("%s", "abc") == "abc");
    CHECK(format("<%s>", "") == "<>");
    CHECK(format("%s, %s", "hello", "world") == "hello, world");
    CHECK(format("%5s|", "abc") == "  abc|");
    CHECK(format("%-5s|", "abc") == "abc  |");
    CHECK(format("%.3s", "abcdef") == "abc");
}

TEST_CASE("format treats a null string as an empty string")
{
    const char *null_str = nullptr;
    CHECK(format_unchecked("<%s>", null_str) == "<>");
}

TEST_CASE("format takes width and precision from arguments for *")
{
    CHECK(format("%*d|", 5, 42) == "   42|");
    CHECK(format("%-*d|", 5, 42) == "42   |");
    CHECK(format("%*d|", -5, 42) == "42   |");
    CHECK(format("%.*s", 3, "abcdef") == "abc");
    CHECK(format("%*.*f", 8, 2, 3.14159) == "    3.14");
}

TEST_CASE("format stores the length output so far for %n")
{
    auto length = -1;
    CHECK(format("abc%nde", &length) == "abcde");
    CHECK(length == 3);

    length = -1;
    CHECK(format("%s%n", "hello", &length) == "hello");
    CHECK(length == 5);

    SUBCASE("after the buffer is extended")
    {
        const std::string str_a(1000, 'a');
        const std::string str_b(1000, 'b');
        length = -1;
        CHECK(format("%s%s%n", str_a.data(), str_b.data(), &length) == cat(str_a, str_b));
        CHECK(length == 2000);
    }
}

TEST_CASE("format formats pointers in the same way as snprintf")
{
    auto value = 0;
    char expected[64]{};
    std::snprintf(expected, sizeof(expected), "%p", static_cast<void *>(&value));
    CHECK(format("%p", static_cast<void *>(&value)) == expected);
}

TEST_CASE("format combines multiple conversion specifications")
{
    CHECK(format("%s has %d HP (%.1f%%)", "Player", 50, 62.5) == "Player has 50 HP (62.5%)");
}

TEST_CASE("format capitalizes the first letter of the string for %s^")
{
    CHECK(format("%s^", "orc") == "Orc");
    CHECK(format("%s^ attacks.", "the orc") == "The orc attacks.");
    CHECK(format("%s^", "Orc") == "Orc");
    CHECK(format("%s^", "123abc") == "123abc");
    CHECK(format("%s^", "") == "");

    SUBCASE("capitalizes the first non-space character")
    {
        CHECK(format("%5s^|", "abc") == "  Abc|");
        CHECK(format("%s^", " \torc") == " \tOrc");
        CHECK(format("%s^|", "   ") == "   |");
    }

    SUBCASE("treats a null string as an empty string")
    {
        const char *null_str = nullptr;
        CHECK(format_unchecked("<%s^>", null_str) == "<>");
    }

    SUBCASE("outputs ^ as is after conversions other than %s")
    {
        CHECK(format("%d^", 5) == "5^");
    }

    SUBCASE("affects only the argument of %s^")
    {
        CHECK(format("%s^ and %s", "orc", "troll") == "Orc and troll");
        CHECK(format("the %s^", "orc") == "the Orc");
    }
}

TEST_CASE("format returns an empty string for an invalid format")
{
    CHECK(format_unchecked("abc%y", 1) == "");
    CHECK(format_unchecked("abc%") == "");
    CHECK(format_unchecked("abc%5") == "");
    CHECK(format_unchecked(nullptr) == "");

    SUBCASE("length modifiers other than l, ll and L are not supported")
    {
        CHECK(format_unchecked("%hd", 1) == "");
        CHECK(format_unchecked("%zu", std::size_t{ 1 }) == "");
    }

    const auto too_long_spec = cat("%", std::string(120, '1'), "d");
    CHECK(format_unchecked(too_long_spec.data(), 1) == "");
}

TEST_CASE("format outputs a long string correctly")
{
    const std::string str_a(1000, 'a');
    const std::string str_b(1000, 'b');
    CHECK(format("%s%s%s%d", str_a.data(), str_b.data(), "tail", 42) == cat(str_a, str_b, "tail42"));

    CHECK(format("%s%s%*d", str_a.data(), str_b.data(), 5, 42) == cat(str_a, str_b, "   42"));

    // バッファの拡張が2回以上起きる
    const std::string long_fmt(5000, 'x');
    CHECK(format_unchecked(cat(long_fmt, "%d%s").data(), 42, "end") == cat(long_fmt, "42end"));
}

TEST_CASE("format outputs a string around the initial buffer size correctly")
{
    for (auto length = 1018; length <= 1023; ++length) {
        CAPTURE(length);
        const std::string str(length, 'a');
        CHECK(format("%s%s%d", str.data(), "xyz", 7) == cat(str, "xyz7"));
        CHECK(format_unchecked(cat(str, "%%%d").data(), 7) == cat(str, "%7"));
    }
}

TEST_CASE("format truncates the result of a single conversion to 1023 bytes")
{
    const std::string long_str(2000, 'a');
    CHECK(format("%s", long_str.data()) == std::string(1023, 'a'));
}

#ifdef JP

//
// 2バイト文字の扱いのテスト (EUC-JP / Shift_JIS 共通)
//

TEST_CASE("format outputs multibyte characters as is")
{
    const auto kanji = cat(KANJI_KAN, KANJI_JI);
    CHECK(format("%s", kanji.data()) == kanji);
    CHECK(format("[%s]", kanji.data()) == cat("[", kanji, "]"));
    CHECK(format_unchecked(cat(KANJI_KAN, "%d", KANJI_JI).data(), 10) == cat(KANJI_KAN, "10", KANJI_JI));
    CHECK(format_unchecked(cat(KANJI_KAN, "%s").data(), KANJI_JI.data()) == kanji);
}

TEST_CASE("format counts width and precision of %s in bytes for multibyte characters")
{
    const auto kanji = cat(KANJI_KAN, KANJI_JI);
    CHECK(format("%6s|", kanji.data()) == cat("  ", kanji, "|"));
    CHECK(format("%-6s|", kanji.data()) == cat(kanji, "  |"));
    CHECK(format("%.4s|", kanji.data()) == cat(kanji, "|"));
    CHECK(format("%.2s|", kanji.data()) == cat(KANJI_KAN, "|"));
}

TEST_CASE("format replaces a multibyte character split by the precision of %s with a space")
{
    const auto kanji = cat(KANJI_KAN, KANJI_JI);
    CHECK(format("%.3s|", kanji.data()) == cat(KANJI_KAN, " |"));
    CHECK(format("%.1s|", kanji.data()) == " |");
    CHECK(format("%.3s%s", kanji.data(), "end") == cat(KANJI_KAN, " end"));

    const auto long_kanji = repeat(KANJI_KAN, 20);
    CHECK(format("%.30s", long_kanji.data()) == repeat(KANJI_KAN, 15));
    CHECK(format("%.30s", cat("a", long_kanji).data()) == cat("a", repeat(KANJI_KAN, 14), " "));
    CHECK(format("%.*s", 5, cat("ab", KANJI_KAN, KANJI_JI).data()) == cat("ab", KANJI_KAN, " "));
}

TEST_CASE("format replaces the first byte of a multibyte character given to %c with a space")
{
    CHECK(format("[%c]", KANJI_KAN[0]) == "[ ]");
}

TEST_CASE("format does not capitalize a string containing multibyte characters for %s^")
{
    CHECK(format("%s^", cat("abc", KANJI_KAN).data()) == cat("abc", KANJI_KAN));
    CHECK(format("%s^", cat(KANJI_KAN, "abc").data()) == cat(KANJI_KAN, "abc"));

    SUBCASE("capitalizes a string without multibyte characters")
    {
        CHECK(format_unchecked(cat(KANJI_KAN, "%s^").data(), "orc") == cat(KANJI_KAN, "Orc"));
    }
}

TEST_CASE("format outputs a multibyte character across the initial buffer size correctly")
{
    const auto kanji = cat(KANJI_KAN, KANJI_JI);
    for (auto length = 1019; length <= 1023; ++length) {
        CAPTURE(length);
        const std::string str(length, 'a');
        CHECK(format("%s%s%s", str.data(), kanji.data(), "end") == cat(str, kanji, "end"));

        const auto fmt = cat(str, kanji, "%s");
        CHECK(format_unchecked(fmt.data(), "end") == cat(str, kanji, "end"));
    }
}

TEST_CASE("format replaces a split multibyte character at the end of a truncated conversion with a space")
{
    const auto long_kanji = repeat(KANJI_KAN, 600);
    CHECK(format("%s", long_kanji.data()) == cat(repeat(KANJI_KAN, 511), " "));
    CHECK(format("%s", cat("a", long_kanji).data()) == cat("a", repeat(KANJI_KAN, 511)));
}

#endif

#if defined(JP) && defined(SJIS)

//
// ダメ文字のテスト (Shift_JIS のみ)
//

TEST_CASE("format outputs Shift_JIS characters whose second byte is in the ASCII range as is")
{
    const auto dame = cat(DAME_SO, DAME_KANA_A, DAME_KANA_DI, DAME_KANA_TA);
    CHECK(format("%s", dame.data()) == dame);
    CHECK(format_unchecked(cat(DAME_SO, "%d", DAME_KANA_TA).data(), 10) == cat(DAME_SO, "10", DAME_KANA_TA));
}

TEST_CASE("format replaces a Shift_JIS character with an ASCII second byte split by the precision of %s with a space")
{
    CHECK(format("%.3s|", cat(DAME_SO, DAME_KANA_A).data()) == cat(DAME_SO, " |"));
    CHECK(format("%.4s|", cat(DAME_SO, DAME_KANA_A).data()) == cat(DAME_SO, DAME_KANA_A, "|"));
}

TEST_CASE("format does not capitalize a string containing Shift_JIS characters with a lowercase second byte for %s^")
{
    CHECK(format("%s^", cat("a", DAME_KANA_DI).data()) == cat("a", DAME_KANA_DI));
}

TEST_CASE("format outputs a Shift_JIS character with an ASCII second byte across the initial buffer size correctly")
{
    for (auto length = 1021; length <= 1023; ++length) {
        CAPTURE(length);
        const std::string str(length, 'a');
        CHECK(format("%s%s%s", str.data(), DAME_SO.data(), "end") == cat(str, DAME_SO, "end"));
    }
}

#endif
