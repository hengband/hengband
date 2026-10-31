#include "game-option/special-options.h"
#include "io/cursor.h"
#include "io/screen-util.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/system-variables.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "term/gameterm.h"
#include "term/tile-layout.h"
#include "term/z-term.h"
#include "test/scoped-restore.h"
#include "test/system/monrace-list-test-access.h"
#include "util/finalizer.h"
#include "view/actor-readability.h"
#include "view/display-map.h"
#include "view/map-tile-layout.h"
#include "view/original-item-catalog.h"
#include "view/original-monster-catalog.h"
#include "view/original-player-tiles.h"
#include "view/original-symbol-tiles.h"
#include "view/original-tiles.h"
#include "window/main-window-util.h"
#include "world/world.h"
#include <array>
#include <cmath>
#include <doctest/doctest.h>
#include <memory>
#include <set>
#include <string>

TEST_CASE("Original tiles zoom the main map without changing text or subwindows")
{
    const auto restore = test::scoped_restore(game_term, angband_terms, use_bigtile, use_graphics, ANGBAND_GRAF, original_tiles::double_map_size, panel_row_min, panel_col_min, panel_row_prt);
    term_type primary;
    term_type secondary;
    term_init(&primary, 80, 24, 16);
    term_init(&secondary, 80, 24, 16);
    game_term = &primary;
    angband_terms[0] = &primary;
    use_bigtile = use_graphics = true;
    ANGBAND_GRAF = "original32";
    panel_row_min = 10;
    panel_row_prt = 9;
    panel_col_min = 20;
    original_tiles::double_map_size = false;
    CHECK(get_screen_size() == std::pair<int, int>{ 33, 22 });
    original_tiles::double_map_size = true;
    CHECK(get_screen_size() == std::pair<int, int>{ 16, 11 });
    CHECK(term_get_size().first == 80);
    CHECK(term_get_size().second == 24);
    CHECK(panel_col_of(21) == 17);
    CHECK(panel_row_of(11) == 3);
    move_cursor_relative(20, 35);
    CHECK(primary.scr->cx == 73);
    CHECK(primary.scr->cy == 21);
    term_putstr(0, 0, 3, 1, "ABC");
    CHECK(primary.scr->c[0][0] == 'A');
    CHECK(primary.scr->c[0][1] == 'B');
    CHECK(primary.scr->c[0][2] == 'C');
    game_term = &secondary;
    CHECK(original_tiles::map_tile_scale() == 1);
    CHECK(get_screen_size() == std::pair<int, int>{ 33, 22 });
    game_term = &primary;
    ANGBAND_GRAF = "ne2";
    CHECK(original_tiles::map_tile_scale() == 1);
}

TEST_CASE("Original tiles zoom blocks repaint and erase safely from every covered cell")
{
    const auto restore = test::scoped_restore(game_term, angband_terms, use_bigtile, use_graphics, ANGBAND_GRAF, original_tiles::double_map_size);
    term_type terminal;
    term_init(&terminal, 16, 6, 16);
    game_term = &terminal;
    angband_terms[0] = &terminal;
    use_bigtile = use_graphics = true;
    ANGBAND_GRAF = "original32";
    original_tiles::double_map_size = true;
    terminal.active_flag = terminal.mapped_flag = true;
    terminal.higher_pict = terminal.soft_cursor = true;
    struct Probe {
        int calls = 0;
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
        bool old_buffer = false;
    } probe;
    terminal.data = &probe;
    terminal.pict_hook = [](TERM_LEN x, TERM_LEN y, int n, const TERM_COLOR *a, concptr c, const TERM_COLOR *, concptr) -> errr {
        auto &p = *static_cast<Probe *>(game_term->data);
        for (auto i = 0; i < n; ++i) {
            if (!(a[i] & 0x80) || !(c[i] & 0x80) || term_is_tile_continuation({ a[i], c[i] })) {
                continue;
            }
            const auto block = term_tile_block_at(term_pict_buffer(), x + i, y);
            ++p.calls;
            p.x = x + i;
            p.y = y;
            p.width = block.width;
            p.height = block.height;
            p.old_buffer = &term_pict_buffer() == game_term->old.get();
        }
        return 0;
    };
    term_fresh();
    const DisplaySymbolPair tile{ original_tiles::player_tile_symbol(0, 1), original_tiles::tile_symbol(0, 0) };
    term_putstr(3, 2, 1, 1, "L");
    term_fresh();
    auto errors = 0;
    for (const auto always : { false, true }) {
        terminal.always_pict = always;
        for (auto family = 0; family < 5; ++family) {
            const auto count = family == 4 ? original_tiles::MONSTER_CATALOG_ICONS : family == 3 ? original_tiles::CATALOG_ICONS
                                                                                 : family == 2   ? original_tiles::PLAYER_ICONS
                                                                                                 : 96;
            for (auto color = 0; color < 16; ++color) {
                for (auto icon = 0; icon < count; ++icon) {
                    const auto symbol = family == 4 ? original_tiles::monster_catalog_tile_symbol(icon, color) : family == 3 ? original_tiles::catalog_tile_symbol(icon, color)
                                                                                                             : family == 2   ? original_tiles::player_tile_symbol(icon, color)
                                                                                                             : family == 1   ? original_tiles::monster_tile_symbol(icon, color)
                                                                                                                             : original_tiles::item_tile_symbol(icon, color);
                    original_tiles::queue_map_tile(4, 2, { symbol, tile.symbol_background });
                    probe.calls = 0;
                    term_fresh();
                    errors += probe.calls != 1 || probe.x != 4 || probe.y != 2 || probe.width != 4 || probe.height != 2;
                }
            }
        }
        for (auto dy = 0; dy < 2; ++dy) {
            for (auto dx = 0; dx < 4; ++dx) {
                probe.calls = 0;
                term_redraw_section(4 + dx, 2 + dy, 4 + dx, 2 + dy);
                errors += probe.calls != 1 || probe.x != 4 || probe.y != 2 || probe.width != 4 || probe.height != 2;
            }
        }
    }
    CHECK(errors == 0);
    term_gotoxy(4, 2);
    term_set_cursor(true);
    term_fresh();
    probe.calls = 0;
    term_gotoxy(10, 0);
    term_fresh();
    CHECK(probe.calls == 1);
    CHECK(probe.old_buffer);
    CHECK(probe.width == 4);
    CHECK(probe.height == 2);
    term_set_cursor(false);
    term_putstr(6, 3, 1, 1, "X");
    CHECK_FALSE(term_tile_block_at(*terminal.scr, 4, 2).zoomed());
    CHECK(terminal.scr->c[3][6] == 'X');
    CHECK(terminal.scr->c[2][3] == 'L');
    original_tiles::queue_map_tile(4, 2, tile);
    term_erase(7, 3, 1);
    CHECK_FALSE(term_tile_block_at(*terminal.scr, 4, 2).zoomed());
    CHECK(terminal.scr->c[2][4] == ' ');
    CHECK(terminal.scr->c[2][3] == 'L');
    const auto before = terminal.scr->clone();
    CHECK_FALSE(original_tiles::queue_map_tile(13, 2, tile));
    CHECK_FALSE(original_tiles::queue_map_tile(4, 5, tile));
    CHECK(terminal.scr->a == before->a);
    CHECK(terminal.scr->c == before->c);
    original_tiles::queue_map_tile(4, 2, tile);
    original_tiles::double_map_size = false;
    original_tiles::queue_map_tile(4, 2, tile);
    CHECK_FALSE(term_tile_block_at(*terminal.scr, 4, 2).zoomed());
    CHECK(terminal.scr->c[3][4] == ' ');
}

