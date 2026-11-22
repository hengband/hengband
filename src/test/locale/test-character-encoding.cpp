/*!
 * @brief 文字コード処理のテスト
 */

#include "locale/character-encoding.h"

#include "test/string-helpers.h"

#include <doctest/doctest.h>

#include <string>
#include <string_view>
#include <utility>

using namespace test;

TEST_CASE("utf8_next_char_byte_length returns the byte length of the first UTF-8 character")
{
    CHECK(utf8_next_char_byte_length("a") == 1);
    CHECK(utf8_next_char_byte_length("\xc2\x80") == 2);
    CHECK(utf8_next_char_byte_length("\xe3\x81\x82") == 3); // あ
    CHECK(utf8_next_char_byte_length("\xf0\x9f\x98\x80") == 4); // U+1F600
    CHECK(utf8_next_char_byte_length("\xf4\x8f\xbf\xbf") == 4); // U+10FFFF
    CHECK(utf8_next_char_byte_length("") == 0);
}

TEST_CASE("utf8_next_char_byte_length rejects invalid lead bytes, overlong forms, surrogates and values over U+10FFFF")
{
    CHECK(utf8_next_char_byte_length("\xc0\xa1") == 0); // EUC-JP の「澄」
    CHECK(utf8_next_char_byte_length("\xc1\xbf") == 0);
    CHECK(utf8_next_char_byte_length("\xe0\x9f\xbf") == 0);
    CHECK(utf8_next_char_byte_length("\xed\xa0\x80") == 0); // U+D800
    CHECK(utf8_next_char_byte_length("\xf0\x8f\xbf\xbf") == 0);
    CHECK(utf8_next_char_byte_length("\xf4\x90\x80\x80") == 0); // U+110000
    CHECK(utf8_next_char_byte_length("\xf5\x80\x80\x80") == 0);
    CHECK(utf8_next_char_byte_length("\x80") == 0);
    CHECK(utf8_next_char_byte_length("\xe3\x81") == 0);
    CHECK(utf8_next_char_byte_length("\xe3\x81:") == 0);
}

#ifdef JP

