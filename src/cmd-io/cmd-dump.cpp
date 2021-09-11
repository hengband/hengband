/*!
 * @brief プレイヤーのインターフェイスに関するコマンドの実装 / Interface commands
 * @date 2014/01/02
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * 2020 Hourier Rearranged
 */

#include "cmd-io/cmd-dump.h"
#include "cmd-io/feeling-table.h"
#include "core/asking-player.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "floor/floor-town.h"
#include "io-dump/dump-remover.h"
#include "io-dump/dump-util.h"
#include "io/files-util.h"
#include "io/input-key-acceptor.h"
#include "io/interpret-pref-file.h"
#include "io/read-pref-file.h"
#include "locale/english.h"
#include "main/sound-of-music.h"
#include "mutation/mutation-flag-types.h"
#include "player/player-personality-types.h"
#include "player/player-status-flags.h"
#include "player/player-status.h"
#include "system/angband-version.h"
#include "system/floor-type-definition.h"
#include "system/player-type-definition.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/angband-files.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "view/display-player.h" // 暫定。後で消す.
#include "world/world.h"

/*!
 * @brief 画面を再描画するコマンドのメインルーチン
 * Hack -- redraw the screen
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details
 * Allow absolute file names?
 */
void do_cmd_pref(player_type *player_ptr)
{
    char buf[80];
    strcpy(buf, "");
    if (!get_string(_("設定変更コマンド: ", "Pref: "), buf, 80))
        return;

    (void)interpret_pref_file(player_ptr, buf);
}

/*
 * Interact with "colors"
 */
void do_cmd_colors(player_type *player_ptr)
{
    int i;
    char tmp[160];
    char buf[1024];
    FILE *auto_dump_stream;
    screen_save();
    while (true) {
        term_clear();
        prt(_("[ カラーの設定 ]", "Interact with Colors"), 2, 0);
        prt(_("(1) ユーザー設定ファイルのロード", "(1) Load a user pref file"), 4, 5);
        prt(_("(2) カラーの設定をファイルに書き出す", "(2) Dump colors"), 5, 5);
        prt(_("(3) カラーの設定を変更する", "(3) Modify colors"), 6, 5);
        prt(_("コマンド: ", "Command: "), 8, 0);
        i = inkey();
        if (i == ESCAPE)
            break;

        if (i == '1') {
            prt(_("コマンド: ユーザー設定ファイルをロードします", "Command: Load a user pref file"), 8, 0);
            prt(_("ファイル: ", "File: "), 10, 0);
            sprintf(tmp, "%s.prf", player_ptr->base_name);
            if (!askfor(tmp, 70))
                continue;

            (void)process_pref_file(player_ptr, tmp, true);
            term_xtra(TERM_XTRA_REACT, 0);
            term_redraw();
        } else if (i == '2') {
            static concptr mark = "Colors";
            prt(_("コマンド: カラーの設定をファイルに書き出します", "Command: Dump colors"), 8, 0);
            prt(_("ファイル: ", "File: "), 10, 0);
            sprintf(tmp, "%s.prf", player_ptr->base_name);
            if (!askfor(tmp, 70))
                continue;

            path_build(buf, sizeof(buf), ANGBAND_DIR_USER, tmp);
            if (!open_auto_dump(&auto_dump_stream, buf, mark))
                continue;

            auto_dump_printf(auto_dump_stream, _("\n# カラーの設定\n\n", "\n# Color redefinitions\n\n"));
            for (i = 0; i < 256; i++) {
                int kv = angband_color_table[i][0];
                int rv = angband_color_table[i][1];
                int gv = angband_color_table[i][2];
                int bv = angband_color_table[i][3];

                concptr name = _("未知", "unknown");
                if (!kv && !rv && !gv && !bv)
                    continue;

                if (i < 16)
                    name = color_names[i];

                auto_dump_printf(auto_dump_stream, _("# カラー '%s'\n", "# Color '%s'\n"), name);
                auto_dump_printf(auto_dump_stream, "V:%d:0x%02X:0x%02X:0x%02X:0x%02X\n\n", i, kv, rv, gv, bv);
            }

            close_auto_dump(&auto_dump_stream, mark);
            msg_print(_("カラーの設定をファイルに書き出しました。", "Dumped color redefinitions."));
        } else if (i == '3') {
            static byte a = 0;
            prt(_("コマンド: カラーの設定を変更します", "Command: Modify colors"), 8, 0);
            while (true) {
                concptr name;
                clear_from(10);
                for (byte j = 0; j < 16; j++) {
                    term_putstr(j * 4, 20, -1, a, "###");
                    term_putstr(j * 4, 22, -1, j, format("%3d", j));
                }

                name = ((a < 16) ? color_names[a] : _("未定義", "undefined"));
                term_putstr(5, 10, -1, TERM_WHITE, format(_("カラー = %d, 名前 = %s", "Color = %d, Name = %s"), a, name));
                term_putstr(5, 12, -1, TERM_WHITE,
                    format("K = 0x%02x / R,G,B = 0x%02x,0x%02x,0x%02x", angband_color_table[a][0], angband_color_table[a][1], angband_color_table[a][2],
                        angband_color_table[a][3]));
                term_putstr(0, 14, -1, TERM_WHITE, _("コマンド (n/N/k/K/r/R/g/G/b/B): ", "Command (n/N/k/K/r/R/g/G/b/B): "));
                i = inkey();
                if (i == ESCAPE)
                    break;

                if (i == 'n')
                    a = (byte)(a + 1);
                if (i == 'N')
                    a = (byte)(a - 1);
                if (i == 'k')
                    angband_color_table[a][0] = (byte)(angband_color_table[a][0] + 1);
                if (i == 'K')
                    angband_color_table[a][0] = (byte)(angband_color_table[a][0] - 1);
                if (i == 'r')
                    angband_color_table[a][1] = (byte)(angband_color_table[a][1] + 1);
                if (i == 'R')
                    angband_color_table[a][1] = (byte)(angband_color_table[a][1] - 1);
                if (i == 'g')
                    angband_color_table[a][2] = (byte)(angband_color_table[a][2] + 1);
                if (i == 'G')
                    angband_color_table[a][2] = (byte)(angband_color_table[a][2] - 1);
                if (i == 'b')
                    angband_color_table[a][3] = (byte)(angband_color_table[a][3] + 1);
                if (i == 'B')
                    angband_color_table[a][3] = (byte)(angband_color_table[a][3] - 1);

                term_xtra(TERM_XTRA_REACT, 0);
                term_redraw();
            }
        } else {
            bell();
        }

        msg_erase();
    }

    screen_load();
}

