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
#include "core/score-file.h"
#include "core/score-util.h"
#include "core/turn-compensator.h"
#include "game-option/birth-options.h"
#include "game-option/game-play-options.h"
#include "io/input-context.h"
#include "io/report.h"
#include "io/signal-handlers.h"
#include "locale/japanese.h"
#include "player-base/player-race.h"
#include "player-info/class-info.h"
#include "player/player-personality.h"
#include "player/race-info-table.h"
#include "system/inner-game-data.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/enum-converter.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "view/display-scores.h"
#include "world/world.h"
#include <algorithm>

/*!
 * @brief スコアサーバへの転送処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param do_send 実際に転送ア処置を行うか否か
 * @return 転送が成功したらTRUEを返す
 */
bool send_world_score(PlayerType *player_ptr, bool do_send)
{
#ifdef WORLD_SCORE
    if (!send_score || !do_send) {
        return true;
    }

    auto is_registration = input_check_strict(
        player_ptr, _("スコアをスコア・サーバに登録しますか? ", "Do you send score to the world score server? "), { UserCheck::NO_ESCAPE, UserCheck::NO_HISTORY });
    if (!is_registration) {
        return true;
    }

    prt("", 0, 0);
    prt(_("送信中．．", "Sending..."), 0, 0);
    term_fresh();
    screen_save();
    auto successful_send = report_score(player_ptr);
    screen_load();
    if (!successful_send) {
        return false;
    }

    constexpr auto prompt = _("完了。何かキーを押してください。", "Completed.  Hit any key.");
    prt(prompt, 0, 0);
    (void)input_key_with_context(InputKind::ANY_KEY, prompt);
#else
    (void)player_ptr;
    (void)do_send;
#endif
    return true;
}

/*!
 * @brief スコアの過去二十位内ランキングを表示する
 * Enters a players name on a hi-score table, if "legal", and in any
 * case, displays some relevant portion of the high score list.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return エラーコード
 * @details
 * Assumes "signals_ignore_tstp()" has been called.
 */
errr top_twenty(PlayerType *player_ptr)
{
    auto score = HighScore::create(player_ptr);
    auto ct = time((time_t *)0);
    char date[11]{};
    // 日付を求められないときは、空の日付で記録する
    if (const auto *tm = localtime(&ct); (tm == nullptr) || (strftime(date, sizeof(date), "%Y-%m-%d", tm) == 0)) {
        date[0] = '\0';
    }

    score.date = date;
    score.died_from = HighScore::from_sys(player_ptr->died_from);

    auto j = -1;
    const auto scores = add_score_to_file(score, j);
    if (!scores) {
        msg_print(_("スコア・ファイルに記録できません。", "Failed to record the score."));
        msg_erase();
        return 1;
    }

    if (j < 10) {
        display_scores(*scores, 0, 15, j, nullptr);
        return 0;
    }

    display_scores(*scores, 0, 5, j, nullptr);
    display_scores(*scores, j - 2, j + 7, j, nullptr);
    return 0;
}

/*!
 * @brief プレイヤーの現在のスコアをランキングに挟む /
 * Predict the players location, and display it.
 * @return エラーコード
 */
errr predict_score(PlayerType *player_ptr)
{
    const auto scores = read_score_file();
    if (!scores) {
        msg_print(_("スコア・ファイルが使用できません。", "Score file unavailable."));
        msg_erase();
        return 0;
    }

    auto the_score = HighScore::create(player_ptr);
    the_score.date = HighScore::from_sys(_("今日", "TODAY"));
    the_score.died_from = _("yet", "nobody (yet!)");
    auto j = find_score_rank(*scores, the_score.points);
    if (j < 10) {
        display_scores(*scores, 0, 15, j, &the_score);
        return 0;
    }

    display_scores(*scores, 0, 5, -1, nullptr);
    display_scores(*scores, j - 2, j + 7, j, &the_score);
    return 0;
}

/*!
 * @brief スコアランキングの簡易表示 /
 * show_highclass - selectively list highscores based on class -KMW-
 */