namespace {

using namespace std::string_view_literals;

constexpr std::string_view NIHON_EUC = "\xc6\xfc\xcb\xdc"; //!< 日本 (EUC-JP)
constexpr std::string_view NIHON_SJIS = "\x93\xfa\x96\x7b"; //!< 日本 (Shift_JIS)

/*
 * 第2水準漢字 (1バイト目が E0 以上)
 * 燹 の EUC-JP と 爍 の Shift_JIS は同じバイト列 (E0 A1) で、どちらの文字コードとも解釈できる
 */
constexpr std::string_view KANJI_SEN_EUC = "\xe0\xa1"; //!< 燹 (EUC-JP、Shift_JIS とも解釈できる)
constexpr std::string_view KANJI_SEN_SJIS = "\xe0\x9f"; //!< 燹 (Shift_JIS、2バイト目が 80-A0 なので Shift_JIS に限られる)
constexpr std::string_view KANJI_SHAKU_EUC = "\xe0\xa3"; //!< 爍 (EUC-JP、Shift_JIS とも解釈できる)
constexpr std::string_view KANJI_SHAKU_SJIS = "\xe0\xa1"; //!< 爍 (Shift_JIS、EUC-JP とも解釈できる)
constexpr std::string_view KANJI_YOU_EUC = "\xe0\xfe"; //!< 珱 (EUC-JP、2バイト目が FD 以上なので EUC-JP に限られる)
constexpr std::string_view KANJI_YOU_SJIS = "\xe0\xfc"; //!< 珱 (Shift_JIS、EUC-JP とも解釈できる)
constexpr std::string_view KANJI_SEN2_EUC = "\xf0\xa1"; //!< 陝 (EUC-JP、1バイト目が F0 以上なので EUC-JP に限られる)
constexpr std::string_view KANJI_SEN2_SJIS = "\xe8\x9f"; //!< 陝 (Shift_JIS)

constexpr std::string_view FULLWIDTH_TILDE_UTF8 = "\xef\xbd\x9e"; //!< ～ (U+FF5E)
constexpr std::string_view FULLWIDTH_HYPHEN_MINUS_UTF8 = "\xef\xbc\x8d"; //!< － (U+FF0D)
constexpr std::string_view HALFWIDTH_KATAKANA_UTF8 = "\xef\xbd\xb1\xef\xbd\xb2\xef\xbd\xb3"; //!< ｱｲｳ (UTF-8 では1文字3バイト)
constexpr std::string_view MS_SYMBOLS_UTF8 = "\xe2\x88\xa5\xef\xbf\xa0\xef\xbf\xa1\xef\xbf\xa2"; //!< ∥￠￡￢ (U+2225・U+FFE0・U+FFE1・U+FFE2)
#if defined(UTF8) || defined(EUC)
constexpr std::string_view MS_SYMBOLS_EUC = "\xa1\xc2\xa1\xf1\xa1\xf2\xa2\xcc"; //!< ∥￠￡￢ (EUC-JP)
#endif

#if defined(UTF8)
constexpr std::string_view NIHON_SYS = "\xe6\x97\xa5\xe6\x9c\xac"; //!< 日本 (UTF-8)
constexpr std::string_view KANJI_SEN_SYS = "\xe7\x87\xb9"; //!< 燹 (UTF-8)
constexpr std::string_view KANJI_SHAKU_SYS = "\xe7\x88\x8d"; //!< 爍 (UTF-8)
constexpr std::string_view KANJI_YOU_SYS = "\xe7\x8f\xb1"; //!< 珱 (UTF-8)
constexpr std::string_view KANJI_SEN2_SYS = "\xe9\x99\x9d"; //!< 陝 (UTF-8)
constexpr auto FULLWIDTH_TILDE_SYS = FULLWIDTH_TILDE_UTF8; //!< ～ (UTF-8 では変換しない)
constexpr auto FULLWIDTH_HYPHEN_MINUS_SYS = FULLWIDTH_HYPHEN_MINUS_UTF8; //!< － (UTF-8 では変換しない)
constexpr auto HALFWIDTH_KATAKANA_SYS = HALFWIDTH_KATAKANA_UTF8; //!< ｱｲｳ (UTF-8 では変換しない)
constexpr auto MS_SYMBOLS_SYS = MS_SYMBOLS_UTF8; //!< ∥￠￡￢ (UTF-8 では変換しない)
#elif defined(EUC)
constexpr auto NIHON_SYS = NIHON_EUC;
constexpr auto KANJI_SEN_SYS = KANJI_SEN_EUC;
constexpr auto KANJI_SHAKU_SYS = KANJI_SHAKU_EUC;
constexpr auto KANJI_YOU_SYS = KANJI_YOU_EUC;
constexpr auto KANJI_SEN2_SYS = KANJI_SEN2_EUC;
constexpr std::string_view FULLWIDTH_TILDE_SYS = "\xa1\xc1"; //!< ～ (EUC-JP では波ダッシュに置き換える)
constexpr std::string_view FULLWIDTH_HYPHEN_MINUS_SYS = "\xa1\xdd"; //!< － (EUC-JP ではマイナス記号に置き換える)
constexpr std::string_view HALFWIDTH_KATAKANA_SYS = "\x8e\xb1\x8e\xb2\x8e\xb3"; //!< ｱｲｳ (EUC-JP)
constexpr auto MS_SYMBOLS_SYS = MS_SYMBOLS_EUC; //!< ∥￠￡￢ (EUC-JP)
#else
constexpr auto NIHON_SYS = NIHON_SJIS;
constexpr auto KANJI_SEN_SYS = KANJI_SEN_SJIS;
constexpr auto KANJI_SHAKU_SYS = KANJI_SHAKU_SJIS;
constexpr auto KANJI_YOU_SYS = KANJI_YOU_SJIS;
constexpr auto KANJI_SEN2_SYS = KANJI_SEN2_SJIS;
constexpr std::string_view FULLWIDTH_TILDE_SYS = "\x81\x60"; //!< ～ (CP932)
constexpr std::string_view FULLWIDTH_HYPHEN_MINUS_SYS = "\x81\x7c"; //!< － (CP932)
constexpr std::string_view HALFWIDTH_KATAKANA_SYS = "\xb1\xb2\xb3"; //!< ｱｲｳ (CP932 では1文字1バイト)
constexpr std::string_view MS_SYMBOLS_SYS = "\x81\x61\x81\x91\x81\x92\x81\xca"; //!< ∥￠￡￢ (CP932)
#endif

}

