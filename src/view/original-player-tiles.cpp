#include "view/original-player-tiles.h"
#include "system/player-type-definition.h"
#include "view/original-tiles.h"

namespace original_tiles {
static_assert(PLAYER_ROW == SUPPLEMENT_ROW + (SUPPLEMENT_TILES * 3 + 127) / 128);
static_assert(PLAYER_BASE_ROW == MONSTER_BASE_ROW + MONSTER_TILES / COLUMNS);

int player_tile_index(PlayerClassType player_class, player_sex sex)
{
    const auto job = static_cast<int>(player_class);
    const auto gender = static_cast<int>(sex);
    if (job < 0 || job >= PLAYER_CLASS_TYPE_MAX || gender < 0 || gender >= MAX_SEXES) {
        return -1;
    }
    return job * MAX_SEXES + gender;
}

DisplaySymbol player_tile_symbol(int icon, int color)
{
    const auto payload = (color & 15) * PLAYER_ICONS + icon;
    return { static_cast<uint8_t>(0x80 + PLAYER_ROW + payload / 128), static_cast<char>(0x80 + payload % 128) };
}

tl::optional<int> decode_player_tile(DisplaySymbol symbol)
{
    const auto row = (symbol.color & 0x7f) - PLAYER_ROW;
    if (symbol.is_ascii_graphics() || !(symbol.character & 0x80) || row < 0 || row > (PLAYER_TILES - 1) / 128) {
        return tl::nullopt;
    }
    const auto index = row * 128 + (symbol.character & 0x7f);
    return index < PLAYER_TILES ? tl::make_optional(index) : tl::nullopt;
}

DisplaySymbol player_symbol(const PlayerType &player, DisplaySymbol fallback)
{
    if (!is_enabled() || !fallback.is_ascii_graphics()) {
        return fallback;
    }
    const auto icon = player_tile_index(player.pclass, player.psex);
    return icon < 0 ? monster_symbol(fallback) : player_tile_symbol(icon, fallback.color);
}
}
