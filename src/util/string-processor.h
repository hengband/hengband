#pragma once

#include <charconv>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <system_error>
#include <tl/optional.hpp>
#include <vector>

/*!
 * @brief 文字列を数値に変換する
 * @param str 変換する文字列
 * @param base 基数（2〜36。省略した場合のデフォルト値は10）
 * @return 変換した数値。文字列全体が数値として解釈できない場合や基数が範囲外の場合はtl::nullopt
 */
template <typename T>
tl::optional<T> str_to_num(std::string_view str, int base = 10)
{
    // std::from_charsは2〜36以外の基数を渡すと未定義動作となる
    if (str.empty() || (base < 2) || (base > 36)) {
        return tl::nullopt;
    }

    const auto begin = str.data();
    const auto end = str.data() + str.size();
    T value;
    if (const auto [ptr, ec] = std::from_chars(begin, end, value, base); (ec == std::errc()) && (ptr == end)) {
        return value;
    }

    return tl::nullopt;
}

size_t angband_strcpy(char *buf, std::string_view src, size_t bufsize);
size_t angband_strcat(char *buf, std::string_view src, size_t bufsize);
char *angband_strstr(const char *haystack, std::string_view needle);
char *angband_strchr(const char *ptr, char ch);
char *ltrim(char *p);
char *rtrim(char *p);
int strrncmp(const char *s1, const char *s2, int len);
bool str_find(const std::string &src, std::string_view find);
std::string str_trim(std::string_view str);
std::string str_rtrim(std::string_view str);
std::string str_ltrim(std::string_view str);
std::vector<std::string> str_split(std::string_view str, char delim, bool trim = false, int num = 0);
std::vector<std::string> str_separate(std::string_view str, size_t len);
std::string str_erase(std::string str, std::string_view erase_chars);
std::string str_replace(std::string_view str, std::string_view old_str, std::string_view new_str);
std::string str_substr(std::string_view sv, size_t pos = 0, size_t n = std::string_view::npos);
std::string str_substr(std::string &&str, size_t pos = 0, size_t n = std::string_view::npos);
std::string str_substr(const char *str, size_t pos = 0, size_t n = std::string_view::npos);
std::string str_toupper(std::string_view str);
std::string str_tolower(std::string_view str);
std::string str_upcase_first(std::string_view str);
std::set<int> str_find_all_multibyte_chars(std::string_view str);
tl::optional<int> str_to_int(std::string_view str, int base = 10);
tl::optional<std::string_view> extract_suffix(std::string_view str, char find);
tl::optional<std::string_view> extract_suffix(std::string_view str, std::string_view find);
int count_digits(int value, int base = 10);
char hexify_upper(uint8_t value);
char hexify_lower(uint8_t value);
char octify(uint8_t i);
bool is_numeric(char c);
