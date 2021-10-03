#pragma once

#include "system/angband.h"

#include <string>
#include <vector>

extern std::vector<char> macro__buf;

extern bool get_com_no_macros;

extern std::vector<std::string> macro__pat;
extern std::vector<std::string> macro__act;
extern int16_t macro__num;

int macro_find_exact(concptr pat);
int macro_find_check(concptr pat);
errr macro_add(concptr pat, concptr act);
int macro_find_maybe(concptr pat);
int macro_find_ready(concptr pat);
