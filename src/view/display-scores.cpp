#include "view/display-scores.h"
#include "core/score-util.h"
#include "io/files-util.h"
#include "io/input-key-acceptor.h"
#include "player/player-class.h"
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
void display_scores_aux(int from, int to, int note, high_score *score)
{
    int i, j, k, n, place;
    TERM_COLOR attr;

    high_score the_score;

    GAME_TEXT out_val[256];
    GAME_TEXT tmp_val[160];

    TERM_LEN wid, hgt, per_screen;

    term_get_size(&wid, &hgt);
    per_screen = (hgt - 4) / 4;

    /* Paranoia -- it may not have opened */
    if (highscore_fd < 0)
        return;

    /* Assume we will show the first 10 */
    if (from < 0)
        from = 0;
    if (to < 0)
        to = 10;
    if (to > MAX_HISCORES)
        to = MAX_HISCORES;

    /* Seek to the beginning */
    if (highscore_seek(0))
        return;

    /* Hack -- Count the high scores */
    for (i = 0; i < MAX_HISCORES; i++) {
        if (highscore_read(&the_score))
            break;
    }

    /* Hack -- allow "fake" entry to be last */
    if ((note == i) && score)
        i++;

    /* Forget about the last entries */
    if (i > to)
        i = to;

    /* Show per_screen per page, until "done" */
    for (k = from, place = k + 1; k < i; k += per_screen) {
        term_clear();

        /* Title */
        put_str(_("                変愚蛮怒: 勇者の殿堂", "                Hengband Hall of Fame"), 0, 0);

        /* Indicate non-top scores */
        if (k > 0) {
            sprintf(tmp_val, _("( %d 位以下 )", "(from position %d)"), k + 1);
            put_str(tmp_val, 0, 40);
        }

        /* Dump per_screen entries */
        for (j = k, n = 0; j < i && n < per_screen; place++, j++, n++) {
            int pr, pc, pa, clev, mlev, cdun, mdun;

            concptr user, gold, when, aged;

            /* Hack -- indicate death in yellow */
            attr = (j == note) ? TERM_YELLOW : TERM_WHITE;

            /* Mega-Hack -- insert a "fake" record */
            if ((note == j) && score) {
                the_score = (*score);
                attr = TERM_L_GREEN;
                score = NULL;
                note = -1;
                j--;
            }

            /* Read a normal record */
            else {
                /* Read the proper record */
                if (highscore_seek(j))
                    break;
                if (highscore_read(&the_score))
                    break;
            }

            /* Extract the race/class */
            pr = atoi(the_score.p_r);
            pc = atoi(the_score.p_c);
            pa = atoi(the_score.p_a);

            /* Extract the level info */
            clev = atoi(the_score.cur_lev);
            mlev = atoi(the_score.max_lev);
            cdun = atoi(the_score.cur_dun);
            mdun = atoi(the_score.max_dun);

            /* Hack -- extract the gold and such */
            for (user = the_score.uid; iswspace(*user); user++) /* loop */
                ;
            for (when = the_score.day; iswspace(*when); when++) /* loop */
                ;
            for (gold = the_score.gold; iswspace(*gold); gold++) /* loop */
                ;
            for (aged = the_score.turns; iswspace(*aged); aged++) /* loop */
                ;

            /* Clean up standard encoded form of "when" */
            if ((*when == '@') && strlen(when) == 9) {
                sprintf(tmp_val, "%.4s-%.2s-%.2s", when + 1, when + 5, when + 7);
                when = tmp_val;
            }

            /* Dump some info */
#ifdef JP
            /*sprintf(out_val, "%3d.%9s  %s%s%sという名の%sの%s (レベル %d)", */
            sprintf(out_val, "%3d.%9s  %s%s%s - %s%s (レベル %d)", place, the_score.pts, personality_info[pa].title, (personality_info[pa].no ? "の" : ""),
                the_score.who, race_info[pr].title, class_info[pc].title, clev);

#else
            sprintf(out_val, "%3d.%9s  %s %s the %s %s, Level %d", place, the_score.pts, personality_info[pa].title, the_score.who, race_info[pr].title,
                class_info[pc].title, clev);
#endif

            /* Append a "maximum level" */
            if (mlev > clev)
                strcat(out_val, format(_(" (最高%d)", " (Max %d)"), mlev));

            /* Dump the first line */
            c_put_str(attr, out_val, n * 4 + 2, 0);

            /* Another line of info */
#ifdef JP
            if (mdun != 0)
                sprintf(out_val, "    最高%3d階", mdun);
            else
                sprintf(out_val, "             ");

            /* 死亡原因をオリジナルより細かく表示 */
            if (streq(the_score.how, "yet")) {
                sprintf(out_val + 13, "  まだ生きている (%d%s)", cdun, "階");
            } else if (streq(the_score.how, "ripe")) {
                sprintf(out_val + 13, "  勝利の後に引退 (%d%s)", cdun, "階");
            } else if (streq(the_score.how, "Seppuku")) {
                sprintf(out_val + 13, "  勝利の後に切腹 (%d%s)", cdun, "階");
            } else {
                codeconv(the_score.how);

                /* Some people die outside of the dungeon */
                if (!cdun)
                    sprintf(out_val + 13, "  地上で%sに殺された", the_score.how);
                else
                    sprintf(out_val + 13, "  %d階で%sに殺された", cdun, the_score.how);
            }

#else
            /* Some people die outside of the dungeon */
            if (!cdun)
                sprintf(out_val, "               Killed by %s on the surface", the_score.how);
            else
                sprintf(out_val, "               Killed by %s on %s %d", the_score.how, "Dungeon Level", cdun);

            /* Append a "maximum level" */
            if (mdun > cdun)
                strcat(out_val, format(" (Max %d)", mdun));
#endif

            /* Dump the info */
            c_put_str(attr, out_val, n * 4 + 3, 0);

            /* And still another line of info */
#ifdef JP
            {
                char buf[11];

                /* 日付を 19yy/mm/dd の形式に変更する */
                if (strlen(when) == 8 && when[2] == '/' && when[5] == '/') {
                    sprintf(buf, "%d%s/%.5s", 19 + (when[6] < '8'), when + 6, when);
                    when = buf;
                }
                sprintf(out_val, "        (ユーザー:%s, 日付:%s, 所持金:%s, ターン:%s)", user, when, gold, aged);
            }

#else
            sprintf(out_val, "               (User %s, Date %s, Gold %s, Turn %s).", user, when, gold, aged);
#endif

            c_put_str(attr, out_val, n * 4 + 4, 0);
        }

        /* Wait for response */
        prt(_("[ ESCで中断, その他のキーで続けます ]", "[Press ESC to quit, any other key to continue.]"), hgt - 1, _(21, 17));

        j = inkey();
        prt("", hgt - 1, 0);

        /* Hack -- notice Escape */
        if (j == ESCAPE)
            break;
    }
}

/*!
 * @brief スコア表示処理メインルーチン / Hack -- Display the scores in a given range and quit.
 * @param from 順位先頭
 * @param to 順位末尾
 * @details
 * <pre>
 * This function is only called from "main.c" when the user asks
 * to see the "high scores".
 * </pre>
 */
void display_scores(int from, int to)
{
    char buf[1024];
    path_build(buf, sizeof(buf), ANGBAND_DIR_APEX, "scores.raw");

    /* Open the binary high score file, for reading */
    highscore_fd = fd_open(buf, O_RDONLY);

    /* Paranoia -- No score file */
    if (highscore_fd < 0)
        quit(_("スコア・ファイルが使用できません。", "Score file unavailable."));
    term_clear();

    /* Display the scores */
    display_scores_aux(from, to, -1, NULL);

    /* Shut the high score file */
    (void)fd_close(highscore_fd);

    /* Forget the high score fd */
    highscore_fd = -1;

    /* Quit */
    quit(NULL);
}
