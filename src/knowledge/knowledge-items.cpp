/*!
 * @brief 既知のアイテムとアーティファクトを表示する
 * @date 2020/04/23
 * @author Hourier
 */

#include "knowledge/knowledge-items.h"
#include "core/show-file.h"
#include "core/stuff-handler.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "game-option/special-options.h"
#include "inventory/inventory-slot-types.h"
#include "io-dump/dump-util.h"
#include "io/input-key-acceptor.h"
#include "knowledge/item-group-table.h"
#include "object-enchant/special-object-flags.h"
#include "object/tval-types.h"
#include "perception/identification.h"
#include "perception/object-perception.h"
#include "system/artifact-type-definition.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "tracking/baseitem-tracker.h"
#include "util/angband-files.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <numeric>
#include <set>
#include <vector>

namespace {
auto collect_known_fixed_artifacts(PlayerType *player_ptr)
{
    const auto &artifacts = ArtifactList::get_instance();
    const auto comparer = [&artifacts](auto id1, auto id2) { return artifacts.order(id1, id2); };
    std::set<FixedArtifactId, decltype(comparer)> fa_ids(comparer);
    for (const auto &[fa_id, artifact] : artifacts) {
        if (!artifact.is_generated) {
            continue;
        }

        fa_ids.insert(fa_id);
    }

    const auto &floor = *player_ptr->current_floor_ptr;
    for (auto y = 0; y < floor.height; y++) {
        for (auto x = 0; x < floor.width; x++) {
            const auto &grid = floor.get_grid({ y, x });
            for (const auto this_o_idx : grid.o_idx_list) {
                const auto &item = floor.o_list[this_o_idx];
                if (!item.is_fixed_artifact() || item.is_known()) {
                    continue;
                }

                fa_ids.erase(item.fa_id);
            }
        }
    }

    for (auto i = 0; i < INVEN_TOTAL; i++) {
        const auto &item = player_ptr->inventory_list[i];
        if (!item.is_valid()) {
            continue;
        }

        if (!item.is_fixed_artifact()) {
            continue;
        }

        if (item.is_known()) {
            continue;
        }

        fa_ids.erase(item.fa_id);
    }

    return fa_ids;
}
}

/*!
 * @brief Check the status of "artifacts"
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void do_cmd_knowledge_artifacts(PlayerType *player_ptr)
{
    FILE *fff = nullptr;
    GAME_TEXT file_name[FILE_NAME_SIZE];
    if (!open_temporary_file(&fff, file_name)) {
        return;
    }

    const auto &artifacts = ArtifactList::get_instance();
    const auto fa_ids = collect_known_fixed_artifacts(player_ptr);
    for (const auto fa_id : fa_ids) {
        const auto &artifact = artifacts.get_artifact(fa_id);
        constexpr auto template_basename = _("     %s\n", "     The %s\n");
        ItemEntity item(artifact.bi_key);
        item.fa_id = fa_id;
        item.ident |= IDENT_STORE;
        const auto item_name = describe_flavor(player_ptr, item, (OD_OMIT_PREFIX | OD_NAME_ONLY));
        fprintf(fff, template_basename, item_name.data());
    }

    angband_fclose(fff);
    FileDisplayer(player_ptr->name).display(true, file_name, 0, 0, _("既知の伝説のアイテム", "Artifacts Seen"));
    fd_kill(file_name);
}

/*!
 * @brief ベースアイテムの出現率チェック処理
 * @param mode グループ化モード (0x02 表示専用)
 * @param baseitem ベースアイテムへの参照
 * @return collect_objects() の処理を続行するか否か
 */
static bool check_baseitem_chance(const BIT_FLAGS8 mode, const BaseitemDefinition &baseitem)
{
    if (mode & 0x02) {
        return true;
    }

    if (!AngbandWorld::get_instance().wizard && ((baseitem.flavor == 0) || !baseitem.aware)) {
        return false;
    }

    const auto &alloc_tables = baseitem.alloc_tables;
    const auto sum_chances = std::accumulate(alloc_tables.begin(), alloc_tables.end(), 0, [](int sum, const auto &table) {
        return sum + table.chance;
    });

    return sum_chances > 0;
}

/*
 * Build a list of object indexes in the given group. Return the number
 * of objects in the group.
 *
 * mode & 0x01 : check for non-empty group
 * mode & 0x02 : visual operation only
 */
static short collect_objects(int grp_cur, std::vector<short> &object_idx, BIT_FLAGS8 mode)
{
    short object_cnt = 0;
    const auto group_tval = ITEM_KINDS_GROUP[grp_cur];
    for (const auto &baseitem : BaseitemList::get_instance()) {
        if (baseitem.name.empty() || !check_baseitem_chance(mode, baseitem)) {
            continue;
        }

        const auto tval = baseitem.bi_key.tval();
        if (group_tval == ItemKindType::LIFE_BOOK) {
            if (baseitem.bi_key.is_spell_book()) {
                object_idx[object_cnt++] = baseitem.idx;
            } else {
                continue;
            }
        } else if (tval == group_tval) {
            object_idx[object_cnt++] = baseitem.idx;
        } else {
            continue;
        }

        if (mode & 0x01) {
            break;
        }
    }

    object_idx[object_cnt] = -1;
    return object_cnt;
}

