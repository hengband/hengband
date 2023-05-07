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
#include "flavor/object-flavor.h"
#include "game-option/special-options.h"
#include "inventory/inventory-slot-types.h"
#include "io-dump/dump-util.h"
#include "io/input-key-acceptor.h"
#include "knowledge/object-group-table.h"
#include "object-enchant/special-object-flags.h"
#include "object/object-kind-hook.h"
#include "object/tval-types.h"
#include "perception/identification.h"
#include "perception/object-perception.h"
#include "system/artifact-type-definition.h"
#include "system/baseitem-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/angband-files.h"
#include "util/int-char-converter.h"
#include "util/sort.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <numeric>
#include <set>
#include <vector>

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

    std::set<FixedArtifactId> known_list;

    for (const auto &[a_idx, artifact] : artifacts_info) {
        if (artifact.name.empty() || !artifact.is_generated) {
            continue;
        }

        known_list.insert(known_list.end(), a_idx);
    }

    for (POSITION y = 0; y < player_ptr->current_floor_ptr->height; y++) {
        for (POSITION x = 0; x < player_ptr->current_floor_ptr->width; x++) {
            auto *g_ptr = &player_ptr->current_floor_ptr->grid_array[y][x];
            for (const auto this_o_idx : g_ptr->o_idx_list) {
                const auto *o_ptr = &player_ptr->current_floor_ptr->o_list[this_o_idx];
                if (!o_ptr->is_fixed_artifact() || o_ptr->is_known()) {
                    continue;
                }

                known_list.erase(o_ptr->fixed_artifact_idx);
            }
        }
    }

    for (auto i = 0; i < INVEN_TOTAL; i++) {
        auto *o_ptr = &player_ptr->inventory_list[i];
        if (!o_ptr->is_valid()) {
            continue;
        }
        if (!o_ptr->is_fixed_artifact()) {
            continue;
        }
        if (o_ptr->is_known()) {
            continue;
        }

        known_list.erase(o_ptr->fixed_artifact_idx);
    }

    std::vector<FixedArtifactId> whats(known_list.begin(), known_list.end());

    uint16_t why = 3;
    ang_sort(player_ptr, whats.data(), &why, whats.size(), ang_sort_art_comp, ang_sort_art_swap);
    for (auto a_idx : whats) {
        const auto &artifact = ArtifactsInfo::get_instance().get_artifact(a_idx);
        constexpr auto unknown_art = _("未知の伝説のアイテム", "Unknown Artifact");
        const auto bi_id = lookup_baseitem_id(artifact.bi_key);
        constexpr auto template_basename = _("     %s\n", "     The %s\n");
        if (bi_id == 0) {
            fprintf(fff, template_basename, unknown_art);
            continue;
        }

        ItemEntity item;
        item.prep(bi_id);
        item.fixed_artifact_idx = a_idx;
        item.ident |= IDENT_STORE;
        const auto item_name = describe_flavor(player_ptr, &item, (OD_OMIT_PREFIX | OD_NAME_ONLY));
        fprintf(fff, template_basename, item_name.data());
    }

    angband_fclose(fff);
    (void)show_file(player_ptr, true, file_name, _("既知の伝説のアイテム", "Artifacts Seen"), 0, 0);
    fd_kill(file_name);
}

/*!
 * @brief ベースアイテムの出現率チェック処理
 * @param mode グループ化モード (0x02 表示専用)
 * @param baseitem ベースアイテムへの参照
 * @return collect_objects() の処理を続行するか否か
 */
