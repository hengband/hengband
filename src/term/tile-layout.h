#pragma once

#include "view/display-symbol.h"

class term_win;
class DisplaySymbolPair;

struct TermTileBlock {
    int x;
    int y;
    int width;
    int height;
    bool zoomed() const
    {
        return width == 4 && height == 2;
    }
};

constexpr bool term_is_zoom_continuation(DisplaySymbol symbol)
{
    const auto c = static_cast<unsigned char>(symbol.character);
    return symbol.color == 0xf0 && c >= 0x81 && c <= 0x87;
}

constexpr bool term_is_tile_continuation(DisplaySymbol symbol)
{
    return (symbol.color == 0xf0 && static_cast<unsigned char>(symbol.character) == 0xff) || term_is_zoom_continuation(symbol);
}

TermTileBlock term_tile_block_at(const term_win &screen, int x, int y);
const term_win &term_pict_buffer();
bool term_queue_scaled_bigchar(int x, int y, const DisplaySymbolPair &symbols, int scale);
