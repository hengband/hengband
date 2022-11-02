#include "cmd-io/cmd-macro.h"
#include "cmd-io/cmd-gameoption.h"
#include "cmd-io/macro-util.h"
#include "core/asking-player.h"
#include "game-option/input-options.h"
#include "io/files-util.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "io/read-pref-file.h"
#include "main/sound-of-music.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/angband-files.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

/*!
 * @brief マクロ情報をprefファイルに保存する /
 * @param fname ファイル名
 */
static void macro_dump(FILE **fpp, concptr fname)
{
    static concptr mark = "Macro Dump";
    char buf[1024];
    path_build(buf, sizeof(buf), ANGBAND_DIR_USER, fname);
    if (!open_auto_dump(fpp, buf, mark)) {
        return;
    }

    auto_dump_printf(*fpp, _("\n# 自動マクロセーブ\n\n", "\n# Automatic macro dump\n\n"));

    for (int i = 0; i < macro__num; i++) {
        ascii_to_text(buf, macro__act[i], sizeof(buf));
        auto_dump_printf(*fpp, "A:%s\n", buf);
        ascii_to_text(buf, macro__pat[i], sizeof(buf));
        auto_dump_printf(*fpp, "P:%s\n", buf);
        auto_dump_printf(*fpp, "\n");
    }

    close_auto_dump(fpp, mark);
}

/*!
 * @brief マクロのトリガーキーを取得する /
 * Hack -- ask for a "trigger" (see below)
 * @param buf キー表記を保管するバッファ
 * @details
 * <pre>
 * Note the complex use of the "inkey()" function from "util.c".
 *
 * Note that both "flush()" calls are extremely important.
 * </pre>
 */
static void do_cmd_macro_aux(char *buf)
{
    flush();
    inkey_base = true;
    char i = inkey();
    int n = 0;
    while (i) {
        buf[n++] = i;
        inkey_base = true;
        inkey_scan = true;
        i = inkey();
    }

    buf[n] = '\0';
    flush();
    char tmp[1024];
    ascii_to_text(tmp, buf, sizeof(tmp));
    term_addstr(-1, TERM_WHITE, tmp);
}

/*!
 * @brief マクロのキー表記からアスキーコードを得てターミナルに表示する /
 * Hack -- ask for a keymap "trigger" (see below)
 * @param buf キー表記を取得するバッファ
 * @details
 * <pre>
 * Note that both "flush()" calls are extremely important.  This may
 * no longer be true, since "util.c" is much simpler now.
 * </pre>
 */
static void do_cmd_macro_aux_keymap(char *buf)
{
    char tmp[1024];
    flush();
    buf[0] = inkey();
    buf[1] = '\0';
    ascii_to_text(tmp, buf, sizeof(tmp));
    term_addstr(-1, TERM_WHITE, tmp);
    flush();
}

/*!
 * @brief キーマップをprefファイルにダンプする /
 * Hack -- append all keymaps to the given file
 * @param fname ファイルネーム
 * @return エラーコード
 * @details
 */
static errr keymap_dump(concptr fname)
{
    FILE *auto_dump_stream;
    static concptr mark = "Keymap Dump";
    char key[1024];
    char buf[1024];
    BIT_FLAGS mode;
    if (rogue_like_commands) {
        mode = KEYMAP_MODE_ROGUE;
    } else {
        mode = KEYMAP_MODE_ORIG;
    }

    path_build(buf, sizeof(buf), ANGBAND_DIR_USER, fname);
    if (!open_auto_dump(&auto_dump_stream, buf, mark)) {
        return -1;
    }

    auto_dump_printf(auto_dump_stream, _("\n# 自動キー配置セーブ\n\n", "\n# Automatic keymap dump\n\n"));
    for (int i = 0; i < 256; i++) {
        concptr act;
        act = keymap_act[mode][i];
        if (!act) {
            continue;
        }

        buf[0] = (char)i;
        buf[1] = '\0';
        ascii_to_text(key, buf, sizeof(key));
        ascii_to_text(buf, act, sizeof(buf));
        auto_dump_printf(auto_dump_stream, "A:%s\n", buf);
        auto_dump_printf(auto_dump_stream, "C:%d:%s\n", mode, key);
    }

    close_auto_dump(&auto_dump_stream, mark);
    return 0;
}

/*!
 * @brief マクロを設定するコマンドのメインルーチン /
 * Interact with "macros"
 * @details
 * <pre>
 * Note that the macro "action" must be defined before the trigger.
 *
 * Could use some helpful instructions on this page.
 * </pre>
 */
