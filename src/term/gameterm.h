#pragma once

#include "system/angband.h"
#include "util/point-2d.h"
#include <array>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>

constexpr auto MAX_TERM_DATA = 8; //!< Maximum number of terminals
constexpr auto TERM_DEFAULT_COLS = 80;
constexpr auto TERM_DEFAULT_ROWS = 24;
constexpr auto MAIN_TERM_MIN_COLS = TERM_DEFAULT_COLS;
constexpr auto MAIN_TERM_MIN_ROWS = TERM_DEFAULT_ROWS;
constexpr auto TERM_MAX_COLS = 255; //!< 端末の列数の上限 (term_type::wid の注記に合わせる)
constexpr auto TERM_MAX_ROWS = 255; //!< 端末の行数の上限 (term_type::hgt の注記に合わせる)

/*!
 * @brief 端末の大きさ
 */
struct TermSize {
    int cols; //!< 列数
    int rows; //!< 行数

    bool operator==(const TermSize &) const = default;
};

extern const concptr color_names[16];
extern const concptr window_flag_desc[32];
extern const concptr ident_info[];

TermSize get_term_min_size(int index);
bool is_valid_term_size(int index, const TermSize &size);

extern std::array<term_type *, 8> angband_terms;
#define term_screen (angband_terms[0])

extern std::array<DisplaySymbol, 256> ds_bolt;
extern TERM_COLOR tval_to_attr[128];
extern const char angband_term_name[8][16];
extern byte angband_color_table[256][4];

enum class AttributeType : int;
extern std::map<AttributeType, std::string> gf_colors;
extern TERM_COLOR color_char_to_attr(char c);

extern const std::unordered_map<std::string_view, TERM_COLOR> color_list;

class DisplaySymbol;
const DisplaySymbol &bolt_pict(const Pos2D &pos_src, const Pos2D &pos_dst, AttributeType typ);
