#include "cmd-visual/cmd-visuals.h"
#include "cmd-visual/cmd-draw.h"
#include "core/asking-player.h"
#include "core/visuals-reseter.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "game-option/special-options.h"
#include "grid/feature.h"
#include "io/files-util.h"
#include "io/input-key-acceptor.h"
#include "io/read-pref-file.h"
#include "knowledge/knowledge-features.h"
#include "knowledge/knowledge-items.h"
#include "knowledge/knowledge-monsters.h"
#include "knowledge/lighting-level-table.h"
#include "main/sound-of-music.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/item-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/terrain-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "util/angband-files.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include <optional>

/*!
 * @brief キャラクタのビジュアルIDを変更する際の対象指定
 * @param i 指定対象となるキャラクタコード
 * @param initial_visual_id 指定されたビジュアルID
 * @param max ビジュアルIDの最大数
 * @return 新しいビジュアルID
 */
template <typename T>
static std::optional<T> input_new_visual_id(int i, T initial_visual_id, int max)
{
    if (iscntrl(i)) {
        const auto new_visual_id = input_integer("Input new number", 0, max - 1, initial_visual_id);
        if (!new_visual_id) {
            return std::nullopt;
        }

        return static_cast<T>(*new_visual_id);
    }

    if (isupper(i)) {
        return static_cast<T>((initial_visual_id + max - 1) % max);
    }

    return static_cast<T>((initial_visual_id + 1) % max);
}

/*!
 * @brief キャラクタの変更メニュー表示
 * @param choice_msg 選択メッセージ
 */
static void print_visuals_menu(concptr choice_msg)
{
    prt(_("[ 画面表示の設定 ]", "Interact with Visuals"), 1, 0);
    prt(_("(0) ユーザー設定ファイルのロード", "(0) Load a user pref file"), 3, 5);
    prt(_("(1) モンスターの 色/文字 をファイルに書き出す", "(1) Dump monster attr/chars"), 4, 5);
    prt(_("(2) アイテムの   色/文字 をファイルに書き出す", "(2) Dump object attr/chars"), 5, 5);
    prt(_("(3) 地形の       色/文字 をファイルに書き出す", "(3) Dump feature attr/chars"), 6, 5);
    prt(_("(4) モンスターの 色/文字 を変更する (数値操作)", "(4) Change monster attr/chars (numeric operation)"), 7, 5);
    prt(_("(5) アイテムの   色/文字 を変更する (数値操作)", "(5) Change object attr/chars (numeric operation)"), 8, 5);
    prt(_("(6) 地形の       色/文字 を変更する (数値操作)", "(6) Change feature attr/chars (numeric operation)"), 9, 5);
    prt(_("(7) モンスターの 色/文字 を変更する (シンボルエディタ)", "(7) Change monster attr/chars (visual mode)"), 10, 5);
    prt(_("(8) アイテムの   色/文字 を変更する (シンボルエディタ)", "(8) Change object attr/chars (visual mode)"), 11, 5);
    prt(_("(9) 地形の       色/文字 を変更する (シンボルエディタ)", "(9) Change feature attr/chars (visual mode)"), 12, 5);
    prt(_("(R) 画面表示方法の初期化", "(R) Reset visuals"), 13, 5);
    prt(format(_("コマンド: %s", "Command: %s"), choice_msg ? choice_msg : _("", "")), 15, 0);
}

/*
 * Interact with "visuals"
 */
