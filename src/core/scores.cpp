/*!
 * @file scores.c
 * @brief ハイスコア処理 / Highscores handling
 * @date 2014/07/14
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * 2014 Deskull rearranged comment for Doxygen.
 */

#include "core/scores.h"
#include "cmd-io/cmd-dump.h"
#include "core/asking-player.h"
#include "core/score-util.h"
#include "core/turn-compensator.h"
#include "dungeon/dungeon.h"
#include "game-option/birth-options.h"
#include "game-option/game-play-options.h"
#include "io/files-util.h"
#include "io/input-key-acceptor.h"
#include "io/report.h"
#include "io/signal-handlers.h"
#include "io/uid-checker.h"
#include "locale/japanese.h"
#include "player-info/class-info.h"
#include "player/player-personality.h"
#include "player/player-status.h"
#include "player/race-info-table.h"
#include "system/angband-version.h"
#include "system/floor-type-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/angband-files.h"
#include "util/enum-converter.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "view/display-scores.h"
#include "world/world.h"

/*!
 * @brief 所定ポインタへスコア情報を書き込む / Write one score to the highscore file
 * @param score スコア情報参照ポインタ
 * @return エラーコード(問題がなければ0を返す)
 */
static int highscore_write(high_score *score)
{
    /* Write the record, note failure */
    return (fd_write(highscore_fd, (char *)(score), sizeof(high_score)));
}

/*!
 * @brief スコア情報を全て得るまで繰り返し取得する / Just determine where a new score *would* be placed
 * @param score スコア情報参照ポインタ
 * @return 正常ならば(MAX_HISCORES - 1)、問題があれば-1を返す
 */
static int highscore_where(high_score *score)
{
    /* Paranoia -- it may not have opened */
    if (highscore_fd < 0)
        return -1;

    /* Go to the start of the highscore file */
    if (highscore_seek(0))
        return -1;

    /* Read until we get to a higher score */
    high_score the_score;
    int my_score = atoi(score->pts);
    for (int i = 0; i < MAX_HISCORES; i++) {
        int old_score;
        if (highscore_read(&the_score))
            return (i);
        old_score = atoi(the_score.pts);
        if (my_score > old_score)
            return (i);
    }

    /* The "last" entry is always usable */
    return MAX_HISCORES - 1;
}

/*!
 * @brief スコア情報をバッファの末尾に追加する / Actually place an entry into the high score file
 * @param score スコア情報参照ポインタ
 * @return 正常ならば書き込んだスロット位置、問題があれば-1を返す / Return the location (0 is best) or -1 on "failure"
 */
static int highscore_add(high_score *score)
{
    /* Paranoia -- it may not have opened */
    if (highscore_fd < 0)
        return -1;

    /* Determine where the score should go */
    int slot = highscore_where(score);

    /* Hack -- Not on the list */
    if (slot < 0)
        return -1;

    /* Hack -- prepare to dump the new score */
    high_score the_score = (*score);

    /* Slide all the scores down one */
    bool done = false;
    high_score tmpscore;
    for (int i = slot; !done && (i < MAX_HISCORES); i++) {
        /* Read the old guy, note errors */
        if (highscore_seek(i))
            return -1;
        if (highscore_read(&tmpscore))
            done = true;

        /* Back up and dump the score we were holding */
        if (highscore_seek(i))
            return -1;
        if (highscore_write(&the_score))
            return -1;

        /* Hack -- Save the old score, for the next pass */
        the_score = tmpscore;
    }

    /* Return location used */
    return slot;
}

/*!
 * @brief スコアサーバへの転送処理
 * @param current_player_ptr プレイヤーへの参照ポインタ
 * @param do_send 実際に転送ア処置を行うか否か
 * @return 転送が成功したらTRUEを返す
 */
bool send_world_score(PlayerType *current_player_ptr, bool do_send)
{
#ifdef WORLD_SCORE
    if (!send_score || !do_send) {
        return true;
    }

    auto is_registration = get_check_strict(
        current_player_ptr, _("スコアをスコア・サーバに登録しますか? ", "Do you send score to the world score server? "), (CHECK_NO_ESCAPE | CHECK_NO_HISTORY));
    if (!is_registration) {
        return true;
    }

    prt("", 0, 0);
    prt(_("送信中．．", "Sending..."), 0, 0);
    term_fresh();
    screen_save();
    auto successful_send = report_score(current_player_ptr);
    screen_load();
    if (!successful_send) {
        return false;
    }

    prt(_("完了。何かキーを押してください。", "Completed.  Hit any key."), 0, 0);
    (void)inkey();
#else
    (void)current_player_ptr;
    (void)do_send;
    (void)display_player;
#endif
    return true;
}

