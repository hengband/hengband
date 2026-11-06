#pragma once

#include "term/tile-layout.h"
#include "view/display-symbol.h"

namespace original_tiles {
constexpr bool is_bigtile_continuation(DisplaySymbol symbol)
{
    return term_is_tile_continuation(symbol);
}
int item_symbol_tile_index(char character);
DisplaySymbol item_symbol_fallback(DisplaySymbol symbol);
}
