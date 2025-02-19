#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

void text_to_ascii(char *buf, std::string_view sv, size_t bufsize);
void ascii_to_text(char *buf, std::string_view sv, size_t bufsize);
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
std::string str_substr(std::string_view sv, size_t pos = 0, size_t n = std::string_view::npos);
std::string str_substr(std::string &&str, size_t pos = 0, size_t n = std::string_view::npos);
std::string str_substr(const char *str, size_t pos = 0, size_t n = std::string_view::npos);
std::string str_toupper(std::string_view str);
std::string str_tolower(std::string_view str);
std::set<int> str_find_all_multibyte_chars(std::string_view str);
char hexify_upper(uint8_t value);
char hexify_lower(uint8_t value);
