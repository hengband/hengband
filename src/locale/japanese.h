#pragma once

#include "system/angband.h"

#ifdef JP
#define JVERB_AND 1
#define JVERB_TO  2
#define JVERB_OR  3
void jverb(concptr in, char *out, int flag);

void sindarin_to_kana(char *kana, concptr sindarin);
void sjis2euc(char *str);
void euc2sjis(char *str);
byte codeconv(char *str);
bool iskanji2(concptr s, int x);
void guess_convert_to_system_encoding(char* strbuf, int buflen);
#endif
