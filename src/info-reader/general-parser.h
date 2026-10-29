#pragma once

#include "info-reader/parse-error-types.h"
#include "object-enchant/object-ego.h"
#include "system/angband.h"
#include <functional>
#include <string_view>
#include <tl/expected.hpp>
#include <tl/optional.hpp>

enum class FixedArtifactId : short;
enum class TerrainTag;
struct dungeon_grid {
    FEAT_IDX feature; /* Terrain feature */
    MONSTER_IDX monster; /* Monster */
    OBJECT_IDX object; /* Object */
    EgoType ego; /* Ego-Item */
    FixedArtifactId artifact; /* Artifact */
    IDX trap; /* Trap */
    BIT_FLAGS cave_info; /* Flags for CAVE_MARK, CAVE_GLOW, CAVE_NO_TELEPORT_DEST, CAVE_ROOM */
    int16_t special; /* Reserved for special terrain info */
    int random; /* Number of the random effect */

    void set_terrain_id(TerrainTag tag);
    void set_trap_id(TerrainTag tag);
};

extern dungeon_grid letter[255];

/*! @brief マップの1セルは印字可能なASCIIの1バイトで表す。letter[] の添字にも使用できる。 */
constexpr bool is_fixed_map_symbol(unsigned char symbol)
{
    return symbol >= ' ' && symbol <= '~';
}

/*! @brief 検証済みの固定マップ記号で letter[] を参照する。 */
dungeon_grid &fixed_map_letter_at(unsigned char symbol);
tl::optional<char> parse_fixed_map_symbol(std::string_view symbol);

struct FixedMapRowLimits {
    size_t height;
    size_t width;
    ParseErrorType overflow_error = ParseErrorType::OUT_OF_BOUNDS;
    bool allow_empty = true;
    tl::optional<size_t> expected_width = tl::nullopt;
};

tl::expected<void, ParseErrorType> validate_fixed_map_row(std::string_view row, size_t row_index, const FixedMapRowLimits &limits,
    const std::function<bool(unsigned char)> &valid_symbol = is_fixed_map_symbol);