/*
 * Display the objects in a group.
 */
static void display_object_list(int col, int row, int per_page, const std::vector<short> &object_idx, int object_cur, int object_top, bool visual_only)
{
    const auto is_wizard = AngbandWorld::get_instance().wizard;
    const auto &baseitems = BaseitemList::get_instance();
    int i;
    for (i = 0; i < per_page && (object_idx[object_top + i] >= 0); i++) {
        const short bi_id = object_idx[object_top + i];
        const auto &baseitem = baseitems.get_baseitem(bi_id);
        TERM_COLOR attr = ((baseitem.aware || visual_only) ? TERM_WHITE : TERM_SLATE);
        byte cursor = ((baseitem.aware || visual_only) ? TERM_L_BLUE : TERM_BLUE);
        const auto &flavor_baseitem = !visual_only && baseitem.flavor ? baseitems.get_baseitem(baseitem.flavor) : baseitem;

        attr = ((i + object_top == object_cur) ? cursor : attr);
        const auto is_flavor_only = (baseitem.flavor != 0) && (visual_only || !baseitem.aware);
        const auto o_name = is_flavor_only ? flavor_baseitem.flavor_name : baseitem.stripped_name();
        c_prt(attr, o_name.data(), row + i, col);
        const auto &symbol_config = flavor_baseitem.symbol_config;
        if (per_page == 1) {
            c_prt(attr, format("%02x/%02x", symbol_config.color, symbol_config.character), row + i, (is_wizard || visual_only) ? 64 : 68);
        }

        if (is_wizard || visual_only) {
            c_prt(attr, format("%d", bi_id), row + i, 70);
        }

        term_queue_bigchar(use_bigtile ? 76 : 77, row + i, { symbol_config, {} });
    }

    for (; i < per_page; i++) {
        term_erase(col, row + i);
    }
}

/*
 * Describe fake object
 */
static void desc_obj_fake(PlayerType *player_ptr, short bi_id)
{
    ItemEntity item(bi_id);
    item.ident |= IDENT_KNOWN;
    handle_stuff(player_ptr);
    if (screen_object(player_ptr, item, SCROBJ_FAKE_OBJECT | SCROBJ_FORCE_DETAIL)) {
        return;
    }

    msg_print(_("特に変わったところはないようだ。", "You see nothing special."));
    msg_print(nullptr);
}

/**
 * @brief Display known objects
 */