void do_cmd_macros(PlayerType *player_ptr)
{
    char tmp[1024];
    char buf[1024];
    static char macro_buf[1024];
    FILE *auto_dump_stream;
    BIT_FLAGS mode = rogue_like_commands ? KEYMAP_MODE_ROGUE : KEYMAP_MODE_ORIG;
    screen_save();
    term_clear();

    auto print_macro_menu = [] {
        prt(_("[ マクロの設定 ]", "Interact with Macros"), 2, 0);
        prt(_("(1) ユーザー設定ファイルのロード", "(1) Load a user pref file"), 4, 5);
        prt(_("(2) ファイルにマクロを追加", "(2) Append macros to a file"), 5, 5);
        prt(_("(3) マクロの確認", "(3) Query a macro"), 6, 5);
        prt(_("(4) マクロの作成", "(4) Create a macro"), 7, 5);
        prt(_("(5) マクロの削除", "(5) Remove a macro"), 8, 5);
        prt(_("(6) ファイルにキー配置を追加", "(6) Append keymaps to a file"), 9, 5);
        prt(_("(7) キー配置の確認", "(7) Query a keymap"), 10, 5);
        prt(_("(8) キー配置の作成", "(8) Create a keymap"), 11, 5);
        prt(_("(9) キー配置の削除", "(9) Remove a keymap"), 12, 5);
        prt(_("(0) マクロ行動の入力", "(0) Enter a new action"), 13, 5);
    };
    print_macro_menu();

    while (true) {
        msg_print(_("コマンド: ", "Command: "));
        const int key = inkey();
        if (key == ESCAPE) {
            break;
        }
        clear_from(1);
        print_macro_menu();

        if (key == '1') {
            prt(_("コマンド: ユーザー設定ファイルのロード", "Command: Load a user pref file"), 16, 0);
            prt(_("ファイル: ", "File: "), 18, 0);
            sprintf(tmp, "%s.prf", player_ptr->base_name);
            if (!askfor(tmp, 80)) {
                continue;
            }

            errr err = process_pref_file(player_ptr, tmp, true);
            if (-2 == err) {
                msg_format(_("標準の設定ファイル'%s'を読み込みました。", "Loaded default '%s'."), tmp);
            } else if (err) {
                msg_format(_("'%s'の読み込みに失敗しました！", "Failed to load '%s'!"), tmp);
            } else {
                msg_format(_("'%s'を読み込みました。", "Loaded '%s'."), tmp);
            }
        } else if (key == '2') {
            prt(_("コマンド: マクロをファイルに追加する", "Command: Append macros to a file"), 16, 0);
            prt(_("ファイル: ", "File: "), 18, 0);
            sprintf(tmp, "%s.prf", player_ptr->base_name);
            if (!askfor(tmp, 80)) {
                continue;
            }

            macro_dump(&auto_dump_stream, tmp);
            msg_print(_("マクロを追加しました。", "Appended macros."));
        } else if (key == '3') {
            prt(_("コマンド: マクロの確認", "Command: Query a macro"), 16, 0);
            prt(_("マクロ行動が(もしあれば)下に表示されます:", "Current action (if any) shown below:"), 20, 0);
            prt(_("トリガーキー: ", "Trigger: "), 18, 0);
            do_cmd_macro_aux(buf);
            int k = macro_find_exact(buf);
            if (k < 0) {
                msg_print(_("そのキーにはマクロは定義されていません。", "Found no macro."));
            } else {
                // マクロの作成時に参照するためmacro_bufにコピーする
                strncpy(macro_buf, macro__act[k].data(), sizeof(macro_buf) - 1);
                // too long macro must die
                strncpy(tmp, macro_buf, 80);
                tmp[80] = '\0';
                ascii_to_text(buf, tmp, sizeof(buf));
                prt(buf, 22, 0);
                msg_print(_("マクロを確認しました。", "Found a macro."));
            }
        } else if (key == '4') {
            prt(_("コマンド: マクロの作成", "Command: Create a macro"), 16, 0);
            prt(_("トリガーキー: ", "Trigger: "), 18, 0);
            do_cmd_macro_aux(buf);
            c_prt(TERM_L_RED,
                _("カーソルキーの左右でカーソル位置を移動。BackspaceかDeleteで一文字削除。",
                    "Press Left/Right arrow keys to move cursor. Backspace/Delete to delete a char."),
                22, 0);
            prt(_("マクロ行動: ", "Action: "), 20, 0);
            // 最後に参照したマクロデータを元に作成する（コピーを行えるように）
            macro_buf[80] = '\0';
            ascii_to_text(tmp, macro_buf, sizeof(tmp));
            if (askfor(tmp, 80)) {
                text_to_ascii(macro_buf, tmp, sizeof(macro_buf));
                macro_add(buf, macro_buf);
                msg_print(_("マクロを追加しました。", "Added a macro."));
            }
        } else if (key == '5') {
            prt(_("コマンド: マクロの削除", "Command: Remove a macro"), 16, 0);
            prt(_("トリガーキー: ", "Trigger: "), 18, 0);
            do_cmd_macro_aux(buf);
            macro_add(buf, buf);
            msg_print(_("マクロを削除しました。", "Removed a macro."));
        } else if (key == '6') {
            prt(_("コマンド: キー配置をファイルに追加する", "Command: Append keymaps to a file"), 16, 0);
            prt(_("ファイル: ", "File: "), 18, 0);
            sprintf(tmp, "%s.prf", player_ptr->base_name);
            if (!askfor(tmp, 80)) {
                continue;
            }

            (void)keymap_dump(tmp);
            msg_print(_("キー配置を追加しました。", "Appended keymaps."));
        } else if (key == '7') {
            prt(_("コマンド: キー配置の確認", "Command: Query a keymap"), 16, 0);
            prt(_("マクロ行動が(もしあれば)下に表示されます:", "Current action (if any) shown below:"), 20, 0);
            prt(_("押すキー: ", "Keypress: "), 18, 0);
            do_cmd_macro_aux_keymap(buf);
            concptr act = keymap_act[mode][(byte)(buf[0])];
            if (!act) {
                msg_print(_("キー配置は定義されていません。", "Found no keymap."));
            } else {
                // マクロの作成時に参照するためmacro_bufにコピーする
                strncpy(macro_buf, act, sizeof(macro_buf) - 1);
                // too long macro must die
                strncpy(tmp, macro_buf, 80);
                tmp[80] = '\0';
                ascii_to_text(buf, tmp, sizeof(buf));
                prt(buf, 22, 0);
                msg_print(_("キー配置を確認しました。", "Found a keymap."));
            }
        } else if (key == '8') {
            prt(_("コマンド: キー配置の作成", "Command: Create a keymap"), 16, 0);
            prt(_("押すキー: ", "Keypress: "), 18, 0);
            do_cmd_macro_aux_keymap(buf);
            c_prt(TERM_L_RED,
                _("カーソルキーの左右でカーソル位置を移動。BackspaceかDeleteで一文字削除。",
                    "Press Left/Right arrow keys to move cursor. Backspace/Delete to delete a char."),
                22, 0);
            prt(_("行動: ", "Action: "), 20, 0);
            // 最後に参照したマクロデータを元に作成する（コピーを行えるように）
            macro_buf[80] = '\0';
            ascii_to_text(tmp, macro_buf, sizeof(tmp));
            if (askfor(tmp, 80)) {
                text_to_ascii(macro_buf, tmp, sizeof(macro_buf));
                string_free(keymap_act[mode][(byte)(buf[0])]);
                keymap_act[mode][(byte)(buf[0])] = string_make(macro_buf);
                msg_print(_("キー配置を追加しました。", "Added a keymap."));
            }
        } else if (key == '9') {
            prt(_("コマンド: キー配置の削除", "Command: Remove a keymap"), 16, 0);
            prt(_("押すキー: ", "Keypress: "), 18, 0);
            do_cmd_macro_aux_keymap(buf);
            string_free(keymap_act[mode][(byte)(buf[0])]);
            keymap_act[mode][(byte)(buf[0])] = nullptr;
            msg_print(_("キー配置を削除しました。", "Removed a keymap."));
        } else if (key == '0') {
            prt(_("コマンド: マクロ行動の入力", "Command: Enter a new action"), 16, 0);
            c_prt(TERM_L_RED,
                _("カーソルキーの左右でカーソル位置を移動。BackspaceかDeleteで一文字削除。",
                    "Press Left/Right arrow keys to move cursor. Backspace/Delete to delete a char."),
                22, 0);
            prt(_("マクロ行動: ", "Action: "), 20, 0);
            buf[0] = '\0';
            if (!askfor(buf, 80)) {
                continue;
            }

            text_to_ascii(macro_buf, buf, sizeof(macro_buf));
        } else {
            bell();
        }

        msg_erase();
    }

    screen_load();
}