TEST_CASE("guess_legacy_encoding detects EUC-JP followed by ASCII")
{
    const auto str = cat(NIHON_EUC, "abc");
    CHECK(guess_legacy_encoding(str) == CharacterEncoding::EUC_JP);
    CHECK(legacy_to_sys(str, CharacterEncoding::EUC_JP) == cat(NIHON_SYS, "abc"));
}

TEST_CASE("guess_legacy_encoding detects Shift_JIS followed by ASCII")
{
    const auto str = cat(NIHON_SJIS, "abc");
    CHECK(guess_legacy_encoding(str) == CharacterEncoding::SHIFT_JIS);
    CHECK(legacy_to_sys(str, CharacterEncoding::SHIFT_JIS) == cat(NIHON_SYS, "abc"));
}

TEST_CASE("guess_legacy_encoding detects EUC-JP surrounded by ASCII")
{
    const auto str = cat("abc", NIHON_EUC, "def");
    CHECK(guess_legacy_encoding(str) == CharacterEncoding::EUC_JP);
    CHECK(legacy_to_sys(str, CharacterEncoding::EUC_JP) == cat("abc", NIHON_SYS, "def"));
}

TEST_CASE("guess_legacy_encoding rejects EUC-JP and Shift_JIS mixed with ASCII between them")
{
    CHECK(guess_legacy_encoding(cat(NIHON_EUC, "abc", NIHON_SJIS)) == CharacterEncoding::UNKNOWN);
}

TEST_CASE("guess_legacy_encoding detects Shift_JIS whose second byte is 0x80-0xA0 after a lead byte of 0xE0 or above")
{
    const auto str = cat(KANJI_SEN_SJIS, "abc");
    CHECK(guess_legacy_encoding(str) == CharacterEncoding::SHIFT_JIS);
    CHECK(legacy_to_sys(str, CharacterEncoding::SHIFT_JIS) == cat(KANJI_SEN_SYS, "abc"));
}

TEST_CASE("guess_legacy_encoding detects EUC-JP whose second byte is 0xFD or above after a lead byte of 0xE0 or above")
{
    const auto str = cat(KANJI_YOU_EUC, "abc");
    CHECK(guess_legacy_encoding(str) == CharacterEncoding::EUC_JP);
    CHECK(legacy_to_sys(str, CharacterEncoding::EUC_JP) == cat(KANJI_YOU_SYS, "abc"));
}

TEST_CASE("guess_legacy_encoding defers ambiguous characters and decides by a following Shift_JIS character")
{
    const auto str = cat(KANJI_SHAKU_SJIS, KANJI_YOU_SJIS, KANJI_SEN_SJIS);
    CHECK(guess_legacy_encoding(str) == CharacterEncoding::SHIFT_JIS);
    CHECK(legacy_to_sys(str, CharacterEncoding::SHIFT_JIS) == cat(KANJI_SHAKU_SYS, KANJI_YOU_SYS, KANJI_SEN_SYS));
}

TEST_CASE("guess_legacy_encoding defers ambiguous characters and decides by a following EUC-JP character")
{
    const auto str = cat(KANJI_SEN_EUC, KANJI_SHAKU_EUC, NIHON_EUC);
    CHECK(guess_legacy_encoding(str) == CharacterEncoding::EUC_JP);
    CHECK(legacy_to_sys(str, CharacterEncoding::EUC_JP) == cat(KANJI_SEN_SYS, KANJI_SHAKU_SYS, NIHON_SYS));
}

TEST_CASE("guess_legacy_encoding detects EUC-JP whose lead byte is 0xF0 or above")
{
    const auto str = cat(KANJI_SEN2_EUC, "abc");
    CHECK(guess_legacy_encoding(str) == CharacterEncoding::EUC_JP);
    CHECK(legacy_to_sys(str, CharacterEncoding::EUC_JP) == cat(KANJI_SEN2_SYS, "abc"));
}

TEST_CASE("guess_legacy_encoding does not treat Shift_JIS user-defined characters as Shift_JIS")
{
    // 1バイト目が F0 以上の Shift_JIS は sjis2euc() で EUC-JP に変換できないため、Shift_JIS とみなさない
    CHECK(guess_legacy_encoding(cat(NIHON_SJIS, "\xf0\x40")) == CharacterEncoding::UNKNOWN);
}

