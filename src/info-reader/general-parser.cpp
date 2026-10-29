#include "info-reader/general-parser.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
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