/*!
 * @brief スコアの過去二十位内ランキングを表示する
 * Enters a players name on a hi-score table, if "legal", and in any
 * case, displays some relevant portion of the high score list.
 * @param current_player_ptr プレイヤーへの参照ポインタ
 * @return エラーコード
 * @details
 * Assumes "signals_ignore_tstp()" has been called.
 */
errr top_twenty(PlayerType *current_player_ptr)
{
    high_score the_score = {};
    char buf[32];

    /* Save the version */
    sprintf(the_score.what, "%u.%u.%u", FAKE_VER_MAJOR, FAKE_VER_MINOR, FAKE_VER_PATCH);

    /* Calculate and save the points */
    sprintf(the_score.pts, "%9ld", (long)calc_score(current_player_ptr));
    the_score.pts[9] = '\0';

    /* Save the current gold */
    sprintf(the_score.gold, "%9lu", (long)current_player_ptr->au);
    the_score.gold[9] = '\0';

    /* Save the current turn */
    sprintf(the_score.turns, "%9lu", (long)turn_real(current_player_ptr, w_ptr->game_turn));
    the_score.turns[9] = '\0';

    time_t ct = time((time_t *)0);

    /* Save the date in standard encoded form (9 chars) */
    strftime(the_score.day, 10, "@%Y%m%d", localtime(&ct));

    /* Save the player name (15 chars) */
    sprintf(the_score.who, "%-.15s", current_player_ptr->name);

    /* Save the player info */
    sprintf(the_score.uid, "%7u", current_player_ptr->player_uid);
    sprintf(the_score.sex, "%c", (current_player_ptr->psex ? 'm' : 'f'));
    snprintf(buf, sizeof(buf), "%2d", std::min(enum2i(current_player_ptr->prace), MAX_RACES));
    memcpy(the_score.p_r, buf, 3);
    snprintf(buf, sizeof(buf), "%2d", enum2i(std::min(current_player_ptr->pclass, PlayerClassType::MAX)));
    memcpy(the_score.p_c, buf, 3);
    snprintf(buf, sizeof(buf), "%2d", std::min(current_player_ptr->ppersonality, MAX_PERSONALITIES));
    memcpy(the_score.p_a, buf, 3);

    /* Save the level and such */
    sprintf(the_score.cur_lev, "%3d", std::min<ushort>(current_player_ptr->lev, 999));
    sprintf(the_score.cur_dun, "%3d", (int)current_player_ptr->current_floor_ptr->dun_level);
    sprintf(the_score.max_lev, "%3d", std::min<ushort>(current_player_ptr->max_plv, 999));
    sprintf(the_score.max_dun, "%3d", (int)max_dlv[current_player_ptr->dungeon_idx]);

    /* Save the cause of death (31 chars) */
    if (strlen(current_player_ptr->died_from) >= sizeof(the_score.how)) {
#ifdef JP
        angband_strcpy(the_score.how, current_player_ptr->died_from, sizeof(the_score.how) - 2);
        strcat(the_score.how, "…");
#else
        angband_strcpy(the_score.how, current_player_ptr->died_from, sizeof(the_score.how) - 3);
        strcat(the_score.how, "...");
#endif
    } else {
        strcpy(the_score.how, current_player_ptr->died_from);
    }

    /* Grab permissions */
    safe_setuid_grab(current_player_ptr);

    /* Lock (for writing) the highscore file, or fail */
    errr err = fd_lock(highscore_fd, F_WRLCK);

    /* Drop permissions */
    safe_setuid_drop();

    if (err)
        return 1;

    /* Add a new entry to the score list, see where it went */
    int j = highscore_add(&the_score);

    /* Grab permissions */
    safe_setuid_grab(current_player_ptr);

    /* Unlock the highscore file, or fail */
    err = fd_lock(highscore_fd, F_UNLCK);

    /* Drop permissions */
    safe_setuid_drop();

    if (err)
        return 1;

    /* Hack -- Display the top fifteen scores */
    if (j < 10) {
        display_scores(0, 15, j, nullptr);
        return 0;
    }

    /* Display the scores surrounding the player */
    display_scores(0, 5, j, nullptr);
    display_scores(j - 2, j + 7, j, nullptr);
    return 0;
}

/*!
 * @brief プレイヤーの現在のスコアをランキングに挟む /
 * Predict the players location, and display it.
 * @return エラーコード
 */