void show_highclass(PlayerType *player_ptr)
{
    screen_save();
    const auto scores = read_score_file();
    if (!scores) {
        msg_print(_("スコア・ファイルが使用できません。", "Score file unavailable."));
        msg_erase();
        return;
    }

    const auto count = std::min(static_cast<int>(scores->size()), 9);
    for (auto i = 0; i < count; i++) {
        const auto &score = (*scores)[i];
        const auto clev = static_cast<PLAYER_LEVEL>(score.level);
        constexpr auto fmt = _("   {0:3}) {2}の{1} (レベル {3:2})", "{0:3}) {1} the {2} (Level {3:2})");
        prt(fmt::format(fmt, i + 1, HighScore::to_sys(score.name), race_info[score.race].title, clev), i + 7, 0);
    }

    constexpr auto fmt = _("あなた) {1}の{0} (レベル {2:2})", "You) {0} the {1} (Level {2:2})");
    prt(fmt::format(fmt, player_ptr->name, race_info[enum2i(player_ptr->prace)].title, player_ptr->lev), count + 8, 0);

    constexpr auto prompt = _("何かキーを押すとゲームに戻ります", "Hit any key to continue");
    prt(prompt, 0, 0);

    (void)input_key_with_context(InputKind::ANY_KEY, prompt);

    for (auto j = 5; j < 18; j++) {
        prt("", j, 0);
    }
    screen_load();
}

/*!
 * @brief スコアランキングの簡易表示(種族毎)サブルーチン /
 * Race Legends -KMW-
 * @param scores スコアファイルの記録 (スコアの高い順)
 * @param race_num 種族ID
 */
static void race_score(PlayerType *player_ptr, const std::vector<HighScore> &scores, int race_num)
{
    auto m = 0;
    auto lastlev = 0;

    /* rr9: TODO - pluralize the race */
    prt(std::string(_("最高の", "The Greatest of all the ")).append(race_info[race_num].title), 5, 15);
    for (const auto &score : scores) {
        if (score.race != race_num) {
            continue;
        }

        constexpr auto fmt = _("   {0:3}) {2}の{1} (レベル {3:2})", "{0:3}) {1} the {2} (Level {3:3})");
        prt(fmt::format(fmt, m + 1, HighScore::to_sys(score.name), race_info[score.race].title, score.level), m + 7, 0);
        m++;
        lastlev = score.level;
    }

    /* add player if qualified */
    if ((enum2i(player_ptr->prace) == race_num) && (player_ptr->lev >= lastlev)) {
        constexpr auto fmt = _("あなた) {1}の{0} (レベル {2:2})", "You) {0} the {1} (Level {2:3})");
        prt(fmt::format(fmt, player_ptr->name, race_info[enum2i(player_ptr->prace)].title, player_ptr->lev), m + 8, 0);
    }
}

/*!
 * @brief スコアランキングの簡易表示(種族毎)メインルーチン /
 * Race Legends -KMW-
 */
void race_legends(PlayerType *player_ptr)
{
    // 種族ごとに読み直さないよう、スコアファイルは 1 回だけ読む
    const auto scores = read_score_file();
    if (!scores) {
        msg_print(_("スコア・ファイルが使用できません。", "Score file unavailable."));
        msg_erase();
        return;
    }

    for (int i = 0; i < MAX_RACES; i++) {
        race_score(player_ptr, *scores, i);
        msg_print(_("何かキーを押すとゲームに戻ります", "Hit any key to continue"));
        msg_erase();
        for (int j = 5; j < 19; j++) {
            prt("", j, 0);
        }
    }
}

/*!
 * @brief スコアファイル出力
 * Display some character info
 */
bool check_score(PlayerType *player_ptr)
{
    term_clear();

    // 前の版と同じく、記録できない (読めても書けない) ときはワールドスコアにも送らない
    if (!can_add_score_to_file()) {
        msg_print(_("スコア・ファイルが使用できません。", "Score file unavailable."));
        msg_erase();
        return false;
    }

    /* Wizard-mode pre-empts scoring */
    const auto no_score = InnerGameData::get_instance().get_no_score();
    if (no_score & 0x000F) {
        msg_print(_("ウィザード・モードではスコアが記録されません。", "Score not registered for wizards."));
        msg_erase();
        return false;
    }

    /* Cheaters are not scored */
    if (no_score & 0xFF00) {
        msg_print(_("詐欺をやった人はスコアが記録されません。", "Score not registered for cheaters."));
        msg_erase();
        return false;
    }

    /* Interupted */
    const auto is_total_winner = AngbandWorld::get_instance().total_winner != 0;
    if (!is_total_winner && streq(player_ptr->died_from, _("強制終了", "Interrupting"))) {
        msg_print(_("強制終了のためスコアが記録されません。", "Score not registered due to interruption."));
        msg_erase();
        return false;
    }

    /* Quitter */
    if (!is_total_winner && streq(player_ptr->died_from, _("途中終了", "Quitting"))) {
        msg_print(_("途中終了のためスコアが記録されません。", "Score not registered due to quitting."));
        msg_erase();
        return false;
    }
    return true;
}
