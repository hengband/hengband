#include "info-reader/general-parser.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include <algorithm>
#include <stdexcept>

dungeon_grid letter[255];

dungeon_grid &fixed_map_letter_at(unsigned char symbol)
{
    if (!is_fixed_map_symbol(symbol)) {
        throw std::out_of_range("Invalid fixed map symbol");
    }

    return letter[symbol];
}

void dungeon_grid::set_terrain_id(TerrainTag tag)
{
    this->feature = TerrainList::get_instance().get_terrain_id(tag);
}

void dungeon_grid::set_trap_id(TerrainTag tag)
{
    this->trap = TerrainList::get_instance().get_terrain_id(tag);
}

/*!
 * @brief 1バイトの印字可能なASCII記号を読み取る。
 * @param symbol 凡例のキーの文字列
 * @return 有効な記号、または文字数や文字種が不正なら空。
 */
tl::optional<char> parse_fixed_map_symbol(std::string_view symbol)
{
    if (symbol.size() != 1 || !is_fixed_map_symbol(static_cast<unsigned char>(symbol.front()))) {
        return tl::nullopt;
    }
    return symbol.front();
}

/*!
 * @brief 町のマップ・町の共通凡例で使う記号を読み取る。
 * @details 固定マップの記号のうち ':' '/' '\\' は町の記号に使えない。
 * @param symbol 凡例のキーの文字列
 * @return 有効な記号、または文字数や文字種が不正か、町で使えない記号なら空。
 */
tl::optional<char> parse_town_map_symbol(std::string_view symbol)
{
    const auto parsed = parse_fixed_map_symbol(symbol);
    if (!parsed || *parsed == ':' || *parsed == '/' || *parsed == '\\') {
        return tl::nullopt;
    }
    return parsed;
}

/*!
 * @brief 固定マップの行の上限・幅・記号を共通の順序で検証する。
 * @param row 検証する行
 * @param row_index 行の番号（0から）
 * @param limits 呼出側の上限と既存のエラー分類、空行・不揃い行の許容条件。
 * @param valid_symbol セルの検証。荒野では定義済み記号に限る。
 * @return 上限超過は指定のエラー、空行・不揃い行・不正記号はINVALID_VALUE。
 */
tl::expected<void, ParseErrorType> validate_fixed_map_row(std::string_view row, size_t row_index, const FixedMapRowLimits &limits,
    const std::function<bool(unsigned char)> &valid_symbol)
{
    if (row_index >= limits.height || row.size() > limits.width) {
        return tl::make_unexpected(limits.overflow_error);
    }
    if ((!limits.allow_empty && row.empty()) || (limits.expected_width && row.size() != *limits.expected_width) ||
        !std::all_of(row.begin(), row.end(), valid_symbol)) {
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }
    return {};
}