static bool check_baseitem_chance(const BIT_FLAGS8 mode, const BaseitemInfo &baseitem)
{
    if (mode & 0x02) {
        return true;
    }

    if (!w_ptr->wizard && ((baseitem.flavor == 0) || !baseitem.aware)) {
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
    const auto group_tval = object_group_tval[grp_cur];
    for (const auto &baseitem : baseitems_info) {
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
    int i;
    for (i = 0; i < per_page && (object_idx[object_top + i] >= 0); i++) {
        TERM_COLOR a;
        short bi_id = object_idx[object_top + i];
        const auto &baseitem = baseitems_info[bi_id];
        TERM_COLOR attr = ((baseitem.aware || visual_only) ? TERM_WHITE : TERM_SLATE);
        byte cursor = ((baseitem.aware || visual_only) ? TERM_L_BLUE : TERM_BLUE);
        const auto &flavor_baseitem = !visual_only && baseitem.flavor ? baseitems_info[baseitem.flavor] : baseitems_info[bi_id];

        attr = ((i + object_top == object_cur) ? cursor : attr);
        const auto is_flavor_only = (baseitem.flavor != 0) && (visual_only || !baseitem.aware);
        const auto o_name = is_flavor_only ? flavor_baseitem.flavor_name : strip_name(bi_id);
        c_prt(attr, o_name.data(), row + i, col);
        if (per_page == 1) {
            c_prt(attr, format("%02x/%02x", flavor_baseitem.x_attr, flavor_baseitem.x_char), row + i, (w_ptr->wizard || visual_only) ? 64 : 68);
        }

        if (w_ptr->wizard || visual_only) {
            c_prt(attr, format("%d", bi_id), row + i, 70);
        }

        a = flavor_baseitem.x_attr;
        auto c = flavor_baseitem.x_char;

        term_queue_bigchar(use_bigtile ? 76 : 77, row + i, a, c, 0, 0);
    }

    for (; i < per_page; i++) {
        term_erase(col, row + i, 255);
    }
}

/*
 * Describe fake object
 */
static void desc_obj_fake(PlayerType *player_ptr, short bi_id)
{
    ItemEntity *o_ptr;
    ItemEntity ObjectType_body;
    o_ptr = &ObjectType_body;
    o_ptr->wipe();
    o_ptr->prep(bi_id);

    o_ptr->ident |= IDENT_KNOWN;
    handle_stuff(player_ptr);

    if (screen_object(player_ptr, o_ptr, SCROBJ_FAKE_OBJECT | SCROBJ_FORCE_DETAIL)) {
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
    short grp_idx[100];
    int object_cnt;

    bool visual_list = false;
    TERM_COLOR attr_top = 0;
    byte char_left = 0;
    byte mode;

    TERM_LEN wid, hgt;
    term_get_size(&wid, &hgt);

    int browser_rows = hgt - 8;
    std::vector<short> object_idx(baseitems_info.size());

    int len;
    int max = 0;
    int grp_cnt = 0;
    if (direct_k_idx < 0) {
        mode = visual_only ? 0x03 : 0x01;
        for (IDX i = 0; object_group_text[i] != nullptr; i++) {
            len = strlen(object_group_text[i]);
            if (len > max) {
                max = len;
            }

            if (collect_objects(i, object_idx, mode)) {
                grp_idx[grp_cnt++] = i;
            }
        }

        object_old = -1;
        object_cnt = 0;
    } else {
        auto &baseitem = baseitems_info[direct_k_idx];
        auto &flavor_baseitem = !visual_only && baseitem.flavor ? baseitems_info[baseitem.flavor] : baseitem;
        object_idx[0] = direct_k_idx;
        object_old = direct_k_idx;
        object_cnt = 1;
        object_idx[1] = -1;
        const auto height = browser_rows - 1;
        const auto width = wid - (max + 3);
        auto *x_attr = &flavor_baseitem.x_attr;
        auto *x_char = &flavor_baseitem.x_char;
        (void)visual_mode_command(
            'v', &visual_list, height, width, &attr_top, &char_left, x_attr, x_char, need_redraw);
    }

    grp_idx[grp_cnt] = -1;
    mode = visual_only ? 0x02 : 0x00;
    IDX old_grp_cur = -1;
    IDX grp_cur = 0;
    IDX grp_top = 0;
    IDX object_cur = object_top = 0;
    bool flag = false;
    bool redraw = true;
    int column = 0;
    while (!flag) {
        if (redraw) {
            clear_from(0);

#ifdef JP
            prt(format("%s - アイテム", !visual_only ? "知識" : "表示"), 2, 0);
            if (direct_k_idx < 0) {
                prt("グループ", 4, 0);
            }
            prt("名前", 4, max + 3);
            if (w_ptr->wizard || visual_only) {
                prt("Idx", 4, 70);
            }
            prt("文字", 4, 74);
#else
            prt(format("%s - objects", !visual_only ? "Knowledge" : "Visuals"), 2, 0);
            if (direct_k_idx < 0) {
                prt("Group", 4, 0);
            }
            prt("Name", 4, max + 3);
            if (w_ptr->wizard || visual_only) {
                prt("Idx", 4, 70);
            }
            prt("Sym", 4, 75);
#endif

            for (IDX i = 0; i < 78; i++) {
                term_putch(i, 5, TERM_WHITE, '=');
            }

            if (direct_k_idx < 0) {
                for (IDX i = 0; i < browser_rows; i++) {
                    term_putch(max + 1, 6 + i, TERM_WHITE, '|');
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

            std::vector<concptr> tmp_texts;
            for (auto &text : object_group_text) {
                tmp_texts.push_back(text);
            }

            display_group_list(0, 6, max, browser_rows, grp_idx, tmp_texts.data(), grp_cur, grp_top);
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
            display_object_list(max + 3, 6, browser_rows, object_idx, object_cur, object_top, visual_only);
        } else {
            object_top = object_cur;
            display_object_list(max + 3, 6, 1, object_idx, object_cur, object_top, visual_only);
            display_visual_list(max + 3, 7, browser_rows - 1, wid - (max + 3), attr_top, char_left);
        }

        auto &baseitem = baseitems_info[object_idx[object_cur]];
        auto &flavor_baseitem = !visual_only && baseitem.flavor ? baseitems_info[baseitem.flavor] : baseitem;

#ifdef JP
        prt(format("<方向>%s%s%s, ESC", (!visual_list && !visual_only) ? ", 'r'で詳細を見る" : "", visual_list ? ", ENTERで決定" : ", 'v'でシンボル変更",
                (attr_idx || char_idx) ? ", 'c', 'p'でペースト" : ", 'c'でコピー"),
            hgt - 1, 0);
#else
        prt(format("<dir>%s%s%s, ESC", (!visual_list && !visual_only) ? ", 'r' to recall" : "", visual_list ? ", ENTER to accept" : ", 'v' for visuals",
                (attr_idx || char_idx) ? ", 'c', 'p' to paste" : ", 'c' to copy"),
            hgt - 1, 0);
#endif

        if (!visual_only) {
            if (object_cnt) {
                object_kind_track(player_ptr, object_idx[object_cur]);
            }

            if (object_old != object_idx[object_cur]) {
                handle_stuff(player_ptr);
                object_old = object_idx[object_cur];
            }
        }

        if (visual_list) {
            place_visual_list_cursor(max + 3, 7, flavor_baseitem.x_attr, flavor_baseitem.x_char, attr_top, char_left);
        } else if (!column) {
            term_gotoxy(0, 6 + (grp_cur - grp_top));
        } else {
            term_gotoxy(max + 3, 6 + (object_cur - object_top));
        }

        char ch = inkey();
        const auto height = browser_rows - 1;
        const auto width = wid - (max + 3);
        auto *x_attr = &flavor_baseitem.x_attr;
        auto *x_char = &flavor_baseitem.x_char;
        if (visual_mode_command(
                ch, &visual_list, height, width, &attr_top, &char_left, x_attr, x_char, need_redraw)) {
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
            if (!visual_list && !visual_only && (grp_cnt > 0)) {
                desc_obj_fake(player_ptr, object_idx[object_cur]);
                redraw = true;
            }

            break;
        }

        default: {
            browser_cursor(ch, &column, &grp_cur, grp_cnt, &object_cur, object_cnt);
            break;
        }
        }
    }
}