TEST_CASE("Original tiles keep pending text separate across the lower zoom row")
{
    const auto restore = test::scoped_restore(game_term, angband_terms, use_bigtile, use_graphics, ANGBAND_GRAF, original_tiles::double_map_size);
    for (auto mode = 0; mode < 3; ++mode) {
        term_type terminal;
        term_init(&terminal, 16, 4, 16);
        game_term = &terminal;
        angband_terms[0] = &terminal;
        use_bigtile = use_graphics = true;
        ANGBAND_GRAF = "original32";
        original_tiles::double_map_size = true;
        terminal.active_flag = terminal.mapped_flag = true;
        terminal.higher_pict = terminal.soft_cursor = true;
        terminal.always_text = mode == 1;
        struct Probe {
            std::array<std::array<char, 16>, 4> pixels{};
        } probe;
        terminal.data = &probe;
        terminal.text_hook = [](TERM_LEN x, TERM_LEN y, int n, TERM_COLOR, concptr text) -> errr {
            auto &pixels = static_cast<Probe *>(game_term->data)->pixels;
            for (auto i = 0; i < n; ++i) {
                pixels[y][x + i] = text[i];
            }
            return 0;
        };
        terminal.wipe_hook = [](TERM_LEN x, TERM_LEN y, int n) -> errr {
            auto &pixels = static_cast<Probe *>(game_term->data)->pixels;
            for (auto i = 0; i < n; ++i) {
                pixels[y][x + i] = ' ';
            }
            return 0;
        };
        terminal.pict_hook = [](TERM_LEN x, TERM_LEN y, int n, const TERM_COLOR *a, concptr c, const TERM_COLOR *, concptr) -> errr {
            auto &pixels = static_cast<Probe *>(game_term->data)->pixels;
            for (auto i = 0; i < n; ++i) {
                if (term_is_tile_continuation({ a[i], c[i] })) {
                    continue;
                }
                const auto block = term_tile_block_at(term_pict_buffer(), x + i, y);
                for (auto dy = 0; dy < block.height; ++dy) {
                    for (auto dx = 0; dx < block.width; ++dx) {
                        pixels[y + dy][x + i + dx] = '@';
                    }
                }
            }
            return 0;
        };
        for (auto y = 0; y < 4; ++y) {
            term_putstr(0, y, 16, 1, "XXXXXXXXXXXXXXXX");
        }
        term_fresh();
        const DisplaySymbolPair blank{ { 0, ' ' }, {} };
        const auto symbol = original_tiles::tile_symbol(0, 0);
        CHECK(original_tiles::queue_map_tile(0, 1, blank));
        CHECK(original_tiles::queue_map_tile(4, 1, { symbol, symbol }));
        CHECK(original_tiles::queue_map_tile(8, 1, blank));
        if (mode == 2) {
            term_putstr(0, 2, 4, 2, "ABCD");
            term_putstr(8, 2, 4, 2, "EFGH");
        }
        term_fresh();
        INFO(mode);
        CHECK(std::string(probe.pixels[0].begin(), probe.pixels[0].end()) == "XXXXXXXXXXXXXXXX");
        CHECK(std::string(probe.pixels[1].begin(), probe.pixels[1].end()) == "    @@@@    XXXX");
        CHECK(std::string(probe.pixels[2].begin(), probe.pixels[2].end()) == (mode == 2 ? "ABCD@@@@EFGHXXXX" : "    @@@@    XXXX"));
        CHECK(std::string(probe.pixels[3].begin(), probe.pixels[3].end()) == "XXXXXXXXXXXXXXXX");
    }
}

TEST_CASE("Original tiles distinguish permanent glass walls from permanent rock")
{
    const auto restore = test::scoped_restore(use_graphics, ANGBAND_GRAF);
    use_graphics = true;
    ANGBAND_GRAF = "original32";
    auto player = std::make_unique<PlayerType>();
    TerrainType terrain;
    for (auto lighting = 0; lighting < 3; ++lighting) {
        for (auto x = 0; x < 64; ++x) {
            const Pos2D pos{ 2, static_cast<POSITION>(x) };
            terrain.tag = "GLASS_WALL";
            const auto glass = original_tiles::terrain_symbol(*player, pos, terrain, lighting, { 1, '#' });
            terrain.tag = "PERMANENT_GLASS_WALL";
            const auto permanent_glass = original_tiles::terrain_symbol(*player, pos, terrain, lighting, { 1, '#' });
            CHECK(permanent_glass.color == glass.color);
            CHECK(permanent_glass.character == glass.character);
            terrain.tag = "PERMANENT_WALL";
            const auto rock = original_tiles::terrain_symbol(*player, pos, terrain, lighting, { 1, '#' });
            CHECK((permanent_glass.color != rock.color || permanent_glass.character != rock.character));
        }
    }
}

TEST_CASE("Original tiles keep stair directions and shaft palettes in separate slots")
{
    const auto restore = test::scoped_restore(use_graphics, ANGBAND_GRAF);
    use_graphics = true;
    ANGBAND_GRAF = "original32";
    auto player = std::make_unique<PlayerType>();
    TerrainType terrain;
    auto errors = 0;
    for (auto lighting = 0; lighting < 3; ++lighting) {
        for (auto x = 0; x < 64; ++x) {
            const auto matches = [&](std::string_view tag, int first, int count) {
                terrain.tag = tag;
                const auto symbol = original_tiles::terrain_symbol(*player, { 2, static_cast<POSITION>(x) }, terrain, lighting, { 1, '<' });
                const auto slot = ((symbol.color & 0x7f) - lighting * original_tiles::LIGHTING_ROWS) * original_tiles::COLUMNS + (symbol.character & 0x7f);
                return slot >= first && slot < first + count;
            };
            errors += !matches("UP_STAIR", 24, 4);
            errors += !matches("DOWN_STAIR", 176, 8);
            errors += !matches("SHAFT_UP", 188, 4);
            errors += !matches("SHAFT_DOWN", 184, 4);
            errors += !matches("QUEST_UP", 24, 4);
            errors += !matches("QUEST_DOWN", 176, 8);
        }
    }
    CHECK(errors == 0);
}

TEST_CASE("Original tiles catalog codes fit unused terrain columns without collisions")
{
    std::set<int> payloads;
    auto errors = 0;
    for (auto icon = 0; icon < original_tiles::CATALOG_ICONS; ++icon) {
        for (auto color = 0; color < 16; ++color) {
            const auto symbol = original_tiles::catalog_tile_symbol(icon, color);
            const auto payload = original_tiles::decode_catalog_tile(symbol);
            if (!payload || *payload < 0 || *payload >= original_tiles::CATALOG_FRAMES || symbol.color >= 0xf0 || (symbol.character & 0x7f) < 16 || term_is_tile_continuation(symbol)) {
                ++errors;
            } else {
                payloads.insert(*payload);
            }
        }
    }
    CHECK(errors == 0);
    CHECK(payloads.size() == original_tiles::CATALOG_FRAMES);
    for (auto lighting = 0; lighting < 3; ++lighting) {
        for (auto tile = 0; tile < 288; ++tile) {
            errors += original_tiles::decode_catalog_tile(original_tiles::tile_symbol(tile, lighting)).has_value();
        }
    }
    CHECK(errors == 0);
    CHECK_FALSE(original_tiles::decode_catalog_tile(original_tiles::item_tile_symbol(95, 15)).has_value());
    CHECK_FALSE(original_tiles::decode_catalog_tile(original_tiles::monster_tile_symbol(95, 15)).has_value());
    CHECK_FALSE(original_tiles::decode_catalog_tile({ 0xf0, static_cast<char>(0xff) }).has_value());
    CHECK_FALSE(original_tiles::decode_catalog_tile({ 0xb5, static_cast<char>(0xff) }).has_value());
    CHECK(original_tiles::catalog_tile_symbol(-1, 1) == DisplaySymbol{});
    CHECK(original_tiles::catalog_tile_symbol(original_tiles::CATALOG_ICONS, 1) == DisplaySymbol{});
}