TEST_CASE("guess_legacy_encoding treats lead byte 0xF0 or above with second byte 0xA1 or above as EUC-JP")
{
    // Shift_JIS のユーザー定義文字とも読めるが EUC-JP とみなすので、後に Shift_JIS が続くと壊れた文字列になる
    CHECK(guess_legacy_encoding(cat(KANJI_SEN2_EUC, KANJI_SEN2_SJIS)) == CharacterEncoding::UNKNOWN);
}

TEST_CASE("guess_legacy_encoding does not treat Shift_JIS IBM extended characters as EUC-JP")
{
    // 髙 (Shift_JIS の IBM 拡張文字 FB FC) の1バイト目は EUC-JP の JIS X 0208 の範囲外なので、EUC-JP とみなさない
    CHECK(guess_legacy_encoding(cat("\xfb\xfc", KANJI_SEN_EUC)) == CharacterEncoding::UNKNOWN);
}

TEST_CASE("guess_legacy_encoding returns UNKNOWN for string with ambiguous characters only")
{
    CHECK(guess_legacy_encoding(cat(KANJI_SEN_EUC, "abc", KANJI_SHAKU_EUC)) == CharacterEncoding::UNKNOWN);
}

TEST_CASE("guess_legacy_encoding returns UNKNOWN for string ending with a lead byte")
{
    CHECK(guess_legacy_encoding(cat(NIHON_EUC, "\xc6")) == CharacterEncoding::UNKNOWN);
}

TEST_CASE("guess_legacy_encoding returns UNKNOWN for ASCII-only string")
{
    CHECK(guess_legacy_encoding("abc") == CharacterEncoding::UNKNOWN);
}

TEST_CASE("guess_legacy_encoding returns UNKNOWN for empty string")
{
    CHECK(guess_legacy_encoding("") == CharacterEncoding::UNKNOWN);
}

TEST_CASE("legacy_to_sys_replacing_invalid converts a string saved in UTF-8")
{
    // 内部の文字コードを UTF-8 にした構成で書いたセーブの文字列は、どの構成でも内部の文字コードにする
    CHECK(legacy_to_sys_replacing_invalid("a\xe6\x97\xa5\xe6\x9c\xac"sv, CharacterEncoding::UTF_8) == cat("a", NIHON_SYS));
}

#ifdef EUC
TEST_CASE("legacy_to_sys_replacing_invalid replaces characters which cannot be converted from a string saved in UTF-8")
{
    // EUC-JP に無い文字 (U+1F600) と読めないバイトは '?' にし、ほかの文字は変換する
    CHECK(legacy_to_sys_replacing_invalid("\xe6\x97\xa5\xf0\x9f\x98\x80\xff\xe6\x9c\xac"sv, CharacterEncoding::UTF_8) == cat(NIHON_EUC.substr(0, 2), "??", NIHON_EUC.substr(2)));
}
#endif

TEST_CASE("utf8_to_sys converts the characters which EUC-JP-MS and EUC-JP map differently")
{
    // ∥￠￡￢ (EUC-JP-MS で読んだ旧版の EUC-JP の A1C2・A1F1・A1F2・A2CC)
    CHECK(utf8_to_sys(MS_SYMBOLS_UTF8) == MS_SYMBOLS_SYS);
}

TEST_CASE("utf8_to_sys converts fullwidth tilde and hyphen-minus")
{
    CHECK(utf8_to_sys(cat(FULLWIDTH_TILDE_UTF8, FULLWIDTH_HYPHEN_MINUS_UTF8)) == cat(FULLWIDTH_TILDE_SYS, FULLWIDTH_HYPHEN_MINUS_SYS));
}

TEST_CASE("sys_to_utf8 converts half-width katakana")
{
    CHECK(sys_to_utf8(HALFWIDTH_KATAKANA_SYS) == HALFWIDTH_KATAKANA_UTF8);
}

TEST_CASE("sys_to_utf8 converts an empty string to an empty string")
{
    CHECK(sys_to_utf8(""sv) == ""sv);
}

TEST_CASE("utf8_to_sys rejects a string with an embedded NUL")
{
    // 途中の '\0' より後ろが黙って欠けないよう、不正な入力として変換しない
    CHECK_FALSE(utf8_to_sys("a\0b"sv).has_value());
}

#ifdef EUC

TEST_CASE("utf8_to_euc converts fullwidth tilde and hyphen-minus to wave dash and minus sign")
{
    CHECK(utf8_to_euc(cat(FULLWIDTH_TILDE_UTF8, FULLWIDTH_HYPHEN_MINUS_UTF8)) == cat(FULLWIDTH_TILDE_SYS, FULLWIDTH_HYPHEN_MINUS_SYS));
}

