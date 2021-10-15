#pragma once

#include "system/angband.h"

#ifdef JP
constexpr int JVERB_AND = 1;
constexpr int JVERB_TO = 2;
constexpr int JVERB_OR = 3;
void jverb(concptr in, char *out, int flag);

void sindarin_to_kana(char *kana, concptr sindarin);
void sjis2euc(char *str);
void euc2sjis(char *str);
byte codeconv(char *str);
bool iskanji2(concptr s, int x);
void guess_convert_to_system_encoding(char *strbuf, int buflen);

#define lbtokg(x) ((int)((x)*5)) /*!< 変愚蛮怒基準のポンド→キログラム変換定義(全体) */
#define lbtokg1(x) (lbtokg(x) / 100) /*!< 変愚蛮怒基準のポンド→キログラム変換定義(整数部) */
#define lbtokg2(x) ((lbtokg(x) % 100) / 10) /*!< 変愚蛮怒基準のポンド→キログラム変換定義(少数部) */

#ifdef EUC
int utf8_to_euc(char *utf8_str, size_t utf8_str_len, char *euc_buf, size_t euc_buf_len);
int euc_to_utf8(const char *euc_str, size_t euc_str_len, char *utf8_buf, size_t utf8_buf_len);
#endif

#endif