TEST_CASE("Original tiles catalog keeps unknown artifact identities hidden")
{
    const auto base = original_tiles::catalog_base_index(30);
    const auto appearance = original_tiles::catalog_base_index(31);
    const auto artifact = original_tiles::catalog_artifact_index(1);
    REQUIRE(base >= 0);
    REQUIRE(appearance >= 0);
    REQUIRE(artifact >= 0);
    CHECK(original_tiles::catalog_visible_index(30, 31, 1, false) == appearance);
    CHECK(original_tiles::catalog_visible_index(30, 31, 2, false) == appearance);
    CHECK(original_tiles::catalog_visible_index(30, 31, 0, true) == appearance);
    CHECK(original_tiles::catalog_visible_index(30, 31, 1, true) == artifact);
    CHECK(original_tiles::catalog_visible_index(30, 0, 1, false) == base);
    CHECK(original_tiles::catalog_visible_index(30, 0, 1, true) == artifact);
    CHECK(original_tiles::catalog_base_index(-1) == -1);
    CHECK(original_tiles::catalog_artifact_index(0) == -1);
}

TEST_CASE("Original monster catalog covers every species without colliding with items or terrain")
{
    std::set<int> payloads;
    std::set<int> codes;
    auto errors = 0;
    for (auto icon = 0; icon < original_tiles::MONSTER_CATALOG_ICONS; ++icon) {
        errors += original_tiles::monster_catalog_index(icon) != icon;
        for (auto color = 0; color < 16; ++color) {
            const auto symbol = original_tiles::monster_catalog_tile_symbol(icon, color);
            const auto payload = original_tiles::decode_monster_catalog_tile(symbol);
            if (!payload || *payload < 0 || *payload >= original_tiles::MONSTER_CATALOG_FRAMES || symbol.color >= 0xf0 || term_is_tile_continuation(symbol)) {
                ++errors;
                continue;
            }
            payloads.insert(*payload);
            codes.insert((symbol.color << 8) | static_cast<unsigned char>(symbol.character));
        }
    }
    CHECK(errors == 0);
    CHECK(payloads.size() == original_tiles::MONSTER_CATALOG_FRAMES);
    CHECK(codes.size() == payloads.size());
    for (auto icon = 0; icon < original_tiles::CATALOG_ICONS; ++icon) {
        for (auto color = 0; color < 16; ++color) {
            errors += original_tiles::decode_monster_catalog_tile(original_tiles::catalog_tile_symbol(icon, color)).has_value();
        }
    }
    for (auto lighting = 0; lighting < 3; ++lighting) {
        for (auto tile = 0; tile < 288; ++tile) {
            errors += original_tiles::decode_monster_catalog_tile(original_tiles::tile_symbol(tile, lighting)).has_value();
        }
        for (auto tile = 0; tile < original_tiles::SUPPLEMENT_TILES; ++tile) {
            const DisplaySymbol symbol{ static_cast<uint8_t>(0x80 + original_tiles::SUPPLEMENT_ROW + (lighting * 64 + tile) / 128), static_cast<char>(0x80 + (lighting * 64 + tile) % 128) };
            errors += original_tiles::decode_monster_catalog_tile(symbol).has_value();
        }
    }
    for (auto color = 0; color < 16; ++color) {
        for (auto icon = 0; icon < original_tiles::PLAYER_ICONS; ++icon) {
            errors += original_tiles::decode_monster_catalog_tile(original_tiles::player_tile_symbol(icon, color)).has_value();
        }
        for (auto ch = 32; ch < 127; ++ch) {
            errors += original_tiles::decode_monster_catalog_tile({ static_cast<uint8_t>(0x80 + original_tiles::GLYPH_ROW + color), static_cast<char>(0x80 + ch) }).has_value();
        }
    }
    CHECK(errors == 0);
    CHECK_FALSE(original_tiles::decode_monster_catalog_tile({ 0xf0, static_cast<char>(0xff) }));
    CHECK(original_tiles::monster_catalog_index(-1) == -1);
    CHECK(original_tiles::monster_catalog_index(1419) == -1);
}

TEST_CASE("Original tiles improve actor silhouettes without touching neighboring cells")
{
    original_tiles::ActorTile empty;
    empty.fill(original_tiles::ACTOR_TRANSPARENT);
    CHECK(original_tiles::readable_actor_tile(empty, false) == empty);
    auto actor = empty;
    for (auto y = 3; y < 29; ++y) {
        for (auto x = 3; x < 29; ++x) {
            actor[y * 32 + x] = 0x00204020;
        }
    }
    actor[10 * 32 + 10] = 0xffffff;
    actor[10 * 32 + 11] = 0;
    const auto player = original_tiles::readable_actor_tile(actor, true);
    const auto monster = original_tiles::readable_actor_tile(actor, false);
    CHECK(player == original_tiles::readable_actor_tile(actor, true));
    CHECK(player != actor);
    CHECK(player != monster);
    CHECK(std::count(player.begin(), player.end(), 0x00f0d890U) > 0);
    CHECK(std::count(monster.begin(), monster.end(), 0x009cacb8U) > 0);
    CHECK(std::count(monster.begin(), monster.end(), original_tiles::ACTOR_TRANSPARENT) > 0);
    CHECK(std::count(player.begin(), player.end(), 0x00141820U) > 0);

    std::vector<uint32_t> pixels(512 * 64, original_tiles::ACTOR_TRANSPARENT);
    for (auto y = 0; y < 32; ++y) {
        for (auto x = 0; x < 32; ++x) {
            pixels[y * 512 + x] = actor[y * 32 + x];
            pixels[y * 512 + 32 + x] = 0x123456;
        }
    }
    const auto before = pixels;
    REQUIRE(original_tiles::make_actor_region_readable(pixels, 512, 64, 0, 1, false));
    auto changed = 0;
    auto unrelated = 0;
    for (auto y = 0; y < 64; ++y) {
        for (auto x = 0; x < 512; ++x) {
            if (pixels[y * 512 + x] != before[y * 512 + x]) {
                ++changed;
                unrelated += x >= 32 || y >= 32;
            }
        }
    }
    CHECK(changed > 0);
    CHECK(unrelated == 0);
    const auto valid = pixels;
    CHECK_FALSE(original_tiles::make_actor_region_readable(pixels, 511, 64, 0, 1, false));
    CHECK_FALSE(original_tiles::make_actor_region_readable(pixels, 512, 64, -1, 1, false));
    CHECK_FALSE(original_tiles::make_actor_region_readable(pixels, 512, 64, 0, 33, false));
    CHECK_FALSE(original_tiles::make_actor_region_readable(pixels, 512, 64, 0, 2147483647, false));
    CHECK(pixels == valid);
}

