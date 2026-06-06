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
};

int utf8_next_char_byte_length(std::string_view str);
bool is_utf8_str(std::string_view str);
std::string utf8_to_local(std::string_view str_utf8);

#ifdef JP

void sjis2euc(char *str);
void euc2sjis(char *str);
CharacterEncoding codeconv(char *str);
bool iskanji2(const char *s, int x);
tl::optional<std::string> sys_to_utf8(std::string_view str);
tl::optional<std::string> utf8_to_sys(std::string_view utf8_str);
size_t guess_convert_to_system_encoding(char *strbuf, int buflen);

#ifdef EUC
int utf8_to_euc(char *utf8_str, size_t utf8_str_len, char *euc_buf, size_t euc_buf_len);
int euc_to_utf8(const char *euc_str, size_t euc_str_len, char *utf8_buf, size_t utf8_buf_len);
#endif

#else

inline tl::optional<std::string> sys_to_utf8(std::string_view str)
{
    return tl::make_optional<std::string>(str);
}

#endif