TEST_CASE("utf8_to_euc converts fullwidth tilde after an embedded NUL")
{
    // 途中に '\0' があっても、文字列の長さの範囲はすべて置き換えてから変換する
    CHECK(utf8_to_euc(cat("a\0"sv, FULLWIDTH_TILDE_UTF8)) == "a\0\xa1\xc1"sv);
}

TEST_CASE("utf8_to_euc and utf8_to_sys convert an empty string to an empty string")
{
    CHECK(utf8_to_euc(""sv) == ""sv);
    CHECK(utf8_to_sys(""sv) == ""sv);
}

TEST_CASE("utf8_to_euc rejects a truncated UTF-8 character")
{
    constexpr std::pair<std::string_view, std::string_view> truncated_chars[] = {
        { "E3", "\xe3"sv },
        { "E3 81", "\xe3\x81"sv },
        { "F0 9F", "\xf0\x9f"sv },
        { "F0 9F 98", "\xf0\x9f\x98"sv },
    };

    for (const auto &[label, truncated] : truncated_chars) {
        CAPTURE(label);

        // 文字の途中で終わる文字列は、黙って捨てずに変換の失敗とする
        CHECK_FALSE(utf8_to_euc(truncated).has_value());
        CHECK_FALSE(utf8_to_euc(cat(truncated, "\0"sv)).has_value());
    }
}

TEST_CASE("utf8_to_sys replaces JIS X 0212 characters with question marks")
{
    // é (UTF-8 で2バイト) は EUC-JP では JIS X 0212 の3バイト (8F AB B1) になり、正しく表示できないので '?' に置き換える
    CHECK(utf8_to_sys("caf\xc3\xa9"sv) == "caf?"sv);
    CHECK(utf8_to_sys("\xc3\xa9\xc3\xa9\xc3\xa9"sv) == "???"sv);
    CHECK(utf8_to_sys("\xe6\x97\xa5\xe6\x9c\xac\xc3\xa9"sv) == cat(NIHON_EUC, "?"));
}

TEST_CASE("utf8_to_euc replaces JIS X 0212 characters with question marks")
{
    CHECK(utf8_to_euc(cat("a\xc3\xa9"sv, "b")) == "a?b"sv);
}

TEST_CASE("utf8_to_sys rejects invalid UTF-8")
{
    CHECK_FALSE(utf8_to_sys("\xff"sv).has_value());
}

#endif

// Windows の 20932 と macOS の libiconv は C1 制御文字を EUC-JP に変換できず、utf8_to_euc() が失敗するので、glibc の iconv だけで確かめる
#if (defined(EUC) || defined(UTF8)) && defined(__GLIBC__)
TEST_CASE("utf8_to_euc keeps C1 control characters which iconv converts into one byte")
{
    // glibc の iconv の EUC-JP は U+0085 を 1 バイトの 85 にする。後ろの文字を 2 バイト目とみなして '?' にしない
    CHECK(utf8_to_euc(cat("\xc2\x85"sv, "a")) == "\x85\x61"sv); // 61 は 'a'
    CHECK(utf8_to_euc("\xc2\x85\xe6\x97\xa5\xe6\x9c\xac"sv) == cat("\x85"sv, NIHON_EUC)); // U+0085 と「日本」
}
#endif

#ifdef UTF8

TEST_CASE("utf8_to_sys and sys_to_utf8 reject invalid UTF-8 when the system encoding is UTF-8")
{
    CHECK_FALSE(utf8_to_sys("\xff"sv).has_value());
    CHECK_FALSE(sys_to_utf8("\xe3\x81"sv).has_value());
    CHECK_FALSE(sys_to_utf8(NIHON_EUC).has_value());
}

TEST_CASE("legacy_to_sys converts Shift_JIS half-width katakana into UTF-8")
{
    CHECK(legacy_to_sys("\xb1\xb2\xb3"sv, CharacterEncoding::SHIFT_JIS) == HALFWIDTH_KATAKANA_UTF8);
}

TEST_CASE("legacy_to_sys converts EUC-JP half-width katakana into UTF-8")
{
    CHECK(legacy_to_sys("\x8e\xb1\x8e\xb2\x8e\xb3"sv, CharacterEncoding::EUC_JP) == HALFWIDTH_KATAKANA_UTF8);
}