TEST_CASE("Original tiles restrict player readability to player and legacy avatar frames")
{
    original_tiles::ActorTile source;
    source.fill(original_tiles::ACTOR_TRANSPARENT);
    for (auto y = 3; y < 29; ++y) {
        for (auto x = 4; x < 28; ++x) {
            source[y * 32 + x] = 0x004080c0;
        }
    }
    const auto monster = original_tiles::readable_actor_tile(source, false);
    const auto player = original_tiles::readable_actor_tile(source, true);
    REQUIRE(monster != player);
    auto monster_errors = 0;
    auto legacy_errors = 0;
    auto player_errors = 0;
    auto unrelated_errors = 0;
    for (const auto family : { 0, 1, 2 }) {
        const auto count = family == 0 ? original_tiles::MONSTER_CATALOG_FRAMES : family == 1 ? original_tiles::MONSTER_TILES
                                                                                              : original_tiles::PLAYER_TILES;
        constexpr auto base_row = 1;
        const auto height = (base_row + (count + 15) / 16) * 32;
        std::vector<uint32_t> pixels(512 * height, original_tiles::ACTOR_TRANSPARENT);
        pixels[0] = 0x123456;
        for (auto frame = 0; frame < count; ++frame) {
            for (auto y = 0; y < 32; ++y) {
                for (auto x = 0; x < 32; ++x) {
                    pixels[((base_row + frame / 16) * 32 + y) * 512 + frame % 16 * 32 + x] = source[y * 32 + x];
                }
            }
        }
        REQUIRE(original_tiles::make_actor_region_readable(pixels, 512, height, base_row, count, family == 2, family == 1));
        for (auto frame = 0; frame < count; ++frame) {
            const auto &expected = family == 2 || (family == 1 && frame % original_tiles::MONSTER_ICONS == 76) ? player : monster;
            for (auto y = 0; y < 32; ++y) {
                for (auto x = 0; x < 32; ++x) {
                    const auto mismatch = pixels[((base_row + frame / 16) * 32 + y) * 512 + frame % 16 * 32 + x] != expected[y * 32 + x];
                    if (family == 0) {
                        monster_errors += mismatch;
                    } else if (family == 1) {
                        legacy_errors += mismatch;
                    } else {
                        player_errors += mismatch;
                    }
                }
            }
        }
        unrelated_errors += pixels[0] != 0x123456;
        for (auto offset = count * 32; offset < ((count + 15) / 16) * 512; ++offset) {
            const auto y = (base_row + offset / 512) * 32;
            const auto x = offset % 512;
            for (auto dy = 0; dy < 32; ++dy) {
                unrelated_errors += pixels[(y + dy) * 512 + x] != original_tiles::ACTOR_TRANSPARENT;
            }
        }
    }
    CHECK(monster_errors == 0);
    CHECK(legacy_errors == 0);
    CHECK(player_errors == 0);
    CHECK(unrelated_errors == 0);
}

TEST_CASE("Original tiles survive incremental terminal refresh without continuation collisions")
{
    const auto restore = test::scoped_restore(game_term, use_bigtile);
    term_type terminal;
    term_init(&terminal, 12, 3, 16);
    game_term = &terminal;
    terminal.active_flag = true;
    terminal.mapped_flag = true;
    terminal.higher_pict = true;
    terminal.soft_cursor = true;
    struct Probe {
        int calls = 0;
        TERM_LEN x = -1;
        DisplaySymbolPair symbols{ {}, {} };
    } probe;
    terminal.data = &probe;
    terminal.pict_hook = [](TERM_LEN x, TERM_LEN, int n, const TERM_COLOR *a, concptr c, const TERM_COLOR *ta, concptr tc) -> errr {
        auto &record = *static_cast<Probe *>(game_term->data);
        for (auto i = 0; i < n; ++i) {
            if (original_tiles::is_bigtile_continuation({ a[i], c[i] }) || !(a[i] & 0x80) || !(c[i] & 0x80)) {
                continue;
            }
            ++record.calls;
            record.x = x + i;
            record.symbols = { { a[i], c[i] }, { ta[i], tc[i] } };
        }
        return 0;
    };
    term_fresh();
    const auto background = original_tiles::tile_symbol(0, 0);
    auto errors = 0;
    auto collisions = 0;
    auto repaint_errors = 0;
    for (const auto mode : { 0, 1, 2, 3 }) {
        const auto bigtile = (mode & 1) != 0;
        terminal.always_pict = (mode & 2) != 0;
        use_bigtile = bigtile;
        for (auto color = 0; color < 16; ++color) {
            for (auto icon = 0; icon < original_tiles::MONSTER_CATALOG_ICONS; ++icon) {
                for (const auto family : { 0, 1, 2, 3, 4 }) {
                    if ((family < 3 && icon >= 96) || (family == 3 && icon >= original_tiles::CATALOG_ICONS)) {
                        continue;
                    }
                    if (family == 2 && icon >= original_tiles::PLAYER_ICONS) {
                        continue;
                    }
                    const auto symbol = family == 4 ? original_tiles::monster_catalog_tile_symbol(icon, color) : family == 3 ? original_tiles::catalog_tile_symbol(icon, color)
                                                                                                             : family == 2   ? original_tiles::player_tile_symbol(icon, color)
                                                                                                             : family == 1   ? original_tiles::monster_tile_symbol(icon, color)
                                                                                                                             : original_tiles::item_tile_symbol(icon, color);
                    collisions += (symbol.color & 0xf0) == 0xf0;
                    probe.calls = 0;
                    term_queue_bigchar(4, 1, { symbol, background });
                    term_fresh();
                    errors += probe.calls != 1 || probe.x != 4 || probe.symbols != DisplaySymbolPair{ symbol, background };
                    probe.calls = 0;
                    term_fresh();
                    errors += probe.calls != 0;
                }
            }
        }
        const DisplaySymbolPair expected{ original_tiles::monster_catalog_tile_symbol(original_tiles::MONSTER_CATALOG_ICONS - 1, 15), background };
        probe.calls = 0;
        term_redraw();
        repaint_errors += probe.calls != 1 || probe.x != 4 || probe.symbols != expected;
        if (bigtile) {
            probe.calls = 0;
            term_redraw_section(5, 1, 5, 1);
            repaint_errors += probe.calls != 1 || probe.x != 4 || probe.symbols != expected;
        }
    }
    CHECK(collisions == 0);
    CHECK(errors == 0);
    CHECK(repaint_errors == 0);
    CHECK(original_tiles::is_bigtile_continuation({ 0xf0, static_cast<char>(0xff) }));

#ifdef JP
    // Tile rows also contain 0x10/0x20, but these are not Shift-JIS halves.
    use_bigtile = false;
    term_queue_char(3, 1, { { 1, 'L' }, {} });
    term_fresh();
    term_queue_char(4, 1, { original_tiles::tile_symbol(0, 2), background });
    CHECK(terminal.x1[1] == 4);
    term_fresh();
    term_putstr(4, 1, 1, 1, "X");
    CHECK(terminal.scr->c[1][3] == 'L');
    CHECK(terminal.scr->a[1][3] == 1);
    term_putstr(4, 1, 2, 1, "あ");
    term_putstr(5, 1, 1, 1, "X");
    CHECK(terminal.scr->c[1][4] == ' ');
#endif
}

TEST_CASE("Original tiles normalize diagonals to 47 connected shapes")
{
    std::set<int> shapes;
    for (auto mask = 0; mask < 256; ++mask) {
        const auto normalized = original_tiles::normalize_pit_mask(mask);
        CHECK(original_tiles::normalize_pit_mask(normalized) == normalized);
        shapes.insert(normalized);
    }
    CHECK(shapes.size() == 47);
    CHECK(original_tiles::normalize_pit_mask(16) == 0);
    CHECK(original_tiles::normalize_pit_mask(1 | 2 | 16) == (1 | 2 | 16));
    CHECK(original_tiles::normalize_pit_mask(255) == 255);
}

