#include "cmd-visual/cmd-visuals.h"
#include "cmd-visual/cmd-draw.h"
#include "core/asking-player.h"
#include "core/visuals-reseter.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "flavor/object-flavor.h"
#include "game-option/special-options.h"
#include "io/files-util.h"
#include "io/input-key-acceptor.h"
#include "io/read-pref-file.h"
#include "knowledge/knowledge-features.h"
#include "knowledge/knowledge-items.h"
#include "knowledge/knowledge-monsters.h"
#include "knowledge/lighting-level-table.h"
#include "main/sound-of-music.h"
#include "monster-race/monster-race.h"
#include "object/object-kind.h"
#include "system/monster-race-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/angband-files.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"

/*!
 * @brief キャラクタのビジュアルIDを変更する際の対象指定関数
 * @param i 指定対象となるキャラクタコード
 * @param num 指定されたビジュアルIDを返す参照ポインタ
 * @param max ビジュアルIDの最大数
 * @return 指定が実際に行われた場合TRUE、キャンセルされた場合FALSE
 */
static bool cmd_visuals_aux(int i, IDX *num, IDX max)
{
    if (iscntrl(i)) {
        char str[10] = "";
        sprintf(str, "%d", *num);
        if (!get_string(format("Input new number(0-%d): ", max - 1), str, 4))
            return false;

        IDX tmp = (IDX)strtol(str, nullptr, 0);
        if (tmp >= 0 && tmp < max)
            *num = tmp;
    } else if (isupper(i))
        *num = (*num + max - 1) % max;
    else
        *num = (*num + 1) % max;

    return true;
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
void do_cmd_visuals(player_type *player_ptr)
{
    FILE *auto_dump_stream;
    char tmp[160];
    char buf[1024];
    bool need_redraw = false;
    concptr empty_symbol = "<< ? >>";
    if (use_bigtile)
        empty_symbol = "<< ?? >>";

    screen_save();
    while (true) {
        term_clear();
        print_visuals_menu(nullptr);
        int i = inkey();
        if (i == ESCAPE)
            break;

        switch (i) {
        case '0': {
            prt(_("コマンド: ユーザー設定ファイルのロード", "Command: Load a user pref file"), 15, 0);
            prt(_("ファイル: ", "File: "), 17, 0);
            sprintf(tmp, "%s.prf", player_ptr->base_name);
            if (!askfor(tmp, 70))
                continue;

            (void)process_pref_file(player_ptr, tmp, true);
            need_redraw = true;
            break;
        }
        case '1': {
            static concptr mark = "Monster attr/chars";
            prt(_("コマンド: モンスターの[色/文字]をファイルに書き出します", "Command: Dump monster attr/chars"), 15, 0);
            prt(_("ファイル: ", "File: "), 17, 0);
            sprintf(tmp, "%s.prf", player_ptr->base_name);
            if (!askfor(tmp, 70))
                continue;

            path_build(buf, sizeof(buf), ANGBAND_DIR_USER, tmp);
            if (!open_auto_dump(&auto_dump_stream, buf, mark))
                continue;

            auto_dump_printf(auto_dump_stream, _("\n# モンスターの[色/文字]の設定\n\n", "\n# Monster attr/char definitions\n\n"));
            for (const auto& r_ref : r_info) {
                if (r_ref.name.empty())
                    continue;

                auto_dump_printf(auto_dump_stream, "# %s\n", r_ref.name.c_str());
                auto_dump_printf(auto_dump_stream, "R:%d:0x%02X/0x%02X\n\n", i, (byte)(r_ref.x_attr), (byte)(r_ref.x_char));
            }

            close_auto_dump(&auto_dump_stream, mark);
            msg_print(_("モンスターの[色/文字]をファイルに書き出しました。", "Dumped monster attr/chars."));
            break;
        }
        case '2': {
            static concptr mark = "Object attr/chars";
            prt(_("コマンド: アイテムの[色/文字]をファイルに書き出します", "Command: Dump object attr/chars"), 15, 0);
            prt(_("ファイル: ", "File: "), 17, 0);
            sprintf(tmp, "%s.prf", player_ptr->base_name);
            if (!askfor(tmp, 70))
                continue;

            path_build(buf, sizeof(buf), ANGBAND_DIR_USER, tmp);
            if (!open_auto_dump(&auto_dump_stream, buf, mark))
                continue;

            auto_dump_printf(auto_dump_stream, _("\n# アイテムの[色/文字]の設定\n\n", "\n# Object attr/char definitions\n\n"));
            for (const auto &k_ref : k_info) {
                GAME_TEXT o_name[MAX_NLEN];
                if (k_ref.name.empty())
                    continue;

                if (!k_ref.flavor) {
                    strip_name(o_name, k_ref.idx);
                } else {
                    object_type dummy;
                    dummy.prep(k_ref.idx);
                    describe_flavor(player_ptr, o_name, &dummy, OD_FORCE_FLAVOR);
                }

                auto_dump_printf(auto_dump_stream, "# %s\n", o_name);
                auto_dump_printf(auto_dump_stream, "K:%d:0x%02X/0x%02X\n\n", (int)k_ref.idx, (byte)(k_ref.x_attr), (byte)(k_ref.x_char));
            }

            close_auto_dump(&auto_dump_stream, mark);
            msg_print(_("アイテムの[色/文字]をファイルに書き出しました。", "Dumped object attr/chars."));
            break;
        }
        case '3': {
            static concptr mark = "Feature attr/chars";
            prt(_("コマンド: 地形の[色/文字]をファイルに書き出します", "Command: Dump feature attr/chars"), 15, 0);
            prt(_("ファイル: ", "File: "), 17, 0);
            sprintf(tmp, "%s.prf", player_ptr->base_name);
            if (!askfor(tmp, 70))
                continue;

            path_build(buf, sizeof(buf), ANGBAND_DIR_USER, tmp);
            if (!open_auto_dump(&auto_dump_stream, buf, mark))
                continue;

            auto_dump_printf(auto_dump_stream, _("\n# 地形の[色/文字]の設定\n\n", "\n# Feature attr/char definitions\n\n"));
            for (const auto &f_ref : f_info) {
                if (f_ref.name.empty())
                    continue;
                if (f_ref.mimic != f_ref.idx)
                    continue;

                auto_dump_printf(auto_dump_stream, "# %s\n", (f_ref.name.c_str()));
                auto_dump_printf(auto_dump_stream, "F:%d:0x%02X/0x%02X:0x%02X/0x%02X:0x%02X/0x%02X\n\n", f_ref.idx, (byte)(f_ref.x_attr[F_LIT_STANDARD]),
                    (byte)(f_ref.x_char[F_LIT_STANDARD]), (byte)(f_ref.x_attr[F_LIT_LITE]), (byte)(f_ref.x_char[F_LIT_LITE]),
                    (byte)(f_ref.x_attr[F_LIT_DARK]), (byte)(f_ref.x_char[F_LIT_DARK]));
            }

            close_auto_dump(&auto_dump_stream, mark);
            msg_print(_("地形の[色/文字]をファイルに書き出しました。", "Dumped feature attr/chars."));
            break;
        }
        case '4': {
            static concptr choice_msg = _("モンスターの[色/文字]を変更します", "Change monster attr/chars");
            static MONRACE_IDX r = 0;
            prt(format(_("コマンド: %s", "Command: %s"), choice_msg), 15, 0);
            while (true) {
                monster_race *r_ptr = &r_info[r];
                int c;
                IDX t;

                TERM_COLOR da = r_ptr->d_attr;
                byte dc = r_ptr->d_char;
                TERM_COLOR ca = r_ptr->x_attr;
                byte cc = r_ptr->x_char;

                term_putstr(5, 17, -1, TERM_WHITE, format(_("モンスター = %d, 名前 = %-40.40s", "Monster = %d, Name = %-40.40s"), r, r_ptr->name.c_str()));
                term_putstr(10, 19, -1, TERM_WHITE, format(_("初期値  色 / 文字 = %3u / %3u", "Default attr/char = %3u / %3u"), da, dc));
                term_putstr(40, 19, -1, TERM_WHITE, empty_symbol);
                term_queue_bigchar(43, 19, da, dc, 0, 0);
                term_putstr(10, 20, -1, TERM_WHITE, format(_("現在値  色 / 文字 = %3u / %3u", "Current attr/char = %3u / %3u"), ca, cc));
                term_putstr(40, 20, -1, TERM_WHITE, empty_symbol);
                term_queue_bigchar(43, 20, ca, cc, 0, 0);
                term_putstr(0, 22, -1, TERM_WHITE, _("コマンド (n/N/^N/a/A/^A/c/C/^C/v/V/^V): ", "Command (n/N/^N/a/A/^A/c/C/^C/v/V/^V): "));
                i = inkey();
                if (i == ESCAPE)
                    break;

                if (iscntrl(i))
                    c = 'a' + i - KTRL('A');
                else if (isupper(i))
                    c = 'a' + i - 'A';
                else
                    c = i;

                switch (c) {
                case 'n': {
                    IDX prev_r = r;
                    do {
                        if (!cmd_visuals_aux(i, &r, max_r_idx)) {
                            r = prev_r;
                            break;
                        }
                    } while (r_info[r].name.empty());
                }

                break;
                case 'a':
                    t = (int)r_ptr->x_attr;
                    (void)cmd_visuals_aux(i, &t, 256);
                    r_ptr->x_attr = (byte)t;
                    need_redraw = true;
                    break;
                case 'c':
                    t = (int)r_ptr->x_char;
                    (void)cmd_visuals_aux(i, &t, 256);
                    r_ptr->x_char = (byte)t;
                    need_redraw = true;
                    break;
                case 'v':
                    do_cmd_knowledge_monsters(player_ptr, &need_redraw, true, r);
                    term_clear();
                    print_visuals_menu(choice_msg);
                    break;
                }
            }

            break;
        }
        case '5': {
            static concptr choice_msg = _("アイテムの[色/文字]を変更します", "Change object attr/chars");
            static IDX k = 0;
            prt(format(_("コマンド: %s", "Command: %s"), choice_msg), 15, 0);
            while (true) {
                object_kind *k_ptr = &k_info[k];
                int c;
                IDX t;

                TERM_COLOR da = k_ptr->d_attr;
                SYMBOL_CODE dc = k_ptr->d_char;
                TERM_COLOR ca = k_ptr->x_attr;
                SYMBOL_CODE cc = k_ptr->x_char;

                term_putstr(5, 17, -1, TERM_WHITE,
                    format(
                        _("アイテム = %d, 名前 = %-40.40s", "Object = %d, Name = %-40.40s"), k, (!k_ptr->flavor ? k_ptr->name : k_ptr->flavor_name).c_str()));
                term_putstr(10, 19, -1, TERM_WHITE, format(_("初期値  色 / 文字 = %3d / %3d", "Default attr/char = %3d / %3d"), da, dc));
                term_putstr(40, 19, -1, TERM_WHITE, empty_symbol);
                term_queue_bigchar(43, 19, da, dc, 0, 0);
                term_putstr(10, 20, -1, TERM_WHITE, format(_("現在値  色 / 文字 = %3d / %3d", "Current attr/char = %3d / %3d"), ca, cc));
                term_putstr(40, 20, -1, TERM_WHITE, empty_symbol);
                term_queue_bigchar(43, 20, ca, cc, 0, 0);
                term_putstr(0, 22, -1, TERM_WHITE, _("コマンド (n/N/^N/a/A/^A/c/C/^C/v/V/^V): ", "Command (n/N/^N/a/A/^A/c/C/^C/v/V/^V): "));

                i = inkey();
                if (i == ESCAPE)
                    break;

                if (iscntrl(i))
                    c = 'a' + i - KTRL('A');
                else if (isupper(i))
                    c = 'a' + i - 'A';
                else
                    c = i;

                switch (c) {
                case 'n': {
                    IDX prev_k = k;
                    do {
                        if (!cmd_visuals_aux(i, &k, max_k_idx)) {
                            k = prev_k;
                            break;
                        }
                    } while (k_info[k].name.empty());
                }

                break;
                case 'a':
                    t = (int)k_ptr->x_attr;
                    (void)cmd_visuals_aux(i, &t, 256);
                    k_ptr->x_attr = (byte)t;
                    need_redraw = true;
                    break;
                case 'c':
                    t = (int)k_ptr->x_char;
                    (void)cmd_visuals_aux(i, &t, 256);
                    k_ptr->x_char = (byte)t;
                    need_redraw = true;
                    break;
                case 'v':
                    do_cmd_knowledge_objects(player_ptr, &need_redraw, true, k);
                    term_clear();
                    print_visuals_menu(choice_msg);
                    break;
                }
            }

            break;
        }
        case '6': {
            static concptr choice_msg = _("地形の[色/文字]を変更します", "Change feature attr/chars");
            static IDX f = 0;
            static IDX lighting_level = F_LIT_STANDARD;
            prt(format(_("コマンド: %s", "Command: %s"), choice_msg), 15, 0);
            while (true) {
                feature_type *f_ptr = &f_info[f];
                int c;
                IDX t;

                TERM_COLOR da = f_ptr->d_attr[lighting_level];
                byte dc = f_ptr->d_char[lighting_level];
                TERM_COLOR ca = f_ptr->x_attr[lighting_level];
                byte cc = f_ptr->x_char[lighting_level];

                prt("", 17, 5);
                term_putstr(5, 17, -1, TERM_WHITE,
                    format(_("地形 = %d, 名前 = %s, 明度 = %s", "Terrain = %d, Name = %s, Lighting = %s"), f, (f_ptr->name.c_str()),
                        lighting_level_str[lighting_level]));
                term_putstr(10, 19, -1, TERM_WHITE, format(_("初期値  色 / 文字 = %3d / %3d", "Default attr/char = %3d / %3d"), da, dc));
                term_putstr(40, 19, -1, TERM_WHITE, empty_symbol);
                term_queue_bigchar(43, 19, da, dc, 0, 0);
                term_putstr(10, 20, -1, TERM_WHITE, format(_("現在値  色 / 文字 = %3d / %3d", "Current attr/char = %3d / %3d"), ca, cc));
                term_putstr(40, 20, -1, TERM_WHITE, empty_symbol);
                term_queue_bigchar(43, 20, ca, cc, 0, 0);
                term_putstr(0, 22, -1, TERM_WHITE,
                    _("コマンド (n/N/^N/a/A/^A/c/C/^C/l/L/^L/d/D/^D/v/V/^V): ", "Command (n/N/^N/a/A/^A/c/C/^C/l/L/^L/d/D/^D/v/V/^V): "));

                i = inkey();
                if (i == ESCAPE)
                    break;

                if (iscntrl(i))
                    c = 'a' + i - KTRL('A');
                else if (isupper(i))
                    c = 'a' + i - 'A';
                else
                    c = i;

                switch (c) {
                case 'n': {
                    IDX prev_f = f;
                    do {
                        if (!cmd_visuals_aux(i, &f, max_f_idx)) {
                            f = prev_f;
                            break;
                        }
                    } while (f_info[f].name.empty() || (f_info[f].mimic != f));
                }

                break;
                case 'a':
                    t = (int)f_ptr->x_attr[lighting_level];
                    (void)cmd_visuals_aux(i, &t, 256);
                    f_ptr->x_attr[lighting_level] = (byte)t;
                    need_redraw = true;
                    break;
                case 'c':
                    t = (int)f_ptr->x_char[lighting_level];
                    (void)cmd_visuals_aux(i, &t, 256);
                    f_ptr->x_char[lighting_level] = (byte)t;
                    need_redraw = true;
                    break;
                case 'l':
                    (void)cmd_visuals_aux(i, &lighting_level, F_LIT_MAX);
                    break;
                case 'd':
                    apply_default_feat_lighting(f_ptr->x_attr, f_ptr->x_char);
                    need_redraw = true;
                    break;
                case 'v':
                    do_cmd_knowledge_features(&need_redraw, true, f, &lighting_level);
                    term_clear();
                    print_visuals_menu(choice_msg);
                    break;
                }
            }

            break;
        }
        case '7':
            do_cmd_knowledge_monsters(player_ptr, &need_redraw, true, -1);
            break;
        case '8':
            do_cmd_knowledge_objects(player_ptr, &need_redraw, true, -1);
            break;
        case '9': {
            IDX lighting_level = F_LIT_STANDARD;
            do_cmd_knowledge_features(&need_redraw, true, -1, &lighting_level);
            break;
        }
        case 'R':
        case 'r':
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
    if (need_redraw)
        do_cmd_redraw(player_ptr);
}