TEST_CASE("legacy_to_sys fails on a string broken in the given encoding")
{
    CHECK_FALSE(legacy_to_sys(cat(NIHON_EUC, "\xc6"), CharacterEncoding::EUC_JP).has_value());

    // EUC-JP の 80-A0 (8E・8F を除く) は C1 制御文字として変換せず、失敗とする
    CHECK_FALSE(legacy_to_sys("\x8a"sv, CharacterEncoding::EUC_JP).has_value());
    CHECK_FALSE(legacy_to_sys("a\xa0"sv, CharacterEncoding::EUC_JP).has_value());

    // EUC-JP の 3 バイトの文字の先頭 (8F) で終わる文字列と、Shift_JIS の 80 は、C1 制御文字として変換せず、失敗とする
    CHECK_FALSE(legacy_to_sys(cat(NIHON_EUC, "\x8f"), CharacterEncoding::EUC_JP).has_value());
    CHECK_FALSE(legacy_to_sys(cat(NIHON_SJIS, "\x80"), CharacterEncoding::SHIFT_JIS).has_value());

    // EUC-JP の 2 バイト目が A1 未満の並びは、JIS X 0212 の文字として読まず、失敗とする
    CHECK_FALSE(legacy_to_sys(cat(NIHON_EUC, "\xc6:"), CharacterEncoding::EUC_JP).has_value());
}

TEST_CASE("guess_convert_to_system_encoding keeps UTF-8 and converts a legacy encoding when the system encoding is UTF-8")
{
    auto utf8 = cat("a", NIHON_SYS);
    CHECK(guess_convert_to_system_encoding(utf8.data(), static_cast<int>(utf8.size() + 1)) == utf8.length());
    CHECK(utf8 == cat("a", NIHON_SYS));

    // 変換で長くなるので、変換後の長さが収まるバッファを用意する
    std::string buf(16, '\0');
    const auto euc = cat("a", NIHON_EUC);
    std::copy(euc.begin(), euc.end(), buf.begin());
    CHECK(guess_convert_to_system_encoding(buf.data(), static_cast<int>(buf.size())) == 7);
    CHECK(std::string_view(buf.data()) == cat("a", NIHON_SYS));

    // std::string で受ける形は、変換で長くなってもバッファの大きさに縛られない
    CHECK(guess_convert_to_system_encoding(euc) == cat("a", NIHON_SYS));
    CHECK(guess_convert_to_system_encoding(cat("a", NIHON_SYS)) == cat("a", NIHON_SYS));

    // EUC-JP の「澄」(C0 A1) は UTF-8 とみなさずに変換する
    CHECK(guess_convert_to_system_encoding(std::string("\xc0\xa1")) == "\xe6\xbe\x84"sv);

    // 推定できない文字列は EUC-JP として読む
    CHECK(guess_convert_to_system_encoding(cat(KANJI_SEN_EUC, "abc", KANJI_SHAKU_EUC)) == cat(KANJI_SEN_SYS, "abc", KANJI_SHAKU_SYS));
}

TEST_CASE("legacy_to_sys_replacing_invalid replaces bytes which cannot be read with question marks")
{
    // 末尾で欠けた文字
    CHECK(legacy_to_sys_replacing_invalid(cat(NIHON_EUC, "\xc6"), CharacterEncoding::EUC_JP) == cat(NIHON_SYS, "?"));
    CHECK(legacy_to_sys_replacing_invalid(cat(NIHON_SJIS, "\x93"), CharacterEncoding::SHIFT_JIS) == cat(NIHON_SYS, "?"));

    // EUC-JP の 80-A0 (8E・8F を除く) は読めないバイトとし、後ろの文字は変換を続ける
    CHECK(legacy_to_sys_replacing_invalid(cat("\x8a", NIHON_EUC), CharacterEncoding::EUC_JP) == cat("?", NIHON_SYS));
    CHECK(legacy_to_sys_replacing_invalid(cat("\x8f", "a", NIHON_EUC), CharacterEncoding::EUC_JP) == cat("?a", NIHON_SYS));
    CHECK(legacy_to_sys_replacing_invalid(cat("\x80", NIHON_SJIS), CharacterEncoding::SHIFT_JIS) == cat("?", NIHON_SYS));
    CHECK(legacy_to_sys_replacing_invalid(cat(NIHON_EUC, "\xc6:"), CharacterEncoding::EUC_JP) == cat(NIHON_SYS, "?:"));

#ifdef _WIN32
    // Windows の 20932 は JIS X 0212 の文字 (8F B0 A1 の「丂」) を読めないので、3 バイトを 1 つの '?' にする
    CHECK(legacy_to_sys_replacing_invalid(cat("\x8f\xb0\xa1", NIHON_EUC), CharacterEncoding::EUC_JP) == cat("?", NIHON_SYS));
#endif

    // UTF-8 の文字列は、UTF-8 として読めないバイトを置き換える
    CHECK(legacy_to_sys_replacing_invalid(cat(NIHON_SYS, "\xff\xe6"), CharacterEncoding::UTF_8) == cat(NIHON_SYS, "??"));
    CHECK(legacy_to_sys_replacing_invalid(NIHON_SYS, CharacterEncoding::UTF_8) == NIHON_SYS);
    CHECK(legacy_to_sys_replacing_invalid("\xc0\xa1"sv, CharacterEncoding::US_ASCII) == "??");
}

