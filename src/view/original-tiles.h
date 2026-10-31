#pragma once

#include "util/point-2d.h"
#include "view/display-symbol.h"
#include <string_view>
#include <tl/optional.hpp>

class PlayerType;
class TerrainType;
class ItemEntity;
enum class ItemKindType : short;

namespace original_tiles {
constexpr auto COLUMNS = 16;
constexpr auto LIGHTING_ROWS = 18;
constexpr auto GLYPH_ROW = 66;
constexpr auto QUEST_ENTRANCE_TILE = 120;
constexpr auto DUNGEON_ENTRANCE_TILE = 124;
constexpr auto CONNECTED_FLOOR_ROW = 96;
constexpr auto CRACK_TILE_BASE_ROW = LIGHTING_ROWS * 3;
constexpr auto EXPANDED_FLOOR_ROW = 99;
constexpr auto EXPANDED_FLOOR_BASE_ROW = 78;
constexpr auto EXPANDED_FLOOR_PATTERNS = 4;
constexpr auto SQUARE_FLOOR_BASE = EXPANDED_FLOOR_PATTERNS * 9;
constexpr auto HORIZONTAL_FLOOR_BASE = SQUARE_FLOOR_BASE + 4 * 4;
constexpr auto VERTICAL_FLOOR_BASE = HORIZONTAL_FLOOR_BASE + 4 * 2;
constexpr auto EXPANDED_FLOOR_TILES = VERTICAL_FLOOR_BASE + 4 * 2;
constexpr auto SUPPLEMENT_ROW = 101;
constexpr auto SUPPLEMENT_BASE_ROW = 91;
constexpr auto SUPPLEMENT_TILES = 64;
constexpr auto ITEM_ROW = 54;
constexpr auto ITEM_BASE_ROW = 103;
constexpr auto ITEM_ICONS = 96;
constexpr auto ITEM_COLORS = 16;
constexpr auto ITEM_TILES = ITEM_ICONS * ITEM_COLORS;
static_assert(ITEM_ROW + (ITEM_TILES - 1) / 128 < 128);
constexpr auto MONSTER_ROW = 82;
constexpr auto MONSTER_BASE_ROW = 199;
constexpr auto MONSTER_ICONS = 96;
constexpr auto MONSTER_COLORS = 16;
constexpr auto MONSTER_TILES = MONSTER_ICONS * MONSTER_COLORS;
static_assert(MONSTER_ROW + (MONSTER_TILES - 1) / 128 < 128);
// 0xf0〜0xffはタイル行番号ではなく、端末の拡大タイル継続属性として予約されている。
static_assert(ITEM_ROW == LIGHTING_ROWS * 3);
static_assert(ITEM_ROW + (ITEM_TILES + 127) / 128 == GLYPH_ROW);
static_assert(GLYPH_ROW + 16 == MONSTER_ROW);
static_assert(MONSTER_ROW + (MONSTER_TILES + 127) / 128 <= CONNECTED_FLOOR_ROW);
static_assert(SUPPLEMENT_ROW + (SUPPLEMENT_TILES * 3 + 127) / 128 <= 0x70);

struct FloorConnections {
    int ports; //!< 北・東・南・西に各2ビットを使う。0は接続なし、2は中央で接続する。
    int grade;
    int lighting;
    int variant;
};

bool is_enabled();
int item_tile_index(ItemKindType type, int subtype, int appearance);
DisplaySymbol item_tile_symbol(int icon, int color);
tl::optional<int> decode_item_tile(DisplaySymbol symbol);
DisplaySymbol item_symbol(const ItemEntity &item, DisplaySymbol fallback);
int monster_tile_index(char character, int color);
DisplaySymbol monster_tile_symbol(int icon, int color);
tl::optional<int> decode_monster_tile(DisplaySymbol symbol);
DisplaySymbol monster_symbol(DisplaySymbol fallback, int appearance_color = -1);
int normalize_pit_mask(int mask);
int floor_variant(const Pos2D &pos, uint32_t seed);
int supplemental_tile_index(std::string_view tag);
bool is_animated_liquid(std::string_view tag);
int liquid_variant(const Pos2D &pos, uint32_t frame);
bool is_liquid_animation_visible(const PlayerType &player, const Pos2D &pos);
void advance_liquid_animation(PlayerType &player);
int standalone_floor_variant(const Pos2D &pos, uint32_t seed);
int floor_tile_index(int variant);
int expanded_floor_tile(const Pos2D &pos, uint32_t seed);
DisplaySymbol connected_floor_symbol(int ports, int grade, int lighting, int variant = 0);
tl::optional<FloorConnections> decode_connected_floor(DisplaySymbol symbol);
int connected_floor_tile_index(const FloorConnections &connections);
bool needs_neighbor_redraw(const PlayerType &player, const Pos2D &pos);
DisplaySymbol tile_symbol(int index, int lighting);
DisplaySymbol terrain_symbol(const PlayerType &player, const Pos2D &pos, const TerrainType &terrain, int lighting, DisplaySymbol fallback);
DisplaySymbolPair encode_foreground(const PlayerType &player, const Pos2D &pos, DisplaySymbolPair symbols);
}
