#pragma once

#include "system/angband.h"
#include <string>
#include <string_view>
#include <tl/optional.hpp>

#ifdef JP

enum class CharacterEncoding : uint8_t;

enum class JVerbConjugationType {
    AND = 0,
    TO = 1,
    OR = 2,
};
std::string conjugate_jverb(std::string_view in, JVerbConjugationType type);

std::string sindarin_to_kana(std::string_view sindarin);
bool is_kinsoku(std::string_view ch);
void sjis2euc(char *str);
void euc2sjis(char *str);
CharacterEncoding codeconv(char *str);
bool iskanji2(concptr s, int x);
tl::optional<std::string> sys_to_utf8(std::string_view str);
tl::optional<std::string> utf8_to_sys(std::string_view utf8_str);
size_t guess_convert_to_system_encoding(char *strbuf, int buflen);

int lb_to_kg_integer(int x);
int lb_to_kg_fraction(int x);

#ifdef EUC
int utf8_to_euc(char *utf8_str, size_t utf8_str_len, char *euc_buf, size_t euc_buf_len);
int euc_to_utf8(const char *euc_str, size_t euc_str_len, char *utf8_buf, size_t utf8_buf_len);
#endif

/*!
 * @brief インチ→cm変換
 */
constexpr int inch_to_cm(int inch)
{
    return inch * 254 / 100;
}

/*!
 * @brief ポンド→kg変換
 *
 * 体重表記用
 * アイテムの重量は0.5kg単位にするためlb_to_kg_integer/fractionを使用する
 */
constexpr int lb_to_kg(int lb)
{
    return lb * 4536 / 10000;
}

#else

constexpr bool is_kinsoku(std::string_view)
{
    return false;
}

inline tl::optional<std::string> sys_to_utf8(std::string_view str)
{
    return tl::make_optional<std::string>(str);
}

#endif
