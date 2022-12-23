/*!
 * @brief 自動拾いエディタを表示させる
 * @date 2020/04/26
 * @author Hourier
 */

#include "autopick/autopick-drawer.h"
#include "autopick/autopick-describer.h"
#include "autopick/autopick-dirty-flags.h"
#include "autopick/autopick-entry.h"
#include "autopick/autopick-util.h"
#include "io/pref-file-expressor.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "view/display-util.h"

#define DESCRIPT_HGT 3

static void process_dirty_expression(PlayerType *player_ptr, text_body_type *tb)
{
    if ((tb->dirty_flags & DIRTY_EXPRESSION) == 0) {
        return;
    }

    byte state = 0;
    for (int y = 0; tb->lines_list[y]; y++) {
        concptr s = tb->lines_list[y];
        tb->states[y] = state;

        if (*s++ != '?') {
            continue;
        }
        if (*s++ != ':') {
            continue;
        }

        if (streq(s, "$AUTOREGISTER")) {
            state |= LSTAT_AUTOREGISTER;
        }

        auto s_keep = string_make(s);
        //! @note string_make の戻り値は const char* だが process_pref_file_expr で書き換える
        // 可能性があるのでconstを外す必要がある。バッファ領域内のみの操作なので安全なはず。
        auto ss = const_cast<char *>(s_keep);

        char f;
        concptr v = process_pref_file_expr(player_ptr, &ss, &f);
        if (streq(v, "0")) {
            state |= LSTAT_BYPASS;
        } else {
            state &= ~LSTAT_BYPASS;
        }

        string_free(s_keep);
        tb->states[y] = state | LSTAT_EXPRESSION;
    }

    tb->dirty_flags |= DIRTY_ALL;
}

/*!
 * @brief Draw text
 */
