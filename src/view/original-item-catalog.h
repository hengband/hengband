#pragma once

#include "view/display-symbol.h"
#include <tl/optional.hpp>

namespace original_tiles {
// Ordinary terrain uses only columns0..15 of each virtual128-column row.
// Catalog icons occupy the unused columns16..127, below reserved attributes.
constexpr int CATALOG_ICONS = 915;
constexpr int CATALOG_FRAMES = 5895;
constexpr int CATALOG_BASE_ROW = 353;
constexpr int CATALOG_NATIVE_HEIGHT = (CATALOG_BASE_ROW + (CATALOG_FRAMES + 15) / 16) * 32;
static_assert(CATALOG_FRAMES <= 54 * 112);
static_assert(CATALOG_NATIVE_HEIGHT < 32768);
extern bool catalog_available;
int catalog_base_index(int id);
int catalog_artifact_index(int id);
int catalog_visible_index(int base_id, int appearance_id, int artifact_id, bool known);
DisplaySymbol catalog_tile_symbol(int icon, int color);
tl::optional<int> decode_catalog_tile(DisplaySymbol symbol);
}
