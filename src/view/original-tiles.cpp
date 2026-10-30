#include "view/original-tiles.h"
#include "game-option/special-options.h"
#include "grid/grid.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "system/system-variables.h"
#include "system/terrain/terrain-definition.h"
#include "term/gameterm.h"
#include "term/tile-layout.h"
#include "term/z-term.h"
#include "timed-effect/timed-effects.h"
#include "view/display-map.h"
#include "view/map-tile-layout.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <utility>

namespace original_tiles {
bool double_map_size = false;

int map_tile_scale()
{
    return double_map_size && is_enabled() && use_bigtile && game_term && game_term == angband_terms[0] ? 2 : 1;
}

int map_tile_columns()
{
    return (use_bigtile ? 2 : 1) * map_tile_scale();
}
int map_tile_rows()
{
    return map_tile_scale();
}

bool queue_map_tile(int x, int y, DisplaySymbolPair symbols)
{
    const auto scale = map_tile_scale();
    auto &foreground = symbols.symbol_foreground;
    if (scale == 2 && foreground.is_ascii_graphics() && foreground.character > ' ') {
        foreground = { static_cast<uint8_t>(0x80 + GLYPH_ROW + (foreground.color & 15)), static_cast<char>(0x80 | (foreground.character & 0x7f)) };
    }
    return term_queue_scaled_bigchar(x, y, symbols, scale);
}

namespace {
    uint32_t liquid_animation_frame = 0;
    uint32_t coordinate_hash(int x, int y, uint32_t seed)
    {
        auto value = static_cast<uint32_t>(x) * 0x9e3779b9U ^ static_cast<uint32_t>(y) * 0x85ebca6bU ^ seed * 0xc2b2ae35U;
        value ^= value >> 16;
        value *= 0x7feb352dU;
        return value ^ (value >> 15);
    }

    double floor_noise(const Pos2D &pos, uint32_t seed, int scale)
    {
        const auto cell_x = static_cast<int>(std::floor(static_cast<double>(pos.x) / scale));
        const auto cell_y = static_cast<int>(std::floor(static_cast<double>(pos.y) / scale));
        const auto smooth = [](double value) { return value * value * (3.0 - 2.0 * value); };
        const auto x = smooth(static_cast<double>(pos.x - cell_x * scale) / scale);
        const auto y = smooth(static_cast<double>(pos.y - cell_y * scale) / scale);
        const auto sample = [seed](int sx, int sy) { return static_cast<double>(coordinate_hash(sx, sy, seed) & 0xffffU) / 65535.0; };
        const auto north = sample(cell_x, cell_y) * (1.0 - x) + sample(cell_x + 1, cell_y) * x;
        const auto south = sample(cell_x, cell_y + 1) * (1.0 - x) + sample(cell_x + 1, cell_y + 1) * x;
        return north * (1.0 - y) + south * y;
    }

    double floor_wear(const Pos2D &pos, uint32_t seed)
    {
        return 0.8 * floor_noise(pos, seed, 10) + 0.2 * floor_noise(pos, seed ^ 0xa511e9b3U, 20);
    }

    uint32_t floor_seed(const PlayerType &player)
    {
        const auto &floor = *player.current_floor_ptr;
        return static_cast<uint32_t>(floor.dun_level) * 0x9e3779b9U ^ static_cast<uint32_t>(floor.quest_number);
    }

