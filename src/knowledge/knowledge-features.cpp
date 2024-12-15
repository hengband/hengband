/*!
 * @brief 地形に関する情報を表示する
 * @date 2020/04/24
 * @author Hourier
 */

#include "knowledge/knowledge-features.h"
#include "core/show-file.h"
#include "game-option/special-options.h"
#include "grid/feature.h"
#include "io-dump/dump-util.h"
#include "io/input-key-acceptor.h"
#include "knowledge/lighting-level-table.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/dungeon/dungeon-record.h"
#include "system/monrace/monrace-definition.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/angband-files.h"
#include "util/int-char-converter.h"
#include "world/world.h"

/*
 * Build a list of feature indexes in the given group. Return the number
 * of features in the group.
 *
 * mode & 0x01 : check for non-empty group
 */
static FEAT_IDX collect_features(FEAT_IDX *feat_idx, BIT_FLAGS8 mode)
{
    FEAT_IDX feat_cnt = 0;
    for (const auto &terrain : TerrainList::get_instance()) {
        if (terrain.name.empty()) {
            continue;
        }
        if (terrain.mimic != terrain.idx) {
            continue;
        }

        feat_idx[feat_cnt++] = terrain.idx;
        if (mode & 0x01) {
            break;
        }
    }

    feat_idx[feat_cnt] = -1;
    return feat_cnt;
}

/*
 * Display the features in a group.
 */
static void display_feature_list(int col, int row, int per_page, FEAT_IDX *feat_idx, FEAT_IDX feat_cur, FEAT_IDX feat_top, bool visual_only, int lighting_level)
{
    int lit_col[F_LIT_MAX]{};
    int f_idx_col = use_bigtile ? 62 : 64;

    lit_col[F_LIT_STANDARD] = use_bigtile ? (71 - F_LIT_MAX) : 71;
    for (auto i = F_LIT_NS_BEGIN; i < F_LIT_MAX; i++) {
        lit_col[i] = lit_col[F_LIT_STANDARD] + 2 + (i - F_LIT_NS_BEGIN) * 2 + (use_bigtile ? i : 0);
    }

    const auto is_wizard = AngbandWorld::get_instance().wizard;
    const auto &terrains = TerrainList::get_instance();
    int i;
    for (i = 0; i < per_page && (feat_idx[feat_top + i] >= 0); i++) {
        TERM_COLOR attr;
        auto terrain_id = feat_idx[feat_top + i];
        const auto &terrain = terrains.get_terrain(terrain_id);
        int row_i = row + i;
        attr = ((i + feat_top == feat_cur) ? TERM_L_BLUE : TERM_WHITE);
        c_prt(attr, terrain.name.data(), row_i, col);
        if (per_page == 1) {
            c_prt(attr, format("(%s)", lighting_level_str[lighting_level]), row_i, col + 1 + terrain.name.size());
            const auto &symbol_config = terrain.symbol_configs.at(lighting_level);
            c_prt(attr, format("%02x/%02x", symbol_config.color, static_cast<uint8_t>(symbol_config.character)), row_i,
                f_idx_col - ((is_wizard || visual_only) ? 6 : 2));
        }
        if (is_wizard || visual_only) {
            c_prt(attr, format("%d", terrain_id), row_i, f_idx_col);
        }

        const auto &symbol_standard = terrain.symbol_configs.at(F_LIT_STANDARD);
        term_queue_bigchar(lit_col[F_LIT_STANDARD], row_i, { symbol_standard, {} });
        term_putch(lit_col[F_LIT_NS_BEGIN], row_i, { TERM_SLATE, '(' });
        for (int j = F_LIT_NS_BEGIN + 1; j < F_LIT_MAX; j++) {
            term_putch(lit_col[j], row_i, { TERM_SLATE, '/' });
        }

        term_putch(lit_col[F_LIT_MAX - 1] + (use_bigtile ? 3 : 2), row_i, { TERM_SLATE, ')' });
        for (int j = F_LIT_NS_BEGIN; j < F_LIT_MAX; j++) {
            const auto &symbol_config = terrain.symbol_configs.at(j);
            term_queue_bigchar(lit_col[j] + 1, row_i, { symbol_config, {} });
        }
    }

    for (; i < per_page; i++) {
        term_erase(col, row + i);
    }
}

/*
 * Interact with feature visuals.
 */
