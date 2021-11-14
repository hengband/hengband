/*!
 * @file signal-handlers.c
 * @brief シグナルハンドラの管理 / Controlling signal handlers
 * @date 2020/02/23
 * @author Hourier
 */

#include "io/signal-handlers.h"
#include "cmd-io/cmd-dump.h"
#include "core/game-closer.h"
#include "floor/floor-events.h"
#include "io/write-diary.h"
#include "monster-floor/monster-lite.h"
#include "game-option/cheat-options.h"
#include "save/save.h"
#include "system/system-variables.h"
#include "system/player-type-definition.h"
#include "term/term-color-types.h"
#include "world/world.h"

int16_t signal_count; /* Hack -- Count interupts */

#include <signal.h>

/*!
 * @brief OSからのシグナルを受けてサスペンド状態に入る /
 * Handle signals -- suspend
 * @param sig 受け取ったシグナル
 * @details
 * Actually suspend the game, and then resume cleanly
 */
static void handle_signal_suspend(int sig)
{
    (void)signal(sig, SIG_IGN);
#ifdef SIGSTOP
    term_fresh();
    term_xtra(TERM_XTRA_ALIVE, 0);
    (void)kill(0, SIGSTOP);
    term_xtra(TERM_XTRA_ALIVE, 1);
    term_redraw();
    term_fresh();
#endif
    (void)signal(sig, handle_signal_suspend);
}

/*!
 * @brief OSからのシグナルを受けて中断、終了する /
 * Handle signals -- simple (interrupt and quit)
 * @param sig 受け取ったシグナル
 * @details
 * <pre>
 * This function was causing a *huge* number of problems, so it has
 * been simplified greatly.  We keep a global variable which counts
 * the number of times the user attempts to kill the process, and
 * we commit suicide if the user does this a certain number of times.
 * We attempt to give "feedback" to the user as he approaches the
 * suicide thresh-hold, but without penalizing accidental keypresses.
 * To prevent messy accidents, we should reset this global variable
 * whenever the user enters a keypress, or something like that.
 * </pre>
 * @todo ここにPlayerTypeを追加すると関数ポインタ周りの収拾がつかなくなるので保留
 */
static void handle_signal_simple(int sig)
{
    (void)signal(sig, SIG_IGN);
    if (!w_ptr->character_generated || w_ptr->character_saved)
        quit(nullptr);

    signal_count++;
    if (p_ptr->is_dead) {
        (void)strcpy(p_ptr->died_from, _("強制終了", "Abortion"));
        forget_lite(p_ptr->current_floor_ptr);
        forget_view(p_ptr->current_floor_ptr);
        clear_mon_lite(p_ptr->current_floor_ptr);
        close_game(p_ptr);
        quit(_("強制終了", "interrupt"));
    } else if (signal_count >= 5) {
        (void)strcpy(p_ptr->died_from, _("強制終了中", "Interrupting"));
        forget_lite(p_ptr->current_floor_ptr);
        forget_view(p_ptr->current_floor_ptr);
        clear_mon_lite(p_ptr->current_floor_ptr);
        p_ptr->playing = false;
        if (!cheat_immortal)
            p_ptr->is_dead = true;
        p_ptr->leaving = true;
        close_game(p_ptr);
        quit(_("強制終了", "interrupt"));
    } else if (signal_count >= 4) {
        term_xtra(TERM_XTRA_NOISE, 0);
        term_erase(0, 0, 255);
        term_putstr(0, 0, -1, TERM_WHITE, _("熟慮の上の自殺！", "Contemplating suicide!"));
        term_fresh();
    } else if (signal_count >= 2) {
        term_xtra(TERM_XTRA_NOISE, 0);
    }

    (void)signal(sig, handle_signal_simple);
}

/*!
 * @brief OSからのシグナルを受けて強制終了する /
 * Handle signal -- abort, kill, etc
 * @param sig 受け取ったシグナル
 * @details
 * <pre>
 * This function was causing a *huge* number of problems, so it has
 * been simplified greatly.  We keep a global variable which counts
 * the number of times the user attempts to kill the process, and
 * we commit suicide if the user does this a certain number of times.
 * We attempt to give "feedback" to the user as he approaches the
 * suicide thresh-hold, but without penalizing accidental keypresses.
 * To prevent messy accidents, we should reset this global variable
 * whenever the user enters a keypress, or something like that.
 * </pre>
 * @todo ここにp_ptrを追加すると関数ポインタ周りの収拾がつかなくなるので保留
 */
