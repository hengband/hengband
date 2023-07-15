#pragma once

#include "system/angband.h"
#include <array>
#include <map>
#include <string>
#include <utility>

constexpr auto TERM_DEFAULT_COLS = 80;
constexpr auto TERM_DEFAULT_ROWS = 24;
constexpr auto MAIN_TERM_MIN_COLS = TERM_DEFAULT_COLS;
constexpr auto MAIN_TERM_MIN_ROWS = TERM_DEFAULT_ROWS;

extern const concptr color_names[16];
extern const concptr window_flag_desc[32];
extern const concptr ident_info[];

extern std::array<term_type *, 8> angband_terms;
#define term_screen (angband_terms[0])

extern TERM_COLOR misc_to_attr[256];
extern char misc_to_char[256];
extern TERM_COLOR tval_to_attr[128];
extern const char angband_term_name[8][16];
extern byte angband_color_table[256][4];

enum class AttributeType : int;
extern std::map<AttributeType, std::string> gf_colors;
extern TERM_COLOR color_char_to_attr(char c);

std::pair<TERM_COLOR, char> bolt_pict(POSITION y, POSITION x, POSITION ny, POSITION nx, AttributeType typ);
