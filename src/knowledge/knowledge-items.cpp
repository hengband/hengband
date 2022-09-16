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
#include "object/object-kind.h"
#include "perception/identification.h"
#include "perception/object-perception.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/angband-files.h"
#include "util/int-char-converter.h"
#include "util/sort.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <numeric>
#include <set>

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

    for (const auto &[a_idx, a_ref] : a_info) {
        if (a_ref.name.empty()) {
            continue;
        }
        if (!a_ref.is_generated) {
            continue;
        }

        known_list.insert(known_list.end(), a_idx);
    }

    for (POSITION y = 0; y < player_ptr->current_floor_ptr->height; y++) {
        for (POSITION x = 0; x < player_ptr->current_floor_ptr->width; x++) {
            auto *g_ptr = &player_ptr->current_floor_ptr->grid_array[y][x];
            for (const auto this_o_idx : g_ptr->o_idx_list) {
                ObjectType *o_ptr;
                o_ptr = &player_ptr->current_floor_ptr->o_list[this_o_idx];
                if (!o_ptr->is_fixed_artifact()) {
                    continue;
                }
                if (o_ptr->is_known()) {
                    continue;
                }

                known_list.erase(o_ptr->fixed_artifact_idx);
            }
        }
    }

    for (auto i = 0; i < INVEN_TOTAL; i++) {
        auto *o_ptr = &player_ptr->inventory_list[i];
        if (!o_ptr->k_idx) {
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
        const auto &a_ref = a_info.at(a_idx);
        GAME_TEXT base_name[MAX_NLEN];
        strcpy(base_name, _("未知の伝説のアイテム", "Unknown Artifact"));
        const auto z = lookup_kind(a_ref.tval, a_ref.sval);
        if (z != 0) {
            ObjectType forge;
            ObjectType *q_ptr;
            q_ptr = &forge;
            q_ptr->prep(z);
            q_ptr->fixed_artifact_idx = a_idx;
            q_ptr->ident |= IDENT_STORE;
            describe_flavor(player_ptr, base_name, q_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
        }

        fprintf(fff, _("     %s\n", "     The %s\n"), base_name);
    }

    angband_fclose(fff);
    (void)show_file(player_ptr, true, file_name, _("既知の伝説のアイテム", "Artifacts Seen"), 0, 0);
    fd_kill(file_name);
}

/*
 * Build a list of object indexes in the given group. Return the number
 * of objects in the group.
 *
 * mode & 0x01 : check for non-empty group
 * mode & 0x02 : visual operation only
 */
static KIND_OBJECT_IDX collect_objects(int grp_cur, KIND_OBJECT_IDX object_idx[], BIT_FLAGS8 mode)
{
    KIND_OBJECT_IDX object_cnt = 0;
    auto group_tval = object_group_tval[grp_cur];
    for (const auto &k_ref : k_info) {
        if (k_ref.name.empty()) {
            continue;
        }

        if (!(mode & 0x02)) {
            if (!w_ptr->wizard) {
                if (!k_ref.flavor) {
                    continue;
                }
                if (!k_ref.aware) {
                    continue;
                }
            }

            auto k = std::reduce(std::begin(k_ref.chance), std::end(k_ref.chance), 0);
            if (!k) {
                continue;
            }
        }

        if (group_tval == ItemKindType::LIFE_BOOK) {
            if (ItemKindType::LIFE_BOOK <= k_ref.tval && k_ref.tval <= ItemKindType::HEX_BOOK) {
                object_idx[object_cnt++] = k_ref.idx;
            } else {
                continue;
            }
        } else if (k_ref.tval == group_tval) {
            object_idx[object_cnt++] = k_ref.idx;
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
static void display_object_list(int col, int row, int per_page, IDX object_idx[], int object_cur, int object_top, bool visual_only)
{
    int i;
    for (i = 0; i < per_page && (object_idx[object_top + i] >= 0); i++) {
        GAME_TEXT o_name[MAX_NLEN];
        TERM_COLOR a;
        object_kind *flavor_k_ptr;
        KIND_OBJECT_IDX k_idx = object_idx[object_top + i];
        auto *k_ptr = &k_info[k_idx];
        TERM_COLOR attr = ((k_ptr->aware || visual_only) ? TERM_WHITE : TERM_SLATE);
        byte cursor = ((k_ptr->aware || visual_only) ? TERM_L_BLUE : TERM_BLUE);
        if (!visual_only && k_ptr->flavor) {
            flavor_k_ptr = &k_info[k_ptr->flavor];
        } else {
            flavor_k_ptr = k_ptr;
        }

        attr = ((i + object_top == object_cur) ? cursor : attr);
        if (!k_ptr->flavor || (!visual_only && k_ptr->aware)) {
            strip_name(o_name, k_idx);
        } else {
            strcpy(o_name, flavor_k_ptr->flavor_name.c_str());
        }

        c_prt(attr, o_name, row + i, col);
        if (per_page == 1) {
            c_prt(attr, format("%02x/%02x", flavor_k_ptr->x_attr, flavor_k_ptr->x_char), row + i, (w_ptr->wizard || visual_only) ? 64 : 68);
        }

        if (w_ptr->wizard || visual_only) {
            c_prt(attr, format("%d", k_idx), row + i, 70);
        }

        a = flavor_k_ptr->x_attr;
        auto c = flavor_k_ptr->x_char;

        term_queue_bigchar(use_bigtile ? 76 : 77, row + i, a, c, 0, 0);
    }

    for (; i < per_page; i++) {
        term_erase(col, row + i, 255);
    }
}

/*
 * Describe fake object
 */
static void desc_obj_fake(PlayerType *player_ptr, KIND_OBJECT_IDX k_idx)
{
    ObjectType *o_ptr;
    ObjectType ObjectType_body;
    o_ptr = &ObjectType_body;
    o_ptr->wipe();
    o_ptr->prep(k_idx);

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
void do_cmd_knowledge_objects(PlayerType *player_ptr, bool *need_redraw, bool visual_only, KIND_OBJECT_IDX direct_k_idx)
{
    KIND_OBJECT_IDX object_old, object_top;
    KIND_OBJECT_IDX grp_idx[100];
    int object_cnt;

    bool visual_list = false;
    TERM_COLOR attr_top = 0;
    byte char_left = 0;
    byte mode;

    TERM_LEN wid, hgt;
    term_get_size(&wid, &hgt);

    int browser_rows = hgt - 8;
    std::vector<KIND_OBJECT_IDX> object_idx(k_info.size());

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

            if (collect_objects(i, object_idx.data(), mode)) {
                grp_idx[grp_cnt++] = i;
            }
        }

        object_old = -1;
        object_cnt = 0;
    } else {
        auto *k_ptr = &k_info[direct_k_idx];
        object_kind *flavor_k_ptr;

        if (!visual_only && k_ptr->flavor) {
            flavor_k_ptr = &k_info[k_ptr->flavor];
        } else {
            flavor_k_ptr = k_ptr;
        }

        object_idx[0] = direct_k_idx;
        object_old = direct_k_idx;
        object_cnt = 1;
        object_idx[1] = -1;
        (void)visual_mode_command(
            'v', &visual_list, browser_rows - 1, wid - (max + 3), &attr_top, &char_left, &flavor_k_ptr->x_attr, &flavor_k_ptr->x_char, need_redraw);
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
        object_kind *k_ptr, *flavor_k_ptr;

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
                object_cnt = collect_objects(grp_idx[grp_cur], object_idx.data(), mode);
            }

            while (object_cur < object_top) {
                object_top = std::max<short>(0, object_top - browser_rows / 2);
            }

            while (object_cur >= object_top + browser_rows) {
                object_top = std::min<short>(object_cnt - browser_rows, object_top + browser_rows / 2);
            }
        }

        if (!visual_list) {
            display_object_list(max + 3, 6, browser_rows, object_idx.data(), object_cur, object_top, visual_only);
        } else {
            object_top = object_cur;
            display_object_list(max + 3, 6, 1, object_idx.data(), object_cur, object_top, visual_only);
            display_visual_list(max + 3, 7, browser_rows - 1, wid - (max + 3), attr_top, char_left);
        }

        k_ptr = &k_info[object_idx[object_cur]];

        if (!visual_only && k_ptr->flavor) {
            flavor_k_ptr = &k_info[k_ptr->flavor];
        } else {
            flavor_k_ptr = k_ptr;
        }

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
            place_visual_list_cursor(max + 3, 7, flavor_k_ptr->x_attr, flavor_k_ptr->x_char, attr_top, char_left);
        } else if (!column) {
            term_gotoxy(0, 6 + (grp_cur - grp_top));
        } else {
            term_gotoxy(max + 3, 6 + (object_cur - object_top));
        }

        char ch = inkey();
        if (visual_mode_command(
                ch, &visual_list, browser_rows - 1, wid - (max + 3), &attr_top, &char_left, &flavor_k_ptr->x_attr, &flavor_k_ptr->x_char, need_redraw)) {
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