static void handle_signal_abort(int sig)
{
    int wid, hgt;
    term_get_size(&wid, &hgt);

    (void)signal(sig, SIG_IGN);
    if (!w_ptr->character_generated || w_ptr->character_saved)
        quit(nullptr);

    forget_lite(p_ptr->current_floor_ptr);
    forget_view(p_ptr->current_floor_ptr);
    clear_mon_lite(p_ptr->current_floor_ptr);

    term_erase(0, hgt - 1, 255);
    term_putstr(0, hgt - 1, -1, TERM_RED, _("恐ろしいソフトのバグが飛びかかってきた！", "A gruesome software bug LEAPS out at you!"));

    term_putstr(45, hgt - 1, -1, TERM_RED, _("緊急セーブ...", "Panic save..."));

    exe_write_diary(p_ptr, DIARY_GAMESTART, 0, _("----ゲーム異常終了----", "-- Tried Panic Save and Aborted Game --"));
    term_fresh();

    p_ptr->panic_save = 1;
    (void)strcpy(p_ptr->died_from, _("(緊急セーブ)", "(panic save)"));

    signals_ignore_tstp();

    if (save_player(p_ptr, SAVE_TYPE_CLOSE_GAME)) {
        term_putstr(45, hgt - 1, -1, TERM_RED, _("緊急セーブ成功！", "Panic save succeeded!"));
    } else {
        term_putstr(45, hgt - 1, -1, TERM_RED, _("緊急セーブ失敗！", "Panic save failed!"));
    }

    term_fresh();
    quit(_("ソフトのバグ", "software bug"));
}

/*!
 * @brief OSからのSIGTSTPシグナルを無視する関数 /
 * Ignore SIGTSTP signals (keyboard suspend)
 * @details
 */
void signals_ignore_tstp(void)
{
#ifdef SIGTSTP
    (void)signal(SIGTSTP, SIG_IGN);
#endif
}

/*!
 * @brief OSからのSIGTSTPシグナルハンドラ /
 * Handle SIGTSTP signals (keyboard suspend)
 * @details
 */
void signals_handle_tstp(void)
{
#ifdef SIGTSTP
    (void)signal(SIGTSTP, handle_signal_suspend);
#endif
}

/*!
 * @brief OSからのシグナルハンドルを初期化する /
 * Prepare to handle the relevant signals
 * @details
 */
void signals_init(void)
{
#ifdef SIGHUP
    (void)signal(SIGHUP, SIG_IGN);
#endif

#ifdef SIGTSTP
    (void)signal(SIGTSTP, handle_signal_suspend);
#endif

#ifdef SIGINT
    (void)signal(SIGINT, handle_signal_simple);
#endif

#ifdef SIGQUIT
    (void)signal(SIGQUIT, handle_signal_simple);
#endif

#ifdef SIGFPE
    (void)signal(SIGFPE, handle_signal_abort);
#endif

#ifdef SIGILL
    (void)signal(SIGILL, handle_signal_abort);
#endif

#ifdef SIGTRAP
    (void)signal(SIGTRAP, handle_signal_abort);
#endif

#ifdef SIGIOT
    (void)signal(SIGIOT, handle_signal_abort);
#endif

#ifdef SIGKILL
    (void)signal(SIGKILL, handle_signal_abort);
#endif

#ifdef SIGBUS
    (void)signal(SIGBUS, handle_signal_abort);
#endif

#ifdef SIGSEGV
    (void)signal(SIGSEGV, handle_signal_abort);
#endif

#ifdef SIGTERM
    (void)signal(SIGTERM, handle_signal_abort);
#endif

#ifdef SIGPIPE
    (void)signal(SIGPIPE, handle_signal_abort);
#endif

#ifdef SIGEMT
    (void)signal(SIGEMT, handle_signal_abort);
#endif

#ifdef SIGDANGER
    (void)signal(SIGDANGER, handle_signal_abort);
#endif

#ifdef SIGSYS
    (void)signal(SIGSYS, handle_signal_abort);
#endif

#ifdef SIGXCPU
    (void)signal(SIGXCPU, handle_signal_abort);
#endif

#ifdef SIGPWR
    (void)signal(SIGPWR, handle_signal_abort);
#endif
}
