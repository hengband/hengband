#include "view/original-monster-catalog.h"
#include "view/original-tiles.h"
#include <algorithm>
#include <iterator>

namespace original_tiles {
bool monster_catalog_available = false;
namespace {
    struct CatalogMonster {
        int id;
        int color_start;
        int color;
        char character;
    };
    constexpr CatalogMonster monsters[] = {
#include "view/original-monster-catalog.inc"
    };
    static_assert(std::size(monsters) == MONSTER_CATALOG_ICONS);

    // 旧モンスターバンクと、他のバンクで未使用と確認した枠を再利用する。
    // 実フォントバンクの制御文字は描画可能な字形としては使われない。
    constexpr int catalog_code(int payload)
    {
        if (payload < 1536) {
            return 82 * 128 + payload;
        }
        payload -= 1536;
        if (payload < 153) {
            const auto p = payload + 5895;
            return p / 112 * 128 + 16 + p % 112;
        }
        payload -= 153;
        if (payload < 512) {
            return (66 + payload / 32) * 128 + payload % 32;
        }
        payload -= 512;
        if (payload < 16) {
            return (66 + payload) * 128 + 127;
        }
        payload -= 16;
        if (payload < 52) {
            return 100 * 128 + 76 + payload;
        }
        payload -= 52;
        if (payload < 64) {
            return 102 * 128 + 64 + payload;
        }
        payload -= 64;
        if (payload < 96) {
            return 110 * 128 + 32 + payload;
        }
        return 111 * 128 + payload - 96;
    }
}

int monster_catalog_index(int apparent_id)
{
    const auto found = std::lower_bound(std::begin(monsters), std::end(monsters), apparent_id, [](const auto &entry, int id) { return entry.id < id; });
    return found == std::end(monsters) || found->id != apparent_id ? -1 : static_cast<int>(found - std::begin(monsters));
}

DisplaySymbol monster_catalog_tile_symbol(int icon, int color)
{
    if (icon < 0 || icon >= MONSTER_CATALOG_ICONS) {
        return {};
    }
    const auto &entry = monsters[icon];
    color &= 15;
    const auto extra = color < entry.color ? color : color - 1;
    const auto payload = entry.color_start < 0 || color == entry.color ? icon : entry.color_start + extra;
    const auto code = catalog_code(payload);
    return { static_cast<uint8_t>(0x80 + code / 128), static_cast<char>(0x80 + code % 128) };
}

tl::optional<int> decode_monster_catalog_tile(DisplaySymbol symbol)
{
    if (symbol.is_ascii_graphics() || !(symbol.character & 0x80)) {
        return tl::nullopt;
    }
    const auto row = symbol.color & 0x7f;
    const auto col = static_cast<unsigned char>(symbol.character) & 0x7f;
    auto payload = -1;
    if (row >= 82 && row < 94) {
        payload = (row - 82) * 128 + col;
    } else if (row < 54 && col >= 16) {
        const auto p = row * 112 + col - 16;
        if (p >= 5895) {
            payload = 1536 + p - 5895;
        }
    } else if (row >= 66 && row < 82 && col < 32) {
        payload = 1689 + (row - 66) * 32 + col;
    } else if (row >= 66 && row < 82 && col == 127) {
        payload = 2201 + row - 66;
    } else if (row == 100 && col >= 76) {
        payload = 2217 + col - 76;
    } else if (row == 102 && col >= 64) {
        payload = 2269 + col - 64;
    } else if (row == 110 && col >= 32) {
        payload = 2333 + col - 32;
    } else if (row == 111) {
        payload = 2429 + col;
    }
    return payload >= 0 && payload < MONSTER_CATALOG_FRAMES ? tl::optional<int>{ payload } : tl::nullopt;
}

DisplaySymbol monster_catalog_symbol(int apparent_id, DisplaySymbol fallback)
{
    if (!is_enabled() || !monster_catalog_available || !fallback.is_ascii_graphics()) {
        return monster_symbol(fallback);
    }
    const auto icon = monster_catalog_index(apparent_id);
    return icon < 0 ? monster_catalog_generic(fallback) : monster_catalog_tile_symbol(icon, fallback.color);
}

DisplaySymbol monster_catalog_generic(DisplaySymbol fallback)
{
    const auto first = std::find_if(std::begin(monsters), std::end(monsters), [&](const auto &entry) { return entry.character == fallback.character; });
    if (first == std::end(monsters)) {
        return fallback;
    }
    const auto match = std::find_if(first, std::end(monsters), [&](const auto &entry) { return entry.character == fallback.character && entry.color == (fallback.color & 15); });
    const auto selected = match == std::end(monsters) ? first : match;
    return monster_catalog_tile_symbol(static_cast<int>(selected - std::begin(monsters)), fallback.color);
}

int monster_tile_index(char character, int color)
{
    // 公開されている文字と表示色だけで輪郭を選ぶ。真の種族・能力・レベルや、
    // ユニークかどうかは参照しない。
    switch (character) {
    case ',':
        return 57;
    case '!':
        return 87;
    case '?':
        return 88;
    case '.':
        return 40;
    case '(':
        return 86;
    case '[':
        return 85;
    case ']':
        return 78;
    case '{':
        return 41;
    case '@':
        return 76;
    case '*':
        return 93;
    case '/':
        return 83;
    case '\\':
        return 84;
    case '&':
        return color == 7 ? 80 : 31;
    case '#':
        return color == 5 || color == 13 || color == 7 ? 77 : 92;
    case '%':
        return 74;
    case '`':
        return 58;
    case '+':
        return 90;
    case '=':
        return 89;
    case '>':
        return 91;
    case '|':
        return 82;
    case '~':
        return 94;
    case '$':
        return 81;
    case '7':
        return 75;
    case 'a':
        return 0;
    case 'A':
        return color == 11 ? 46 : 45;
    case 'b':
        return 1;
    case 'B':
        return 64;
    case 'c':
        return 2;
    case 'C':
        return color == 7 ? 68 : 3;
    case 'd':
        return 48;
    case 'D':
        return 49;
    case 'e':
        return 53;
    case 'E':
        if (color == 4 || color == 12 || color == 3) {
            return 62;
        }
        if (color == 6 || color == 14) {
            return 61;
        }
        return color == 7 || color == 15 ? 59 : 60;
    case 'f':
        return 4;
    case 'F':
        return 5;
    case 'g':
        return 58;
    case 'G':
        return 34;
    case 'h':
        return color == 7 || color == 15 || color == 5 ? 17 : 18;
    case 'H':
        return color == 11 ? 66 : color == 1 || color == 2 ? 65
                                                           : 73;
    case 'i':
        return 55;
    case 'I':
        return 14;
    case 'j':
        return 54;
    case 'J':
        return 7;
    case 'k':
        return 19;
    case 'K':
        return 6;
    case 'l':
        return color == 5 ? 72 : 8;
    case 'L':
        return 36;
    case 'm':
        return 56;
    case 'M':
        return 50;
    case 'n':
        return 51;
    case 'N':
        return 47;
    case 'o':
        return 20;
    case 'O':
        return 21;
    case 'p':
        switch (color) {
        case 4:
        case 12:
        case 10:
        case 11:
        case 3:
            return 25;
        case 6:
        case 14:
            return 26;
        case 5:
        case 13:
            return 28;
        case 8:
            return 27;
        case 2:
            return 30;
        case 9:
            return 29;
        default:
            return 24;
        }
    case 'P':
        return 23;
    case 'q':
        return 12;
    case 'Q':
        return 52;
    case 'r':
        return color == 1 ? 70 : 11;
    case 'R':
        return color == 5 || color == 13 ? 71 : 9;
    case 's':
        return 32;
    case 'S':
        return 10;
    case 't':
        return 16;
    case 'T':
        return 22;
    case 'u':
        return color == 1 ? 43 : 42;
    case 'U':
        return 44;
    case 'v':
        return 63;
    case 'V':
        return 37;
    case 'w':
        return 15;
    case 'W':
        return 35;
    case 'X':
        return 67;
    case 'x':
        return 79;
    case 'y':
        return 19;
    case 'Y':
        return 13;
    case 'z':
        return color == 2 ? 38 : color == 1 ? 39
                                            : 33;
    case 'Z':
        return 69;
    default:
        return character > ' ' ? 95 : -1;
    }
}

DisplaySymbol monster_tile_symbol(int icon, int color)
{
    const auto payload = (color & 15) * MONSTER_ICONS + icon;
    return { static_cast<uint8_t>(0x80 + MONSTER_ROW + payload / 128), static_cast<char>(0x80 + payload % 128) };
}

tl::optional<int> decode_monster_tile(DisplaySymbol symbol)
{
    const auto row = (symbol.color & 0x7f) - MONSTER_ROW;
    if (symbol.is_ascii_graphics() || !(symbol.character & 0x80) || row < 0 || row > (MONSTER_TILES - 1) / 128) {
        return tl::nullopt;
    }
    return row * 128 + (symbol.character & 0x7f);
}

DisplaySymbol monster_symbol(DisplaySymbol fallback, int appearance_color)
{
    if (!is_enabled() || !fallback.is_ascii_graphics()) {
        return fallback;
    }
    if (monster_catalog_available) {
        return monster_catalog_generic(fallback);
    }
    const auto icon = monster_tile_index(fallback.character, appearance_color < 0 ? fallback.color : appearance_color);
    return icon < 0 ? fallback : monster_tile_symbol(icon, fallback.color);
}
}
