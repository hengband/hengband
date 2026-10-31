#pragma once

#include "view/display-symbol.h"
#include <tl/optional.hpp>

namespace original_tiles {
constexpr int MONSTER_CATALOG_ICONS = 1419;
constexpr int MONSTER_CATALOG_FRAMES = 2529;
constexpr int MONSTER_CATALOG_BASE_ROW = 722;
constexpr int MONSTER_CATALOG_NATIVE_HEIGHT = (MONSTER_CATALOG_BASE_ROW + (MONSTER_CATALOG_FRAMES + 15) / 16) * 32;
constexpr int MONSTER_CATALOG_CAPACITY = 1536 + 153 + 528 + 52 + 64 + 96 + 128;
static_assert(MONSTER_CATALOG_FRAMES <= MONSTER_CATALOG_CAPACITY);
static_assert(MONSTER_CATALOG_NATIVE_HEIGHT < 32768);
extern bool monster_catalog_available;
int monster_catalog_index(int apparent_id);
DisplaySymbol monster_catalog_tile_symbol(int icon, int color);
tl::optional<int> decode_monster_catalog_tile(DisplaySymbol symbol);
DisplaySymbol monster_catalog_symbol(int apparent_id, DisplaySymbol fallback);
DisplaySymbol monster_catalog_generic(DisplaySymbol fallback);
}