TEST_CASE("Original tiles select all player classes and sexes without reserved attributes")
{
    std::set<int> appearances;
    std::set<int> images;
    auto errors = 0;
    for (auto job = 0; job < PLAYER_CLASS_TYPE_MAX; ++job) {
        for (auto sex = 0; sex < MAX_SEXES; ++sex) {
            const auto index = original_tiles::player_tile_index(static_cast<PlayerClassType>(job), static_cast<player_sex>(sex));
            errors += index != job * MAX_SEXES + sex;
            appearances.insert(index);
            for (auto color = 0; color < 16; ++color) {
                const auto symbol = original_tiles::player_tile_symbol(index, color);
                const auto decoded = original_tiles::decode_player_tile(symbol);
                errors += !decoded || *decoded != color * original_tiles::PLAYER_ICONS + index || (symbol.color & 0xf0) == 0xf0;
                errors += original_tiles::decode_monster_tile(symbol).has_value() || original_tiles::decode_item_tile(symbol).has_value();
                if (decoded) {
                    images.insert(*decoded);
                }
            }
        }
    }
    CHECK(errors == 0);
    CHECK(appearances.size() == 58);
    CHECK(images.size() == 928);
    CHECK(original_tiles::player_tile_index(PlayerClassType::MAX, SEX_FEMALE) == -1);
    CHECK(original_tiles::player_tile_index(PlayerClassType::WARRIOR, MAX_SEXES) == -1);
    CHECK_FALSE(original_tiles::decode_player_tile(original_tiles::monster_tile_symbol(0, 0)));
    CHECK_FALSE(original_tiles::decode_player_tile({ 1, '@' }));
    CHECK_FALSE(original_tiles::decode_player_tile({ static_cast<uint8_t>(0x80 + original_tiles::PLAYER_ROW + 7), static_cast<char>(0xff) }));
}

TEST_CASE("Original tiles cover public monster appearances and preserve animated silhouettes")
{
    constexpr std::pair<int, int> definitions[] = {
#include "test/view/original-monster-coverage.inc"
    };
    auto mapped = 0;
    auto errors = 0;
    for (const auto &[character, color] : definitions) {
        const auto icon = original_tiles::monster_tile_index(static_cast<char>(character), color);
        ++mapped;
        errors += icon < 0 || icon >= original_tiles::MONSTER_ICONS;
    }
    CHECK(std::size(definitions) == 1419);
    CHECK(mapped == 1419);
    CHECK(errors == 0);
    std::set<int> images;
    auto roundtrip_errors = 0;
    for (auto color = 0; color < original_tiles::MONSTER_COLORS; ++color) {
        for (auto icon = 0; icon < original_tiles::MONSTER_ICONS; ++icon) {
            const auto symbol = original_tiles::monster_tile_symbol(icon, color);
            const auto decoded = original_tiles::decode_monster_tile(symbol);
            if (!decoded || *decoded != color * original_tiles::MONSTER_ICONS + icon || original_tiles::decode_item_tile(symbol)) {
                ++roundtrip_errors;
            } else {
                images.insert(*decoded);
            }
        }
    }
    CHECK(roundtrip_errors == 0);
    CHECK(images.size() == 1536);
    CHECK_FALSE(original_tiles::decode_monster_tile({ 1, 'D' }));
    CHECK_FALSE(original_tiles::decode_monster_tile(original_tiles::item_tile_symbol(0, 15)));
    const auto graphics_restore = test::scoped_restore(use_graphics, ANGBAND_GRAF);
    use_graphics = true;
    ANGBAND_GRAF = "original32";
    auto animated_errors = 0;
    for (const auto character : std::string_view("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz")) {
        const auto silhouette = original_tiles::monster_tile_index(character, 5);
        for (auto color = 0; color < 16; ++color) {
            const auto symbol = original_tiles::monster_symbol({ static_cast<uint8_t>(color), character }, 5);
            const auto decoded = original_tiles::decode_monster_tile(symbol);
            animated_errors += !decoded || *decoded % 96 != silhouette || *decoded / 96 != color;
        }
    }
    CHECK(animated_errors == 0);
    const DisplaySymbol mimic{ 4, '!' };
    CHECK(original_tiles::decode_monster_tile(original_tiles::monster_symbol(mimic)).has_value());
    ANGBAND_GRAF = "ne2";
    const DisplaySymbol original{ 4, 'D' };
    CHECK(original_tiles::monster_symbol(original) == original);
}

TEST_CASE("Original tiles give every defined item and monster symbol an image")
{
    struct SymbolCase {
        bool monster;
        int character;
        int color;
        int icon;
    };
    constexpr SymbolCase symbols[] = {
#include "test/view/original-symbol-coverage.inc"
    };
    const auto graphics_restore = test::scoped_restore(use_graphics, ANGBAND_GRAF);
    use_graphics = true;
    ANGBAND_GRAF = "original32";
    auto errors = 0;
    std::set<int> items;
    std::set<int> monsters;
    for (const auto &entry : symbols) {
        const DisplaySymbol original{ static_cast<uint8_t>(entry.color), static_cast<char>(entry.character) };
        if (entry.monster) {
            monsters.insert(entry.character);
            errors += original_tiles::monster_tile_index(original.character, original.color) != entry.icon;
            const auto decoded = original_tiles::decode_monster_tile(original_tiles::monster_symbol(original));
            errors += !decoded || *decoded % 96 != entry.icon;
        } else {
            items.insert(entry.character);
            errors += original_tiles::item_symbol_tile_index(original.character) != entry.icon;
            const auto decoded = original_tiles::decode_item_tile(original_tiles::item_symbol_fallback(original));
            errors += !decoded || *decoded % 96 != entry.icon;
        }
    }
    CHECK(errors == 0);
    CHECK(items.size() == 21);
    CHECK(monsters.size() == 74);
    CHECK(original_tiles::decode_item_tile(original_tiles::item_symbol(ItemEntity{}, { 1, '&' })).has_value());
    const DisplaySymbol blank{ 1, ' ' };
    CHECK(original_tiles::item_symbol_fallback(blank) == blank);
    CHECK(original_tiles::monster_symbol(blank) == blank);
}

TEST_CASE("Original tiles vary liquid frames deterministically within their eight images")
{
    for (const auto tag : { "SHALLOW_WATER", "DEEP_WATER", "SHALLOW_LAVA", "DEEP_LAVA" }) {
        CHECK(original_tiles::is_animated_liquid(tag));
    }
    for (const auto tag : { "FLOOR", "MAGMA_VEIN", "DARK_PIT", "MAGMA_TREASURE", "RUBBLE_ON_SHALLOW_WATER" }) {
        CHECK_FALSE(original_tiles::is_animated_liquid(tag));
    }
    std::set<int> frames;
    auto errors = 0;
    for (uint32_t frame = 0; frame < 64; ++frame) {
        const auto variant = original_tiles::liquid_variant({ 7, 13 }, frame);
        errors += variant < 0 || variant >= 8 || variant != original_tiles::liquid_variant({ 7, 13 }, frame);
        frames.insert(variant);
    }
    CHECK(errors == 0);
    CHECK(frames.size() == 8);
}