    int terrain_row(std::string_view tag)
    {
        constexpr std::array<std::string_view, 28> tags = { {
            "FLOOR",
            "PERMANENT",
            "CLOSED_DOOR",
            "UP_STAIR",
            "SHALLOW_WATER",
            "DEEP_WATER",
            "SHALLOW_LAVA",
            "DEEP_LAVA",
            "DIRT",
            "GRASS",
            "GRANITE",
            "RUBBLE",
            "MAGMA_VEIN",
            "QUARTZ_VEIN",
            "SWAMP",
            "DARK_PIT",
            "TREE",
            "MOUNTAIN",
            "FLOWER",
            "BRAKE",
            "OPEN_DOOR",
            "BROKEN_DOOR",
            "DOWN_STAIR",
            "SHAFT_DOWN",
            "GLASS_FLOOR",
            "GLASS_WALL",
            "CLOSED_GLASS_DOOR",
            "MIRROR",
        } };
        for (size_t i = 0; i < tags.size(); ++i) {
            if (tags[i] == tag) {
                return static_cast<int>(i);
            }
        }
        if (tag.starts_with("PERMANENT_")) {
            return 1;
        }
        if (tag.starts_with("GRANITE_") || tag == "MOUNTAIN_WALL") {
            return 10;
        }
        if (tag.starts_with("LOCKED_DOOR_") || tag.starts_with("JAMMED_DOOR_")) {
            return 2;
        }
        if (tag.starts_with("LOCKED_GLASS_DOOR_") || tag.starts_with("JAMMED_GLASS_DOOR_")) {
            return 26;
        }
        if (tag == "PERMANENT_GLASS_WALL") {
            return 25;
        }
        if (tag == "QUEST_EXIT" || tag == "QUEST_UP") {
            return 3;
        }
        if (tag == "QUEST_DOWN") {
            return 22;
        }
        return -1;
    }

    bool is_known_pit(const PlayerType &player, const Pos2D &pos)
    {
        const auto &floor = *player.current_floor_ptr;
        return floor.contains(pos, FloorBoundary::OUTER_WALL_INCLUSIVE) && is_map_terrain_visible(player, pos) && floor.get_grid(pos).get_terrain(TerrainKind::MIMIC).tag == "DARK_PIT";
    }

