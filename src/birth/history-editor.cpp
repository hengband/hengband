#include "birth/history-editor.h"
#include "io/input-key-acceptor.h"
#include "io/read-pref-file.h"
#include "locale/japanese.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include "view/display-player.h" // 暫定。後で消す.

/*!
 * @brief 生い立ちメッセージを編集する。/Character background edit-mode
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void edit_history(PlayerType *player_ptr)
{
    std::string old_history[4];
    for (int i = 0; i < 4; i++) {
        old_history[i] = player_ptr->history[i];
    }

    for (int i = 0; i < 4; i++) {
        /* loop */
        int j;
        for (j = 0; player_ptr->history[i][j]; j++) {
            ;
        }

        for (; j < 59; j++) {
            player_ptr->history[i][j] = ' ';
        }
        player_ptr->history[i][59] = '\0';
    }

    (void)display_player(player_ptr, 1);
    c_put_str(TERM_L_GREEN, _("(キャラクターの生い立ち - 編集モード)", "(Character Background - Edit Mode)"), 11, 20);
    put_str(_("[ カーソルキーで移動、Enterで終了、Ctrl-Aでファイル読み込み ]", "[ Cursor key for Move, Enter for End, Ctrl-A for Read pref ]"), 17, 10);
    TERM_LEN y = 0;
    TERM_LEN x = 0;
    while (true) {
        char c;

        for (int i = 0; i < 4; i++) {
            put_str(player_ptr->history[i], i + 12, 10);
        }
#ifdef JP
        if (iskanji2(player_ptr->history[y], x)) {
            char kanji[3] = { player_ptr->history[y][x], player_ptr->history[y][x + 1], '\0' };
            c_put_str(TERM_L_BLUE, format("%s", kanji), y + 12, x + 10);
        } else
#endif
            c_put_str(TERM_L_BLUE, format("%c", player_ptr->history[y][x]), y + 12, x + 10);

        term_gotoxy(x + 10, y + 12);
        int skey = inkey_special(true);
        if (!(skey & SKEY_MASK)) {
            c = (char)skey;
        } else {
            c = 0;
        }

        if (skey == SKEY_UP || c == KTRL('p')) {
            y--;
            if (y < 0) {
                y = 3;
            }
#ifdef JP
            if ((x > 0) && (iskanji2(player_ptr->history[y], x - 1))) {
                x--;
            }
#endif
        } else if (skey == SKEY_DOWN || c == KTRL('n')) {
            y++;
            if (y > 3) {
                y = 0;
            }
#ifdef JP
            if ((x > 0) && (iskanji2(player_ptr->history[y], x - 1))) {
                x--;
            }
#endif
        } else if (skey == SKEY_RIGHT || c == KTRL('f')) {
#ifdef JP
            if (iskanji2(player_ptr->history[y], x)) {
                x++;
            }
#endif
            x++;
            if (x > 58) {
                x = 0;
                if (y < 3) {
                    y++;
                }
            }
        } else if (skey == SKEY_LEFT || c == KTRL('b')) {
            x--;
            if (x < 0) {
                if (y) {
                    y--;
                    x = 58;
                } else {
                    x = 0;
                }
            }

#ifdef JP
            if ((x > 0) && (iskanji2(player_ptr->history[y], x - 1))) {
                x--;
            }
#endif
        } else if (c == '\r' || c == '\n') {
            term_erase(0, 11);
            term_erase(0, 17);
            put_str(_("(キャラクターの生い立ち - 編集済み)", "(Character Background - Edited)"), 11, 20);
            break;
        } else if (c == ESCAPE) {
            clear_from(11);
            put_str(_("(キャラクターの生い立ち)", "(Character Background)"), 11, 25);
            for (int i = 0; i < 4; i++) {
                angband_strcpy(player_ptr->history[i], old_history[i], sizeof(player_ptr->history[i]));
                put_str(player_ptr->history[i], i + 12, 10);
            }

            break;
        } else if (c == KTRL('A')) {
            if (read_histpref(player_ptr)) {
#ifdef JP
                if ((x > 0) && (iskanji2(player_ptr->history[y], x - 1))) {
                    x--;
                }
#endif
            }
        } else if (c == '\010') {
            x--;
            if (x < 0) {
                if (y) {
                    y--;
                    x = 58;
                } else {
                    x = 0;
                }
            }

            player_ptr->history[y][x] = ' ';
#ifdef JP
            if ((x > 0) && (iskanji2(player_ptr->history[y], x - 1))) {
                x--;
                player_ptr->history[y][x] = ' ';
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
            if (iskanji2(player_ptr->history[y], x)) {
                player_ptr->history[y][x + 1] = ' ';
            }

            if (iskanji(c)) {
                if (x > 57) {
                    x = 0;
                    y++;
                    if (y > 3) {
                        y = 0;
                    }
                }

                if (iskanji2(player_ptr->history[y], x + 1)) {
                    player_ptr->history[y][x + 2] = ' ';
                }

                player_ptr->history[y][x++] = c;

                c = inkey();
            }
#endif
            player_ptr->history[y][x++] = c;
            if (x > 58) {
                x = 0;
                y++;
                if (y > 3) {
                    y = 0;
                }
            }
        }
    }
}