TEST_CASE("legacy_to_sys_replacing_invalid reads a string in an unknown encoding as EUC-JP")
{
    // EUC-JP とも Shift_JIS とも読める文字列は、旧版の Unix 版の文字コード (EUC-JP) として読む
    const auto str = cat(KANJI_SEN_EUC, "abc", KANJI_SHAKU_EUC);
    REQUIRE(guess_legacy_encoding(str) == CharacterEncoding::UNKNOWN);
    CHECK(legacy_to_sys_replacing_invalid(str, CharacterEncoding::UNKNOWN) == cat(KANJI_SEN_SYS, "abc", KANJI_SHAKU_SYS));
}

TEST_CASE("legacy_to_sys returns a string in an unknown encoding as is")
{
    CHECK(legacy_to_sys(NIHON_EUC, CharacterEncoding::UNKNOWN) == NIHON_EUC);
}

TEST_CASE("legacy_to_sys reads EUC-JP as EUC-JP-MS like the old versions built with euc-jp-ms")
{
    // 旧版は文字列リテラルを euc-jp-ms で変換したので、全角チルダ (～) は A1 C1、全角ハイフンマイナス (－) は A1 DD になっている。
    // iconv が EUC-JP-MS を持たない環境でも、EUC-JP-MS で読んだ文字になる
    CHECK(legacy_to_sys("\xa1\xc1"sv, CharacterEncoding::EUC_JP) == FULLWIDTH_TILDE_UTF8);
    CHECK(legacy_to_sys("\xa1\xdd"sv, CharacterEncoding::EUC_JP) == FULLWIDTH_HYPHEN_MINUS_UTF8);
    CHECK(legacy_to_sys(MS_SYMBOLS_EUC, CharacterEncoding::EUC_JP) == MS_SYMBOLS_UTF8);
}

TEST_CASE("utf8_to_euc_keeping_width converts each character into as many EUC-JP bytes as its display width")
{
    CHECK(utf8_to_euc_keeping_width(cat("a", NIHON_SYS)) == cat("a", NIHON_EUC));

    // 半角カナは EUC-JP でも 2 バイト (8E xx)、全角チルダは EUC-JP の波ダッシュ (A1 C1) になる
    CHECK(utf8_to_euc_keeping_width(HALFWIDTH_KATAKANA_UTF8) == "\x8e\xb1\x8e\xb2\x8e\xb3"sv);
    CHECK(utf8_to_euc_keeping_width("\xef\xbd\x9e"sv) == "\xa1\xc1"sv);

    // EUC-JP に無い文字 (U+1F600) と JIS X 0212 の文字 (é) は、2 桁のまま「？」(A1 A9) にする
    CHECK(utf8_to_euc_keeping_width("\xf0\x9f\x98\x80"sv) == "\xa1\xa9"sv);
    CHECK(utf8_to_euc_keeping_width(cat("a", "\xc3\xa9", "b")) == cat("a", "\xa1\xa9", "b"));

    // UTF-8 として読めないバイトは 1 バイトずつ '?' にする
    CHECK(utf8_to_euc_keeping_width("a\xff\xe3\x81"sv) == "a???"sv);
}

#endif

#else

TEST_CASE("utf8_to_sys returns the input as is in the English version")
{
    CHECK(utf8_to_sys(std::string_view("abc")) == std::string_view("abc"));
}

#endif
