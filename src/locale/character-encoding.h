#pragma once

#include "system/h-basic.h"
#include <cstdint>
#include <string>
#include <string_view>
#include <tl/optional.hpp>

enum class CharacterEncoding : uint8_t {
    UNKNOWN = 0,
    US_ASCII = 1,
    EUC_JP = 2,
    SHIFT_JIS = 3,
    UTF_8 = 4,
};

/*!
 * @brief UTF-8 の先頭バイトから、その文字のバイト数を返す
 * @param lead 文字の先頭バイト
 * @return 文字のバイト数。ASCII なら 1、先頭に現れないバイト (後続のバイト 80-BF と C0・C1・F5-FF) なら 0
 * @details 後続のバイトは確かめない。後続のバイトまで確かめるなら utf8_next_char_byte_length() を使う。
 */
constexpr int utf8_lead_byte_length(unsigned char lead)
{
    if (lead <= 0x7f) {
        return 1;
    }

    if ((lead >= 0xc2) && (lead <= 0xdf)) {
        return 2;
    }

    if ((lead >= 0xe0) && (lead <= 0xef)) {
        return 3;
    }

    return ((lead >= 0xf0) && (lead <= 0xf4)) ? 4 : 0;
}

int utf8_next_char_byte_length(std::string_view str);
bool is_utf8_str(std::string_view str);

//! 内部の文字コード (セーブファイルに記録する値)
#if defined(JP) && defined(EUC)
constexpr auto SYSTEM_CHARACTER_ENCODING = CharacterEncoding::EUC_JP;
#elif defined(JP) && defined(SJIS)
constexpr auto SYSTEM_CHARACTER_ENCODING = CharacterEncoding::SHIFT_JIS;
#elif defined(JP) && defined(UTF8)
constexpr auto SYSTEM_CHARACTER_ENCODING = CharacterEncoding::UTF_8;
#else
constexpr auto SYSTEM_CHARACTER_ENCODING = CharacterEncoding::US_ASCII;
#endif

#if defined(JP) && defined(UTF8)
/*!
 * @brief 旧版の内部の文字コード (内部の文字コードを UTF-8 にする前に、同じプラットフォームの版が使っていた文字コード)
 * @details 旧版のデータは Windows 版なら Shift_JIS、Unix 系なら EUC-JP で書かれているので、文字コードを推定できない文字列はこれで読む
 */
#ifdef _WIN32
constexpr auto LEGACY_CHARACTER_ENCODING = CharacterEncoding::SHIFT_JIS;
#else
constexpr auto LEGACY_CHARACTER_ENCODING = CharacterEncoding::EUC_JP;
#endif
#endif

#ifdef JP

void sjis2euc(char *str);
void euc2sjis(char *str);
CharacterEncoding guess_legacy_encoding(std::string_view str);
size_t legacy_lead_byte_length(char lead, CharacterEncoding encoding);
tl::optional<std::string> legacy_to_sys(std::string_view str, CharacterEncoding encoding);
std::string legacy_to_sys_replacing_invalid(std::string_view str, CharacterEncoding encoding);
tl::optional<std::string> sys_to_utf8(std::string_view str);
tl::optional<std::string> utf8_to_sys(std::string_view utf8_str);
std::string guess_convert_to_system_encoding(std::string str);
size_t guess_convert_to_system_encoding(char *strbuf, int buflen);

#if defined(EUC) || defined(UTF8)
tl::optional<std::string> utf8_to_euc(std::string_view utf8_str);
#endif

#ifdef UTF8
std::string utf8_to_euc_keeping_width(std::string_view utf8_str);
#endif

#ifdef EUC
int euc_to_utf8(const char *euc_str, size_t euc_str_len, char *utf8_buf, size_t utf8_buf_len);
#endif

#else

inline tl::optional<std::string> sys_to_utf8(std::string_view str)
{
    return tl::make_optional<std::string>(str);
}

inline tl::optional<std::string> utf8_to_sys(std::string_view utf8_str)
{
    return tl::make_optional<std::string>(utf8_str);
}

#endif