void do_cmd_knowledge_features(bool *need_redraw, bool visual_only, IDX direct_f_idx, IDX *lighting_level)
{
    TermCenteredOffsetSetter tcos(MAIN_TERM_MIN_COLS, std::nullopt);
    std::map<int, DisplaySymbol> symbols;
    const auto &[wid, hgt] = term_get_size();
    auto &terrains = TerrainList::get_instance();
    std::vector<FEAT_IDX> feat_idx(terrains.size());

    const std::string terrain_group(_("地形    ", "Terrains")); //!< @details 他と合わせるためgroupと呼ぶ.
    const auto max_length = terrain_group.length();
    int feat_cnt;
    std::vector<short> grp_idx;
    TERM_COLOR attr_top = 0;
    bool visual_list = false;
    byte char_left = 0;
    TERM_LEN browser_rows = hgt - 8;
    if (direct_f_idx < 0) {
        if (collect_features(feat_idx.data(), 0x01)) {
            grp_idx.push_back(0);
        }

        feat_cnt = 0;
    } else {
        auto &terrain = terrains.get_terrain(direct_f_idx);
        auto &symbol_config = terrain.symbol_configs.at(*lighting_level);
        feat_idx[0] = direct_f_idx;
        feat_cnt = 1;
        feat_idx[1] = -1;
        (void)visual_mode_command('v', &visual_list, browser_rows - 1, wid - max_length + 3, &attr_top, &char_left, &symbol_config.color,
            &symbol_config.character, need_redraw);

        for (FEAT_IDX i = 0; i < F_LIT_MAX; i++) {
            symbols[i] = symbol_config;
        }
    }

    FEAT_IDX old_grp_cur = -1;
    FEAT_IDX grp_cur = 0;
    FEAT_IDX grp_top = 0;
    FEAT_IDX feat_cur = 0;
    FEAT_IDX feat_top = 0;
    TERM_LEN column = 0;
    bool flag = false;
    bool redraw = true;
    const auto is_wizard = AngbandWorld::get_instance().wizard;
    auto &symbols_cb = DisplaySymbolsClipboard::get_instance();
    while (!flag) {
        char ch;
        if (redraw) {
            clear_from(0);

            prt(_("表示 - 地形", "Visuals - features"), 2, 0);
            if (direct_f_idx < 0) {
                prt(_("グループ", "Group"), 4, 0);
            }
            prt(_("名前", "Name"), 4, max_length + 3);
            if (use_bigtile) {
                if (is_wizard || visual_only) {
                    prt("Idx", 4, 62);
                }
                prt(_("文字 ( l/ d)", "Sym ( l/ d)"), 4, 66);
            } else {
                if (is_wizard || visual_only) {
                    prt("Idx", 4, 64);
                }
                prt(_("文字 (l/d)", "Sym (l/d)"), 4, 68);
            }

            for (FEAT_IDX i = 0; i < 78; i++) {
                term_putch(i, 5, { TERM_WHITE, '=' });
            }

            if (direct_f_idx < 0) {
                for (FEAT_IDX i = 0; i < browser_rows; i++) {
                    term_putch(max_length + 1, 6 + i, { TERM_WHITE, '|' });
                }
            }

            redraw = false;
        }

        if (direct_f_idx < 0) {
            if (grp_cur < grp_top) {
                grp_top = grp_cur;
            }
            if (grp_cur >= grp_top + browser_rows) {
                grp_top = grp_cur - browser_rows + 1;
            }

            static const std::vector<short> terrain_group_num = { 0 }; // size - 1 を引数に入れる必要があるので0.
            display_group_list(max_length, browser_rows, terrain_group_num, { terrain_group }, grp_cur, grp_top);
            if (old_grp_cur != grp_cur) {
                old_grp_cur = grp_cur;
                feat_cnt = collect_features(feat_idx.data(), 0x00);
            }

            while (feat_cur < feat_top) {
                feat_top = std::max<short>(0, feat_top - browser_rows / 2);
            }
            while (feat_cur >= feat_top + browser_rows) {
                feat_top = std::min<short>(feat_cnt - browser_rows, feat_top + browser_rows / 2);
            }
        }

        if (!visual_list) {
            display_feature_list(max_length + 3, 6, browser_rows, feat_idx.data(), feat_cur, feat_top, visual_only, F_LIT_STANDARD);
        } else {
            feat_top = feat_cur;
            display_feature_list(max_length + 3, 6, 1, feat_idx.data(), feat_cur, feat_top, visual_only, *lighting_level);
            display_visual_list(max_length + 3, 7, browser_rows - 1, wid - max_length + 3, attr_top, char_left);
        }

        prt(format(_("<方向>%s, 'd'で標準光源効果%s, ESC", "<dir>%s, 'd' for default lighting%s, ESC"),
                visual_list ? _(", ENTERで決定, 'a'で対象明度変更", ", ENTER to accept, 'a' for lighting level")
                            : _(", 'v'でシンボル変更", ", 'v' for visuals"),
                (symbols_cb.symbol != DisplaySymbol()) ? _(", 'c', 'p'でペースト", ", 'c', 'p' to paste") : _(", 'c'でコピー", ", 'c' to copy")),
            hgt - 1, 0);

        auto &terrain = terrains.get_terrain(feat_idx[feat_cur]);
        auto &symbol_orig = terrain.symbol_configs.at(*lighting_level);
        if (visual_list) {
            place_visual_list_cursor(max_length + 3, 7, symbol_orig.color, static_cast<uint8_t>(symbol_orig.character), attr_top, char_left);
        } else if (!column) {
            term_gotoxy(0, 6 + (grp_cur - grp_top));
        } else {
            term_gotoxy(max_length + 3, 6 + (feat_cur - feat_top));
        }

        ch = inkey();
        if (visual_list && ((ch == 'A') || (ch == 'a'))) {
            int prev_lighting_level = *lighting_level;
            if (ch == 'A') {
                if (*lighting_level <= 0) {
                    *lighting_level = F_LIT_MAX - 1;
                } else {
                    (*lighting_level)--;
                }
            } else {
                if (*lighting_level >= F_LIT_MAX - 1) {
                    *lighting_level = 0;
                } else {
                    (*lighting_level)++;
                }
            }

            const auto &symbol_previous = terrain.symbol_configs.at(prev_lighting_level);
            const auto &symbol_lightning = terrain.symbol_configs.at(*lighting_level);
            if (symbol_previous.color != symbol_lightning.color) {
                attr_top = std::max<int8_t>(0, (symbol_lightning.color & 0x7f) - 5);
            }

            if (symbol_previous.character != symbol_lightning.character) {
                char_left = std::max<int8_t>(0, symbol_lightning.character - 10);
            }

            continue;
        }

        if ((ch == 'D') || (ch == 'd')) {
            const auto &symbol_previous = terrain.symbol_configs.at(*lighting_level);
            terrain.reset_lighting();
            const auto &symbol_current = terrain.symbol_configs.at(*lighting_level);
            if (visual_list) {
                if (symbol_previous.color != symbol_current.color) {
                    attr_top = std::max<int8_t>(0, (symbol_current.color & 0x7f) - 5);
                }

                if (symbol_previous.character != symbol_current.character) {
                    char_left = std::max<int8_t>(0, symbol_current.character - 10);
                }
            } else {
                *need_redraw = true;
            }

            continue;
        }

        if (visual_mode_command(ch, &visual_list, browser_rows - 1, wid - max_length + 3, &attr_top, &char_left, &symbol_orig.color, &symbol_orig.character, need_redraw)) {
            switch (ch) {
            case ESCAPE:
                terrain.symbol_configs = symbols;

                [[fallthrough]];
            case '\n':
            case '\r':
                if (direct_f_idx >= 0) {
                    flag = true;
                } else {
                    *lighting_level = F_LIT_STANDARD;
                }
                break;
            case 'V':
            case 'v':
                symbols = terrain.symbol_configs;
                *lighting_level = F_LIT_STANDARD;
                break;

            case 'C':
            case 'c':
                if (visual_list) {
                    break;
                }

                symbols_cb.set_symbol(terrain.symbol_configs);
                break;
            case 'P':
            case 'p': {
                if (visual_list) {
                    break;
                }

                auto &symbols_cb_map = symbols_cb.symbols;
                for (auto i = F_LIT_NS_BEGIN; i < F_LIT_MAX; i++) {
                    auto &symbol_config = terrain.symbol_configs.at(i);
                    auto &symbol = symbols_cb_map.at(i);
                    const auto has_character = symbol.has_character();
                    if ((symbol.color != 0) || (!(symbol.character & 0x80) && has_character)) {
                        symbol_config.color = symbol.color;
                    }

                    if (has_character) {
                        symbol_config.character = symbol.character;
                    }
                }

                break;
            }
            }
            continue;
        }

        switch (ch) {
        case ESCAPE: {
            flag = true;
            break;
        }

        default: {
            constexpr auto dummy_count = 0;
            browser_cursor(ch, &column, &grp_cur, dummy_count, &feat_cur, feat_cnt);
            break;
        }
        }
    }
}