errr predict_score(PlayerType *current_player_ptr)
{
    high_score the_score;
    char buf[32];

    /* No score file */
    if (highscore_fd < 0) {
        msg_print(_("スコア・ファイルが使用できません。", "Score file unavailable."));
        msg_print(nullptr);
        return 0;
    }

    /* Save the version */
    sprintf(the_score.what, "%u.%u.%u", FAKE_VER_MAJOR, FAKE_VER_MINOR, FAKE_VER_PATCH);

    /* Calculate and save the points */
    sprintf(the_score.pts, "%9ld", (long)calc_score(current_player_ptr));

    /* Save the current gold */
    sprintf(the_score.gold, "%9lu", (long)current_player_ptr->au);

    /* Save the current turn */
    sprintf(the_score.turns, "%9lu", (long)turn_real(current_player_ptr, w_ptr->game_turn));

    /* Hack -- no time needed */
    strcpy(the_score.day, _("今日", "TODAY"));

    /* Save the player name (15 chars) */
    sprintf(the_score.who, "%-.15s", current_player_ptr->name);

    /* Save the player info */
    sprintf(the_score.uid, "%7u", current_player_ptr->player_uid);
    sprintf(the_score.sex, "%c", (current_player_ptr->psex ? 'm' : 'f'));
    snprintf(buf, sizeof(buf), "%2d", std::min(enum2i(current_player_ptr->prace), MAX_RACES));
    memcpy(the_score.p_r, buf, 3);
    snprintf(buf, sizeof(buf), "%2d", enum2i(std::min(current_player_ptr->pclass, PlayerClassType::MAX)));
    memcpy(the_score.p_c, buf, 3);
    snprintf(buf, sizeof(buf), "%2d", std::min(current_player_ptr->ppersonality, MAX_PERSONALITIES));
    memcpy(the_score.p_a, buf, 3);

    /* Save the level and such */
    sprintf(the_score.cur_lev, "%3d", std::min<ushort>(current_player_ptr->lev, 999));
    sprintf(the_score.cur_dun, "%3d", (int)current_player_ptr->current_floor_ptr->dun_level);
    sprintf(the_score.max_lev, "%3d", std::min<ushort>(current_player_ptr->max_plv, 999));
    sprintf(the_score.max_dun, "%3d", (int)max_dlv[current_player_ptr->dungeon_idx]);

    /* まだ死んでいないときの識別文字 */
    strcpy(the_score.how, _("yet", "nobody (yet!)"));

    /* See where the entry would be placed */
    int j = highscore_where(&the_score);

    /* Hack -- Display the top fifteen scores */
    if (j < 10) {
        display_scores(0, 15, j, &the_score);
        return 0;
    }

    display_scores(0, 5, -1, nullptr);
    display_scores(j - 2, j + 7, j, &the_score);
    return 0;
}

/*!
 * @brief スコアランキングの簡易表示 /
 * show_highclass - selectively list highscores based on class -KMW-
 */
void show_highclass(PlayerType *current_player_ptr)
{
    screen_save();
    char buf[1024], out_val[256];
    path_build(buf, sizeof(buf), ANGBAND_DIR_APEX, "scores.raw");

    highscore_fd = fd_open(buf, O_RDONLY);

    if (highscore_fd < 0) {
        msg_print(_("スコア・ファイルが使用できません。", "Score file unavailable."));
        msg_print(nullptr);
        return;
    }

    if (highscore_seek(0))
        return;

    high_score the_score;
    for (int i = 0; i < MAX_HISCORES; i++)
        if (highscore_read(&the_score))
            break;

    int m = 0;
    int j = 0;
    PLAYER_LEVEL clev = 0;
    int pr;
    while ((m < 9) && (j < MAX_HISCORES)) {
        if (highscore_seek(j))
            break;
        if (highscore_read(&the_score))
            break;
        pr = atoi(the_score.p_r);
        clev = (PLAYER_LEVEL)atoi(the_score.cur_lev);

#ifdef JP
        sprintf(out_val, "   %3d) %sの%s (レベル %2d)", (m + 1), race_info[pr].title, the_score.who, clev);
#else
        sprintf(out_val, "%3d) %s the %s (Level %2d)", (m + 1), the_score.who, race_info[pr].title, clev);
#endif

        prt(out_val, (m + 7), 0);
        m++;
        j++;
    }

#ifdef JP
    sprintf(out_val, "あなた) %sの%s (レベル %2d)", race_info[enum2i(current_player_ptr->prace)].title, current_player_ptr->name, current_player_ptr->lev);
#else
    sprintf(out_val, "You) %s the %s (Level %2d)", current_player_ptr->name, race_info[enum2i(current_player_ptr->prace)].title, current_player_ptr->lev);
#endif

    prt(out_val, (m + 8), 0);

    (void)fd_close(highscore_fd);
    highscore_fd = -1;
    prt(_("何かキーを押すとゲームに戻ります", "Hit any key to continue"), 0, 0);

    (void)inkey();

    for (j = 5; j < 18; j++)
        prt("", j, 0);
    screen_load();
}

