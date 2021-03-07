#include "birth/history-editor.h"
#include "io/input-key-acceptor.h"
#include "io/read-pref-file.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/int-char-converter.h"
#include "view/display-player.h" // 暫定。後で消す.
#ifdef  JP
#include "locale/japanese.h"
#endif

/*!
 * @brief 生い立ちメッセージを編集する。/Character background edit-mode
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param process_autopick_file_command 自動拾いファイルコマンドへの関数ポインタ
 * @return なし
 */
void edit_history(player_type *creature_ptr, void (*process_autopick_file_command)(char *))
{
    char old_history[4][60];
    for (int i = 0; i < 4; i++) {
        sprintf(old_history[i], "%s", creature_ptr->history[i]);
    }

    for (int i = 0; i < 4; i++) {
        /* loop */
        int j;
        for (j = 0; creature_ptr->history[i][j]; j++)
            ;

        for (; j < 59; j++)
            creature_ptr->history[i][j] = ' ';
        creature_ptr->history[i][59] = '\0';
    }

    display_player(creature_ptr, 1);
    c_put_str(TERM_L_GREEN, _("(キャラクターの生い立ち - 編集モード)", "(Character Background - Edit Mode)"), 11, 20);
    put_str(_("[ カーソルキーで移動、Enterで終了、Ctrl-Aでファイル読み込み ]", "[ Cursor key for Move, Enter for End, Ctrl-A for Read pref ]"), 17, 10);
    TERM_LEN y = 0;
    TERM_LEN x = 0;
    while (TRUE) {
        char c;

        for (int i = 0; i < 4; i++) {
            put_str(creature_ptr->history[i], i + 12, 10);
        }
#ifdef JP
        if (iskanji2(creature_ptr->history[y], x))
            c_put_str(TERM_L_BLUE, format("%c%c", creature_ptr->history[y][x], creature_ptr->history[y][x + 1]), y + 12, x + 10);
        else
#endif
            c_put_str(TERM_L_BLUE, format("%c", creature_ptr->history[y][x]), y + 12, x + 10);

        term_gotoxy(x + 10, y + 12);
        int skey = inkey_special(TRUE);
        if (!(skey & SKEY_MASK))
            c = (char)skey;
        else
            c = 0;

        if (skey == SKEY_UP || c == KTRL('p')) {
            y--;
            if (y < 0)
                y = 3;
#ifdef JP
            if ((x > 0) && (iskanji2(creature_ptr->history[y], x - 1)))
                x--;
#endif
        } else if (skey == SKEY_DOWN || c == KTRL('n')) {
            y++;
            if (y > 3)
                y = 0;
#ifdef JP
            if ((x > 0) && (iskanji2(creature_ptr->history[y], x - 1)))
                x--;
#endif
        } else if (skey == SKEY_RIGHT || c == KTRL('f')) {
#ifdef JP
            if (iskanji2(creature_ptr->history[y], x))
                x++;
#endif
            x++;
            if (x > 58) {
                x = 0;
                if (y < 3)
                    y++;
            }
        } else if (skey == SKEY_LEFT || c == KTRL('b')) {
            x--;
            if (x < 0) {
                if (y) {
                    y--;
                    x = 58;
                } else
                    x = 0;
            }

#ifdef JP
            if ((x > 0) && (iskanji2(creature_ptr->history[y], x - 1)))
                x--;
#endif
        } else if (c == '\r' || c == '\n') {
            term_erase(0, 11, 255);
            term_erase(0, 17, 255);
            put_str(_("(キャラクターの生い立ち - 編集済み)", "(Character Background - Edited)"), 11, 20);
            break;
        } else if (c == ESCAPE) {
            clear_from(11);
            put_str(_("(キャラクターの生い立ち)", "(Character Background)"), 11, 25);
            for (int i = 0; i < 4; i++) {
                sprintf(creature_ptr->history[i], "%s", old_history[i]);
                put_str(creature_ptr->history[i], i + 12, 10);
            }

            break;
        } else if (c == KTRL('A')) {
            if (read_histpref(creature_ptr, process_autopick_file_command)) {
#ifdef JP
                if ((x > 0) && (iskanji2(creature_ptr->history[y], x - 1)))
                    x--;
#endif
            }
        } else if (c == '\010') {
            x--;
            if (x < 0) {
                if (y) {
                    y--;
                    x = 58;
                } else
                    x = 0;
            }

            creature_ptr->history[y][x] = ' ';
#ifdef JP
            if ((x > 0) && (iskanji2(creature_ptr->history[y], x - 1))) {
                x--;
                creature_ptr->history[y][x] = ' ';
            }
#endif
        }
#ifdef JP
        else if (iskanji(c) || isprint(c))
#else
        else if (isprint(c)) /* BUGFIX */
#endif
        {
#ifdef JP
            if (iskanji2(creature_ptr->history[y], x)) {
                creature_ptr->history[y][x + 1] = ' ';
            }

            if (iskanji(c)) {
                if (x > 57) {
                    x = 0;
                    y++;
                    if (y > 3)
                        y = 0;
                }

                if (iskanji2(creature_ptr->history[y], x + 1)) {
                    creature_ptr->history[y][x + 2] = ' ';
                }

                creature_ptr->history[y][x++] = c;

                c = inkey();
            }
#endif
            creature_ptr->history[y][x++] = c;
            if (x > 58) {
                x = 0;
                y++;
                if (y > 3)
                    y = 0;
            }
        }
    }
}