/*
 * Note something in the message recall
 */
void do_cmd_note(void)
{
    char buf[80];
    strcpy(buf, "");
    if (!get_string(_("メモ: ", "Note: "), buf, 60))
        return;
    if (!buf[0] || (buf[0] == ' '))
        return;

    msg_format(_("メモ: %s", "Note: %s"), buf);
}

/*
 * Mention the current version
 */
void do_cmd_version(void)
{
    char buf[120];
    put_version(buf);
    msg_print(buf);
}

/*
 * Note that "feeling" is set to zero unless some time has passed.
 * Note that this is done when the level is GENERATED, not entered.
 */
void do_cmd_feeling(player_type *player_ptr)
{
    if (player_ptr->wild_mode)
        return;

    if (player_ptr->current_floor_ptr->inside_quest && !random_quest_number(player_ptr, player_ptr->current_floor_ptr->dun_level)) {
        msg_print(_("典型的なクエストのダンジョンのようだ。", "Looks like a typical quest level."));
        return;
    }

    if (player_ptr->town_num && !player_ptr->current_floor_ptr->dun_level) {
        if (!strcmp(town_info[player_ptr->town_num].name, _("荒野", "wilderness"))) {
            msg_print(_("何かありそうな荒野のようだ。", "Looks like a strange wilderness."));
            return;
        }

        msg_print(_("典型的な町のようだ。", "Looks like a typical town."));
        return;
    }

    if (!is_in_dungeon(player_ptr)) {
        msg_print(_("典型的な荒野のようだ。", "Looks like a typical wilderness."));
        return;
    }

    if (has_good_luck(player_ptr))
        msg_print(do_cmd_feeling_text_lucky[player_ptr->feeling]);
    else if (is_echizen(player_ptr))
        msg_print(do_cmd_feeling_text_combat[player_ptr->feeling]);
    else
        msg_print(do_cmd_feeling_text[player_ptr->feeling]);
}

/*
 * Display the time and date
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void do_cmd_time(player_type *player_ptr)
{
    int day, hour, min;
    extract_day_hour_min(player_ptr, &day, &hour, &min);

    char desc[1024];
    strcpy(desc, _("変な時刻だ。", "It is a strange time."));

    char day_buf[20];
    if (day < MAX_DAYS)
        sprintf(day_buf, "%d", day);
    else
        strcpy(day_buf, "*****");

    msg_format(_("%s日目, 時刻は%d:%02d %sです。", "This is day %s. The time is %d:%02d %s."), day_buf, (hour % 12 == 0) ? 12 : (hour % 12), min,
        (hour < 12) ? "AM" : "PM");

    char buf[1024];
    if (!randint0(10) || player_ptr->image) {
        path_build(buf, sizeof(buf), ANGBAND_DIR_FILE, _("timefun_j.txt", "timefun.txt"));
    } else {
        path_build(buf, sizeof(buf), ANGBAND_DIR_FILE, _("timenorm_j.txt", "timenorm.txt"));
    }

    FILE *fff;
    fff = angband_fopen(buf, "rt");

    if (!fff)
        return;

    int full = hour * 100 + min;
    int start = 9999;
    int end = -9999;
    int num = 0;
    while (!angband_fgets(fff, buf, sizeof(buf))) {
        if (!buf[0] || (buf[0] == '#'))
            continue;
        if (buf[1] != ':')
            continue;

        if (buf[0] == 'S') {
            start = atoi(buf + 2);
            end = start + 59;
            continue;
        }

        if (buf[0] == 'E') {
            end = atoi(buf + 2);
            continue;
        }

        if ((start > full) || (full > end))
            continue;

        if (buf[0] == 'D') {
            num++;
            if (!randint0(num))
                strcpy(desc, buf + 2);

            continue;
        }
    }

    msg_print(desc);
    angband_fclose(fff);
}
