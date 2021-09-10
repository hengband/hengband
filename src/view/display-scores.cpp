#include "view/display-scores.h"
#include "core/score-util.h"
#include "io/files-util.h"
#include "io/input-key-acceptor.h"
#include "player-info/class-info.h"
#include "player/player-personality.h"
#include "player/race-info-table.h"
#include "system/angband.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/angband-files.h"
#include "util/int-char-converter.h"

#ifdef JP
#include "locale/japanese.h"
#endif

/*!
 * @brief 指定された順位範囲でスコアを並べて表示する / Display the scores in a given range.
 * @param from 順位先頭
 * @param to 順位末尾
 * @param note 黄色表示でハイライトする順位
 * @param score スコア配列参照ポインタ
 * @details
 * <pre>
 * Assumes the high score list is already open.
 * Only five entries per line, too much info.
 *
 * Mega-Hack -- allow "fake" entry at the given position.
 * </pre>
 */
void display_scores(int from, int to, int note, high_score *score)
{
    TERM_LEN wid, hgt;
    term_get_size(&wid, &hgt);
    auto per_screen = (TERM_LEN)((hgt - 4) / 4);
    if (highscore_fd < 0) {
        return;
    }

    if (from < 0) {
        from = 0;
    }

    if (to < 0) {
        to = 10;
    }

    if (to > MAX_HISCORES) {
        to = MAX_HISCORES;
    }

    if (highscore_seek(0)) {
        return;
    }

    auto num_scores = 0;
    high_score the_score;
    for (; num_scores < MAX_HISCORES; num_scores++) {
        if (highscore_read(&the_score)) {
            break;
        }
    }

    if ((note == num_scores) && score) {
        num_scores++;
    }

    if (num_scores > to) {
        num_scores = to;
    }

    for (auto k = from, place = k + 1; k < num_scores; k += per_screen) {
        term_clear();
        put_str(_("                変愚蛮怒: 勇者の殿堂", "                Hengband Hall of Fame"), 0, 0);
        GAME_TEXT tmp_val[160];
        if (k > 0) {
            sprintf(tmp_val, _("( %d 位以下 )", "(from position %d)"), k + 1);
            put_str(tmp_val, 0, 40);
        }

        for (auto n = 0, j = k; (j < num_scores) && (n < per_screen); place++, j++, n++) {
            auto attr = (j == note) ? TERM_YELLOW : TERM_WHITE;
            if ((note == j) && score) {
                the_score = (*score);
                attr = TERM_L_GREEN;
                score = nullptr;
                note = -1;
                j--;
            } else if (highscore_seek(j) || highscore_read(&the_score)) {
                break;
            }

            auto pr = atoi(the_score.p_r);
            auto pc = atoi(the_score.p_c);
            auto pa = atoi(the_score.p_a);

            auto clev = atoi(the_score.cur_lev);
            auto mlev = atoi(the_score.max_lev);
            auto cdun = atoi(the_score.cur_dun);
            auto mdun = atoi(the_score.max_dun);

            concptr user;
            for (user = the_score.uid; iswspace(*user); user++) /* loop */
                ;

            concptr when;
            for (when = the_score.day; iswspace(*when); when++) /* loop */
                ;

            concptr gold;
            for (gold = the_score.gold; iswspace(*gold); gold++) /* loop */
                ;

            concptr aged;
            for (aged = the_score.turns; iswspace(*aged); aged++) /* loop */
                ;

            if ((*when == '@') && strlen(when) == 9) {
                sprintf(tmp_val, "%.4s-%.2s-%.2s", when + 1, when + 5, when + 7);
                when = tmp_val;
            }

            GAME_TEXT out_val[256];
#ifdef JP
            /*sprintf(out_val, "%3d.%9s  %s%s%sという名の%sの%s (レベル %d)", */
            sprintf(out_val, "%3d.%9s  %s%s%s - %s%s (レベル %d)", place, the_score.pts, personality_info[pa].title, (personality_info[pa].no ? "の" : ""),
                the_score.who, race_info[pr].title, class_info[pc].title, clev);

#else
            sprintf(out_val, "%3d.%9s  %s %s the %s %s, Level %d", place, the_score.pts, personality_info[pa].title, the_score.who, race_info[pr].title,
                class_info[pc].title, clev);
#endif
            if (mlev > clev) {
                strcat(out_val, format(_(" (最高%d)", " (Max %d)"), mlev));
            }

            c_put_str(attr, out_val, n * 4 + 2, 0);
#ifdef JP
            if (mdun != 0) {
                sprintf(out_val, "    最高%3d階", mdun);
            } else {
                sprintf(out_val, "             ");
            }

            /* 死亡原因をオリジナルより細かく表示 */
            if (streq(the_score.how, "yet")) {
                sprintf(out_val + 13, "  まだ生きている (%d%s)", cdun, "階");
            } else if (streq(the_score.how, "ripe")) {
                sprintf(out_val + 13, "  勝利の後に引退 (%d%s)", cdun, "階");
            } else if (streq(the_score.how, "Seppuku")) {
                sprintf(out_val + 13, "  勝利の後に切腹 (%d%s)", cdun, "階");
            } else {
                codeconv(the_score.how);
                if (!cdun) {
                    sprintf(out_val + 13, "  地上で%sに殺された", the_score.how);
                } else {
                    sprintf(out_val + 13, "  %d階で%sに殺された", cdun, the_score.how);
                }
            }
#else
            if (!cdun) {
                sprintf(out_val, "               Killed by %s on the surface", the_score.how);
            } else {
                sprintf(out_val, "               Killed by %s on %s %d", the_score.how, "Dungeon Level", cdun);
            }

            if (mdun > cdun) {
                strcat(out_val, format(" (Max %d)", mdun));
            }
#endif
            c_put_str(attr, out_val, n * 4 + 3, 0);
#ifdef JP
            char buf[11];

            /* 日付を 19yy/mm/dd の形式に変更する */
            if (strlen(when) == 8 && when[2] == '/' && when[5] == '/') {
                sprintf(buf, "%d%s/%.5s", 19 + (when[6] < '8'), when + 6, when);
                when = buf;
            }

            sprintf(out_val, "        (ユーザー:%s, 日付:%s, 所持金:%s, ターン:%s)", user, when, gold, aged);
#else
            sprintf(out_val, "               (User %s, Date %s, Gold %s, Turn %s).", user, when, gold, aged);
#endif

            c_put_str(attr, out_val, n * 4 + 4, 0);
        }

        prt(_("[ ESCで中断, その他のキーで続けます ]", "[Press ESC to quit, any other key to continue.]"), hgt - 1, _(21, 17));
        auto key = inkey();
        prt("", hgt - 1, 0);
        if (key == ESCAPE) {
            break;
        }
    }
}

#ifndef WINDOWS
/*!
 * @brief スコア表示処理メインルーチン / Display the scores in a given range and quit.
 * @param from 順位先頭
 * @param to 順位末尾
 * @details
 * スコア表示した後、或いは表示に失敗したら終了する (Windows版は表示処理の中で終了することはない)
 * main.cpp からしか呼ばれていない。暫定でプリプロで包んでおく
 * 取り敢えずWindows版はオーバーロード解決できている。他は不明
 */
void display_scores(int from, int to)
{
    char buf[1024];
    path_build(buf, sizeof(buf), ANGBAND_DIR_APEX, "scores.raw");
    highscore_fd = fd_open(buf, O_RDONLY);
    if (highscore_fd < 0) {
        quit(_("スコア・ファイルが使用できません。", "Score file unavailable."));
    }

    term_clear();
    display_scores(from, to, -1, nullptr);
    (void)fd_close(highscore_fd);
    highscore_fd = -1;
    quit(nullptr);
}
#endif