void do_cmd_knowledge_objects(PlayerType *player_ptr, bool *need_redraw, bool visual_only, short direct_k_idx)
{
    TermCenteredOffsetSetter tcos(MAIN_TERM_MIN_COLS, std::nullopt);

    short object_old, object_top;
    std::vector<short> grp_idx;
    int object_cnt;

    bool visual_list = false;
    TERM_COLOR attr_top = 0;
    byte char_left = 0;
    byte mode;

    const auto &[wid, hgt] = term_get_size();
    auto browser_rows = hgt - 8;
    auto &baseitems = BaseitemList::get_instance();
    std::vector<short> object_idx(baseitems.size());

    const auto max_element = std::max_element(ITEM_KIND_NAMES_GROUP.begin(), ITEM_KIND_NAMES_GROUP.end(),
        [](auto x, auto y) { return x.length() < y.length(); });
    const int max_length = max_element->length();
    const auto width = wid - (max_length + 3);
    if (direct_k_idx < 0) {
        mode = visual_only ? 0x03 : 0x01;
        const auto size = static_cast<short>(ITEM_KIND_NAMES_GROUP.size());
        for (short i = 0; i < size; i++) {
            if (collect_objects(i, object_idx, mode)) {
                grp_idx.push_back(i);
            }
        }

        object_old = -1;
        object_cnt = 0;
    } else {
        auto &baseitem = baseitems.get_baseitem(direct_k_idx);
        auto &flavor_baseitem = !visual_only && baseitem.flavor ? baseitems.get_baseitem(baseitem.flavor) : baseitem;
        object_idx[0] = direct_k_idx;
        object_old = direct_k_idx;
        object_cnt = 1;
        object_idx[1] = -1;
        const auto height = browser_rows - 1;
        auto &symbol_config = flavor_baseitem.symbol_config;
        (void)visual_mode_command(
            'v', &visual_list, height, width, &attr_top, &char_left, &symbol_config.color, &symbol_config.character, need_redraw);
    }

    mode = visual_only ? 0x02 : 0x00;
    IDX old_grp_cur = -1;
    IDX grp_cur = 0;
    IDX grp_top = 0;
    IDX object_cur = object_top = 0;
    bool flag = false;
    bool redraw = true;
    int column = 0;
    auto &tracker = BaseitemTracker::get_instance();
    const auto &world = AngbandWorld::get_instance();
    const auto &symbols_cb = DisplaySymbolsClipboard::get_instance();
    while (!flag) {
        if (redraw) {
            clear_from(0);

#ifdef JP
            prt(format("%s - アイテム", !visual_only ? "知識" : "表示"), 2, 0);
            if (direct_k_idx < 0) {
                prt("グループ", 4, 0);
            }
            prt("名前", 4, max_length + 3);
            if (world.wizard || visual_only) {
                prt("Idx", 4, 70);
            }
            prt("文字", 4, 74);
#else
            prt(format("%s - objects", !visual_only ? "Knowledge" : "Visuals"), 2, 0);
            if (direct_k_idx < 0) {
                prt("Group", 4, 0);
            }
            prt("Name", 4, max_length + 3);
            if (world.wizard || visual_only) {
                prt("Idx", 4, 70);
            }
            prt("Sym", 4, 75);
#endif

            for (IDX i = 0; i < 78; i++) {
                term_putch(i, 5, { TERM_WHITE, '=' });
            }

            if (direct_k_idx < 0) {
                for (IDX i = 0; i < browser_rows; i++) {
                    term_putch(max_length + 1, 6 + i, { TERM_WHITE, '|' });
                }
            }

            redraw = false;
        }

        if (direct_k_idx < 0) {
            if (grp_cur < grp_top) {
                grp_top = grp_cur;
            }
            if (grp_cur >= grp_top + browser_rows) {
                grp_top = grp_cur - browser_rows + 1;
            }

            std::vector<std::string> tmp_texts = ITEM_KIND_NAMES_GROUP;
            display_group_list(max_length, browser_rows, grp_idx, tmp_texts, grp_cur, grp_top);
            if (old_grp_cur != grp_cur) {
                old_grp_cur = grp_cur;
                object_cnt = collect_objects(grp_idx[grp_cur], object_idx, mode);
            }

            while (object_cur < object_top) {
                object_top = std::max<short>(0, object_top - browser_rows / 2);
            }

            while (object_cur >= object_top + browser_rows) {
                object_top = std::min<short>(object_cnt - browser_rows, object_top + browser_rows / 2);
            }
        }

        if (!visual_list) {
            display_object_list(max_length + 3, 6, browser_rows, object_idx, object_cur, object_top, visual_only);
        } else {
            object_top = object_cur;
            display_object_list(max_length + 3, 6, 1, object_idx, object_cur, object_top, visual_only);
            display_visual_list(max_length + 3, 7, browser_rows - 1, wid - (max_length + 3), attr_top, char_left);
        }

        auto &baseitem = baseitems.get_baseitem(object_idx[object_cur]);
        auto &flavor_baseitem = !visual_only && baseitem.flavor ? baseitems.get_baseitem(baseitem.flavor) : baseitem;

#ifdef JP
        prt(format("<方向>%s%s%s, ESC", (!visual_list && !visual_only) ? ", 'r'で詳細を見る" : "", visual_list ? ", ENTERで決定" : ", 'v'でシンボル変更",
                (symbols_cb.symbol != DisplaySymbol()) ? ", 'c', 'p'でペースト" : ", 'c'でコピー"),
            hgt - 1, 0);
#else
        prt(format("<dir>%s%s%s, ESC", (!visual_list && !visual_only) ? ", 'r' to recall" : "", visual_list ? ", ENTER to accept" : ", 'v' for visuals",
                (symbols_cb.symbol != DisplaySymbol()) ? ", 'c', 'p' to paste" : ", 'c' to copy"),
            hgt - 1, 0);
#endif

        if (!visual_only) {
            if (object_cnt) {
                tracker.set_trackee(object_idx[object_cur]);
            }

            if (object_old != object_idx[object_cur]) {
                handle_stuff(player_ptr);
                object_old = object_idx[object_cur];
            }
        }

        auto &symbol_config = flavor_baseitem.symbol_config;
        if (visual_list) {
            place_visual_list_cursor(max_length + 3, 7, symbol_config.color, symbol_config.character, attr_top, char_left);
        } else if (!column) {
            term_gotoxy(0, 6 + (grp_cur - grp_top));
        } else {
            term_gotoxy(max_length + 3, 6 + (object_cur - object_top));
        }

        char ch = inkey();
        const auto height = browser_rows - 1;
        if (visual_mode_command(
                ch, &visual_list, height, width, &attr_top, &char_left, &symbol_config.color, &symbol_config.character, need_redraw)) {
            if (direct_k_idx >= 0) {
                switch (ch) {
                case '\n':
                case '\r':
                case ESCAPE:
                    flag = true;
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

        case 'R':
        case 'r': {
            if (!visual_list && !visual_only && (grp_idx.size() > 0)) {
                desc_obj_fake(player_ptr, object_idx[object_cur]);
                redraw = true;
            }

            break;
        }

        default: {
            browser_cursor(ch, &column, &grp_cur, std::ssize(grp_idx), &object_cur, object_cnt);
            break;
        }
        }
    }
}