/*
 * Dungeon
 */
void do_cmd_knowledge_dungeon(PlayerType *player_ptr)
{
    FILE *fff = nullptr;
    GAME_TEXT file_name[FILE_NAME_SIZE];
    if (!open_temporary_file(&fff, file_name)) {
        return;
    }

    const auto &dungeon_records = DungeonRecords::get_instance();
    for (const auto &[dungeon_id, dungeon] : DungeonList::get_instance()) {
        auto is_conquered = false;
        if (!dungeon.is_dungeon() || !dungeon.maxdepth) {
            continue;
        }

        const auto &dungeon_record = dungeon_records.get_record(dungeon_id);
        if (!dungeon_record.has_entered()) {
            continue;
        }

        const auto max_level = dungeon_record.get_max_level();
        if (dungeon.has_guardian()) {
            if (dungeon.get_guardian().max_num == 0) {
                is_conquered = true;
            }
        } else if (max_level == dungeon.maxdepth) {
            is_conquered = true;
        }

        fprintf(fff, _("%c%-12s :  %3d 階\n", "%c%-16s :  level %3d\n"), is_conquered ? '!' : ' ', dungeon.name.data(), max_level);
    }

    angband_fclose(fff);
    FileDisplayer(player_ptr->name).display(true, file_name, 0, 0, _("今までに入ったダンジョン", "Dungeon"));
    fd_kill(file_name);
}