void do_cmd_visuals(PlayerType *player_ptr)
{
    FILE *auto_dump_stream;
    bool need_redraw = false;
    concptr empty_symbol = "<< ? >>";
    if (use_bigtile) {
        empty_symbol = "<< ?? >>";
    }

    screen_save();
    const auto initial_filename = format("%s.prf", player_ptr->base_name);
    while (true) {
        term_clear();
        print_visuals_menu(nullptr);
        const auto key = inkey();
        if (key == ESCAPE) {
            break;
        }

        switch (key) {
        case '0': {
            prt(_("コマンド: ユーザー設定ファイルのロード", "Command: Load a user pref file"), 15, 0);
            prt(_("ファイル: ", "File: "), 17, 0);
            const auto ask_result = askfor(70, initial_filename);
            if (!ask_result) {
                continue;
            }

            (void)process_pref_file(player_ptr, *ask_result, true);
            need_redraw = true;
            break;
        }
        case '1': {
            prt(_("コマンド: モンスターの[色/文字]をファイルに書き出します", "Command: Dump monster attr/chars"), 15, 0);
            prt(_("ファイル: ", "File: "), 17, 0);
            const auto ask_result = askfor(70, initial_filename);
            if (!ask_result) {
                continue;
            }

            const auto path = path_build(ANGBAND_DIR_USER, *ask_result);
            constexpr auto mark = "Monster attr/chars";
            if (!open_auto_dump(&auto_dump_stream, path, mark)) {
                continue;
            }

            auto_dump_printf(auto_dump_stream, _("\n# モンスターの[色/文字]の設定\n\n", "\n# Monster attr/char definitions\n\n"));
            for (const auto &[monrace_id, monrace] : MonraceList::get_instance()) {
                auto_dump_printf(auto_dump_stream, "# %s\n", monrace.name.data());
                const auto &symbol_config = monrace.symbol_config;
                auto_dump_printf(auto_dump_stream, "R:%d:0x%02X/0x%02X\n\n", enum2i(monrace_id), symbol_config.color, static_cast<uint8_t>(symbol_config.character));
            }

            close_auto_dump(&auto_dump_stream, mark);
            msg_print(_("モンスターの[色/文字]をファイルに書き出しました。", "Dumped monster attr/chars."));
            break;
        }
        case '2': {
            prt(_("コマンド: アイテムの[色/文字]をファイルに書き出します", "Command: Dump object attr/chars"), 15, 0);
            prt(_("ファイル: ", "File: "), 17, 0);
            const auto ask_result = askfor(70, initial_filename);
            if (!ask_result) {
                continue;
            }

            const auto path = path_build(ANGBAND_DIR_USER, *ask_result);
            constexpr auto mark = "Object attr/chars";
            if (!open_auto_dump(&auto_dump_stream, path, mark)) {
                continue;
            }

            auto_dump_printf(auto_dump_stream, _("\n# アイテムの[色/文字]の設定\n\n", "\n# Object attr/char definitions\n\n"));
            for (const auto &baseitem : BaseitemList::get_instance()) {
                if (!baseitem.is_valid()) {
                    continue;
                }

                std::string item_name;
                if (baseitem.flavor == 0) {
                    item_name = baseitem.stripped_name();
                } else {
                    ItemEntity dummy(baseitem.idx);
                    item_name = describe_flavor(player_ptr, dummy, OD_FORCE_FLAVOR);
                }

                auto_dump_printf(auto_dump_stream, "# %s\n", item_name.data());
                const auto &symbol_config = baseitem.symbol_config;
                auto_dump_printf(auto_dump_stream, "K:%d:0x%02X/0x%02X\n\n", (int)baseitem.idx, symbol_config.color, static_cast<uint8_t>(symbol_config.character));
            }

            close_auto_dump(&auto_dump_stream, mark);
            msg_print(_("アイテムの[色/文字]をファイルに書き出しました。", "Dumped object attr/chars."));
            break;
        }
        case '3': {
            prt(_("コマンド: 地形の[色/文字]をファイルに書き出します", "Command: Dump feature attr/chars"), 15, 0);
            prt(_("ファイル: ", "File: "), 17, 0);
            const auto ask_result = askfor(70, initial_filename);
            if (!ask_result) {
                continue;
            }

            const auto path = path_build(ANGBAND_DIR_USER, *ask_result);
            constexpr auto mark = "Feature attr/chars";
            if (!open_auto_dump(&auto_dump_stream, path, mark)) {
                continue;
            }

            auto_dump_printf(auto_dump_stream, _("\n# 地形の[色/文字]の設定\n\n", "\n# Feature attr/char definitions\n\n"));
            for (const auto &terrain : TerrainList::get_instance()) {
                if (terrain.name.empty()) {
                    continue;
                }
                if (terrain.mimic != terrain.idx) {
                    continue;
                }

                auto_dump_printf(auto_dump_stream, "# %s\n", (terrain.name.data()));
                const auto &symbol_standard = terrain.symbol_configs.at(F_LIT_STANDARD);
                const auto &symbol_lite = terrain.symbol_configs.at(F_LIT_LITE);
                const auto &symbol_dark = terrain.symbol_configs.at(F_LIT_DARK);
                auto_dump_printf(auto_dump_stream, "F:%d:0x%02X/0x%02X:0x%02X/0x%02X:0x%02X/0x%02X\n\n", terrain.idx, symbol_standard.color,
                    static_cast<uint8_t>(symbol_standard.character), symbol_lite.color, static_cast<uint8_t>(symbol_lite.character),
                    symbol_dark.color, static_cast<uint8_t>(symbol_dark.character));
            }

            close_auto_dump(&auto_dump_stream, mark);
            msg_print(_("地形の[色/文字]をファイルに書き出しました。", "Dumped feature attr/chars."));
            break;
        }
        case '4': {
            short num = 0;
            auto &monraces = MonraceList::get_instance();
            static auto choice_msg = _("モンスターの[色/文字]を変更します", "Change monster attr/chars");
            static auto monrace_id = monraces.begin()->second.idx;
            prt(format(_("コマンド: %s", "Command: %s"), choice_msg), 15, 0);
            while (true) {
                auto &monrace = monraces.get_monrace(monrace_id);
                int c;
                const auto &symbol_definition = monrace.symbol_definition;
                auto &symbol_config = monrace.symbol_config;
                term_putstr(5, 17, -1, TERM_WHITE, format(_("モンスター = %d, 名前 = %-40.40s", "Monster = %d, Name = %-40.40s"), enum2i(monrace_id), monrace.name.data()));
                term_putstr(10, 19, -1, TERM_WHITE, format(_("初期値  色 / 文字 = %3u / %3u", "Default attr/char = %3u / %3u"), symbol_definition.color, static_cast<uint8_t>(symbol_definition.character)));
                term_putstr(40, 19, -1, TERM_WHITE, empty_symbol);
                term_queue_bigchar(43, 19, { symbol_definition, {} });
                term_putstr(10, 20, -1, TERM_WHITE, format(_("現在値  色 / 文字 = %3u / %3u", "Current attr/char = %3u / %3u"), symbol_config.color, static_cast<uint8_t>(symbol_config.character)));
                term_putstr(40, 20, -1, TERM_WHITE, empty_symbol);
                term_queue_bigchar(43, 20, { symbol_config, {} });
                term_putstr(0, 22, -1, TERM_WHITE, _("コマンド (n/N/^N/a/A/^A/c/C/^C/v/V/^V): ", "Command (n/N/^N/a/A/^A/c/C/^C/v/V/^V): "));
                const auto ch = inkey();
                if (ch == ESCAPE) {
                    break;
                }

                if (iscntrl(ch)) {
                    c = 'a' + ch - KTRL('A');
                } else if (isupper(ch)) {
                    c = 'a' + ch - 'A';
                } else {
                    c = ch;
                }

                switch (c) {
                case 'n': {
                    const auto new_monrace_id_opt = input_new_visual_id(ch, num, static_cast<short>(monraces.size()));
                    if (!new_monrace_id_opt) {
                        break;
                    }

                    const auto new_monrace_id = *new_monrace_id_opt;
                    monrace_id = i2enum<MonraceId>(new_monrace_id);
                    num = new_monrace_id;
                    break;
                }
                case 'a': {
                    const auto visual_id = input_new_visual_id(ch, symbol_config.color, 256);
                    if (!visual_id) {
                        break;
                    }

                    symbol_config.color = *visual_id;
                    need_redraw = true;
                    break;
                }
                case 'c': {
                    const auto visual_id = input_new_visual_id(ch, symbol_config.character, 256);
                    if (!visual_id) {
                        break;
                    }

                    symbol_config.character = *visual_id;
                    need_redraw = true;
                    break;
                }
                case 'v':
                    do_cmd_knowledge_monsters(player_ptr, &need_redraw, true, monrace_id);
                    term_clear();
                    print_visuals_menu(choice_msg);
                    break;
                }
            }

            break;
        }
        case '5': {
            static auto choice_msg = _("アイテムの[色/文字]を変更します", "Change object attr/chars");
            static short bi_id = 0;
            prt(format(_("コマンド: %s", "Command: %s"), choice_msg), 15, 0);
            auto &baseitems = BaseitemList::get_instance();
            while (true) {
                auto &baseitem = baseitems.get_baseitem(bi_id);
                int c;
                const auto &symbol_definition = baseitem.symbol_definition;
                auto &symbol_config = baseitem.symbol_config;
                term_putstr(5, 17, -1, TERM_WHITE,
                    format(
                        _("アイテム = %d, 名前 = %-40.40s", "Object = %d, Name = %-40.40s"), bi_id, (!baseitem.flavor ? baseitem.name : baseitem.flavor_name).data()));
                term_putstr(10, 19, -1, TERM_WHITE, format(_("初期値  色 / 文字 = %3d / %3d", "Default attr/char = %3d / %3d"), symbol_definition.color, symbol_definition.character));
                term_putstr(40, 19, -1, TERM_WHITE, empty_symbol);
                term_queue_bigchar(43, 19, { symbol_definition, {} });
                term_putstr(10, 20, -1, TERM_WHITE, format(_("現在値  色 / 文字 = %3d / %3d", "Current attr/char = %3d / %3d"), symbol_config.color, symbol_config.character));
                term_putstr(40, 20, -1, TERM_WHITE, empty_symbol);
                term_queue_bigchar(43, 20, { symbol_config, {} });
                term_putstr(0, 22, -1, TERM_WHITE, _("コマンド (n/N/^N/a/A/^A/c/C/^C/v/V/^V): ", "Command (n/N/^N/a/A/^A/c/C/^C/v/V/^V): "));

                const auto ch = inkey();
                if (ch == ESCAPE) {
                    break;
                }

                if (iscntrl(ch)) {
                    c = 'a' + ch - KTRL('A');
                } else if (isupper(ch)) {
                    c = 'a' + ch - 'A';
                } else {
                    c = ch;
                }

                switch (c) {
                case 'n': {
                    std::optional<short> new_baseitem_id;
                    const auto previous_bi_id = bi_id;
                    while (true) {
                        new_baseitem_id = input_new_visual_id(ch, bi_id, static_cast<short>(baseitems.size()));
                        if (!new_baseitem_id) {
                            bi_id = previous_bi_id;
                            break;
                        }

                        bi_id = *new_baseitem_id;
                        if (baseitems.get_baseitem(bi_id).is_valid()) {
                            break;
                        }
                    }

                    break;
                }
                case 'a': {
                    const auto visual_id = input_new_visual_id(ch, symbol_config.color, 256);
                    if (!visual_id) {
                        break;
                    }

                    baseitem.symbol_config.color = *visual_id;
                    need_redraw = true;
                    break;
                }
                case 'c': {
                    const auto visual_id = input_new_visual_id(ch, symbol_config.character, 256);
                    if (!visual_id) {
                        break;
                    }

                    baseitem.symbol_config.character = *visual_id;
                    need_redraw = true;
                    break;
                }
                case 'v':
                    do_cmd_knowledge_objects(player_ptr, &need_redraw, true, bi_id);
                    term_clear();
                    print_visuals_menu(choice_msg);
                    break;
                }
            }

            break;
        }
        case '6': {
            static auto choice_msg = _("地形の[色/文字]を変更します", "Change feature attr/chars");
            static short terrain_id = 0;
            static short lighting_level = F_LIT_STANDARD;
            prt(format(_("コマンド: %s", "Command: %s"), choice_msg), 15, 0);
            auto &terrains = TerrainList::get_instance();
            while (true) {
                auto &terrain = terrains.get_terrain(terrain_id);
                int c;
                const auto &symbol_definition = terrain.symbol_definitions[lighting_level];
                const auto &symbol_config = terrain.symbol_configs[lighting_level];
                prt("", 17, 5);
                term_putstr(5, 17, -1, TERM_WHITE,
                    format(_("地形 = %d, 名前 = %s, 明度 = %s", "Terrain = %d, Name = %s, Lighting = %s"), terrain_id, (terrain.name.data()),
                        lighting_level_str[lighting_level]));
                term_putstr(10, 19, -1, TERM_WHITE, format(_("初期値  色 / 文字 = %3d / %3d", "Default attr/char = %3d / %3d"), symbol_definition.color, symbol_definition.character));
                term_putstr(40, 19, -1, TERM_WHITE, empty_symbol);
                term_queue_bigchar(43, 19, { symbol_definition, {} });
                term_putstr(10, 20, -1, TERM_WHITE, format(_("現在値  色 / 文字 = %3d / %3d", "Current attr/char = %3d / %3d"), symbol_config.color, symbol_config.character));
                term_putstr(40, 20, -1, TERM_WHITE, empty_symbol);
                term_queue_bigchar(43, 20, { symbol_config, {} });
                term_putstr(0, 22, -1, TERM_WHITE,
                    _("コマンド (n/N/^N/a/A/^A/c/C/^C/l/L/^L/d/D/^D/v/V/^V): ", "Command (n/N/^N/a/A/^A/c/C/^C/l/L/^L/d/D/^D/v/V/^V): "));

                const auto ch = inkey();
                if (ch == ESCAPE) {
                    break;
                }

                if (iscntrl(ch)) {
                    c = 'a' + ch - KTRL('A');
                } else if (isupper(ch)) {
                    c = 'a' + ch - 'A';
                } else {
                    c = ch;
                }

                switch (c) {
                case 'n': {
                    std::optional<short> new_terrain_id;
                    const auto previous_terrain_id = terrain_id;
                    while (true) {
                        new_terrain_id = input_new_visual_id(ch, terrain_id, static_cast<short>(TerrainList::get_instance().size()));
                        if (!new_terrain_id) {
                            terrain_id = previous_terrain_id;
                            break;
                        }

                        terrain_id = *new_terrain_id;
                        const auto &new_terrain = terrains.get_terrain(terrain_id);
                        if (!new_terrain.name.empty() && (new_terrain.mimic == terrain_id)) {
                            break;
                        }
                    }

                    break;
                }
                case 'a': {
                    auto &color_config = terrain.symbol_configs[lighting_level].color;
                    const auto visual_id = input_new_visual_id(ch, color_config, 256);
                    if (!visual_id) {
                        break;
                    }

                    color_config = *visual_id;
                    need_redraw = true;
                    break;
                }
                case 'c': {
                    auto &character_config = terrain.symbol_configs[lighting_level].character;
                    const auto visual_id = input_new_visual_id(ch, character_config, 256);
                    if (!visual_id) {
                        break;
                    }

                    character_config = *visual_id;
                    need_redraw = true;
                    break;
                }
                case 'l': {
                    const auto visual_id = input_new_visual_id(ch, lighting_level, F_LIT_MAX);
                    if (!visual_id) {
                        break;
                    }

                    lighting_level = *visual_id;
                    break;
                }
                case 'd':
                    terrain.reset_lighting();
                    need_redraw = true;
                    break;
                case 'v':
                    do_cmd_knowledge_features(&need_redraw, true, terrain_id, &lighting_level);
                    term_clear();
                    print_visuals_menu(choice_msg);
                    break;
                }
            }

            break;
        }
        case '7':
            do_cmd_knowledge_monsters(player_ptr, &need_redraw, true);
            break;
        case '8':
            do_cmd_knowledge_objects(player_ptr, &need_redraw, true, -1);
            break;
        case '9': {
            short lighting_level = F_LIT_STANDARD;
            do_cmd_knowledge_features(&need_redraw, true, -1, &lighting_level);
            break;
        }
        case 'r':
        case 'R':
            reset_visuals(player_ptr);
            msg_print(_("画面上の[色/文字]を初期値にリセットしました。", "Visual attr/char tables reset."));
            need_redraw = true;
            break;
        default:
            bell();
            break;
        }

        msg_erase();
    }

    screen_load();
    if (need_redraw) {
        do_cmd_redraw(player_ptr);
    }
}