    int pit_tile(const PlayerType &player, const Pos2D &pos)
    {
        constexpr std::array<std::pair<Pos2DVec, int>, 8> neighbors = { {
            { { -1, 0 }, 1 },
            { { 0, 1 }, 2 },
            { { 1, 0 }, 4 },
            { { 0, -1 }, 8 },
            { { -1, 1 }, 16 },
            { { 1, 1 }, 32 },
            { { 1, -1 }, 64 },
            { { -1, -1 }, 128 },
        } };
        auto mask = 0;
        for (const auto &[offset, bit] : neighbors) {
            if (is_known_pit(player, pos + offset)) {
                mask |= bit;
            }
        }
        mask = normalize_pit_mask(mask);
        auto rank = 0;
        for (auto candidate = 0; candidate < mask; ++candidate) {
            if (normalize_pit_mask(candidate) == candidate) {
                ++rank;
            }
        }
        return 224 + rank;
    }
}

bool is_enabled()
{
    return use_graphics && ANGBAND_GRAF == "original32";
}

int supplemental_tile_index(std::string_view tag)
{
    constexpr std::pair<std::string_view, int> tiles[] = {
#include "view/original-tile-supplement.inc"
    };
    for (const auto &[key, index] : tiles) {
        if (key == tag) {
            return index;
        }
    }
    return -1;
}

bool is_animated_liquid(std::string_view tag)
{
    return tag == "SHALLOW_WATER" || tag == "DEEP_WATER" || tag == "SHALLOW_LAVA" || tag == "DEEP_LAVA";
}

int liquid_variant(const Pos2D &pos, uint32_t frame)
{
    return static_cast<int>(coordinate_hash(pos.x, pos.y, frame) % 8);
}

bool is_liquid_animation_visible(const PlayerType &player, const Pos2D &pos)
{
    const auto &floor = *player.current_floor_ptr;
    if (!floor.contains(pos, FloorBoundary::OUTER_WALL_INCLUSIVE) || player.effects()->blindness().is_active()) {
        return false;
    }
    const auto &grid = floor.get_grid(pos);
    if (!grid.is_view() || !is_map_terrain_visible(player, pos) || (!player.see_nocto && grid.is_darkened())) {
        return false;
    }
    return player.see_nocto || (grid.info & (CAVE_LITE | CAVE_MNLT)) || (grid.info & (CAVE_GLOW | CAVE_MNDK)) == CAVE_GLOW;
}

void advance_liquid_animation(PlayerType &player)
{
    if (!is_enabled()) {
        return;
    }
    ++liquid_animation_frame;
    const auto &floor = *player.current_floor_ptr;
    for (POSITION y = 0; y < floor.height; ++y) {
        for (POSITION x = 0; x < floor.width; ++x) {
            const Pos2D pos{ y, x };
            const auto &grid = floor.get_grid(pos);
            if (grid.is_view() && is_animated_liquid(grid.get_terrain(TerrainKind::MIMIC).tag) && is_liquid_animation_visible(player, pos)) {
                lite_spot(&player, pos);
            }
        }
    }
}

int normalize_pit_mask(int mask)
{
    if ((mask & 3) != 3) {
        mask &= ~16;
    }
    if ((mask & 6) != 6) {
        mask &= ~32;
    }
    if ((mask & 12) != 12) {
        mask &= ~64;
    }
    if ((mask & 9) != 9) {
        mask &= ~128;
    }
    return mask;
}

int floor_variant(const Pos2D &pos, uint32_t seed)
{
    // Smooth fields make worn areas contiguous; a separate coordinate hash varies the four motifs.
    // The field changes slowly enough that cardinal neighbors never skip a damage grade.
    const auto wear = floor_wear(pos, seed);
    const auto grade = wear < 0.58 ? 0 : wear < 0.68 ? 1
                                     : wear < 0.84   ? 2
                                                     : 3;
    return grade * 4 + static_cast<int>((coordinate_hash(pos.x, pos.y, seed) >> 8) % 4);
}

int floor_tile_index(int variant)
{
    // Closed doors and up stairs only use their first four slots, leaving these duplicate slots free.
    if (variant >= 12) {
        return 28 + variant - 12;
    }
    if (variant >= 8) {
        return 20 + variant - 8;
    }
    return variant;
}

int expanded_floor_tile(const Pos2D &pos, uint32_t seed)
{
    // One authored fracture per eligible 7x7 region. Jitter its center while
    // keeping the whole patch inside its region, so separate fractures never overlap.
    const auto region_x = static_cast<int>(std::floor(static_cast<double>(pos.x) / 7));
    const auto region_y = static_cast<int>(std::floor(static_cast<double>(pos.y) / 7));
    const auto hash = coordinate_hash(region_x, region_y, seed ^ 0xd2511f53U);
    const auto center_x = region_x * 7 + 2 + static_cast<int>(hash % 3);
    const auto center_y = region_y * 7 + 2 + static_cast<int>((hash >> 8) % 3);
    const Pos2D center{ static_cast<POSITION>(center_y), static_cast<POSITION>(center_x) };
    // Compact patches are the common accents; reserve the largest patch for worn areas.
    const auto size_choice = (hash >> 24) % 5;
    const auto kind = size_choice == 0 ? 0 : size_choice <= 2 ? 1
                                                              : 2;
    if (floor_wear(center, seed) < (kind == 0 ? 0.68 : 0.48)) {
        return -1;
    }
    const auto pattern = static_cast<int>((hash >> 16) % EXPANDED_FLOOR_PATTERNS);
    auto width = 3;
    auto height = 3;
    auto left = center_x - 1;
    auto top = center_y - 1;
    auto base = pattern * 9;
    if (kind == 1) {
        width = height = 2;
        base = SQUARE_FLOOR_BASE + pattern * 4;
    } else if (kind == 2) {
        const auto horizontal = (hash & (1U << 28)) != 0;
        width = horizontal ? 2 : 1;
        height = horizontal ? 1 : 2;
        left = center_x - (horizontal ? 1 : 0);
        top = center_y - (horizontal ? 0 : 1);
        base = (horizontal ? HORIZONTAL_FLOOR_BASE : VERTICAL_FLOOR_BASE) + pattern * 2;
    }
    const auto x = pos.x - left;
    const auto y = pos.y - top;
    return x >= 0 && x < width && y >= 0 && y < height ? base + y * width + x : -1;
}

int standalone_floor_variant(const Pos2D &pos, uint32_t seed)
{
    const auto selected = floor_variant(pos, seed);
    if (selected < 8) {
        return selected;
    }
    if (selected < 12) {
        return selected % 4;
    }
    const auto priority = coordinate_hash(pos.x, pos.y, seed ^ 0x91e10da5U);
    for (auto dy = -1; dy <= 1; ++dy) {
        for (auto dx = -1; dx <= 1; ++dx) {
            if (dy == 0 && dx == 0) {
                continue;
            }
            const auto neighbor = pos + Pos2DVec{ static_cast<POSITION>(dy), static_cast<POSITION>(dx) };
            // Keep a full eight-neighbor quiet border around standalone medium/heavy damage.
            // Coordinate-only decisions remain stable without consulting hidden terrain.
            if (expanded_floor_tile(neighbor, seed) >= 0) {
                return selected % 4;
            }
            if (floor_variant(neighbor, seed) < 12) {
                continue;
            }
            const auto other = coordinate_hash(neighbor.x, neighbor.y, seed ^ 0x91e10da5U);
            if (other > priority || (other == priority && (dy < 0 || (dy == 0 && dx < 0)))) {
                return selected % 4;
            }
        }
    }
    // Share isolated positions, then choose medium/heavy with an independent equal-weight hash.
    const auto grade = (coordinate_hash(pos.x, pos.y, seed ^ 0x2c1b3c6dU) & 1U) == 0 ? 2 : 3;
    return grade * 4 + selected % 4;
}

DisplaySymbol connected_floor_symbol(int ports, int grade, int lighting, int variant)
{
    auto mask = 0;
    for (auto direction = 0; direction < 4; ++direction) {
        if (((ports >> (direction * 2)) & 3) != 0) {
            mask |= 1 << direction;
        }
    }
    const auto payload = mask | (variant << 4) | ((grade - 2) << 6) | (lighting << 7);
    return { static_cast<uint8_t>(0x80 + CONNECTED_FLOOR_ROW + payload / 128), static_cast<char>(0x80 + payload % 128) };
}

tl::optional<FloorConnections> decode_connected_floor(DisplaySymbol symbol)
{
    const auto row = symbol.color & 0x7f;
    if (symbol.is_ascii_graphics() || !(symbol.character & 0x80) || row < CONNECTED_FLOOR_ROW || row >= CONNECTED_FLOOR_ROW + 3) {
        return tl::nullopt;
    }
    const auto payload = (row - CONNECTED_FLOOR_ROW) * 128 + (symbol.character & 0x7f);
    auto ports = 0;
    for (auto direction = 0; direction < 4; ++direction) {
        if ((payload & (1 << direction)) != 0) {
            ports |= 2 << (direction * 2);
        }
    }
    return FloorConnections{ ports, 2 + ((payload >> 6) & 1), payload >> 7, (payload >> 4) & 3 };
}

int connected_floor_tile_index(const FloorConnections &connections)
{
    auto mask = 0;
    for (auto direction = 0; direction < 4; ++direction) {
        if (((connections.ports >> (direction * 2)) & 3) != 0) {
            mask |= 1 << direction;
        }
    }
    return connections.lighting * 128 + (connections.grade - 2) * 64 + mask * 4 + connections.variant;
}
bool needs_neighbor_redraw(const PlayerType &player, const Pos2D &pos)
{
    const auto &floor = *player.current_floor_ptr;
    if (!floor.contains(pos, FloorBoundary::OUTER_WALL_INCLUSIVE)) {
        return false;
    }
    const auto &terrain = floor.get_grid(pos).get_terrain(TerrainKind::MIMIC);
    return terrain.tag == "DARK_PIT" || (terrain.tag == "FLOOR" && floor_variant(pos, floor_seed(player)) >= 8);
}

DisplaySymbol tile_symbol(int index, int lighting)
{
    return { static_cast<uint8_t>(0x80 + index / COLUMNS + lighting * LIGHTING_ROWS), static_cast<char>(0x80 + index % COLUMNS) };
}

DisplaySymbol terrain_symbol(const PlayerType &player, const Pos2D &pos, const TerrainType &terrain, int lighting, DisplaySymbol fallback)
{
    if (!is_enabled()) {
        return fallback;
    }
    const auto hash = coordinate_hash(pos.x, pos.y, 0);
    const auto variant = is_animated_liquid(terrain.tag) && is_liquid_animation_visible(player, pos)
                             ? liquid_variant(pos, liquid_animation_frame)
                             : static_cast<int>(hash % 8);
    if (terrain.tag == "FLOOR") {
        const auto seed = floor_seed(player);
        const auto patch = expanded_floor_tile(pos, seed);
        if (patch >= 0) {
            const auto payload = lighting * EXPANDED_FLOOR_TILES + patch;
            return { static_cast<uint8_t>(0x80 + EXPANDED_FLOOR_ROW + payload / 128), static_cast<char>(0x80 + payload % 128) };
        }
        const auto standalone = standalone_floor_variant(pos, seed);
        return tile_symbol(floor_tile_index(standalone), lighting);
    }
    if (terrain.tag == "QUEST_ENTER") {
        return tile_symbol(QUEST_ENTRANCE_TILE + variant % 4, lighting);
    }
    if (terrain.tag == "ENTRANCE") {
        return tile_symbol(DUNGEON_ENTRANCE_TILE + variant % 4, lighting);
    }
    if (terrain.tag == "SHAFT_UP") {
        // Share the up-stair silhouette while keeping the shaft's own color.
        return tile_symbol(23 * 8 + 4 + variant % 4, lighting);
    }
    if (terrain.tag == "DARK_PIT") {
        return tile_symbol(pit_tile(player, pos), lighting);
    }
    if (terrain.tag == "MAGMA_VEIN_ON_DEEP_WATER") {
        return tile_symbol(272 + variant, lighting);
    }
    if (terrain.tag == "RUBBLE_ON_SHALLOW_WATER") {
        return tile_symbol(280 + variant, lighting);
    }
    const auto supplement = supplemental_tile_index(terrain.tag);
    if (supplement >= 0) {
        const auto payload = lighting * SUPPLEMENT_TILES + supplement;
        return { static_cast<uint8_t>(0x80 + SUPPLEMENT_ROW + payload / 128), static_cast<char>(0x80 + payload % 128) };
    }
    const auto row = terrain_row(terrain.tag);
    if (row < 0) {
        return fallback;
    }
    const auto selected = (row == 2 || row == 3 || row == 23) ? variant % 4 : variant;
    return tile_symbol(row * 8 + selected, lighting);
}

DisplaySymbolPair encode_foreground(const PlayerType &player, const Pos2D &pos, DisplaySymbolPair symbols)
{
    if (!is_enabled()) {
        return symbols;
    }
    const auto foreground = symbols.symbol_foreground;
    auto &background = symbols.symbol_background;
    if (background.is_ascii_graphics()) {
        // Unknown squares stay black. Unmade terrain keeps its original colored character.
        if (background.character == ' ' || background.character == '\0') {
            if (foreground.character == ' ' || foreground.character == '\0') {
                return symbols;
            }
            background = {};
        } else {
            background = tile_symbol(0, decide_map_terrain_lighting(player, pos));
        }
    }
    if (!foreground.is_ascii_graphics()) {
        return symbols;
    }
    if (foreground.character == ' ' || foreground.character == '\0') {
        symbols.symbol_foreground = background;
    } else {
        // Reserved rows request live font drawing, not a bundled font bitmap.
        symbols.symbol_foreground = { static_cast<uint8_t>(0x80 + GLYPH_ROW + (foreground.color & 15)), static_cast<char>(0x80 | (foreground.character & 0x7f)) };
    }
    return symbols;
}
}
