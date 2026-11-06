#include "object/tval-types.h"
#include "system/baseitem/baseitem-record.h"
#include "system/item/item-entity.h"
#include "view/original-item-catalog.h"
#include "view/original-symbol-tiles.h"
#include "view/original-tiles.h"
#include <algorithm>
#include <iterator>
#include <utility>

namespace original_tiles {
bool catalog_available = false;

namespace {
    constexpr std::pair<int, int> catalog_bases[] = {
#include "view/original-catalog-base.inc"
    };
    constexpr std::pair<int, int> catalog_artifacts[] = {
#include "view/original-catalog-artifacts.inc"
    };
    constexpr int catalog_color_starts[] = {
#include "view/original-catalog-colors.inc"
    };
    static_assert(std::size(catalog_bases) == 644);
    static_assert(std::size(catalog_artifacts) == 271);
    static_assert(std::size(catalog_color_starts) == CATALOG_ICONS);
    template <size_t N>
    int catalog_find(const std::pair<int, int> (&table)[N], int id)
    {
        const auto entry = std::lower_bound(std::begin(table), std::end(table), id, [](const auto &value, int key) { return value.first < key; });
        return entry != std::end(table) && entry->first == id ? entry->second : -1;
    }
}

int catalog_base_index(int id)
{
    return catalog_find(catalog_bases, id);
}

int catalog_artifact_index(int id)
{
    return catalog_find(catalog_artifacts, id);
}

int catalog_visible_index(int base_id, int appearance_id, int artifact_id, bool known)
{
    if (known) {
        const auto artifact = catalog_artifact_index(artifact_id);
        if (artifact >= 0) {
            return artifact;
        }
    }
    return catalog_base_index(appearance_id > 0 ? appearance_id : base_id);
}

DisplaySymbol catalog_tile_symbol(int icon, int color)
{
    if (icon < 0 || icon >= CATALOG_ICONS) {
        return {};
    }
    const auto public_color = color & 15;
    const auto start = catalog_color_starts[icon];
    const auto payload = start >= 0 && public_color != 1 ? start + (public_color == 0 ? 0 : public_color - 1) : icon;
    return { static_cast<uint8_t>(0x80 + payload / 112), static_cast<char>(0x80 + 16 + payload % 112) };
}

tl::optional<int> decode_catalog_tile(DisplaySymbol symbol)
{
    const auto row = symbol.color & 0x7f;
    const auto column = static_cast<unsigned char>(symbol.character) & 0x7f;
    if (symbol.is_ascii_graphics() || !(symbol.character & 0x80) || row >= 54 || column < 16) {
        return tl::nullopt;
    }
    const auto payload = row * 112 + column - 16;
    if (payload >= CATALOG_FRAMES) {
        return tl::nullopt;
    }
    return payload;
}

int item_symbol_tile_index(char character)
{
    switch (character) {
    case '_':
        return 46;
    case '-':
        return 45;
    case ',':
        return 38;
    case '!':
        return 40;
    case '?':
        return 44;
    case '"':
        return 50;
    case '(':
        return 25;
    case ')':
        return 26;
    case '[':
        return 22;
    case ']':
        return 30;
    case '{':
        return 17;
    case '}':
        return 14;
    case '*':
        return 95;
    case '/':
        return 7;
    case '\\':
        return 9;
    case '&':
        return 90;
    case '`':
        return 59;
    case '=':
        return 48;
    case '|':
        return 2;
    case '~':
        return 31;
    case '$':
        return 33;
    default:
        return character > ' ' ? 90 : -1;
    }
}

DisplaySymbol item_symbol_fallback(DisplaySymbol symbol)
{
    if (!is_enabled() || !symbol.is_ascii_graphics()) {
        return symbol;
    }
    const auto icon = item_symbol_tile_index(symbol.character);
    return icon < 0 ? symbol : item_tile_symbol(icon, symbol.color);
}

int item_tile_index(ItemKindType type, int subtype, int appearance)
{
    const auto flavor = std::max(0, appearance);
    const auto tval = static_cast<int>(type);
    if ((tval >= 90 && tval <= 99) || (tval >= 105 && tval <= 107)) {
        return subtype >= 2 ? 93 : 52 + (tval - 90) % 6;
    }
    switch (type) {
    case ItemKindType::FLAVOR_SKELETON:
        return 60;
    case ItemKindType::BOTTLE:
        return 71;
    case ItemKindType::JUNK:
        return subtype == 6 ? 75 : 65;
    case ItemKindType::WHISTLE:
        return 62;
    case ItemKindType::SPIKE:
        return 64;
    case ItemKindType::CHEST:
        return subtype == 50 ? 90 : subtype >= 5 ? 35
                                                 : 34;
    case ItemKindType::FIGURINE:
        return 58;
    case ItemKindType::STATUE:
        return 59;
    case ItemKindType::MONSTER_REMAINS:
        return subtype == 0 ? 79 : 61;
    case ItemKindType::CAPTURE:
        return 63;
    case ItemKindType::SHOT:
        return 19;
    case ItemKindType::ARROW:
        return 17;
    case ItemKindType::BOLT:
        return 18;
    case ItemKindType::BOW:
        if (subtype == 2) {
            return 16;
        }
        if (subtype == 23 || subtype == 24) {
            return 15;
        }
        if (subtype == 50) {
            return 83;
        }
        if (subtype == 51) {
            return 84;
        }
        return 14;
    case ItemKindType::DIGGING:
        return subtype <= 3 ? 13 : 12;
    case ItemKindType::HAFTED:
        if (subtype == 2) {
            return 80;
        }
        if (subtype == 6 || subtype == 13 || subtype == 18) {
            return 10;
        }
        if (subtype == 8 || subtype == 19) {
            return 9;
        }
        if (subtype == 3 || subtype == 7 || subtype == 11 || subtype == 14 || subtype == 16 || subtype == 21 || subtype == 50 || subtype == 63) {
            return 11;
        }
        return 8;
    case ItemKindType::POLEARM:
        if (subtype == 40) {
            return 82;
        }
        if (subtype == 3 || subtype == 17 || subtype == 30 || subtype == 50) {
            return 81;
        }
        if (subtype == 1 || subtype == 10 || subtype == 11 || subtype == 22 || subtype == 25 || subtype == 28) {
            return 5;
        }
        if (subtype == 2 || subtype == 4 || subtype == 5 || subtype == 7 || subtype == 8 || subtype == 20 || subtype == 26 || subtype == 29) {
            return 6;
        }
        return 7;
    case ItemKindType::SWORD:
        if (subtype == 1 || subtype == 2) {
            return 66;
        }
        if (subtype == 4 || subtype == 5 || subtype == 6 || subtype == 32) {
            return 0;
        }
        if (subtype >= 7 && subtype <= 13) {
            return 1;
        }
        if (subtype == 19 || subtype == 20 || subtype == 27) {
            return 3;
        }
        return subtype >= 23 && subtype != 33 ? 4 : 2;
    case ItemKindType::BOOTS:
        return 30;
    case ItemKindType::GLOVES:
        return 29;
    case ItemKindType::HELM:
        return subtype == 1 ? 86 : 27;
    case ItemKindType::CROWN:
        return 28;
    case ItemKindType::SHIELD:
        return 26;
    case ItemKindType::CLOAK:
        return 25;
    case ItemKindType::SOFT_ARMOR:
        return subtype == 1 ? 24 : 20;
    case ItemKindType::HARD_ARMOR:
        return subtype <= 8 ? 21 : 22;
    case ItemKindType::DRAG_ARMOR:
        return 23;
    case ItemKindType::LITE:
        return subtype == 0 ? 31 : subtype <= 3 ? 32
                                                : 85;
    case ItemKindType::AMULET:
        return 50 + flavor % 2;
    case ItemKindType::RING:
        return 48 + flavor % 2;
    case ItemKindType::CARD:
        return 67;
    case ItemKindType::STAFF:
        return 46;
    case ItemKindType::WAND:
        return 45;
    case ItemKindType::ROD:
        return 47;
    case ItemKindType::PARCHMENT:
        return 68;
    case ItemKindType::SCROLL:
        return 44;
    case ItemKindType::POTION:
        return 40 + flavor % 4;
    case ItemKindType::FLASK:
        return 39;
    case ItemKindType::FOOD:
        if (flavor > 0 || subtype < 32) {
            return 38;
        }
        if (subtype == 32 || subtype == 37) {
            return 37;
        }
        if (subtype == 33) {
            return 70;
        }
        if (subtype == 35) {
            return 36;
        }
        if (subtype == 36) {
            return 69;
        }
        if (subtype == 38) {
            return 87;
        }
        if (subtype == 39) {
            return 88;
        }
        return 89;
    case ItemKindType::GOLD:
        if (subtype <= 3) {
            return 92;
        }
        if (subtype <= 6) {
            return 91;
        }
        if (subtype >= 9 && subtype <= 11) {
            return 76;
        }
        return 95;
    default:
        return -1;
    }
}

DisplaySymbol item_tile_symbol(int icon, int color)
{
    const auto payload = (color & 15) * ITEM_ICONS + icon;
    return { static_cast<uint8_t>(0x80 + ITEM_ROW + payload / 128), static_cast<char>(0x80 + payload % 128) };
}

tl::optional<int> decode_item_tile(DisplaySymbol symbol)
{
    const auto row = (symbol.color & 0x7f) - ITEM_ROW;
    if (symbol.is_ascii_graphics() || !(symbol.character & 0x80) || row < 0 || row > (ITEM_TILES - 1) / 128) {
        return tl::nullopt;
    }
    return row * 128 + (symbol.character & 0x7f);
}

DisplaySymbol item_symbol(const ItemEntity &item, DisplaySymbol fallback)
{
    if (!is_enabled() || !fallback.is_ascii_graphics()) {
        return fallback;
    }
    if (!item.is_valid()) {
        return item_symbol_fallback(fallback);
    }
    // 未鑑定品は割り当てられた見た目だけを使う。通常の名前表示で判明するまでは、
    // 固定アーティファクトの専用画像を選ばない。
    const auto appearance = item.get_baseitem_record().get_appearance_id();
    if (catalog_available) {
        const auto known = item.is_known();
        const auto artifact_id = known && item.is_fixed_artifact() ? static_cast<int>(item.fa_id) : 0;
        const auto index = catalog_visible_index(item.bi_id, appearance, artifact_id, known);
        if (index >= 0) {
            return catalog_tile_symbol(index, fallback.color);
        }
    }
    const auto icon = item_tile_index(item.bi_key.tval(), item.bi_key.sval().value_or(0), appearance);
    return icon < 0 ? item_symbol_fallback(fallback) : item_tile_symbol(icon, fallback.color);
}
}