TEST_CASE("Original tiles cover item definitions without revealing flavored item effects")
{
    struct ItemCase {
        int type;
        int subtype;
    };
    constexpr ItemCase definitions[] = {
#include "test/view/original-item-coverage.inc"
    };
    auto mapping_errors = 0;
    for (const auto &definition : definitions) {
        const auto icon = original_tiles::item_tile_index(static_cast<ItemKindType>(definition.type), definition.subtype, 0);
        mapping_errors += definition.type == 0 ? icon != -1 : icon < 0 || icon >= original_tiles::ITEM_ICONS;
    }
    CHECK(std::size(definitions) == 644);
    CHECK(mapping_errors == 0);
    auto flavor_errors = 0;
    for (const auto type : { ItemKindType::POTION, ItemKindType::RING, ItemKindType::AMULET, ItemKindType::SCROLL, ItemKindType::WAND, ItemKindType::ROD, ItemKindType::STAFF, ItemKindType::FOOD }) {
        const auto appearance = original_tiles::item_tile_index(type, 0, 17);
        for (auto effect = 0; effect < 64; ++effect) {
            flavor_errors += original_tiles::item_tile_index(type, effect, 17) != appearance;
        }
    }
    CHECK(flavor_errors == 0);
    auto encoding_errors = 0;
    std::set<int> images;
    for (auto color = 0; color < original_tiles::ITEM_COLORS; ++color) {
        for (auto icon = 0; icon < original_tiles::ITEM_ICONS; ++icon) {
            const auto symbol = original_tiles::item_tile_symbol(icon, color);
            const auto decoded = original_tiles::decode_item_tile(symbol);
            if (!decoded || *decoded != color * original_tiles::ITEM_ICONS + icon) {
                ++encoding_errors;
            } else {
                images.insert(*decoded);
            }
        }
    }
    CHECK(encoding_errors == 0);
    CHECK(images.size() == original_tiles::ITEM_TILES);
    CHECK_FALSE(original_tiles::decode_item_tile({ 1, '!' }));
    CHECK_FALSE(original_tiles::decode_item_tile(original_tiles::tile_symbol(0, 0)));
}

TEST_CASE("Original tiles distribute floor wear smoothly and independently of redraws")
{
    std::array<int, 16> counts{};
    auto out_of_range = 0;
    auto maximum_jump = 0;
    auto repeated_mismatches = 0;
    auto changed_seed_samples = 0;
    for (uint32_t seed = 0; seed < 8; ++seed) {
        for (POSITION y = 0; y < 64; ++y) {
            for (POSITION x = 0; x < 96; ++x) {
                const Pos2D pos{ y, x };
                const auto variant = original_tiles::floor_variant(pos, seed);
                if (variant < 0 || variant >= 16) {
                    ++out_of_range;
                    continue;
                }
                ++counts[variant];
                repeated_mismatches += variant != original_tiles::floor_variant(pos, seed);
                changed_seed_samples += variant != original_tiles::floor_variant(pos, seed + 1);
                for (const auto &offset : { Pos2DVec{ 0, 1 }, Pos2DVec{ 1, 0 } }) {
                    const auto neighbor = original_tiles::floor_variant(pos + offset, seed);
                    maximum_jump = std::max(maximum_jump, std::abs(variant / 4 - neighbor / 4));
                }
            }
        }
    }
    CHECK(out_of_range == 0);
    CHECK(repeated_mismatches == 0);
    CHECK(changed_seed_samples > 0);
    CHECK(maximum_jump <= 1);
    for (const auto count : counts) {
        CHECK(count > 0);
    }
    const auto quiet = counts[0] + counts[1] + counts[2] + counts[3] + counts[4] + counts[5] + counts[6] + counts[7];
    CHECK(quiet > 8 * 64 * 96 * 7 / 10);
    const auto light = counts[4] + counts[5] + counts[6] + counts[7];
    CHECK(light > 8 * 64 * 96 / 10);
    CHECK(light < 8 * 64 * 96 / 5);
    CHECK(original_tiles::floor_tile_index(0) == 0);
    CHECK(original_tiles::floor_tile_index(7) == 7);
    CHECK(original_tiles::floor_tile_index(8) == 20);
    CHECK(original_tiles::floor_tile_index(11) == 23);
    CHECK(original_tiles::floor_tile_index(12) == 28);
    CHECK(original_tiles::floor_tile_index(15) == 31);
}

TEST_CASE("Original tiles place complete expanded fractures without overlap")
{
    auto centers = 0;
    auto errors = 0;
    std::set<int> patterns;
    std::array<int, 4> patch_counts{};
    for (auto seed = 0U; seed < 8; ++seed) {
        for (POSITION y = 0; y < 96; ++y) {
            for (POSITION x = 0; x < 96; ++x) {
                const Pos2D pos{ y, x };
                const auto tile = original_tiles::expanded_floor_tile(pos, seed);
                if (tile < -1 || tile >= original_tiles::EXPANDED_FLOOR_TILES || tile != original_tiles::expanded_floor_tile(pos, seed)) {
                    ++errors;
                }
                if (tile < 0) {
                    continue;
                }
                auto width = 3;
                auto height = 3;
                auto group_base = 0;
                auto group = 0;
                if (tile >= original_tiles::VERTICAL_FLOOR_BASE) {
                    width = 1;
                    height = 2;
                    group_base = original_tiles::VERTICAL_FLOOR_BASE;
                    group = 3;
                } else if (tile >= original_tiles::HORIZONTAL_FLOOR_BASE) {
                    width = 2;
                    height = 1;
                    group_base = original_tiles::HORIZONTAL_FLOOR_BASE;
                    group = 2;
                } else if (tile >= original_tiles::SQUARE_FLOOR_BASE) {
                    width = height = 2;
                    group_base = original_tiles::SQUARE_FLOOR_BASE;
                    group = 1;
                }
                const auto count = width * height;
                const auto base = group_base + (tile - group_base) / count * count;
                if (tile != base) {
                    continue;
                }
                ++centers;
                ++patch_counts[group];
                patterns.insert(base);
                for (auto dy = 0; dy < height; ++dy) {
                    for (auto dx = 0; dx < width; ++dx) {
                        const Pos2D neighbor{ static_cast<POSITION>(y + dy), static_cast<POSITION>(x + dx) };
                        if (original_tiles::expanded_floor_tile(neighbor, seed) != base + dy * width + dx) {
                            ++errors;
                        }
                    }
                }
            }
        }
    }
    CHECK(centers > 0);
    CHECK(errors == 0);
    CHECK(patterns.size() == 16);
    CHECK(patch_counts[1] > patch_counts[0] * 2);
    CHECK(patch_counts[2] + patch_counts[3] > patch_counts[0] * 2);
    CHECK(patch_counts[2] * 2 > patch_counts[3]);
    CHECK(patch_counts[3] * 2 > patch_counts[2]);
}

TEST_CASE("Original tiles isolate equally weighted medium and heavy damage")
{
    auto heavy = 0;
    auto medium = 0;
    auto suppressed = 0;
    auto adjacency_errors = 0;
    for (auto seed = 0U; seed < 8; ++seed) {
        for (POSITION y = 0; y < 64; ++y) {
            for (POSITION x = 0; x < 96; ++x) {
                const Pos2D pos{ y, x };
                if (original_tiles::expanded_floor_tile(pos, seed) >= 0) {
                    continue;
                }
                const auto variant = original_tiles::standalone_floor_variant(pos, seed);
                suppressed += original_tiles::floor_variant(pos, seed) >= 12 && variant < 8;
                if (variant < 8) {
                    continue;
                }
                if (variant >= 12) {
                    ++heavy;
                } else {
                    ++medium;
                }
                for (auto dy = -1; dy <= 1; ++dy) {
                    for (auto dx = -1; dx <= 1; ++dx) {
                        if (dy == 0 && dx == 0) {
                            continue;
                        }
                        const auto neighbor = pos + Pos2DVec{ static_cast<POSITION>(dy), static_cast<POSITION>(dx) };
                        adjacency_errors += original_tiles::expanded_floor_tile(neighbor, seed) >= 0 || original_tiles::standalone_floor_variant(neighbor, seed) >= 8;
                    }
                }
            }
        }
    }
    CHECK(heavy > 0);
    CHECK(medium > 0);
    CHECK(heavy * 2 > medium);
    CHECK(medium * 2 > heavy);
    CHECK(suppressed > 0);
    CHECK(adjacency_errors == 0);
}