/*!
 * @brief スコアランキングの簡易表示(種族毎)サブルーチン /
 * Race Legends -KMW-
 * @param race_num 種族ID
 */
void race_score(PlayerType *current_player_ptr, int race_num)
{
    int i = 0, j, m = 0;
    int pr, clev, lastlev;
    high_score the_score;
    char buf[1024], out_val[256], tmp_str[80];

    lastlev = 0;

    /* rr9: TODO - pluralize the race */
    sprintf(tmp_str, _("最高の%s", "The Greatest of all the %s"), race_info[race_num].title);

    prt(tmp_str, 5, 15);
    path_build(buf, sizeof(buf), ANGBAND_DIR_APEX, "scores.raw");

    highscore_fd = fd_open(buf, O_RDONLY);

    if (highscore_fd < 0) {
        msg_print(_("スコア・ファイルが使用できません。", "Score file unavailable."));
        msg_print(nullptr);
        return;
    }

    if (highscore_seek(0))
        return;

    for (i = 0; i < MAX_HISCORES; i++) {
        if (highscore_read(&the_score))
            break;
    }

    m = 0;
    j = 0;

    while ((m < 10) || (j < MAX_HISCORES)) {
        if (highscore_seek(j))
            break;
        if (highscore_read(&the_score))
            break;
        pr = atoi(the_score.p_r);
        clev = atoi(the_score.cur_lev);

        if (pr == race_num) {
#ifdef JP
            sprintf(out_val, "   %3d) %sの%s (レベル %2d)", (m + 1), race_info[pr].title, the_score.who, clev);
#else
            sprintf(out_val, "%3d) %s the %s (Level %3d)", (m + 1), the_score.who, race_info[pr].title, clev);
#endif

            prt(out_val, (m + 7), 0);
            m++;
            lastlev = clev;
        }
        j++;
    }

    /* add player if qualified */
    if ((enum2i(current_player_ptr->prace) == race_num) && (current_player_ptr->lev >= lastlev)) {
#ifdef JP
        sprintf(out_val, "あなた) %sの%s (レベル %2d)", race_info[enum2i(current_player_ptr->prace)].title, current_player_ptr->name, current_player_ptr->lev);
#else
        sprintf(out_val, "You) %s the %s (Level %3d)", current_player_ptr->name, race_info[enum2i(current_player_ptr->prace)].title, current_player_ptr->lev);
#endif

        prt(out_val, (m + 8), 0);
    }

    (void)fd_close(highscore_fd);
    highscore_fd = -1;
}

/*!
 * @brief スコアランキングの簡易表示(種族毎)メインルーチン /
 * Race Legends -KMW-
 */
void race_legends(PlayerType *current_player_ptr)
{
    for (int i = 0; i < MAX_RACES; i++) {
        race_score(current_player_ptr, i);
        msg_print(_("何かキーを押すとゲームに戻ります", "Hit any key to continue"));
        msg_print(nullptr);
        for (int j = 5; j < 19; j++)
            prt("", j, 0);
    }
}

/*!
 * @brief スコアファイル出力
 * Display some character info
 */
bool check_score(PlayerType *current_player_ptr)
{
    term_clear();

    /* No score file */
    if (highscore_fd < 0) {
        msg_print(_("スコア・ファイルが使用できません。", "Score file unavailable."));
        msg_print(nullptr);
        return false;
    }

    /* Wizard-mode pre-empts scoring */
    if (w_ptr->noscore & 0x000F) {
        msg_print(_("ウィザード・モードではスコアが記録されません。", "Score not registered for wizards."));
        msg_print(nullptr);
        return false;
    }

    /* Cheaters are not scored */
    if (w_ptr->noscore & 0xFF00) {
        msg_print(_("詐欺をやった人はスコアが記録されません。", "Score not registered for cheaters."));
        msg_print(nullptr);
        return false;
    }

    /* Interupted */
    if (!w_ptr->total_winner && streq(current_player_ptr->died_from, _("強制終了", "Interrupting"))) {
        msg_print(_("強制終了のためスコアが記録されません。", "Score not registered due to interruption."));
        msg_print(nullptr);
        return false;
    }

    /* Quitter */
    if (!w_ptr->total_winner && streq(current_player_ptr->died_from, _("途中終了", "Quitting"))) {
        msg_print(_("途中終了のためスコアが記録されません。", "Score not registered due to quitting."));
        msg_print(nullptr);
        return false;
    }
    return true;
}
