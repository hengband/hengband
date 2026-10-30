#pragma once

#include "player-info/class-types.h"
#include "player/player-sex.h"
#include "view/display-symbol.h"
#include <tl/optional.hpp>

class PlayerType;

namespace original_tiles {
constexpr auto PLAYER_ROW = 103;
constexpr auto PLAYER_BASE_ROW = 295;
constexpr auto PLAYER_ICONS = PLAYER_CLASS_TYPE_MAX * MAX_SEXES;
constexpr auto PLAYER_COLORS = 16;
constexpr auto PLAYER_TILES = PLAYER_ICONS * PLAYER_COLORS;
static_assert(PLAYER_ICONS == 58);
static_assert(PLAYER_ROW + (PLAYER_TILES + 127) / 128 <= 0x70);
int player_tile_index(PlayerClassType player_class, player_sex sex);
DisplaySymbol player_tile_symbol(int icon, int color);
tl::optional<int> decode_player_tile(DisplaySymbol symbol);
DisplaySymbol player_symbol(const PlayerType &player, DisplaySymbol fallback);
}
