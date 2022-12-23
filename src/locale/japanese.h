#pragma once

#include "system/angband.h"

#ifdef JP
#include <string>
#include <string_view>

constexpr int JVERB_AND = 1;
constexpr int JVERB_TO = 2;
constexpr int JVERB_OR = 3;
void jverb(concptr in, char *out, int flag);

std::string sindarin_to_kana(std::string_view sindarin);
void sjis2euc(char *str);
void euc2sjis(char *str);
byte codeconv(char *str);
bool iskanji2(concptr s, int x);
void guess_convert_to_system_encoding(char *strbuf, int buflen);

int lb_to_kg_integer(int x);
int lb_to_kg_fraction(int x);

#ifdef EUC
int utf8_to_euc(char *utf8_str, size_t utf8_str_len, char *euc_buf, size_t euc_buf_len);
int euc_to_utf8(const char *euc_str, size_t euc_str_len, char *utf8_buf, size_t utf8_buf_len);
#endif

#endif