TEST_CASE("Original tiles select prepainted directional cracks with matching masks")
{
    auto mismatches = 0;
    std::set<int> image_slots;
    for (auto lighting = 0; lighting < 3; ++lighting) {
        for (auto grade = 2; grade <= 3; ++grade) {
            for (auto mask = 0; mask < 16; ++mask) {
                auto ports = 0;
                for (auto direction = 0; direction < 4; ++direction) {
                    if ((mask & (1 << direction)) != 0) {
                        ports |= 2 << (direction * 2);
                    }
                }
                for (auto variant = 0; variant < 4; ++variant) {
                    const auto symbol = original_tiles::connected_floor_symbol(ports, grade, lighting, variant);
                    const auto decoded = original_tiles::decode_connected_floor(symbol);
                    if (!decoded || decoded->ports != ports || decoded->grade != grade || decoded->lighting != lighting || decoded->variant != variant || symbol.color >= 0xf0) {
                        ++mismatches;
                        continue;
                    }
                    const auto index = original_tiles::connected_floor_tile_index(*decoded);
                    if (index < 0 || index >= 384) {
                        ++mismatches;
                    }
                    image_slots.insert(index);
                }
            }
        }
    }
    CHECK(mismatches == 0);
    CHECK(image_slots.size() == 384);
    CHECK_FALSE(original_tiles::decode_connected_floor({ 1, '.' }));
    CHECK_FALSE(original_tiles::decode_connected_floor(original_tiles::tile_symbol(20, 0)));
}
TEST_CASE("Original tiles keep unseen pit neighbors private and preserve glyphs")
{
    const auto graphics_restore = test::scoped_restore(use_graphics, ANGBAND_GRAF);
    use_graphics = true;
    ANGBAND_GRAF = "original32";
    auto &terrains = TerrainList::get_instance();
    const auto old_size = terrains.size();
    const auto restore = util::make_finalizer([&terrains, old_size] { terrains.resize(old_size); });
    terrains.resize(old_size + 2);
    const auto floor_id = static_cast<short>(old_size);
    const auto pit_id = static_cast<short>(old_size + 1);
    for (const auto id : { floor_id, pit_id }) {
        auto &terrain = terrains.get_terrain(id);
        terrain.idx = id;
        terrain.mimic = id;
        terrain.tag = id == pit_id ? "DARK_PIT" : "FLOOR";
        terrain.flags.set(TerrainCharacteristics::REMEMBER);
        for (auto &symbol : terrain.symbol_configs) {
            symbol = { 1, id == pit_id ? ' ' : '.' };
        }
    }
    auto floor = std::make_unique<FloorType>();
    floor->height = 5;
    floor->width = 5;
    for (auto y = 0; y < 5; ++y) {
        for (auto x = 0; x < 5; ++x) {
            auto &grid = floor->get_grid({ y, x });
            grid.feat = floor_id;
            grid.info = CAVE_MARK;
        }
    }
    auto player = std::make_unique<PlayerType>();
    player->current_floor_ptr = floor.get();
    player->y = 4;
    player->x = 4;
    const Pos2D pos{ 2, 2 };
    floor->get_grid(pos).feat = pit_id;
    auto &north = floor->get_grid({ 1, 2 });
    north.info = 0;
    const auto pit_symbol = [&] {
        return original_tiles::terrain_symbol(*player, pos, terrains.get_terrain(pit_id), 0, { 1, ' ' });
    };
    const auto hidden_floor = pit_symbol();
    north.feat = pit_id;
    CHECK(pit_symbol() == hidden_floor);
    north.info = CAVE_MARK;
    CHECK(pit_symbol() != hidden_floor);

    const auto terrain_symbol = original_tiles::terrain_symbol(*player, { 3, 3 }, terrains.get_terrain(floor_id), 0, { 1, '.' });
    const auto repeated_symbol = original_tiles::terrain_symbol(*player, { 3, 3 }, terrains.get_terrain(floor_id), 0, { 1, '.' });
    CHECK(terrain_symbol == repeated_symbol);
    auto entrance = terrains.get_terrain(floor_id);
    entrance.tag = "QUEST_ENTER";
    const auto quest_entry = original_tiles::terrain_symbol(*player, pos, entrance, 0, { 11, '>' });
    entrance.tag = "ENTRANCE";
    const auto dungeon_entry = original_tiles::terrain_symbol(*player, pos, entrance, 0, { 14, '>' });
    entrance.tag = "DOWN_STAIR";
    const auto normal_stair = original_tiles::terrain_symbol(*player, pos, entrance, 0, { 1, '>' });
    CHECK(quest_entry != dungeon_entry);
    CHECK(quest_entry != normal_stair);
    CHECK(dungeon_entry != normal_stair);
    CHECK((quest_entry.character & 0x7f) >= original_tiles::QUEST_ENTRANCE_TILE % original_tiles::COLUMNS);
    CHECK((dungeon_entry.character & 0x7f) >= original_tiles::DUNGEON_ENTRANCE_TILE % original_tiles::COLUMNS);
    const auto glyph = original_tiles::encode_foreground(*player, pos, { { 12, 'o' }, terrain_symbol });
    CHECK(glyph.symbol_background == terrain_symbol);
    CHECK((glyph.symbol_foreground.color & 0x7f) == original_tiles::GLYPH_ROW + 12);
    CHECK((glyph.symbol_foreground.character & 0x7f) == 'o');
    const DisplaySymbolPair unknown{ { 0, ' ' }, { 0, ' ' } };
    CHECK(original_tiles::encode_foreground(*player, pos, unknown) == unknown);
    const DisplaySymbolPair unknown_white{ { 1, ' ' }, { 1, ' ' } };
    CHECK(original_tiles::encode_foreground(*player, pos, unknown_white) == unknown_white);
    const auto telepathic = original_tiles::encode_foreground(*player, pos, { { 12, 'o' }, { 1, ' ' } });
    CHECK(telepathic.symbol_background == DisplaySymbol{});
    CHECK((telepathic.symbol_foreground.color & 0x7f) == original_tiles::GLYPH_ROW + 12);
    const auto item_icon = original_tiles::item_tile_symbol(40, 4);
    const auto item_pair = original_tiles::encode_foreground(*player, pos, { item_icon, terrain_symbol });
    CHECK(item_pair.symbol_foreground == item_icon);
    CHECK(item_pair.symbol_background == terrain_symbol);

    {
        test::MonraceListTestAccess monrace_restore;
        auto &monraces = MonraceList::get_instance();
        const auto real_id = static_cast<MonraceId>(1);
        const auto appearance_id = static_cast<MonraceId>(2);
        auto &real = monraces.emplace(real_id);
        auto &apparent = monraces.emplace(appearance_id);
        real.idx = real_id;
        apparent.idx = appearance_id;
        real.symbol_config = { 12, 'D' };
        apparent.symbol_config = { 5, 'p' };
        floor->m_list.resize(2);
        auto &monster = floor->m_list[1];
        monster.r_idx = real_id;
        monster.ap_r_idx = appearance_id;
        monster.ml = true;
        const auto autopick_restore = test::scoped_restore(display_autopick, feat_priority);
        display_autopick.clear();
        const auto underlying = map_info(player.get(), pos);
        floor->get_grid(pos).m_idx = 1;
        const auto visible = map_info(player.get(), pos);
        CHECK(visible.symbol_foreground == original_tiles::monster_symbol(apparent.symbol_config));
        CHECK(visible.symbol_background == underlying.symbol_background);
        real.symbol_config = { 11, 'U' };
        CHECK(map_info(player.get(), pos) == visible);
        {
            const auto catalog_restore = test::scoped_restore(original_tiles::monster_catalog_available);
            original_tiles::monster_catalog_available = true;
            const auto catalog_visible = map_info(player.get(), pos);
            CHECK(catalog_visible.symbol_foreground == original_tiles::monster_catalog_symbol(static_cast<int>(appearance_id), apparent.symbol_config));
            CHECK(catalog_visible.symbol_background == underlying.symbol_background);
            real.symbol_config = { 4, 'D' };
            CHECK(map_info(player.get(), pos) == catalog_visible);
        }
        monster.ml = false;
        CHECK(map_info(player.get(), pos) == underlying);
        monster.ml = true;
        apparent.visual_flags.set(MonsterVisualType::MULTI_COLOR);
        auto &world = AngbandWorld::get_instance();
        const auto turn_restore = test::scoped_restore(world.game_turn);
        std::set<int> colors;
        auto animation_errors = 0;
        for (auto turn = 0; turn < 64; ++turn) {
            ++world.game_turn;
            const auto frame = map_info(player.get(), pos);
            const auto decoded = original_tiles::decode_monster_tile(frame.symbol_foreground);
            if (!decoded) {
                ++animation_errors;
                continue;
            }
            animation_errors += *decoded % 96 != original_tiles::monster_tile_index('p', 5);
            animation_errors += frame != map_info(player.get(), pos);
            colors.insert(*decoded / 96);
        }
        CHECK(animation_errors == 0);
        CHECK(colors.size() > 1);
        apparent.visual_flags.clear();
        apparent.visual_flags.set(MonsterVisualType::CLEAR_COLOR);
        CHECK(original_tiles::decode_monster_tile(map_info(player.get(), pos).symbol_foreground).has_value());
        apparent.visual_flags.set(MonsterVisualType::CLEAR);
        const auto transparent = map_info(player.get(), pos);
        CHECK(transparent.symbol_foreground == transparent.symbol_background);
        floor->get_grid(pos).m_idx = 0;
        auto &avatar = monraces.emplace(static_cast<MonraceId>(0));
        avatar.symbol_config = { 1, '@' };
        const auto location_restore = test::scoped_restore(player->y, player->x);
        player->y = pos.y;
        player->x = pos.x;
        auto class_errors = 0;
        const auto profession_restore = test::scoped_restore(player->pclass, player->psex);
        for (auto job = 0; job < PLAYER_CLASS_TYPE_MAX; ++job) {
            for (auto sex = 0; sex < MAX_SEXES; ++sex) {
                player->pclass = static_cast<PlayerClassType>(job);
                player->psex = static_cast<player_sex>(sex);
                const auto actual = map_info(player.get(), pos);
                class_errors += actual.symbol_foreground != original_tiles::player_tile_symbol(job * MAX_SEXES + sex, 1);
                class_errors += actual.symbol_background != underlying.symbol_background;
            }
        }
        CHECK(class_errors == 0);
    }

    floor->get_grid(pos).feat = floor_id;
    north.feat = floor_id;
    north.info = CAVE_MARK;
    const auto floor_symbol = [&] {
        return original_tiles::terrain_symbol(*player, pos, terrains.get_terrain(floor_id), 0, { 1, '.' });
    };
    north.info = 0;
    const auto hidden_neighbor = floor_symbol();
    north.feat = pit_id;
    CHECK(floor_symbol() == hidden_neighbor);

    constexpr std::pair<std::string_view, bool> definitions[] = {
#include "test/view/original-terrain-coverage.inc"
    };
    auto coverage_errors = 0;
    auto supplement_errors = 0;
    auto candidate = terrains.get_terrain(floor_id);
    for (const auto &[tag, expects_tile] : definitions) {
        candidate.tag = tag;
        for (auto lighting = 0; lighting < 3; ++lighting) {
            const auto character = tag == "NONE" ? ' ' : tag == "UNDETECTED" ? 'x'
                                                                             : '.';
            const DisplaySymbol original{ 1, character };
            const auto symbol = original_tiles::terrain_symbol(*player, pos, candidate, lighting, original);
            coverage_errors += symbol.is_ascii_graphics() == expects_tile;
            const auto index = original_tiles::supplemental_tile_index(tag);
            if (index >= 0) {
                const auto payload = ((symbol.color & 0x7f) - original_tiles::SUPPLEMENT_ROW) * 128 + (symbol.character & 0x7f);
                supplement_errors += payload != lighting * original_tiles::SUPPLEMENT_TILES + index;
            }
        }
    }
    CHECK(std::size(definitions) == 190);
    CHECK(coverage_errors == 0);
    CHECK(supplement_errors == 0);
    CHECK(original_tiles::supplemental_tile_index("NONE") == -1);
    CHECK(original_tiles::supplemental_tile_index("INVIS") == -1);
    CHECK(original_tiles::supplemental_tile_index("MAGMA_HIDDEN") == -1);

    auto &liquid_grid = floor->get_grid(pos);
    liquid_grid.info = CAVE_MARK | CAVE_VIEW | CAVE_LITE;
    // The underlying synthetic grid stays FLOOR, so advancing does not invoke a terminal.
    REQUIRE(original_tiles::is_liquid_animation_visible(*player, pos));
    candidate.tag = "SHALLOW_WATER";
    std::set<int> liquid_images;
    auto redraw_mismatches = 0;
    const auto static_floor = floor_symbol();
    for (auto frame = 0; frame < 32; ++frame) {
        const auto symbol = original_tiles::terrain_symbol(*player, pos, candidate, 0, { 1, '~' });
        redraw_mismatches += symbol != original_tiles::terrain_symbol(*player, pos, candidate, 0, { 1, '~' });
        liquid_images.insert(symbol.character & 0x7f);
        original_tiles::advance_liquid_animation(*player);
    }
    CHECK(redraw_mismatches == 0);
    CHECK(liquid_images.size() > 1);
    CHECK(floor_symbol() == static_floor);
    liquid_grid.info = CAVE_MARK;
    CHECK_FALSE(original_tiles::is_liquid_animation_visible(*player, pos));
    const auto remembered = original_tiles::terrain_symbol(*player, pos, candidate, 0, { 1, '~' });
    original_tiles::advance_liquid_animation(*player);
    CHECK(original_tiles::terrain_symbol(*player, pos, candidate, 0, { 1, '~' }) == remembered);
    liquid_grid.info = CAVE_MARK | CAVE_VIEW;
    CHECK_FALSE(original_tiles::is_liquid_animation_visible(*player, pos));

    ANGBAND_GRAF = "ne2";
    const DisplaySymbol fallback{ 1, '.' };
    CHECK(original_tiles::terrain_symbol(*player, pos, terrains.get_terrain(floor_id), 0, fallback) == fallback);
    const DisplaySymbolPair original{ { 12, 'o' }, fallback };
    CHECK(original_tiles::encode_foreground(*player, pos, original) == original);
}