void draw_text_editor(PlayerType *player_ptr, text_body_type *tb)
{
    int by1 = 0, by2 = 0;

    term_get_size(&tb->wid, &tb->hgt);

    /*
     * Top line (-1), description line (-3), separator (-1)
     *  == -5
     */
    tb->hgt -= 2 + DESCRIPT_HGT;

#ifdef JP
    /* Don't let cursor at second byte of kanji */
    for (int i = 0; tb->lines_list[tb->cy][i]; i++) {
        if (iskanji(tb->lines_list[tb->cy][i])) {
            i++;
            if (i == tb->cx) {
                /*
                 * Move to a correct position in the
                 * left or right
                 */
                if (i & 1) {
                    tb->cx--;
                } else {
                    tb->cx++;
                }
                break;
            }
        }
    }
#endif
    if (tb->cy < tb->upper || tb->upper + tb->hgt <= tb->cy) {
        tb->upper = tb->cy - (tb->hgt) / 2;
    }
    if (tb->upper < 0) {
        tb->upper = 0;
    }
    if ((tb->cx < tb->left + 10 && tb->left > 0) || tb->left + tb->wid - 5 <= tb->cx) {
        tb->left = tb->cx - (tb->wid) * 2 / 3;
    }
    if (tb->left < 0) {
        tb->left = 0;
    }

    if (tb->old_wid != tb->wid || tb->old_hgt != tb->hgt) {
        tb->dirty_flags |= DIRTY_SCREEN;
    } else if (tb->old_upper != tb->upper || tb->old_left != tb->left) {
        tb->dirty_flags |= DIRTY_ALL;
    }

    if (tb->dirty_flags & DIRTY_SCREEN) {
        tb->dirty_flags |= (DIRTY_ALL | DIRTY_MODE);
        term_clear();
    }

    if (tb->dirty_flags & DIRTY_MODE) {
        char buf[MAX_LINELEN];
        int sepa_length = tb->wid;
        int j = 0;
        for (; j < sepa_length; j++) {
            buf[j] = '-';
        }
        buf[j] = '\0';
        term_putstr(0, tb->hgt + 1, sepa_length, TERM_WHITE, buf);
    }

    process_dirty_expression(player_ptr, tb);
    if (tb->mark) {
        tb->dirty_flags |= DIRTY_ALL;

        by1 = std::min(tb->my, tb->cy);
        by2 = std::max(tb->my, tb->cy);
    }

    int i;
    for (i = 0; i < tb->hgt; i++) {
        int leftcol = 0;
        concptr msg;
        byte color;
        int y = tb->upper + i;

        if (!(tb->dirty_flags & DIRTY_ALL) && (tb->dirty_line != y)) {
            continue;
        }

        msg = tb->lines_list[y];
        if (!msg) {
            break;
        }

        for (int j = 0; *msg; msg++, j++) {
            if (j == tb->left) {
                break;
            }
#ifdef JP
            if (j > tb->left) {
                leftcol = 1;
                break;
            }
            if (iskanji(*msg)) {
                msg++;
                j++;
            }
#endif
        }

        term_erase(0, i + 1, tb->wid);
        if (tb->states[y] & LSTAT_AUTOREGISTER) {
            color = TERM_L_RED;
        } else {
            if (tb->states[y] & LSTAT_BYPASS) {
                color = TERM_SLATE;
            } else {
                color = TERM_WHITE;
            }
        }

        if (!tb->mark || (y < by1 || by2 < y)) {
            term_putstr(leftcol, i + 1, tb->wid - 1, color, msg);
        } else if (by1 != by2) {
            term_putstr(leftcol, i + 1, tb->wid - 1, TERM_YELLOW, msg);
        } else {
            int x0 = leftcol + tb->left;
            int len = strlen(tb->lines_list[tb->cy]);
            int bx1 = std::min(tb->mx, tb->cx);
            int bx2 = std::max(tb->mx, tb->cx);

            if (bx2 > len) {
                bx2 = len;
            }

            term_gotoxy(leftcol, i + 1);
            if (x0 < bx1) {
                term_addstr(bx1 - x0, color, msg);
            }
            if (x0 < bx2) {
                term_addstr(bx2 - bx1, TERM_YELLOW, msg + (bx1 - x0));
            }
            term_addstr(-1, color, msg + (bx2 - x0));
        }
    }

    for (; i < tb->hgt; i++) {
        term_erase(0, i + 1, tb->wid);
    }

    bool is_dirty_diary = (tb->dirty_flags & (DIRTY_ALL | DIRTY_NOT_FOUND | DIRTY_NO_SEARCH)) != 0;
    bool is_updated = tb->old_cy != tb->cy || is_dirty_diary || tb->dirty_line == tb->cy;
    if (!is_updated) {
        return;
    }

    autopick_type an_entry, *entry = &an_entry;
    std::string str1, str2;
    for (int j = 0; j < DESCRIPT_HGT; j++) {
        term_erase(0, tb->hgt + 2 + j, tb->wid);
    }

    if (tb->dirty_flags & DIRTY_NOT_FOUND) {
        str1 = format(_("パターンが見つかりません: %s", "Pattern not found: %s"), tb->search_str);
    } else if (tb->dirty_flags & DIRTY_SKIP_INACTIVE) {
        str1 = format(_("無効状態の行をスキップしました。(%sを検索中)", "Some inactive lines are skipped. (Searching %s)"), tb->search_str);
    } else if (tb->dirty_flags & DIRTY_INACTIVE) {
        str1 = format(_("無効状態の行だけが見付かりました。(%sを検索中)", "Found only an inactive line. (Searching %s)"), tb->search_str);
    } else if (tb->dirty_flags & DIRTY_NO_SEARCH) {
        str1 = _("検索するパターンがありません(^S で検索)。", "No pattern to search. (Press ^S to search.)");
    } else if (tb->lines_list[tb->cy][0] == '#') {
        str1 = _("この行はコメントです。", "This line is a comment.");
    } else if (tb->lines_list[tb->cy][0] && tb->lines_list[tb->cy][1] == ':') {
        switch (tb->lines_list[tb->cy][0]) {
        case '?':
            str1 = _("この行は条件分岐式です。", "This line is a Conditional Expression.");
            break;
        case 'A':
            str1 = _("この行はマクロの実行内容を定義します。", "This line defines a Macro action.");
            break;
        case 'P':
            str1 = _("この行はマクロのトリガー・キーを定義します。", "This line defines a Macro trigger key.");
            break;
        case 'C':
            str1 = _("この行はキー配置を定義します。", "This line defines a Keymap.");
            break;
        }

        switch (tb->lines_list[tb->cy][0]) {
        case '?':
            if (tb->states[tb->cy] & LSTAT_BYPASS) {
                str2 = _("現在の式の値は「偽(=0)」です。", "The expression is 'False'(=0) currently.");
            } else {
                str2 = _("現在の式の値は「真(=1)」です。", "The expression is 'True'(=1) currently.");
            }
            break;

        default:
            if (tb->states[tb->cy] & LSTAT_AUTOREGISTER) {
                str2 = _("この行は後で削除されます。", "This line will be deleted later.");
            }

            else if (tb->states[tb->cy] & LSTAT_BYPASS) {
                str2 = _("この行は現在は無効な状態です。", "This line is bypassed currently.");
            }
            break;
        }
    } else if (autopick_new_entry(entry, tb->lines_list[tb->cy], false)) {
        char buf[MAX_LINELEN];

        describe_autopick(buf, entry);

        if (tb->states[tb->cy] & LSTAT_AUTOREGISTER) {
            strcat(buf, _("この行は後で削除されます。", "  This line will be deleted later."));
        }

        if (tb->states[tb->cy] & LSTAT_BYPASS) {
            strcat(buf, _("この行は現在は無効な状態です。", "  This line is bypassed currently."));
        }

        display_wrap_around(buf, 81, tb->hgt + 2, 0);
    }

    if (!str1.empty()) {
        prt(str1, tb->hgt + 1 + 1, 0);
    }
    if (!str2.empty()) {
        prt(str2, tb->hgt + 1 + 2, 0);
    }
}
